cmake_minimum_required(VERSION 3.10)
project(MyProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)


include_directories(include)
add_compile_options(-g)

add_executable(MyExecutable src/test_smart.cpp)
add_executable(FunctionTest src/test_func.cpp)

# 只为MyExecutable添加ThreadSanitizer
target_compile_options(MyExecutable PRIVATE -fsanitize=thread)
target_link_options(MyExecutable PRIVATE -fsanitize=thread)

add_custom_target(run
    COMMAND ./MyExecutable
    DEPENDS MyExecutable
)

add_custom_target(run_func
    COMMAND ./FunctionTest
    DEPENDS FunctionTest
)