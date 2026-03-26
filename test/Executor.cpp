#include "Executor.h"
#include <atomic>
#include <catch2/catch_test_macros.hpp>
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
