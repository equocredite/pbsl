cmake_minimum_required(VERSION 3.24)
project(pbsl)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

include_directories(include)

find_package(Threads REQUIRED)
find_package(Parlay CONFIG REQUIRED)

add_executable(pbsl main.cpp include/skip_list.h include/config.h include/util.h)
target_link_libraries(pbsl pthread)