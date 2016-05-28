cmake_minimum_required(VERSION 3.5)
project(serialization)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    pcbSerialization.c
    pcbDeserialization.c
    pcbMain.c.c
    pcbSerialization.h
    pcbDeserialization.h stack.h stack.c serialize.c serialize.h pcb.c pcb.h)

INCLUDE_DIRECTORIES(/usr/include)
LINK_DIRECTORIES(/usr/lib)

add_executable(serialization ${SOURCE_FILES})

TARGET_LINK_LIBRARIES(serialization parser-ansisop commons)