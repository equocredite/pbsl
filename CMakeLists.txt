cmake_minimum_required(VERSION 3.23)
project(pbsl)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

include_directories(include)

find_package(Threads REQUIRED)
find_package(Parlay CONFIG REQUIRED)

add_executable(pbsl main.cpp include/skip_list.hpp include/config.hpp include/util.hpp include/node.hpp)
target_link_libraries(pbsl pthread)
target_link_libraries(pbsl Parlay::parlay)