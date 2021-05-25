cmake_minimum_required(VERSION 3.15)
project(alcfd_cpp)

set(CMAKE_CXX_STANDARD 14)

add_executable(alcfd_cpp main.cpp alcfd.cpp libs/predicate.cpp libs/predicates_space.cpp libs/rule.cpp libs/tuple_pair.cpp
        libs/processing.cpp libs/record.cpp libs/init_sampling.cpp libs/using_name_sp.h libs/re_sampling.cpp libs/CFDMeasure.cpp
        libs/CFDMeasure_2.cpp)