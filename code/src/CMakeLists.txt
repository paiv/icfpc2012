cmake_minimum_required(VERSION 3.0)

project(icfp2012_paiv)

set(CMAKE_CXX_STANDARD 11)

add_executable(lifter lifter.cpp)
add_executable(validate validator.cpp)
add_executable(viz viz.cpp)
