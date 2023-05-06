#!/usr/bin/env bash

CXXFLAGS+=" -std=c++17 -O1 -g"
CXXFLAGS+=" -Wall -Wextra -Wpedantic -Warray-bounds"
CXXFLAGS+=" -fsanitize=address -fsanitize=undefined"

g++ ${CXXFLAGS} Example1.cc -o example_1
g++ ${CXXFLAGS} Example2.cc -o example_2
g++ ${CXXFLAGS} Example3.cc -o example_3
g++ ${CXXFLAGS} Example4.cc -o example_4 -lboost_date_time -lboost_filesystem -lboost_system


