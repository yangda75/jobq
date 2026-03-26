#pragma once
#include "Source.h"

#include <memory>

namespace jobq {
/// 1.wait 2.take 3.execute
class Executor {
  public:
    void registerSource(Source &);
    void run(); // like

    Executor();
    ~Executor();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace jobq
