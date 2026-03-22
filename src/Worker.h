#pragma once
#include "JobQ.h"
#include <memory>

namespace jobq {
class Worker {
  public:
    // return job count finished
    int runUntilEmpty();
    void runForever();

    explicit Worker(Q &);
    ~Worker();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace jobq
