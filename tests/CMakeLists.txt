add_executable(tests tests.cpp)
target_link_libraries(tests PRIVATE slim_optional)
target_compile_options(tests PRIVATE -Wall -Wextra)

add_test(NAME tests COMMAND tests)
