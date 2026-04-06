add_executable(memory_bench memory_usage.cpp)
target_link_libraries(memory_bench PRIVATE slim_optional)
target_compile_options(memory_bench PRIVATE -Wall -Wextra -O2)

add_executable(perf_bench performance.cpp)
target_link_libraries(perf_bench PRIVATE slim_optional)
target_compile_options(perf_bench PRIVATE -Wall -Wextra -O2)
