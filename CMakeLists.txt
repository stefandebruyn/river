cmake_minimum_required(VERSION 3.16)

project(river CXX)

set(CMAKE_CXX_STANDARD 17)

include_directories(src)

# River static library
file(GLOB river_src = "src/*.cpp")
add_library(river ${river_src})
target_compile_options(river PRIVATE
    -Wall
    -Wextra
    -Werror
)

# Unit test executable
file(GLOB test_src "test/*.cpp")
add_executable(test ${test_src})
target_link_libraries(test PRIVATE river CppUTest)
