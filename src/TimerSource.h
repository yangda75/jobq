#pragma once
#include "Source.h"
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

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

    void setReadyCallback(std::function<void()> cb) override;

    ~TimerSource();

  private:
    void timerLoop();
    int timeout_ms_{};
    Mode mode_{};
    std::atomic_bool stopped_{};
    std::chrono::steady_clock::time_point start_time_{};
    Job job_{};
    std::atomic_bool finished_{};
    std::thread timer_thread_{};
    std::mutex mtx_{};
    std::condition_variable cv_{};
};
} // namespace jobq
