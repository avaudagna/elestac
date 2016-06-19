cmake_minimum_required(VERSION 3.5)
project(CPU)

SET(GCC_COVERAGE_COMPILE_FLAGS "-stack=9999 -Wl,--heap=yyyyy")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ${GCC_COVERAGE_COMPILE_FLAGS}")

SET( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}" )

set(valgrind -v --leak-check=yes)

set(SOURCE_FILES
    cpu.c
    cpu_init.c
    implementation_ansisop.c
    libs/pcb.c
    libs/stack.c
    libs/serialize.c
    libs/socketCommons.c
    libs/pcb_tests.c)

INCLUDE_DIRECTORIES(/usr/include)
LINK_DIRECTORIES(/usr/lib)

add_executable(CPU ${SOURCE_FILES})

TARGET_LINK_LIBRARIES(CPU parser-ansisop commons m)