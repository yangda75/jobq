#include "JobQ.h"
#include "EmptyQ.h"
#include <chrono>
#include <iostream>
#include <list>
#include <mutex>
#include <thread>

namespace jobq {

/// impl
struct Q::Impl
{
  void pushJob(Job j)
  {
    std::lock_guard lk{ mtx };
    jobs.emplace_back(std::move(j));
  }
  Job popOne()
  {
    std::lock_guard lk{ mtx };
    while (jobs.empty()) {
      // sleep for some time, and check again
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(10ms);
      std::cout << "no job, waiting\n";
    }
    auto job = jobs.front(); // is this copy ok?
    jobs.pop_front();
    return job;
  }

  Job popOneOrThrow()
  {
    std::lock_guard lk{ mtx };
    if (jobs.empty()) {
      throw EmptyQ{};
    }
    auto job = jobs.front();
    jobs.pop_front();
    return job;
  }

  std::list<Job> jobs;
  std::mutex mtx;
};

/// Q

void
Q::pushJob(Job j)
{
  impl_->pushJob(std::move(j));
}

Job
Q::popOne()
{
  return impl_->popOne();
}

Job
Q::popOneOrThrow()
{
  return impl_->popOneOrThrow();
}

Q::Q()
  : impl_{ std::make_unique<Impl>() }
{
}
Q::~Q() = default;
}
