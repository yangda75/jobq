#pragma once
#include "JobQ.h"
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
    Worker(Worker&&);
    Worker& operator=(Worker&&);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace jobq
