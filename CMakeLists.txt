cmake_minimum_required(VERSION 3.15)
project(ottr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(
  -Wall
  -Wextra
  -Wpedantic
)

# Core library with all non-CLI sources so we can reuse in tests
add_library(ottr_core
  src/model.cpp
  src/parser.cpp
  src/validate.cpp
  src/engine.cpp
  src/alloc.cpp
  src/aggregator.cpp
  src/report.cpp
  src/util.cpp
)

target_include_directories(ottr_core PUBLIC src)

# CLI executable
add_executable(ottr
  src/main.cpp
)

target_link_libraries(ottr PRIVATE ottr_core)

target_include_directories(ottr PRIVATE src)

# Tests with GoogleTest
include(CTest)
if(BUILD_TESTING)
  enable_testing()

  include(FetchContent)
  # Fetch GoogleTest
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  )
  # Prevent GoogleTest from overriding our compiler/linker options on Windows
  set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  add_executable(ottr_tests
    tests/test_utils_and_tokenizer.cpp
    tests/test_parser_validator.cpp
    tests/test_allocator.cpp
    tests/test_engine_report.cpp
  )
  target_link_libraries(ottr_tests PRIVATE ottr_core GTest::gtest_main)
  target_include_directories(ottr_tests PRIVATE src)
  target_compile_definitions(ottr_tests PRIVATE OTTR_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
  add_test(NAME ottr_tests COMMAND ottr_tests)
endif()
