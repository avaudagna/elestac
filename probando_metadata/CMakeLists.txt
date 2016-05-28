cmake_minimum_required(VERSION 3.5)
project(probando_metadata)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES metadatiando.c libs/pcb.c libs/serialize.c libs/stack.c)
#set(SOURCE_FILES basic_serialize_test.c libs/serialize.c)

INCLUDE_DIRECTORIES(/usr/include)
LINK_DIRECTORIES(/usr/lib)

add_executable(probando_metadata ${SOURCE_FILES})

TARGET_LINK_LIBRARIES(probando_metadata parser-ansisop commons)