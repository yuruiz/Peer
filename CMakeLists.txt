cmake_minimum_required(VERSION 2.8.4)
project(15_441_project_2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES src/peer.c src/bt_parse.c src/input_buffer.c src/spiffy.c src/chunk.c src/debug.c src/sha.c src/chunklist.c
src/request.c src/conn.c src/queue.c src/congest.c)
add_executable(15_441_project_2 ${SOURCE_FILES})