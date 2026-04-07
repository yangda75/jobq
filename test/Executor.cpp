#include "Executor.h"
#include "Log.h"
#include "ManualSource.h"
#include "TimerSource.h"
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <thread>

TEST_CASE("run will run") {
    jobq::Executor ex{};
    bool finished = false;
    ex.submitJob([&finished]() { finished = true; });
    std::thread t{[&ex]() { ex.run(); }};
    ex.shutdownAndDrain();
    t.join();
    REQUIRE(finished);
}

// all jobs will run
// jobs submitted before run starts will run
// jobs submitted while run active will run
// jobs submitted after shutdownAndDrain will not run
// shutDownAndDrain is idempotent
TEST_CASE("run will run all jobs") {
    jobq::Executor ex{};
    std::atomic_int cnt{0};
    constexpr auto N = 100;
    constexpr auto M = 50;
    for (int i = 0; i < N; i++) {
        REQUIRE(ex.submitJob([&cnt]() { cnt++; }));
    }
    std::thread t{[&ex]() { ex.run(); }};
    for (int i = 0; i < M; i++) {
        REQUIRE(ex.submitJob([&cnt]() { cnt++; }));
    }
    ex.shutdownAndDrain();
    ex.shutdownAndDrain();
    ex.shutdownAndDrain();
    for (int i = 0; i < M; i++) {
        REQUIRE(!ex.submitJob([&cnt]() { cnt++; }));
    }
    ex.shutdownAndDrain();
    ex.shutdownAndDrain();
    t.join();
    REQUIRE(cnt == N + M);
}

TEST_CASE("exeception does not stop executor") {
    jobq::Executor ex{};
    bool finished = false;
    ex.submitJob([]() { throw std::runtime_error{"BAD THING"}; });
    ex.submitJob([&finished]() { finished = true; });

    std::thread t{[&ex]() { ex.run(); }};
    ex.shutdownAndDrain();
    t.join();
    REQUIRE(finished);
}

TEST_CASE("after shutdown, remaining jobs will not run") {
    jobq::Executor ex{};
    std::atomic_int cnt{};
    for (int i = 0; i < 10; i++) {
        REQUIRE(ex.submitJob([&cnt]() { cnt++; }));
    }
    REQUIRE(ex.submitJob([&ex]() { ex.shutdown(); }));
    for (int i = 0; i < 10; i++) {
        REQUIRE(ex.submitJob([&cnt]() { cnt++; }));
    }

    std::thread t{[&ex]() { ex.run(); }};
    t.join();
    REQUIRE(cnt == 10);
}

TEST_CASE("empty job and run, will wait for new jobs") {
    jobq::Executor ex{};
    std::atomic_bool finished{false};
    std::thread t{[&ex, &finished]() {
        ex.run();
        finished = true;
    }};
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    REQUIRE(!finished);
    // stop will stop
    ex.shutdown();
    t.join();
}

TEST_CASE("shutdown empty executor is fine") {
    jobq::Executor ex{};
    ex.shutdown();
    REQUIRE(!ex.submitJob([]() { jobq::loginfo("some info"); }));
    ex.shutdown();
    REQUIRE(!ex.submitJob([]() { jobq::loginfo("some info"); }));
    ex.shutdownAndDrain();
    REQUIRE(!ex.submitJob([]() { jobq::loginfo("some info"); }));
    ex.shutdownAndDrain();
    REQUIRE(!ex.submitJob([]() { jobq::loginfo("some info"); }));

    // run after shutdown
    std::thread t{[&ex]() { ex.run(); }};
    // finish immediately
    t.join();
}

TEST_CASE("shutdown will get jobs discarded") {
    jobq::Executor ex{};

    std::atomic_bool flag{};
    REQUIRE(ex.submitJob([&flag]() {
        while (!flag) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }));

    std::atomic_int finished_cnt{};
    for (int i = 0; i < 100; i++) {
        REQUIRE(ex.submitJob([&finished_cnt]() { finished_cnt++; }));
    }
    std::thread t{[&ex]() { ex.run(); }};
    // shutdown executor
    ex.shutdown();
    // release blocking job
    flag = true;
    t.join();
    // check all other jobs are discarded
    REQUIRE(finished_cnt == 0);
}

