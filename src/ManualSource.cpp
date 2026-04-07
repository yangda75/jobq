#include "ManualSource.h"

namespace jobq {

ManualSource::ManualSource(std::string id, Job job) : Source{id}, job_{job} {}

bool ManualSource::isReady() { return false; }
std::optional<Job> ManualSource::takeJob() { return std::nullopt; }
bool ManualSource::isFinished() { return false; }
void ManualSource::stop() {}

} // namespace jobq
