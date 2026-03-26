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
    std::vector<Worker> workers{};

    void run() {
        // start worker threads
        for (int i = 0; i < WORKER_THREADS_PER_EXECUTOR; i++) {
            workers.emplace_back(q);
            worker_threads.emplace_back(std::thread{[this, i]() {
                auto &w = workers[i];
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

    bool submitJob(Job j) { return q.pushJob(j); }

    void shutdown() {
        q.close();
        for (auto &w : workers) {
            w.stop();
        }
        // drain the q, discarding
        while (auto job = q.popOne()) {
            // TODO log metadata on discard
        }
    }
};

/// api

Executor::Executor() : impl_{std::make_unique<Impl>()} {}
Executor::~Executor() = default;

void Executor::registerSource(Source *) {
    // TODO
}

void Executor::run() { impl_->run(); }

bool Executor::submitJob(Job j) { return impl_->submitJob(j); }

void Executor::shutdown() { impl_->shutdown(); }

void Executor::shutdownAndDrain() { impl_->shutdownAndDrain(); }

} // namespace jobq
