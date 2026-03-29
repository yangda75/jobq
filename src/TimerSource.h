#pragma once
#include "Source.h"

namespace jobq {
class TimerSource final : public Source {

  public:
    enum class Mode {
        ONE_SHOT,
        REPEATING,
    };

    TimerSource(Mode mode, int timeout_ms);

    bool isReady() override;
    std::optional<Job> takeJob() override;
    bool isFinished() override;
    void stop() override;

    ~TimerSource();

  private:
    int timeout_ms_{};
    Mode mode_{};
    std::optional<Job> ready_job_{};
    bool stopped_{};
};
} // namespace jobq
