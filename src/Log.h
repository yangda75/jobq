#pragma once
#include "spdlog/common.h"
#include <spdlog/spdlog.h>
#include <utility>

// very thin wrapper around spdlog
namespace jobq {

template <typename... Args>
void loginfo(spdlog::format_string_t<Args...> fmt, Args &&...args) {
    spdlog::info(fmt, std::forward<Args>(args)...);
}
template <typename... Args>
void logerror(spdlog::format_string_t<Args...> fmt, Args &&...args) {
    spdlog::error(fmt, std::forward<Args>(args)...);
}

} // namespace jobq
