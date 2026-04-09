#pragma once
#include "Job.h"
#include "Source.h"

#include <memory>

namespace jobq {
struct Stats {
    int jobs_submitted;
    int jobs_executed;
    int jobs_discarded;
    int queue_depth;
    int active_workers;
};
/// 1.wait 2.take 3.execute
class Executor {
  public:
    void registerSource(std::shared_ptr<Source>);
    bool submitJob(JobFn j);
    // like spin, one-shot, blocking
    void run();
    // stop accepting new task, finish current task, and discard remaining tasks
    void shutdown();
    // stop accepting new task, and drain remaining tasks
    void shutdownAndDrain();

    explicit Executor(size_t num_threads = 1);
    ~Executor();
    Stats getStats() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace jobq
