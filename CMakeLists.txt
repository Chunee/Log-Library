# cmake_minimum_required(VERSION 3.12)
# 
# # Set the C++ standard to C++20
# set(CMAKE_CXX_STANDARD 20)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 
# project(log_test LANGUAGES C CXX)
# 
# # Include fmt and benchmark libraries
# set(fmt_DIR "/usr/lib/x86_64-linux-gnu/cmake/fmt/")
# find_package(fmt REQUIRED)
# 
# # Find liburing
# find_path(LIBURING_INCLUDE_DIR NAMES liburing.h PATHS /usr/local/include /usr/include)
# find_library(LIBURING_LIBRARY NAMES uring PATHS /usr/local/lib /usr/lib /lib/x86_64-linux-gnu)
# 
# # Check if the library was found
# if (NOT LIBURING_LIBRARY)
#     message(FATAL_ERROR "liburing not found!")
# endif()
# 
# # Include directories
# include_directories(${LIBURING_INCLUDE_DIR})
# 
# # Set the build type
# set(CMAKE_BUILD_TYPE Debug)
# 
# # Valgrind
# # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
# 
# # ASan
# # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
# # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
# 
# add_subdirectory(SimpleLogger EXCLUDE_FROM_ALL)
# include_directories(SimpleLogger/src)
# 
# include_directories(NanoLog)
# 
# # Create library
# add_library(log_lib STATIC src/log.cpp src/io_context.cpp src/futex.cpp)
# 
# target_include_directories(log_lib PUBLIC ${PROJECT_SOURCE_DIR}/include ${LIBURING_INCLUDE_DIR})
# target_link_libraries(log_lib PUBLIC ${LIBURING_LIBRARY})
# 
# # Add executable
# add_executable(log_test test.cpp)
# 
# # Target include directories
# target_include_directories(log_test PRIVATE 
#     ${benchmark_INCLUDE_DIRS} 
# )
# 
# # Target link libraries
# target_link_libraries(log_test PRIVATE 
#     fmt::fmt 
#     log_lib
#     SimpleLogger_static
# )

cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(log_lib VERSION 1.0.0 LANGUAGES C CXX)

# set(fmt_DIR "/usr/lib/x86_64-linux-gnu/cmake/fmt/")
# find_package(fmt REQUIRED)
# 
# find_path(LIBURING_INCLUDE_DIR NAMES liburing.h)
# find_library(LIBURING_LIBRARY NAMES uring)

# if (NOT LIBURING_INCLUDE_DIR OR NOT LIBURING_LIBRARY) 
#     message(WARNING "liburing not found. You may need to set LIBURING_INCLUDE_DIR and LIBURING_LIBRARY manually")
# endif()

include(GNUInstallDirs)
include(ExternalProject))
# Include the FetchContent module
include(FetchContent)

# Fetch fmt library
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 9.1.0  # You can specify the version you need
)

FetchContent_MakeAvailable(fmt)

# Use ExternalProject_Add for liburing (make-based project)
ExternalProject_Add(
  liburing
  GIT_REPOSITORY https://github.com/axboe/liburing.git
  GIT_TAG liburing-2.1
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND ""
  BUILD_COMMAND make
  INSTALL_COMMAND ""
)

# Manually set liburing include and library paths
set(LIBURING_INCLUDE_DIR ${CMAKE_BINARY_DIR}/_deps/liburing-src/src/include)
set(LIBURING_LIBRARY ${CMAKE_BINARY_DIR}/_deps/liburing-src/liburing.a)

set(CMAKE_BUILD_TYPE Debug)

add_library(log_lib STATIC src/log.cpp src/io_context.cpp src/futex.cpp)

target_include_directories(log_lib PUBLIC 
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include> 
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${LIBURING_INCLUDE_DIR}>  # Include liburing headers
)

add_dependencies(log_lib liburing fmt)
target_link_libraries(log_lib PUBLIC
    fmt::fmt
    ${LIBURING_LIBRARY}
)

install(TARGETS log_lib fmt EXPORT log_libTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION include)

install(EXPORT log_libTargets
    FILE log_libTargets.cmake
    NAMESPACE log_lib::
    DESTINATION lib/cmake/log_lib
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/log_libConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_file(log_libConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/log_libConfig.cmake" @ONLY)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/log_libConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/log_libConfigVersion.cmake"
    DESTINATION lib/cmake/log_lib
)