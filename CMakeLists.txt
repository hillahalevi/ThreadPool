cmake_minimum_required(VERSION 3.12)
project(untitled10 C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

add_executable(untitled10 main.c threadPool.c threadPool.h osqueue.c osqueue.h)