cmake_minimum_required(VERSION 3.0)

project(icfp2012_paiv)

set(CMAKE_CXX_STANDARD 11)

include(CTest)
enable_testing()

add_executable(testrunner test.cpp)
add_test(tests testrunner)
