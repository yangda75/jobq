#include "TimerSource.h"
#include <chrono>
#include <thread>

namespace jobq {

TimerSource::TimerSource(Mode mode, int timeout_ms, Job job)
    : Source{"TimerSource"}, mode_{mode}, timeout_ms_{timeout_ms}, job_{job} {
    start_time_ = std::chrono::steady_clock::now();
}

std::optional<Job> TimerSource::takeJob() {
    if (stopped_ || finished_) {
        return std::nullopt;
    }
    if (mode_ == Mode::ONE_SHOT) {
        finished_ = true;
    }
    return job_;
}

void TimerSource::stop() {
    stopped_ = true;
    cv_.notify_all();
    timer_thread_.join();
}

bool TimerSource::isReady() {
    auto deltat = std::chrono::steady_clock::now() - start_time_;
    auto millis =
        std::chrono::duration_cast<std::chrono::milliseconds>(deltat).count();
    return millis >= timeout_ms_;
}

bool TimerSource::isFinished() { return finished_ || stopped_; }

TimerSource::~TimerSource() {
    if (!stopped_) {
        stop();
    }
}

void TimerSource::setReadyCallback(std::function<void()> cb) {
    Source::setReadyCallback(cb);
    timer_thread_ = std::thread{[this]() { timerLoop(); }};
}

void TimerSource::timerLoop() {
    while (!stopped_) {
        std::unique_lock uniqlock{mtx_};

        cv_.wait_for(uniqlock, std::chrono::milliseconds(timeout_ms_),
                     [this]() { return stopped_.load(); });
        if (stopped_) {
            break;
        }
        ready_callback_();
        if (mode_ == Mode::ONE_SHOT) {
            break;
        }
    }
}

} // namespace jobq
