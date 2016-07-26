cmake_minimum_required(VERSION 3.5)
project(CONSOLE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    libs/socketCommons.c
    libs/socketCommons.h
    libs/serialize.h
    libs/serialize.c
    console.c
    )

include_directories(.)
include_directories(libs)
INCLUDE_DIRECTORIES(/usr/include)
LINK_DIRECTORIES(/usr/lib)

add_executable(CONSOLE ${SOURCE_FILES})

TARGET_LINK_LIBRARIES(CONSOLE commons)