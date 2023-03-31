#!/usr/bin/env bash


g++ -std=c++17 Example1.cc -o example1 -Wall -Wextra -Warray-bounds
g++ -std=c++17 Example2.cc -o example2 -Wall -Wextra -Warray-bounds
g++ -std=c++17 Example3.cc -o example3 -Wall -Wextra -Warray-bounds
g++ -std=c++17 Example4.cc -o example4 -Wall -Wextra -Warray-bounds -lboost_date_time -lboost_filesystem -lboost_system


