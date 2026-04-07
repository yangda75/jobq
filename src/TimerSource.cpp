#include "TimerSource.h"
#include <chrono>

namespace jobq {

TimerSource::TimerSource(Mode mode, int timeout_ms, Job job)
    : Source{"TimerSource"}, mode_{mode}, timeout_ms_{timeout_ms}, job_{job} {
    start_time_ = std::chrono::system_clock::now();
}

std::optional<Job> TimerSource::takeJob() {
    if (stopped_) {
        return std::nullopt;
    }
    if (isReady()) {
        ready_job_ = job_;
        // once the timer is fired, it is finished
        finished_ = true;
        return job_;
    }
    return ready_job_;
}

void TimerSource::stop() { stopped_ = true; }

bool TimerSource::isReady() {
    auto deltat = std::chrono::system_clock::now() - start_time_;
    auto millis =
        std::chrono::duration_cast<std::chrono::milliseconds>(deltat).count();
    return millis > timeout_ms_;
}

bool TimerSource::isFinished() { return finished_; }

TimerSource::~TimerSource() = default;

} // namespace jobq
