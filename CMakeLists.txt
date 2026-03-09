cmake_minimum_required(VERSION 3.31)

project(JobQ LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED 20)

# catch2
include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.8.1)

FetchContent_MakeAvailable(Catch2)
# end of catch2

if(ENABLE_TSAN)
  add_compile_options(-fsanitize=thread)
  add_link_options(-fsanitize=thread)
endif()

add_library(jobq)
target_sources(
  jobq
  PRIVATE src/JobQ.cpp src/Worker.cpp
  PUBLIC FILE_SET
         HEADERS
         BASE_DIRS
         src
         FILES
         src/Worker.h
         src/JobQ.h
         src/EmptyQ.h)

add_executable(BasicQOpsTest test/BasicQOps.cpp)

target_link_libraries(BasicQOpsTest PRIVATE Catch2::Catch2WithMain jobq)

add_executable(WorkerTest test/Worker.cpp)
target_link_libraries(WorkerTest PRIVATE Catch2::Catch2WithMain jobq)

add_executable(MPMCTest test/MPMC.cpp)
target_link_libraries(MPMCTest PRIVATE Catch2::Catch2WithMain jobq)

# catch2 ctest integration
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(BasicQOpsTest)
catch_discover_tests(WorkerTest)
catch_discover_tests(MPMCTest)
