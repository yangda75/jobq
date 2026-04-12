#include "Executor.h"
#include "Log.h"
#include "Source.h"
#include "TimerSource.h"
#include "TriggerSource.h"
#include "Utils.h"
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <thread>

/// helpers

class Signal {
  public:
    std::future<void> getFuture() { return promise_.get_future(); }
    void notify() {
        if (!notified_.exchange(true, std::memory_order_relaxed)) {
            promise_.set_value();
        }
    }

  private:
    std::promise<void> promise_{};
    std::atomic_bool notified_{false};
};

class Gate {
  public:
    void wait() {
        std::unique_lock l{mtx_};
        cv_.wait(l, [this]() { return open_; });
    }
    void open() {
        {
            std::lock_guard l{mtx_};
            open_ = true;
        }
        cv_.notify_all();
    }

  private:
    std::mutex mtx_{};
    std::condition_variable cv_{};
    bool open_{false};
};

using namespace std::chrono_literals;
const std::chrono::steady_clock::duration kEventuallyTimeout = 3s;

template <typename Predicate>
void requireEventually(
    Predicate &&predicate, std::string const &msg,
    std::chrono::steady_clock::duration timeout = kEventuallyTimeout) {

    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) {
            return;
        }
        std::this_thread::yield();
    }

    REQUIRE(predicate());
}

/// testcases

TEST_CASE("run will run") {
    jobq::Executor ex{};
    bool finished = false;
    ex.submitJob([&finished]() { finished = true; });
    std::thread t{[&ex]() { ex.run(); }};
    ex.shutdownAndDrain();
    jobq::loginfo("shutdown and drain done\n");
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
    auto stat = ex.getStats();
    REQUIRE(stat.jobs_submitted == 10);
    ex.submitJob([&ex]() { ex.shutdown(); });
    for (int i = 0; i < 10; i++) {
        REQUIRE(ex.submitJob([&cnt]() { cnt++; }));
    }

    std::thread t{[&ex]() { ex.run(); }};
    t.join();
    REQUIRE(cnt == 10);
}

TEST_CASE("empty job and run, will wait for new jobs") {
    jobq::Executor ex{};
    Signal started{};
    Signal finished{};
    std::thread t{[&ex, &started, &finished]() {
        started.notify();
        ex.run();
        finished.notify();
    }};

    REQUIRE(started.getFuture().wait_for(2s) == std::future_status::ready);
    REQUIRE(finished.getFuture().wait_for(20ms) == std::future_status::timeout);
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

    Signal started{};
    Gate gate{};
    REQUIRE(ex.submitJob([&started, &gate]() {
        started.notify();
        gate.wait(); // gate is not yet opened, here
    }));

    std::atomic_int finished_cnt{};
    for (int i = 0; i < 100; i++) {
        REQUIRE(ex.submitJob([&finished_cnt]() { finished_cnt++; }));
    }
    std::thread t{[&ex]() { ex.run(); }};
    REQUIRE(started.getFuture().wait_for(2s) == std::future_status::ready);
    // shutdown executor
    ex.shutdown();
    // release blocking job
    gate.open(); // now let the first job continue to finish
    t.join();
    // check all other jobs are discarded
    REQUIRE(finished_cnt == 0);
}

TEST_CASE("shutdownAndDrain will get already submitted jobs finished") {
    jobq::Executor ex{};

    Signal started{};
    Gate gate{};
    REQUIRE(ex.submitJob([&started, &gate]() {
        started.notify();
        gate.wait();
    }));

    std::atomic_int finished_cnt{};
    for (int i = 0; i < 100; i++) {
        REQUIRE(ex.submitJob([&finished_cnt]() { finished_cnt++; }));
    }
    std::thread t{[&ex]() { ex.run(); }};
    REQUIRE(started.getFuture().wait_for(2s) == std::future_status::ready);
    // shutdown executor
    ex.shutdownAndDrain();
    // release blocking job
    gate.open();
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
    auto src = std::make_shared<jobq::TimerSource<int64_t, std::milli>>(
        jobq::TimerMode::ONE_SHOT, 100ms, []() { jobq::loginfo("timer!!!"); });
    ex.registerSource(src);
}

TEST_CASE("timerSource working") {
    jobq::Executor ex{};
    std::atomic_bool job_done{false};
    Signal signal{};
    std::shared_ptr<jobq::Source> src = std::make_shared<jobq::TimerSourceMs>(
        jobq::TimerMode::ONE_SHOT, 10ms, [&job_done, &signal]() {
            job_done = true;
            signal.notify();
        });
    ex.registerSource(src);
    std::thread th{[&ex]() { ex.run(); }};
    signal.getFuture().wait_for(1s);
    ex.shutdownAndDrain();
    th.join();

    REQUIRE(job_done);
}

