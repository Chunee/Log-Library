cmake_minimum_required(VERSION 3.12)
project(Log)

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(fmt_DIR "/usr/lib/x86_64-linux-gnu/cmake/fmt/")
find_package(fmt REQUIRED)
find_package(benchmark REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(SimpleLogger EXCLUDE_FROM_ALL)
include_directories(SimpleLogger/src)

add_executable(log Test.cpp Log.cpp)

target_include_directories(log PRIVATE ${benchmark_INCLUDE_DIRS})

target_link_libraries(log PRIVATE fmt::fmt benchmark::benchmark SimpleLogger_static)