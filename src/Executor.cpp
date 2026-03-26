#include "Executor.h"
#include "JobQ.h"
#include "Worker.h"
#include <vector>

namespace jobq {

struct Executor::Impl {
    Q q{};
    std::vector<Worker> workers{};
    std::vector<Source *> sources{};
};

/// api

Executor::Executor() : impl_{std::make_unique<Impl>()} {}
Executor::~Executor() = default;

void Executor::registerSource(Source *) {
    // TODO
}

void Executor::run() {
    // TODO
}

void Executor::submitJob(Job j) {
    // TODO
}

void Executor::shutdown() {
    // TODO
}

void Executor::shutdownAndDrain() {
    // TODO
}

} // namespace jobq
