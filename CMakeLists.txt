cmake_minimum_required(VERSION 3.20)

project(heap_alloc C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

file(GLOB LIB_SOURCES ${CMAKE_SOURCE_DIR}/src/*.c)

list(REMOVE_ITEM LIB_SOURCES "${CMAKE_SOURCE_DIR}/src/heap_alloc_test.c")

add_library(heap_alloc STATIC ${LIB_SOURCES})

add_executable(heap_alloc_test src/heap_alloc_test.c)
target_link_libraries(heap_alloc_test PRIVATE heap_alloc)
