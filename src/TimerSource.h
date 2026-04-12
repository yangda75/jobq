#pragma once
#include "Source.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

namespace jobq {
enum class TimerMode {
    ONE_SHOT,
    REPEATING,
};

template <typename Rep, class Period> class TimerSource final : public Source {
  public:
    TimerSource(TimerMode mode, std::chrono::duration<Rep, Period> timeout,
                JobFn f)
        : Source{"TimerSource"}, mode_{mode}, timeout_{timeout}, job_{.fn = f},
          start_time_{std::chrono::steady_clock::now()} {}

    std::optional<Job> takeJob() override {
        if (stopped_ || finished_) {
            return std::nullopt;
        }
        if (mode_ == TimerMode::ONE_SHOT) {
            finished_ = true;
        }
        return job_;
    }

    bool isFinished() override { return finished_ || stopped_; }
    void stop() override {
        timer_thread_.request_stop();
        stopped_ = true;
    }

    void setReadyCallback(std::function<void()> cb) override {
        Source::setReadyCallback(cb);
        auto f = std::bind_front(&TimerSource::timerLoop, this);
        timer_thread_ = std::jthread{f};
    }

    ~TimerSource() override = default;

  private:
    void timerLoop(std::stop_token token) {
        while (!token.stop_requested()) {
            std::unique_lock uniqlock{mtx_};

            // when stop is requested, wake up
            cv_.wait_for(uniqlock, token, timeout_, []() { return false; });
            if (token.stop_requested()) {
                break;
            }
            ready_callback_();
            if (mode_ == TimerMode::ONE_SHOT) {
                break;
            }
        }
    }
    std::chrono::duration<Rep, Period> timeout_{};
    TimerMode mode_{};
    std::atomic_bool stopped_{};
    std::chrono::steady_clock::time_point start_time_{};
    Job job_{}; // job id is determined in executor
    std::atomic_bool finished_{};
    std::jthread timer_thread_{};
    std::mutex mtx_{};
    std::condition_variable_any cv_{};
};

using TimerSourceMs = TimerSource<int64_t, std::milli>;
} // namespace jobq
