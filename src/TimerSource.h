#pragma once
#include "Source.h"
#include <chrono>

namespace jobq {
class TimerSource final : public Source {

  public:
    enum class Mode {
        ONE_SHOT,
        REPEATING,
    };

    TimerSource(Mode mode, int timeout_ms, Job job);

    bool isReady() override;
    std::optional<Job> takeJob() override;
    bool isFinished() override;
    void stop() override;

    ~TimerSource();

  private:
    int timeout_ms_{};
    Mode mode_{};
    std::optional<Job> ready_job_{};
    std::atomic_bool stopped_{};
    std::chrono::system_clock::time_point start_time_{};
    Job job_{};
    std::atomic_bool finished_{};
};
} // namespace jobq
