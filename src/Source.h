#pragma once
#include "Job.h"
#include <optional>
#include <string>
#include <memory>

namespace jobq {
/// 任务来源
class Source {
  public:

    virtual void setReadyCallback(std::function<void()> cb) {
        ready_callback_ = cb;
    }

    explicit Source(std::string id) : id_{std::move(id)} {}

    virtual std::string id() const { return id_; }
    virtual std::optional<Job> takeJob() = 0;
    virtual bool isFinished() = 0;
    virtual void stop() = 0;
    virtual ~Source() = default;

  protected:
    std::function<void()> ready_callback_{};

  private:
    std::string id_{};
};

using SharedSourcePtr = std::shared_ptr<Source>;

} // namespace jobq
