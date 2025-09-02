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

add_executable(ottr
  src/main.cpp
  src/model.cpp
  src/parser.cpp
  src/validate.cpp
  src/engine.cpp
  src/alloc.cpp
  src/aggregator.cpp
  src/report.cpp
  src/util.cpp
)

target_include_directories(ottr PRIVATE src)
