#pragma once
#include <spdlog/spdlog.h>
#include <utility>

// very thin wrapper around spdlog
namespace jobq {


template <typename... Args> void loginfo(Args &&...args) {
    spdlog::info(std::forward<Args>(args)...);
}

template <typename... Args> void logerror(Args &&...args) {
    spdlog::error(std::forward<Args>(args)...);
}

} // namespace jobq
