cmake_minimum_required(VERSION 3.20)

project(grand_dispatch_comparison LANGUAGES CXX)

if(NOT APPLE)
    message(FATAL_ERROR "This project uses the macOS Grand Central Dispatch SDK.")
endif()

find_path(BOOST_INCLUDE_DIR boost/asio/thread_pool.hpp REQUIRED)
find_package(Threads REQUIRED)

add_executable(grand_dispatch_comparison src/main.cxx)
target_compile_features(grand_dispatch_comparison PRIVATE cxx_std_23)
set_target_properties(grand_dispatch_comparison PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(grand_dispatch_comparison PRIVATE
    -Wall -Wextra -Wpedantic -fblocks)
target_include_directories(grand_dispatch_comparison SYSTEM PRIVATE "${BOOST_INCLUDE_DIR}")
# macOS links libSystem automatically, which provides GCD.
target_link_libraries(grand_dispatch_comparison PRIVATE Threads::Threads)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
