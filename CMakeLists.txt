cmake_minimum_required(VERSION 3.20.0 FATAL_ERROR)

project(atcoder CXX)

file(GLOB_RECURSE PROBLEM_SRCS problems/*.c*)

foreach(src ${PROBLEM_SRCS})
    get_filename_component(problem_name ${src} NAME_WE)
    add_executable(${problem_name} ${src})
endforeach()
