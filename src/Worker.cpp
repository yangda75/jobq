#include "Worker.h"
#include <iostream>

namespace jobq {
struct Worker::Impl {
    Impl(Q &q) : q_ref{q} {}
    int runUntilEmpty() {
        std::cout << "Worker started!\n";
        while (true) {
            try {
                auto job = q_ref.popOneFor(1);
                if (!job.has_value()) {
                    break;
                }
                (*job)();
                job_cnt++;
            } catch (std::exception const &e) {
                std::cerr << "Failed to run job, exception: " << e.what();
            }
        }
        return 0;
    }
    void runForever() {
        while (auto job = q_ref.popOne()) {
            try {
                (*job)();
            } catch (std::exception const &e) {
                std::cerr << "Failed to run job, exception: " << e.what();
            }
        }
    }

    Q &q_ref;
    int job_cnt{};
};

/// Worker interface

Worker::Worker(Q &q) : impl_{std::make_unique<Impl>(q)} {}
Worker::~Worker() = default;

int Worker::runUntilEmpty() { return impl_->runUntilEmpty(); }

void Worker::runForever() { impl_->runForever(); }

} // namespace jobq
