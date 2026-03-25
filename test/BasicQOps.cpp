#include "JobQ.h"
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <thread>

TEST_CASE("test basic push operation compiles") {
    jobq::Q q{};
    auto job = []() {}; // empty lambda
    q.pushJob(job);
}

TEST_CASE("test job can be popped") {
    jobq::Q q{};
    int a = 123;
    auto add_one = [&a]() { a += 1; };
    q.pushJob(add_one);
    auto job = q.popOne();
    (*job)();
    REQUIRE(a == 123 + 1);
}

TEST_CASE("test 100 jobs") {
    jobq::Q q{};
    int N = 100;
    int sum = 0;
    for (auto i = 0; i < N; i++) {
        auto job = [i, &sum]() {
            std::cout << "val : " << i << "\n";
            sum += i + 1;
        };
        q.pushJob(std::move(job));
    }
    for (auto i = 0; i < N; i++) {
        auto job = q.popOne();
        (*job)();
        std::cout << "sum : " << sum << "\n";
    }
    REQUIRE(sum == 5050);
}

TEST_CASE("close unblocks threads blocked in pop") {
    jobq::Q q{};
    std::atomic_bool started{false};
    std::atomic_bool ended{};
    std::optional<jobq::Job> j{std::nullopt};
    std::thread consumer{[&started, &ended, &q, &j]() {
        started = true;
        j = q.popOne();
        ended = true;
    }};

    using namespace std::chrono_literals;
    while (!started) {
        std::this_thread::sleep_for(10ms);
    }
    q.close();

    consumer.join();
    REQUIRE(j == std::nullopt);
    REQUIRE(started);
    REQUIRE(ended);
}

TEST_CASE("push after close returns false") {
    jobq::Q q{};

    auto push_before_close = q.pushJob([]() { std::cout << 1; });
    REQUIRE(push_before_close);

    q.close();
    auto push_after_close = q.pushJob([]() { std::cout << 2; });
    REQUIRE(push_after_close == false);
}

TEST_CASE("drain after close") {
    jobq::Q q{};
    constexpr int N = 10;
    for (int i = 0; i < N; i++) {
        REQUIRE(q.pushJob([i]() { std::cout << i; }));
    }

    q.close();
    for (int i = 0; i < N; i++) {
        auto popped = q.popOne();
        REQUIRE(popped.has_value());
    }

    auto pop_after_close_from_empty_q = q.popOne();
    REQUIRE(pop_after_close_from_empty_q == std::nullopt);
}
