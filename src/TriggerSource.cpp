#include "TriggerSource.h"

namespace jobq {

TriggerSource::TriggerSource(std::string id, JobFn f)
    : Source{std::move(id)}, job_{.fn = f} {}

bool TriggerSource::isReady() { return true; }

std::optional<Job> TriggerSource::takeJob() {
    if (stopped_) {
        return std::nullopt;
    }
    return job_;
}

bool TriggerSource::isFinished() { return stopped_; }
void TriggerSource::stop() { stopped_ = true; }

void TriggerSource::trigger() { ready_callback_(); }

} // namespace jobq
