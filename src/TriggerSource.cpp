#include "TriggerSource.h"

namespace jobq {

TriggerSource::TriggerSource(std::string id, Job job)
    : Source{std::move(id)}, job_{job} {}

bool TriggerSource::isReady() { return true; }

std::optional<Job> TriggerSource::takeJob() {
    if (finished_ || stopped_) {
        return std::nullopt;
    }
    finished_ = true;
    return job_;
}
bool TriggerSource::isFinished() { return finished_; }
void TriggerSource::stop() { stopped_ = true; }

void TriggerSource::trigger() { ready_callback_(); }

} // namespace jobq
