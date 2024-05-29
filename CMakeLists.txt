cmake_minimum_required(VERSION 3.22)
project(include/pbsl)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

include_directories(include include/pbsl include/common)

find_package(Threads REQUIRED)
find_package(Parlay CONFIG REQUIRED)

add_executable(pbsl src/main.cpp
        include/jj/jj.hpp)

target_link_libraries(pbsl Threads::Threads)
target_link_libraries(pbsl Parlay::parlay)