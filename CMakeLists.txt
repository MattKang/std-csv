cmake_minimum_required(VERSION 3.12.0)
project(std-csv)

enable_testing()
set(CMAKE_CXX_STANDARD 17)

add_compile_definitions(STDCSV_PROJECT_DIR="${CMAKE_CURRENT_LIST_DIR}")

add_subdirectory(src)
add_subdirectory(tests)
