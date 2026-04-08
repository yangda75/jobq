#include "Executor.h"
#include "JobQ.h"
#include "Log.h"
#include "Worker.h"
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

namespace jobq {

struct Executor::Impl {
    Q q{};
    std::vector<std::thread> worker_threads{};
    std::vector<std::shared_ptr<Source>> sources{};
    std::vector<Worker> workers{};
    std::mutex m{}; // guards access to workers vector, not worker_threads
    std::thread dispatcher{};
    std::atomic_bool stopped{false};
    size_t nthreads{};
    explicit Impl(size_t num_threads = 1) : nthreads{num_threads} {}
    void fetchAndDispatch() {
        while (!stopped) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            auto all_finished = true;
            auto sources_copy = decltype(sources){};
            {
                std::lock_guard lk{m};
                sources_copy = sources;
            }
            for (auto &src : sources_copy) {
                loginfo("checking src: {}", src->id());
                if (src->isFinished()) {
                    loginfo("src: {} finished", src->id());
                    continue;
                }
                all_finished = false;
                if (src->isReady()) {
                    loginfo("src: {} ready", src->id());
                    if (auto job = src->takeJob()) {
                        loginfo("src: {} got job", src->id());
                        submitJob(*job);
                    }
                }
            }
            if (all_finished) {
                break;
            }
        }
        loginfo("fetchAndDispatch done");
    }
    void run() {
        {
            std::lock_guard lk{m};
            workers.reserve(nthreads);
            // start worker threads
            for (int i = 0; i < nthreads; i++) {
                workers.emplace_back(q);
                worker_threads.emplace_back(std::thread{[this, i]() {
                    auto &w = workers[i];
                    w.runForever();
                }});
            }
        }
        // start dispatcher
        dispatcher = std::thread{[this]() { fetchAndDispatch(); }};
        dispatcher.join();
        for (auto &t : worker_threads) {
            t.join();
        }
    }

    void shutdownAndDrain() {
        q.close();
        stopped = true;
        // shutdown all sources
        std::lock_guard lk{m};
        for (auto src : sources) {
            loginfo("stopping source: {}", src->id());
            src->stop();
        }
        // runForever 会执行完剩下的任务
    }

    bool submitJob(Job j) { return q.pushJob(j); }

    void shutdown() {
        q.close();
        stopped = true;
        {
            std::lock_guard lk{m};
            for (auto src : sources) {
                src->stop();
            }
            for (auto &w : workers) {
                w.stop();
            }
        }
        // drain the q, discarding
        while (auto job = q.popOne()) {
            // TODO log metadata on discard
        }
    }

    void registerSource(std::shared_ptr<Source> src) {
        std::lock_guard lk{m};
        loginfo("register source : {}", src->id());
        sources.push_back(src);
    }
};

/// api

Executor::Executor(size_t num_threads)
    : impl_{std::make_unique<Impl>(num_threads)} {}
Executor::~Executor() = default;

void Executor::registerSource(std::shared_ptr<Source> src) {
    impl_->registerSource(src);
}

void Executor::run() { impl_->run(); }

bool Executor::submitJob(Job j) { return impl_->submitJob(j); }

void Executor::shutdown() { impl_->shutdown(); }

void Executor::shutdownAndDrain() { impl_->shutdownAndDrain(); }

} // namespace jobq
