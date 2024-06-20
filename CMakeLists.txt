cmake_minimum_required(VERSION 3.12)

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(log_test LANGUAGES C CXX)

# Include fmt and benchmark libraries
set(fmt_DIR "/usr/lib/x86_64-linux-gnu/cmake/fmt/")
find_package(fmt REQUIRED)
find_package(benchmark REQUIRED)

# Find liburing
find_path(LIBURING_INCLUDE_DIR NAMES liburing.h PATHS /usr/local/include /usr/include)
find_library(LIBURING_LIBRARY NAMES uring PATHS /usr/local/lib /usr/lib /lib/x86_64-linux-gnu)

# Check if the library was found
if (NOT LIBURING_LIBRARY)
    message(FATAL_ERROR "liburing not found!")
endif()

# Include directories
include_directories(SimpleLogger/src)
include_directories(${LIBURING_INCLUDE_DIR})

# Set the build type
set(CMAKE_BUILD_TYPE Debug)

# Add subdirectories
add_subdirectory(SimpleLogger EXCLUDE_FROM_ALL)

# Create library
add_library(log_lib STATIC src/Log.cpp src/IoContext.cpp)

target_include_directories(log_lib PUBLIC ${PROJECT_SOURCE_DIR}/include ${LIBURING_INCLUDE_DIR})
target_link_libraries(log_lib PUBLIC ${LIBURING_LIBRARY})

# Add executable
add_executable(log_test Test.cpp)

# Target include directories
target_include_directories(log_test PRIVATE 
    ${benchmark_INCLUDE_DIRS} 
)

# Target link libraries
target_link_libraries(log_test PRIVATE 
    fmt::fmt 
    benchmark::benchmark 
    SimpleLogger_static 
    log_lib
)

# Optional flags (uncomment if needed)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

