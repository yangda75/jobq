#pragma once

#include "Executor.h"
#include <thread>

namespace jobq {

// start a thread, and run given executor in it
std::thread runExecutor(Executor &);

} // namespace jobq
