#include "JobQ.h"
#include "EmptyQ.h"
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <list>
#include <mutex>
#include <thread>

namespace jobq {

/// impl
struct Q::Impl {
    void pushJob(Job j) {
        {

            std::lock_guard lk{mtx};
            jobs.emplace_back(std::move(j));
        }
        have_job.notify_one();
    }
    std::optional<Job> popOne() {
        std::unique_lock lk{mtx};
        while (jobs.empty()) {
            have_job.wait(lk, [this]() { return !jobs.empty(); });
        }
        auto job = jobs.front(); // is this copy ok?
        jobs.pop_front();
        return job;
    }

    Job popOneOrThrow() {
        std::lock_guard lk{mtx};
        if (jobs.empty()) {
            throw EmptyQ{};
        }
        auto job = jobs.front();
        jobs.pop_front();
        return job;
    }

    std::optional<Job> popOneFor(int timeout_ms) {
        std::unique_lock lk{mtx};
        if (!have_job.wait_for(lk, std::chrono::milliseconds(timeout_ms),
                               [this]() { return !jobs.empty(); })) {
            return std::nullopt;
        }
        auto job = jobs.front();
        jobs.pop_front();
        return job;
    }

    void close() {}

    std::list<Job> jobs;
    std::mutex mtx;
    std::condition_variable have_job;
};

/// Q

void Q::pushJob(Job j) { impl_->pushJob(std::move(j)); }

std::optional<Job> Q::popOne() { return impl_->popOne(); }

Job Q::popOneOrThrow() { return impl_->popOneOrThrow(); }

std::optional<Job> Q::popOneFor(int timeout_ms) {
    return impl_->popOneFor(timeout_ms);
}

void Q::close() { impl_->close(); }

Q::Q() : impl_{std::make_unique<Impl>()} {}

Q::~Q() = default;

} // namespace jobq
