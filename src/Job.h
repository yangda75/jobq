#pragma once

#include <functional>

namespace jobq {

using Job = std::function<void()>;

}
