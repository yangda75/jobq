#include "Worker.h"
#include "Log.h"
#include <atomic>

namespace jobq {

struct CounterGuard {
    explicit CounterGuard(std::atomic_int *counter_in) : counter{counter_in} {
        if (counter) {
            loginfo("new active worker");
            counter->fetch_add(1, std::memory_order_relaxed);
        }
    }
    ~CounterGuard() {
        if (counter) {
            loginfo("active worker done");
            counter->fetch_sub(1, std::memory_order_relaxed);
        }
    }
    std::atomic_int *counter;
};

struct Worker::Impl {
    std::atomic_bool stopped{};
    std::atomic_int *counter{};
    std::atomic_int *active_worker_counter{};
    Impl(Q &q) : q_ref{q} {}
    int runUntilEmpty() {
        loginfo("worker started");
        if (stopped) {
            logerror("cannot restart stopped worker");
            return 0;
        }
        CounterGuard counter_g{active_worker_counter};
        while (true) {
            try {
                auto job = q_ref.popOneFor(1);
                if (!job.has_value()) {
                    break;
                }
                (job->fn)();
                if (counter) {
                    (*counter)++;
                }
                job_cnt++;
            } catch (std::exception const &e) {
                logerror("Failed to run job, exception: {}", e.what());
            }
        }
        loginfo("worker done after {} jobs", job_cnt);
        return job_cnt;
    }
    int runForever() {
        loginfo("worker start");
        if (stopped) {
            logerror("cannot restart stopped worker");
            return 0;
        }
        CounterGuard counter_g{active_worker_counter};
        while (auto job = q_ref.popOne()) {
            if (stopped) {
                // a job is poped from the q, but will not run, what should I do
                // here?
                break;
            }
            try {
                (job->fn)();
                if (counter) {
                    (*counter)++;
                }
                job_cnt++;
            } catch (std::exception const &e) {
                logerror("Failed to run job, exception: {}", e.what());
            }
        }
        loginfo("worker done");
        return job_cnt;
    }

    void stop() { stopped = true; }
    void setExecutedJobCounter(std::atomic_int &counter_in) {
        counter = &counter_in;
    }
    void setActiveWorkerCounter(std::atomic_int &counter_in) {
        active_worker_counter = &counter_in;
    }

    Q &q_ref;
    int job_cnt{};
};

/// Worker interface

Worker::Worker(Q &q) : impl_{std::make_unique<Impl>(q)} {}
Worker::~Worker() = default;

Worker::Worker(Worker &&rhs) : impl_{std::move(rhs.impl_)} {}
Worker &Worker::operator=(Worker &&rhs) {
    impl_ = std::move(rhs.impl_);
    return *this;
}

int Worker::runUntilEmpty() { return impl_->runUntilEmpty(); }

int Worker::runForever() { return impl_->runForever(); }

void Worker::stop() { impl_->stop(); }

void Worker::setExecutedJobCounter(std::atomic_int &counter) {
    impl_->setExecutedJobCounter(counter);
}

void Worker::setActiveWorkerCounter(std::atomic_int &counter) {
    impl_->setActiveWorkerCounter(counter);
}

} // namespace jobq
