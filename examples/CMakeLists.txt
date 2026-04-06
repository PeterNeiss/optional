add_executable(examples examples.cpp)
target_link_libraries(examples PRIVATE slim_optional)
target_compile_options(examples PRIVATE -Wall -Wextra)
