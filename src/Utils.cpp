#include "Utils.h"

namespace jobq {

std::thread runExecutor(Executor &ex) {
    return std::thread{[&ex]() { ex.run(); }};
}

} // namespace jobq
