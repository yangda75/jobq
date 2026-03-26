#include "Executor.h"

namespace jobq {
struct Executor::Impl {};

/// api

Executor::Executor() : impl_{std::make_unique<Impl>()} {}

void Executor::registerSource(Source &) {
    // TODO
}

void run() {
    // TODO
}

} // namespace jobq
