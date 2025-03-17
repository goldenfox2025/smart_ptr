cmake_minimum_required(VERSION 3.10)
project(MyProject)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)


include_directories(include) 

add_executable(MyExecutable src/test_smart.cpp)

add_custom_target(run
    COMMAND ./MyExecutable
    DEPENDS MyExecutable
)