add_executable(tests tests_main.cpp catch.hpp)
target_link_libraries(tests PRIVATE ${PROJECT_NAME})
add_test(
    NAME tests
    COMMAND tests "-o tests_results.xml -r junit"
)
