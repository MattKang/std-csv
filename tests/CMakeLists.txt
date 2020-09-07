add_executable(test test-main.cpp catch.hpp)
target_link_libraries(test PRIVATE ${PROJECT_NAME})
