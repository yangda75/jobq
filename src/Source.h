#pragma once
#include "Job.h"
#include <optional>

namespace jobq {
/// 任务来源
class Source {
  public:
    std::optional<Job> tryTakeJob() {
        if (!isReady()) {
            return std::nullopt;
        }
        return takeJob();
    }

    explicit Source(std::string id) : id_{std::move(id)} {}

    virtual std::string id() const { return id_; }
    virtual bool isReady() = 0;
    virtual std::optional<Job> takeJob() = 0;
    virtual bool isFinished() = 0;
    virtual void stop() = 0;
    virtual ~Source() = default;

  private:
    std::string id_{};
};

using SharedSourcePtr = std::shared_ptr<Source>;

} // namespace jobq
