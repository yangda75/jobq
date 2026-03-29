#include "TimerSource.h"

namespace jobq {

TimerSource::TimerSource(Mode mode, int timeout_ms)
    : mode_{mode}, timeout_ms_{timeout_ms} {}

std::optional<Job> TimerSource::takeJob() {
    if (stopped_) {
        return std::nullopt;
    }
    return ready_job_;
}

void TimerSource::stop() { stopped_ = true; }

bool TimerSource::isReady() { return false; }
bool TimerSource::isFinished() { return false; }

TimerSource::~TimerSource() = default;

} // namespace jobq
