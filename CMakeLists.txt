cmake_minimum_required(VERSION 3.14)
project(slim_optional LANGUAGES CXX)

add_library(slim_optional INTERFACE)
target_include_directories(slim_optional INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(slim_optional INTERFACE cxx_std_23)

enable_testing()
add_subdirectory(tests)
add_subdirectory(examples)
add_subdirectory(benchmarks)
