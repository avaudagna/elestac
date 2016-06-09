cmake_minimum_required(VERSION 3.5)
project(KERNEL)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    libs/pcb.c
    libs/pcb.h
    libs/serialize.c
    libs/serialize.h
    libs/stack.c
    libs/stack.h
    kernel.c
    kernel.h
        setup.data
    socketCommons.c
    socketCommons.h)

include_directories(.)
include_directories(libs)
INCLUDE_DIRECTORIES(/usr/include)
LINK_DIRECTORIES(/usr/lib)

add_executable(KERNEL ${SOURCE_FILES})

TARGET_LINK_LIBRARIES(KERNEL parser-ansisop commons pthread)