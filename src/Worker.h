#pragma once
#include "JobQ.h"
#include <atomic>
#include <memory>

namespace jobq {
class Worker {
  public:
    // return job count finished
    int runUntilEmpty();
    int runForever();
    void stop();

    explicit Worker(Q &);
    ~Worker();
    Worker(Worker &&);
    Worker &operator=(Worker &&);

    void setExecutedJobCounter(std::atomic_int &count);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace jobq
