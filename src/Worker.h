#pragma once
#include "JobQ.h"
#include <atomic>
#include <memory>
#include <stop_token>

namespace jobq {
class Worker {
  public:
    // return job count finished
    int runUntilEmpty(std::stop_token token);
    int runForever(std::stop_token token);

    explicit Worker(Q &);
    ~Worker();
    Worker(Worker &&);
    Worker &operator=(Worker &&);

    void setExecutedJobCounter(std::atomic_int &);
    void setActiveWorkerCounter(std::atomic_int &);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace jobq
