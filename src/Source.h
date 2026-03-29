#pragma once
#include "Job.h"
#include <optional>

namespace jobq {
/// 任务来源
class Source {
  public:
    virtual bool isReady() = 0;
    virtual std::optional<Job> takeJob() = 0;
    virtual bool isFinished() = 0;
    virtual void stop() = 0;
    virtual ~Source() = default;
};

} // namespace jobq
