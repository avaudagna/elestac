cmake_minimum_required(VERSION 3.5)
project(SWAP)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    socketCommons.c
    socketCommons.h
        swapConf
    swap.c)

INCLUDE_DIRECTORIES(/usr/include)
LINK_DIRECTORIES(/usr/lib)
add_executable(SWAP ${SOURCE_FILES})

TARGET_LINK_LIBRARIES(SWAP commons)