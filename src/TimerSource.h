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

    bool isReady() override { return true; }
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
        stopped_ = true;
        cv_.notify_all();
        timer_thread_.join();
    }

    void setReadyCallback(std::function<void()> cb) override {
        Source::setReadyCallback(cb);
        timer_thread_ = std::thread{[this]() { timerLoop(); }};
    }

    ~TimerSource() {
        if (!isFinished()) {
            stop();
        }
    }

  private:
    void timerLoop() {
        while (!stopped_) {
            std::unique_lock uniqlock{mtx_};

            cv_.wait_for(uniqlock, timeout_,
                         [this]() { return stopped_.load(); });
            if (stopped_) {
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
    std::thread timer_thread_{};
    std::mutex mtx_{};
    std::condition_variable cv_{};
};

using TimerSourceMs = TimerSource<int64_t, std::milli>;
} // namespace jobq