TEST_CASE("shutdownAndDrain will get already submitted jobs finished") {
    jobq::Executor ex{};

    std::atomic_bool flag{};
    REQUIRE(ex.submitJob([&flag]() {
        while (!flag) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }));

    std::atomic_int finished_cnt{};
    for (int i = 0; i < 100; i++) {
        REQUIRE(ex.submitJob([&finished_cnt]() { finished_cnt++; }));
    }
    std::thread t{[&ex]() { ex.run(); }};
    // shutdown executor
    ex.shutdownAndDrain();
    // release blocking job
    flag = true;
    t.join();
    // check all other jobs are discarded
    REQUIRE(finished_cnt == 100);
}

TEST_CASE("shutdown before run with queued jobs") {
    jobq::Executor ex{};
    std::atomic_int finished_cnt{};

    for (int i = 0; i < 10; i++) {
        ex.submitJob([&finished_cnt]() { finished_cnt++; });
    }
    // shutdown before run
    ex.shutdown();
    std::thread t{[&ex]() { ex.run(); }};
    t.join();

    REQUIRE(finished_cnt == 0);
}

TEST_CASE("shutdownAndDrain before run with queued jobs") {
    jobq::Executor ex{};
    std::atomic_int finished_cnt{};

    for (int i = 0; i < 10; i++) {
        ex.submitJob([&finished_cnt]() { finished_cnt++; });
    }
    // shutdown before run
    ex.shutdownAndDrain();
    std::thread t{[&ex]() { ex.run(); }};
    t.join();

    REQUIRE(finished_cnt == 10);
}

TEST_CASE("registerSource compiles") {
    jobq::Executor ex{};
    std::shared_ptr<jobq::Source> src = std::make_shared<jobq::TimerSource>(
        jobq::TimerSource::Mode::ONE_SHOT, 100,
        []() { jobq::loginfo("timer!!!"); });
    ex.registerSource(src);
}

TEST_CASE("timerSource working") {
    jobq::Executor ex{};
    std::atomic_bool job_done{false};
    std::shared_ptr<jobq::Source> src = std::make_shared<jobq::TimerSource>(
        jobq::TimerSource::Mode::ONE_SHOT, 10,
        [&job_done]() { job_done = true; });
    ex.registerSource(src);
    std::thread th{[&ex]() { ex.run(); }};
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(20ms);
    ex.shutdownAndDrain();
    th.join();

    REQUIRE(job_done);
}

TEST_CASE("repeating timer source working") {
    jobq::Executor ex{};
    std::atomic_int num_job_done{false};

    std::shared_ptr<jobq::Source> src = std::make_shared<jobq::TimerSource>(
        jobq::TimerSource::Mode::REPEATING, 10,
        [&num_job_done]() { ++num_job_done; });
    ex.registerSource(src);

    std::thread th{[&ex]() { ex.run(); }};
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(100ms);
    ex.shutdownAndDrain();
    th.join();

    REQUIRE(num_job_done > 1);
}

TEST_CASE("registerSource compiles with ManualSource") {
    jobq::Executor ex{};
    std::shared_ptr<jobq::Source> src = std::make_shared<jobq::ManualSource>(
        "test", []() { jobq::loginfo("manual"); });
    ex.registerSource(src);
}

TEST_CASE("manual source working") {
    jobq::Executor ex{};
    std::atomic_bool job_done{false};
    std::shared_ptr<jobq::Source> src = std::make_shared<jobq::ManualSource>(
        "test", [&job_done]() { job_done = true; });

    ex.registerSource(src);

    std::thread th{[&ex]() { ex.run(); }};
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(20ms);
    ex.shutdownAndDrain();
    th.join();
    REQUIRE(job_done);
}

TEST_CASE("shutdown stops timer source") {
    jobq::Executor ex{};
    std::atomic_int callback_cnt{};
    std::shared_ptr<jobq::Source> src = std::make_shared<jobq::TimerSource>(
        jobq::TimerSource::Mode::REPEATING, 10,
        [&callback_cnt]() { callback_cnt++; });

    ex.registerSource(src);

    std::thread th{[&ex]() { ex.run(); }};
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(50ms);
    // assert callback_cnt > 1
    REQUIRE(callback_cnt > 1);
    auto current_callback_cnt = callback_cnt.load();
    // shutdown and check again
    ex.shutdown();

    // new timer callbacks are not executed

    th.join();
    REQUIRE(callback_cnt == current_callback_cnt);
}
