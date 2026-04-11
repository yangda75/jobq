#include "Executor.h"
#include "JobQ.h"
#include "Log.h"
#include "Source.h"
#include "ThreadName.h"
#include "Worker.h"
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

namespace jobq {

struct Executor::Impl {
    std::atomic_uint64_t jid{};
    std::atomic_int jobs_submitted{};
    std::atomic_int jobs_executed{};
    std::atomic_int jobs_discarded{};
    std::atomic_int active_workers{};
    Q q{};
    std::vector<std::thread> worker_threads{};
    std::vector<Worker> workers{};
    std::mutex m{}; // guards access to workers vector, not worker_threads
    std::thread dispatcher{};
    std::atomic_bool stopped{false};
    size_t nthreads{};
    std::mutex source_mtx{};
    std::condition_variable source_cv{};

    std::deque<SharedSourcePtr> ready_sources{};
    std::vector<std::shared_ptr<Source>> sources{};

    explicit Impl(size_t num_threads = 1) : nthreads{num_threads} {}
    void fetchAndDispatch() {
        while (true) {
            std::unique_lock uniqlock{source_mtx};
            source_cv.wait(uniqlock, [this]() {
                return !ready_sources.empty() || stopped;
            });
            if (stopped) {
                loginfo("fetchAndDispatch done, stopped");
                return;
            }
            // loginfo("fetchAndDispatch wakeup, number of ready srcs: {}",
            //         ready_sources.size());
            while (!ready_sources.empty()) {
                auto src = ready_sources.front();
                ready_sources.pop_front();

                auto job = src->takeJob();
                if (!job) {
                    loginfo("fetchAndDispatch got no job");
                    continue;
                }
                job->id = ++jid; // set id here, make it easy
                if (q.pushJob(*job)) {
                    jobs_submitted++;
                }
            }
            uniqlock.unlock();
            auto all_source_finished = true;
            for (auto const &src : sources) {
                if (!src->isFinished()) {
                    all_source_finished = false;
                    break;
                }
            }
            if (all_source_finished) {
                loginfo("fetchAndDispatch done, all source finished");
                return;
            }
        }
    }
    void run() {
        {
            std::lock_guard lk{m};
            workers.reserve(nthreads);
            // start worker threads
            for (int i = 0; i < nthreads; i++) {
                workers.emplace_back(q);
                auto &worker_i = workers[i];
                worker_i.setExecutedJobCounter(jobs_executed);
                worker_i.setActiveWorkerCounter(active_workers);
                worker_threads.emplace_back(std::thread{[&worker_i, i]() {
                    setCurrentThreadName("jobq-worker-" + std::to_string(i));
                    worker_i.runForever();
                }});
            }
        }
        // start dispatcher
        dispatcher = std::thread{[this]() {
            setCurrentThreadName("jobq-dispatcher");
            fetchAndDispatch();
        }};
        dispatcher.join();
        for (auto &t : worker_threads) {
            t.join();
        }
    }

    void shutdownAndDrain() {
        q.close();
        stopped = true;
        source_cv.notify_all();
        // shutdown all sources
        std::lock_guard lk{m};
        for (auto src : sources) {
            loginfo("stopping source: {}", src->id());
            src->stop();
        }
        // runForever 会执行完剩下的任务
    }

    bool submitJob(JobFn f) {
        jobs_submitted++;
        // atomic_counter for job id
        // build job
        Job j{};
        j.fn = f;
        j.id = ++jid;
        return q.pushJob(j);
    }

    void shutdown() {
        q.close();
        stopped = true;
        source_cv.notify_all();
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
            jobs_discarded++;
        }
    }

    void registerSource(std::shared_ptr<Source> src) {
        std::lock_guard lk{m};
        src->setReadyCallback([this, src]() {
            std::lock_guard lk{source_mtx};
            ready_sources.push_back(src);
            source_cv.notify_one();
        });
        sources.push_back(std::move(src));
    }

    void prepareForDestruction() {
        // stop all sources
        if (!stopped) {
            shutdown();
        }
        if (dispatcher.joinable()) {
            dispatcher.join();
        }
    }

    Stats getStats() {
        Stats s{};
        s.active_workers = active_workers.load();
        s.jobs_discarded = jobs_discarded.load();
        s.jobs_executed = jobs_executed.load();
        s.jobs_submitted = jobs_submitted.load();
        s.queue_depth = q.getDepth();

        return s;
    }
};

/// api

Executor::Executor(size_t num_threads)
    : impl_{std::make_unique<Impl>(num_threads)} {}

Executor::~Executor() { impl_->prepareForDestruction(); };

void Executor::registerSource(std::shared_ptr<Source> src) {
    impl_->registerSource(src);
}

void Executor::run() { impl_->run(); }

bool Executor::submitJob(JobFn j) { return impl_->submitJob(j); }

void Executor::shutdown() { impl_->shutdown(); }

void Executor::shutdownAndDrain() { impl_->shutdownAndDrain(); }

Stats Executor::getStats() const { return impl_->getStats(); }

} // namespace jobq
