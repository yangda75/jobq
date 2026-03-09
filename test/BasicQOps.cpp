#include "EmptyQ.h"
#define CATCH_CONFIG_MAIN
#include "JobQ.h"
#include <catch2/catch_test_macros.hpp>
#include <iostream>

TEST_CASE("test basic push operation compiles")
{
  jobq::Q q{};
  auto job = []() {}; // empty lambda
  q.pushJob(job);
}

TEST_CASE("test job can be popped")
{
  jobq::Q q{};
  int a = 123;
  auto add_one = [&a]() { a += 1; };
  q.pushJob(add_one);
  auto job = q.popOne();
  job();
  REQUIRE(a == 123 + 1);
}

TEST_CASE("test 100 jobs")
{
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
    job();
    std::cout << "sum : " << sum << "\n";
  }
  REQUIRE(sum == 5050);
}

TEST_CASE("empty q throw empty q")
{
  jobq::Q q{};
  REQUIRE_THROWS_AS(q.popOneOrThrow(), jobq::EmptyQ);
}
