#include "JobQ.h"
#include "Worker.h"
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <thread>

TEST_CASE("bursted")
{
  // N producers, M consumers
  int N = 10;
  int M = 8;

  std::vector<std::thread> producers;
  std::vector<std::thread> consumers;

  std::atomic_int produced_cnt{ 0 };
  std::atomic_int consumed_cnt{ 0 };

  jobq::Q q{};

  for (int i = 0; i < N; i++) {
    for (int j = 0; j < 10; j++) {

      producers.emplace_back([&produced_cnt, &consumed_cnt, &q]() {
        produced_cnt++;
        q.pushJob([&consumed_cnt]() { consumed_cnt++; });
      });
    }
  }
  for (int i = 0; i < M; i++) {
    consumers.emplace_back([&q] {
      jobq::Worker worker{ q };
      worker.runUntilEmpty();
    });
  }

  // wait for finish
  for (auto& t : producers) {
    t.join();
  }
  for (auto& t : consumers) {
    t.join();
  }
  REQUIRE(produced_cnt == consumed_cnt);
}
