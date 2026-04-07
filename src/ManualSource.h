#pragma once

#include "Source.h"

namespace jobq {

class ManualSource final : public Source {
  public:
    ManualSource(std::string id, Job job);
    bool isReady() override;
    std::optional<Job> takeJob() override;
    bool isFinished() override;
    void stop() override;

    ~ManualSource() = default;

  private:
    Job job_;
};
} // namespace jobq
