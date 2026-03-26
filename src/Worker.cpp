#include "Worker.h"
#include "Log.h"
#include <atomic>

namespace jobq {
struct Worker::Impl {
    std::atomic_bool stopped{};
    Impl(Q &q) : q_ref{q} {}
    int runUntilEmpty() {
        loginfo("worker started");
        while (true) {
            try {
                auto job = q_ref.popOneFor(1);
                if (!job.has_value()) {
                    break;
                }
                (*job)();
                job_cnt++;
            } catch (std::exception const &e) {
                logerror("Failed to run job, exception: {}", e.what());
            }
        }
        loginfo("worker done after {} jobs", job_cnt);
        return job_cnt;
    }
    void runForever() {
        while (auto job = q_ref.popOne()) {
            try {
                (*job)();
            } catch (std::exception const &e) {
                logerror("Failed to run job, exception: {}", e.what());
            }
            if (stopped) {
                break;
            }
        }
    }

    void stop() { stopped = true; }

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

void Worker::runForever() { impl_->runForever(); }

void Worker::stop() { impl_->stop(); }

} // namespace jobq
