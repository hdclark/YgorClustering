#!/usr/bin/env bash


g++ -std=c++17 Example1.cc -o example1 
g++ -std=c++17 Example2.cc -o example2 
g++ -std=c++17 Example3.cc -o example3 
g++ -std=c++17 Example4.cc -o example4 -lboost_date_time -lboost_filesystem -lboost_system