TEST_CASE("repeating timer source working") {
    jobq::Executor ex{};
    std::atomic_int num_job_done{false};
    Signal signal{};
    std::shared_ptr<jobq::Source> src = std::make_shared<jobq::TimerSourceMs>(
        jobq::TimerMode::REPEATING, 10ms, [&num_job_done, &signal]() {
            ++num_job_done;
            if (num_job_done > 1) {
                signal.notify();
            }
        });
    ex.registerSource(src);

    std::thread th{[&ex]() { ex.run(); }};
    signal.getFuture().wait_for(1s);
    ex.shutdownAndDrain();
    th.join();

    REQUIRE(num_job_done > 1);
}

TEST_CASE("registerSource compiles with TriggerSource") {
    jobq::Executor ex{};
    std::shared_ptr<jobq::Source> src = std::make_shared<jobq::TriggerSource>(
        "test", []() { jobq::loginfo("trigger"); });
    ex.registerSource(src);
}

TEST_CASE("trigger source working") {
    jobq::Executor ex{};
    std::atomic_bool job_done{false};
    Signal signal{};
    auto trigger_src =
        std::make_shared<jobq::TriggerSource>("test", [&job_done, &signal]() {
            job_done = true;
            signal.notify();
        });

    std::shared_ptr<jobq::Source> src = trigger_src;
    ex.registerSource(src);

    std::thread th{[&ex]() { ex.run(); }};
    trigger_src->trigger();
    signal.getFuture().wait_for(1s);
    ex.shutdownAndDrain();
    th.join();
    REQUIRE(job_done);
}

TEST_CASE("shutdown stops timer source") {
    jobq::Executor ex{};
    std::atomic_int callback_cnt{};
    Signal signal{};
    std::shared_ptr<jobq::Source> src = std::make_shared<jobq::TimerSourceMs>(
        jobq::TimerMode::REPEATING, 1ms, [&callback_cnt, &signal]() {
            callback_cnt++;
            if (callback_cnt > 1) {
                signal.notify();
            }
        });

    ex.registerSource(src);

    std::thread th{[&ex]() { ex.run(); }};
    signal.getFuture().wait_for(1s);
    // assert callback_cnt > 1
    REQUIRE(callback_cnt > 1);
    // shutdown and check again
    ex.shutdown();
    auto stat = ex.getStats();
    auto jobs_submitted = stat.jobs_submitted;
    // new timer callbacks are not executed

    th.join();
    stat = ex.getStats();
    REQUIRE(jobs_submitted == stat.jobs_submitted);
}

TEST_CASE("one shot timer is exactly once") {
    jobq::Executor ex{};
    std::atomic_int callback_cnt{};
    Signal signal{};
    std::shared_ptr<jobq::Source> src = std::make_shared<jobq::TimerSourceMs>(
        jobq::TimerMode::ONE_SHOT, 5ms, [&callback_cnt, &signal]() {
            callback_cnt++;
            signal.notify();
        });

    ex.registerSource(src);
    auto th = jobq::runExecutor(ex);
    signal.getFuture().wait_for(1s);
    REQUIRE(callback_cnt == 1);
    ex.shutdownAndDrain();
    th.join();
    REQUIRE(callback_cnt == 1);
}

TEST_CASE("multiple concurrent one shot timers") {
    jobq::Executor ex{};
    std::atomic_int callback_cnt{};
    constexpr int TIMER_CNT = 10;
    Signal signal{};
    for (int i = 0; i < TIMER_CNT; i++) {
        jobq::SharedSourcePtr src = std::make_shared<jobq::TimerSourceMs>(
            jobq::TimerMode::ONE_SHOT, 1ms, [&callback_cnt, &signal]() {
                callback_cnt++;
                if (callback_cnt == TIMER_CNT) {
                    signal.notify();
                }
            });
        ex.registerSource(src);
    }
    auto th = jobq::runExecutor(ex);
    signal.getFuture().wait_for(1s);
    auto stat = ex.getStats();
    REQUIRE(stat.jobs_submitted == TIMER_CNT);
    ex.shutdownAndDrain();
    th.join();
    stat = ex.getStats();
    REQUIRE(stat.jobs_executed == TIMER_CNT);
}

TEST_CASE("multiple concurrent repeating timers") {
    jobq::Executor ex{};
    std::deque<std::atomic_int> callback_cnt_vec{};
    constexpr int TIMER_CNT = 10;
    for (int i = 0; i < TIMER_CNT; i++) {
        callback_cnt_vec.emplace_back(
            0); // deque push back is construction in place
    }
    Gate gate{};
    for (int i = 0; i < TIMER_CNT; i++) {
        jobq::SharedSourcePtr src = std::make_shared<jobq::TimerSourceMs>(
            jobq::TimerMode::REPEATING, 5ms, [&callback_cnt_vec, i, &gate]() {
                gate.wait();
                callback_cnt_vec[i]++;
            });
        ex.registerSource(src);
    }
    auto th = jobq::runExecutor(ex);
    requireEventually(
        [&ex]() {
            auto stat = ex.getStats();
            return stat.jobs_submitted > stat.jobs_executed;
        },
        "more jobs submitted than executed");
    auto stat = ex.getStats();
    REQUIRE(stat.jobs_submitted > stat.jobs_executed);
    ex.shutdownAndDrain();
    gate.open(); // now, all the blocking jobs can execute and finish
    th.join();
    stat = ex.getStats();
    REQUIRE(stat.jobs_submitted == stat.jobs_executed);
}

