#include "Worker.h"
#include "Log.h"
#include <atomic>
#include <stop_token>

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
    std::atomic_int *counter{};
    std::atomic_int *active_worker_counter{};
    Impl(Q &q) : q_ref{q} {}
    int runUntilEmpty(std::stop_token token) {
        loginfo("worker started");
        if (token.stop_requested()) {
            logerror("cannot restart stopped worker");
            return 0;
        }
        CounterGuard counter_g{active_worker_counter};
        while (!token.stop_requested()) {
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
    int runForever(std::stop_token token) {
        loginfo("worker start");
        if (token.stop_requested()) {
            logerror("cannot restart stopped worker");
            return 0;
        }
        CounterGuard counter_g{active_worker_counter};
        while (auto job = q_ref.popOne()) {
            try {
                (job->fn)();
                if (counter) {
                    (*counter)++;
                }
                job_cnt++;
            } catch (std::exception const &e) {
                logerror("Failed to run job, exception: {}", e.what());
            }
            if (token.stop_requested()) {
                // a job is poped from the q, but will not run, what should I do
                // here?
                break;
            }
        }
        loginfo("worker done");
        return job_cnt;
    }

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

int Worker::runUntilEmpty(std::stop_token token) {
    return impl_->runUntilEmpty(token);
}

int Worker::runForever(std::stop_token token) {
    return impl_->runForever(token);
}

void Worker::setExecutedJobCounter(std::atomic_int &counter) {
    impl_->setExecutedJobCounter(counter);
}

void Worker::setActiveWorkerCounter(std::atomic_int &counter) {
    impl_->setActiveWorkerCounter(counter);
}

} // namespace jobq
