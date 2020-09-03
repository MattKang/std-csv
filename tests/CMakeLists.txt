add_executable(test test-main.cpp)
target_include_directories(test PRIVATE /usr/local/include)
target_link_libraries(test PRIVATE ${PROJECT_NAME})
