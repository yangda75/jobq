#pragma once
#include "Job.h"
#include "Source.h"

#include <memory>

namespace jobq {
/// 1.wait 2.take 3.execute
class Executor {
  public:
    void registerSource(Source*);
    void submitJob(Job j);
    // like spin
    void run();
    // stop accepting new task, and discard remaining tasks
    void shutdown();
    // stop accepting new task, and drain remaining tasks
    void shutdownAndDrain();

    Executor();
    ~Executor();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace jobq
