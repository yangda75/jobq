#include "Worker.h"
#include "JobQ.h"
#include <catch2/catch_test_macros.hpp>
#include <mutex>
#include <thread>

TEST_CASE("worker can work") {
    jobq::Q q{};
    jobq::Worker worker{q};

    int sum = 0;
    for (int i = 0; i < 100; i++) {
        q.pushJob({.fn = [i, &sum]() { sum += i + 1; }});
    }
    worker.runUntilEmpty();
    REQUIRE(sum == 5050);
}

TEST_CASE("multiple workers") {
    jobq::Q q{};

    constexpr auto JOB_CNT = 1000;
    std::vector<int> result_slots{};
    result_slots.resize(JOB_CNT);
    for (auto i = 0; i < JOB_CNT; i++) {
        q.pushJob({.fn = [i, &result_slots]() { result_slots[i] = i; }});
    }
    constexpr auto WORKER_CNT = 10;
    std::vector<std::thread> workers;
    for (auto i = 0; i < WORKER_CNT; i++) {
        workers.emplace_back([&q]() {
            jobq::Worker w{q};
            w.runUntilEmpty();
        });
    }
    for (auto &t : workers) {
        t.join();
    }
    for (auto i = 0; i < JOB_CNT; i++) {
        REQUIRE(result_slots[i] == i);
    }
}
