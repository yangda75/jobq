#include "JobQ.h"
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <list>
#include <mutex>
#include <thread>

namespace jobq {

/// impl
struct Q::Impl {
    bool pushJob(Job j) {
        {
            std::lock_guard lk{mtx};
            if (closed) {
                return false;
            }
            jobs.emplace_back(std::move(j));
        }
        can_pop.notify_one();
        return true;
    }
    std::optional<Job> popOne() {
        std::unique_lock lk{mtx};
        while (jobs.empty() && !closed) {
            can_pop.wait(lk, [this]() { return (!jobs.empty()) || closed; });
        }
        if (jobs.empty()) {
            return std::nullopt;
        }
        auto job = jobs.front(); // is this copy ok?
        jobs.pop_front();
        return job;
    }

    std::optional<Job> popOneFor(int timeout_ms) {
        std::unique_lock lk{mtx};
        if (!can_pop.wait_for(lk, std::chrono::milliseconds(timeout_ms),
                              [this]() { return (!jobs.empty()) || closed; })) {
            return std::nullopt;
        }
        if (closed && jobs.empty()) {
            return std::nullopt;
        }
        auto job = jobs.front();
        jobs.pop_front();
        return job;
    }

    void close() {
        {
            std::lock_guard lk{mtx};
            closed = true;
        }
        can_pop.notify_all();
    }

    std::list<Job> jobs;
    std::mutex mtx;
    std::condition_variable can_pop;
    bool closed{false};
};

/// Q

bool Q::pushJob(Job j) { return impl_->pushJob(std::move(j)); }

std::optional<Job> Q::popOne() { return impl_->popOne(); }

std::optional<Job> Q::popOneFor(int timeout_ms) {
    return impl_->popOneFor(timeout_ms);
}

void Q::close() { impl_->close(); }

Q::Q() : impl_{std::make_unique<Impl>()} {}

Q::~Q() = default;

} // namespace jobq
