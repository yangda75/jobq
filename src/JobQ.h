#pragma once

#include "Job.h"
#include <memory>
#include <optional>

namespace jobq {
class Q {
  public:
    // synchronized push, will block until push success
    bool pushJob(Job);
    // synchronized pull, only one, will block until success
    // if q is closed, return nullopt
    std::optional<Job> popOne();

    // blocking with timeout
    std::optional<Job> popOneFor(int timeout_ms);

    // after close, no push is accepted
    void close();

    Q();
    ~Q();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace jobq