TEST_CASE("multiple concurrent repeating timers multiple worker threads") {
    jobq::Executor ex{3};
    std::deque<std::atomic_int> callback_cnt_vec{};
    constexpr int TIMER_CNT = 10;
    for (int i = 0; i < TIMER_CNT; i++) {
        callback_cnt_vec.emplace_back(
            0); // deque push back is construction in place
    }
    Gate gate{};
    for (int i = 0; i < TIMER_CNT; i++) {
        jobq::SharedSourcePtr src = std::make_shared<jobq::TimerSourceMs>(
            jobq::TimerMode::REPEATING, 1ms, [&callback_cnt_vec, i, &gate]() {
                gate.wait();
                callback_cnt_vec[i]++;
            });
        ex.registerSource(src);
    }
    auto th = jobq::runExecutor(ex);
    requireEventually(
        [&ex]() {
            auto stat = ex.getStats();
            return stat.active_workers == 3;
        },
        "3 active workers");
    requireEventually(
        [&ex]() {
            auto stat = ex.getStats();
            return stat.jobs_submitted > stat.jobs_executed;
        },
        "more jobs submitted than executed");
    auto stat = ex.getStats();
    REQUIRE(stat.jobs_submitted > stat.jobs_executed);
    REQUIRE(stat.active_workers == 3);
    ex.shutdownAndDrain();
    gate.open();
    th.join();
    stat = ex.getStats();
    REQUIRE(stat.active_workers == 0);
    REQUIRE(stat.jobs_submitted == stat.jobs_executed);
}

TEST_CASE("shutdownAndDrain finishes already queued callbacks") {
    jobq::Executor ex{};
    std::atomic_int callback_cnt{};
    Signal started{};
    Gate gate{};
    jobq::SharedSourcePtr src = std::make_shared<jobq::TimerSourceMs>(
        jobq::TimerMode::REPEATING, 1ms, [&callback_cnt, &started, &gate]() {
            started.notify();
            gate.wait();
            callback_cnt++;
        });
    ex.registerSource(src);
    auto th = jobq::runExecutor(ex);
    REQUIRE(started.getFuture().wait_for(1s) == std::future_status::ready);
    // already signaled started
    // queued jobs not started now
    requireEventually(
        [&ex]() {
            auto stat = ex.getStats();
            return stat.jobs_submitted > stat.jobs_executed;
        },
        "more jobs submitted than executed");
    auto stat = ex.getStats();
    REQUIRE(stat.jobs_submitted > stat.jobs_executed);
    ex.shutdownAndDrain();
    gate.open(); // let them run to finish
    th.join();
    stat = ex.getStats();
    REQUIRE(stat.jobs_executed == stat.jobs_submitted);
}

TEST_CASE("shutdown discards already queued callbacks") {
    jobq::Executor ex{};
    std::atomic_int callback_cnt{};
    Signal started{};
    Gate gate{};
    jobq::SharedSourcePtr src = std::make_shared<jobq::TimerSourceMs>(
        jobq::TimerMode::REPEATING, 1ms, [&callback_cnt, &started, &gate]() {
            started.notify();
            gate.wait();
            callback_cnt++;
        });
    ex.registerSource(src);
    auto th = jobq::runExecutor(ex);
    REQUIRE(started.getFuture().wait_for(2s) == std::future_status::ready);
    requireEventually(
        [&ex]() {
            auto stat = ex.getStats();
            return stat.jobs_submitted > stat.jobs_executed + 10;
        },
        "more jobs submitted than executed");
    ex.shutdown();
    gate.open();
    th.join();
    auto stat = ex.getStats();
    REQUIRE(stat.jobs_submitted > stat.jobs_executed);
    REQUIRE(stat.jobs_executed == 1);
    REQUIRE(stat.jobs_discarded == stat.jobs_submitted - 1);
}

TEST_CASE("triggerSource trigger multiple times") {
    jobq::Executor ex{};
    std::atomic_int callback_cnt{};
    Signal signal{};
    constexpr auto N = 99;
    auto src = std::make_shared<jobq::TriggerSource>(
        "MulitpleTimeTrigger", [&callback_cnt, &signal]() {
            callback_cnt.fetch_add(1);
            if (callback_cnt == N) {
                signal.notify();
            }
        });

    ex.registerSource(src);

    auto th = jobq::runExecutor(ex);

    for (int i = 0; i < N; i++) {
        src->trigger();
    }
    signal.getFuture().wait_for(1s);
    ex.shutdownAndDrain();
    th.join();

    REQUIRE(callback_cnt == N);
}
