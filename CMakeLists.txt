cmake_minimum_required(VERSION 3.13)
project(Projekt_SK2)

set(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_executable(Projekt_SK2 serwer.cpp)