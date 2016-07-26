cmake_minimum_required(VERSION 3.5)
project(umc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        umc.c
        )

INCLUDE_DIRECTORIES(/usr/include)
LINK_DIRECTORIES(/usr/lib)

add_executable(umc ${SOURCE_FILES})

TARGET_LINK_LIBRARIES(umc commons pthread)