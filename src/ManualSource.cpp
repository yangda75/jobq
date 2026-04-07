#include "ManualSource.h"

namespace jobq {

ManualSource::ManualSource(std::string id, Job job) : Source{id}, job_{job} {}

bool ManualSource::isReady() { return true; }
std::optional<Job> ManualSource::takeJob() {
    if (finished_ || stopped_) {
        return std::nullopt;
    }
    finished_ = true;
    return job_;
}
bool ManualSource::isFinished() { return finished_; }
void ManualSource::stop() { stopped_ = true; }

} // namespace jobq
