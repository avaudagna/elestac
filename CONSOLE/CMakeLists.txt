cmake_minimum_required(VERSION 3.5)
project(CONSOLE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(SOURCE_FILES console.c libs/socketCommons.c libs/serialize.c)
INCLUDE_DIRECTORIES(/usr/include)
LINK_DIRECTORIES(/usr/lib)
add_executable(CONSOLE ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(CONSOLE commons)