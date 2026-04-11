#pragma once

#include <thread>

namespace jobq {
inline void setCurrentThreadName(std::string const &name) {
#if defined(__linux__)
    pthread_setname_np(pthread_self(), name.substr(0, 15).c_str());
#elif defined(__APPLE__)
    pthread_setname_np(name.c_str());
#elif defined(_WIN32)
    std::wstring wname(name.begin(), name.end());
    SetThreadDescription(GetCurrentThread(), wname.c_str());
#endif
}
} // namespace jobq
