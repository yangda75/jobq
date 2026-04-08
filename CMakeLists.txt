cmake_minimum_required(VERSION 3.21)

project(JobQ LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS NO)

enable_testing()

include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.8.1)
FetchContent_MakeAvailable(Catch2)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v2.x)
FetchContent_MakeAvailable(spdlog)

add_library(jobq_jobq)
add_library(jobq::jobq ALIAS jobq_jobq)
target_sources(
  jobq_jobq
  PRIVATE src/JobQ.cpp src/Worker.cpp src/Executor.cpp src/TimerSource.cpp
          src/TriggerSource.cpp src/Utils.cpp
  PUBLIC FILE_SET
         HEADERS
         BASE_DIRS
         src
         FILES
         src/Log.h
         src/Worker.h
         src/JobQ.h
         src/Executor.h
         src/TimerSource.h
         src/Source.h
         src/TriggerSource.h
         src/Utils.h)
target_link_libraries(jobq_jobq PUBLIC spdlog::spdlog)

if(JOBQ_ENABLE_TSAN)
  add_compile_options(-fsanitize=thread)
  add_link_options(-fsanitize=thread)
endif()

if(PROJECT_IS_TOP_LEVEL OR JOBQ_ENABLE_TESTING)
  add_executable(BasicQOpsTest test/BasicQOps.cpp)
  target_link_libraries(BasicQOpsTest PRIVATE Catch2::Catch2WithMain jobq_jobq)

  add_executable(WorkerTest test/Worker.cpp)
  target_link_libraries(WorkerTest PRIVATE Catch2::Catch2WithMain jobq_jobq)

  add_executable(MPMCTest test/MPMC.cpp)
  target_link_libraries(MPMCTest PRIVATE Catch2::Catch2WithMain jobq_jobq)

  add_executable(ExecutorTest test/Executor.cpp)
  target_link_libraries(ExecutorTest PRIVATE Catch2::Catch2WithMain jobq_jobq)

  add_executable(BenchQ test/BenchQ.cpp)
  target_link_libraries(BenchQ PRIVATE Catch2::Catch2WithMain jobq_jobq)

  # catch2 ctest integration
  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
  include(CTest)
  include(Catch)
  catch_discover_tests(BasicQOpsTest)
  catch_discover_tests(WorkerTest)
  catch_discover_tests(MPMCTest)
  catch_discover_tests(ExecutorTest)
  # catch_discover_tests(BenchQ)

endif()
