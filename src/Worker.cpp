#include "Worker.h"
#include "EmptyQ.h"
#include <iostream>

namespace jobq {
struct Worker::Impl
{
  Impl(Q& q)
    : q_ref{ q }
  {
  }
  void runUntilEmpty()
  {
    std::cout << "Worker started!\n";
    while (true) {
      try {
        auto job = q_ref.popOneOrThrow();
        job();
      } catch (EmptyQ const&) {
        return;
      } catch (std::exception const& e) {
        std::cerr << "Failed to run job, exception: " << e.what();
      }
    }
  }
  void runForever()
  {
    while (auto job = q_ref.popOne()) {
      try {
        job();
      } catch (std::exception const& e) {
        std::cerr << "Failed to run job, exception: " << e.what();
      }
    }
  }

  Q& q_ref;
};

/// Worker interface

Worker::Worker(Q& q)
  : impl_{ std::make_unique<Impl>(q) }
{
}
Worker::~Worker() = default;

void
Worker::runUntilEmpty()
{
  impl_->runUntilEmpty();
}

void
Worker::runForever()
{
  impl_->runForever();
}

}
