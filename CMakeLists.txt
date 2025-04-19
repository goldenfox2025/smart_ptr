cmake_minimum_required(VERSION 3.10)
project(MyProject)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)


include_directories(include) 
add_compile_options(-fsanitize=thread -g)

add_executable(MyExecutable src/test_smart.cpp)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
add_custom_target(run
    COMMAND ./MyExecutable
    DEPENDS MyExecutable
)