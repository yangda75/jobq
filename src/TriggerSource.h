#pragma once

#include "Source.h"
#include <atomic>

namespace jobq {

class TriggerSource final : public Source {
  public:
    TriggerSource(std::string id, Job job);
    bool isReady() override;
    std::optional<Job> takeJob() override;
    bool isFinished() override;
    void stop() override;

    void trigger();

    ~TriggerSource() = default;

  private:
    Job job_;
    std::atomic_bool finished_;
    std::atomic_bool stopped_;
};
} // namespace jobq
