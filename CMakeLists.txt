cmake_minimum_required(VERSION 3.12)
project(Log)

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(fmt_DIR "/usr/lib/x86_64-linux-gnu/cmake/fmt/")
find_package(fmt REQUIRED)
find_package(benchmark REQUIRED)

# Find liburing
find_path(LIBURING_INCLUDE_DIR NAMES liburing.h PATHS /usr/local/include /usr/include)
find_library(LIBURING_LIBRARY NAMES uring PATHS /usr/local/lib /usr/lib /lib/x86_64-linux-gnu)

# Check if the library was found
if (NOT LIBURING_LIBRARY)
    message(FATAL_ERROR "liburing not found!")
endif ()

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(SimpleLogger EXCLUDE_FROM_ALL)
include_directories(SimpleLogger/src)

# Include the liburing headers
include_directories(${LIBURING_INCLUDE_DIR})

add_executable(log Test.cpp Log.cpp io_context.cpp)

target_include_directories(log PRIVATE ${benchmark_INCLUDE_DIRS} ${LIBURING_INCLUDE_DIR})

target_link_libraries(log PRIVATE fmt::fmt benchmark::benchmark SimpleLogger_static ${LIBURING_LIBRARY})

# Set the RPATH to ensure the correct library is found at runtime
# set_target_properties(log PROPERTIES
#     INSTALL_RPATH_USE_LINK_PATH TRUE
#     BUILD_WITH_INSTALL_RPATH TRUE
#     INSTALL_RPATH "/lib/x86_64-linux-gnu"
# )