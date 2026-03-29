#include "Executor.h"
#include "JobQ.h"
#include "Worker.h"
#include <mutex>
#include <thread>
#include <vector>

namespace jobq {

constexpr auto WORKER_THREADS_PER_EXECUTOR = 1;

struct Executor::Impl {
    Q q{};
    std::vector<std::thread> worker_threads{};
    std::vector<Source *> sources{};
    std::vector<Worker> workers{};
    std::mutex m{}; // guards access to workers vector, not worker_threads
    void run() {
        {
            std::lock_guard lk{m};
            // start worker threads
            for (int i = 0; i < WORKER_THREADS_PER_EXECUTOR; i++) {
                workers.emplace_back(q);
                worker_threads.emplace_back(std::thread{[this, i]() {
                    auto &w = workers[i];
                    w.runForever();
                }});
            }
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
        {
            std::lock_guard lk{m};
            for (auto &w : workers) {
                w.stop();
            }
        }
        // drain the q, discarding
        while (auto job = q.popOne()) {
            // TODO log metadata on discard
        }
    }

    void registerSource(Source *src) {
        std::lock_guard lk{m};
        sources.push_back(src);
    }
};

/// api

Executor::Executor() : impl_{std::make_unique<Impl>()} {}
Executor::~Executor() = default;

void Executor::registerSource(Source *src) { impl_->registerSource(src); }

void Executor::run() { impl_->run(); }

bool Executor::submitJob(Job j) { return impl_->submitJob(j); }

void Executor::shutdown() { impl_->shutdown(); }

void Executor::shutdownAndDrain() { impl_->shutdownAndDrain(); }

} // namespace jobq
