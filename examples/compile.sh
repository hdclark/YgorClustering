#!/bin/bash


g++ -std=c++14 Example1.cc -o example1 
g++ -std=c++14 Example2.cc -o example2 
g++ -std=c++14 Example3.cc -o example3 
g++ -std=c++14 Example4.cc -o example4 -lboost_date_time -lboost_filesystem -lboost_system


