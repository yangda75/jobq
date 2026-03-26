#include "Executor.h"
#include <catch2/catch_test_macros.hpp>
#include <thread>

TEST_CASE("run will run") {
    jobq::Executor ex{};
    bool finished = false;
    ex.submitJob([&finished]() { finished = true; });
    std::thread t{[&ex]() { ex.run(); }};
    ex.shutdownAndDrain();
    t.join();
    REQUIRE(finished);
}
