cmake_minimum_required(VERSION 3.5)
project(CPU)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    cpu.c
    implementation_ansisop.c
    libs/pcb.c
    libs/stack.c
    libs/serialize.c
    libs/socketCommons.c)

INCLUDE_DIRECTORIES(/usr/include)
LINK_DIRECTORIES(/usr/lib)

add_executable(CPU ${SOURCE_FILES})

TARGET_LINK_LIBRARIES(CPU parser-ansisop commons m)