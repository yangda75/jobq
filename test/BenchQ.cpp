#include "JobQ.h"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <thread>

TEST_CASE("Q push/pop throughput") {
    constexpr int PRODUCERS = 4, CONSUMERS = 4, N = 100000;

    BENCHMARK("list-backed") {
        jobq::Q q;
        std::atomic_int count{0};
        std::vector<std::thread> producer_threads;
        for (int i = 0; i < PRODUCERS; i++) {
            producer_threads.push_back(std::thread{[&count, &q]() {
                for (int j = 0; j < N; j++) {
                    q.pushJob({.fn = [&count]() { count++; }});
                }
            }});
        }
        for (auto &th : producer_threads) {
            th.join();
        }
        q.close();
        std::vector<std::thread> consumer_threads;
        for (int i = 0; i < CONSUMERS; i++) {
            consumer_threads.push_back(std::thread{[&q]() {
                while (auto job = q.popOneFor(10)) {
                    (job->fn)();
                }
            }});
        }
        for (auto &th : consumer_threads) {
            th.join();
        }
        REQUIRE(count == N * PRODUCERS);
    };
}
