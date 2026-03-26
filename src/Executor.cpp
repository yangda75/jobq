#include "Executor.h"
#include "JobQ.h"
#include "Worker.h"
#include <thread>
#include <vector>

namespace jobq {

constexpr auto WORKER_THREADS_PER_EXECUTOR = 1;

struct Executor::Impl {
    Q q{};
    std::vector<std::thread> worker_threads{};
    std::vector<Source *> sources{};

    void run() {
        // start worker threads
        for (int i = 0; i < WORKER_THREADS_PER_EXECUTOR; i++) {
            worker_threads.emplace_back(std::thread{[this, i]() {
                Worker w{q};
                // ? Do I need to use these workers in other place?
                w.runForever();
            }});
        }
        for (auto &t : worker_threads) {
            t.join();
        }
    }

    void shutdownAndDrain() {
        q.close();
        // runForever 会执行完剩下的任务
    }

    void submitJob(Job j) { q.pushJob(j); }
};

/// api

Executor::Executor() : impl_{std::make_unique<Impl>()} {}
Executor::~Executor() = default;

void Executor::registerSource(Source *) {
    // TODO
}

void Executor::run() { impl_->run(); }

void Executor::submitJob(Job j) { impl_->submitJob(j); }

void Executor::shutdown() {
    // TODO
}

void Executor::shutdownAndDrain() { impl_->shutdownAndDrain(); }

} // namespace jobq
