#pragma once

#include "Job.h"
#include <memory>

namespace jobq {
class Q
{
public:
  // synchronized push, will block until push success
  void pushJob(Job);
  // synchronized pull, only one, will block until success
  Job popOne();
  // non blocking pull, throw if empty
  Job popOneOrThrow();
  Q();
  ~Q();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
