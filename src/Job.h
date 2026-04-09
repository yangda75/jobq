#pragma once

#include <functional>
#include <cstdint>

namespace jobq {

struct Job {
    uint64_t id;
    int priority{0};
    std::function<void()> fn;
};

using JobHandle = decltype(Job::id);
using JobFn = decltype(Job::fn);

} // namespace jobq
