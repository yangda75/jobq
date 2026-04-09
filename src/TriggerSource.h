#pragma once

#include "Source.h"
#include <atomic>

namespace jobq {

class TriggerSource final : public Source {
  public:
    TriggerSource(std::string id, JobFn f);
    bool isReady() override;
    std::optional<Job> takeJob() override;
    bool isFinished() override;
    void stop() override;

    void trigger();

    ~TriggerSource() = default;

  private:
    Job job_; // job id is determined in executor
    std::atomic_bool finished_{false};
    std::atomic_bool stopped_{false};
};
} // namespace jobq
