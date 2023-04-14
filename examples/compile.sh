#!/usr/bin/env bash


g++ -std=c++17 -O1 -g -Wall -Wextra -Wpedantic -Warray-bounds Example1.cc -o example_1 
g++ -std=c++17 -O1 -g -Wall -Wextra -Wpedantic -Warray-bounds Example2.cc -o example_2 
g++ -std=c++17 -O1 -g -Wall -Wextra -Wpedantic -Warray-bounds Example3.cc -o example_3 
g++ -std=c++17 -O1 -g -Wall -Wextra -Wpedantic -Warray-bounds Example4.cc -o example_4 -lboost_date_time -lboost_filesystem -lboost_system


