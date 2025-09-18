cmake_minimum_required(VERSION 3.20)

project(heap_alloc C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

file(GLOB FALLBACK_ALLOC_SOURCES ${CMAKE_SOURCE_DIR}/src/fallback_allocator/*.c)
add_library(fallback_alloc SHARED ${FALLBACK_ALLOC_SOURCES})

file(GLOB FAST_ALLOC_SOURCES ${CMAKE_SOURCE_DIR}/src/fast_allocator/*.c)
add_library(fast_alloc SHARED ${FAST_ALLOC_SOURCES})

file(GLOB TEST_SOURCES "test/*.c")

foreach(test_file IN LISTS TEST_SOURCES)
  get_filename_component(test_name ${test_file} NAME_WE)

  add_executable(${test_name} ${test_file})
  target_link_libraries(${test_name} PRIVATE fallback_alloc fast_alloc)

  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${test_name} PRIVATE -Wall -Wextra -Wpedantic
                                                -Werror)
  elseif(MSVC)
    target_compile_options(${test_name} PRIVATE /W4 /WX)
  else()
    message(WARNING "Unknown compiler: not setting warning flags")
  endif()
endforeach()
