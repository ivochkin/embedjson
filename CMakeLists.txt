cmake_minimum_required(VERSION 2.8)
project(embedjson C)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(fw)

set(EMBEDJSON_DYNAMIC_STACK FALSE CACHE BOOL "")
set(EMBEDJSON_STATIC_STACK_SIZE 16 CACHE STRING "")
set(EMBEDJSON_VALIDATE_UTF8 TRUE CACHE BOOL "")
set(EMBEDJSON_COVERAGE FALSE CACHE BOOL "")

if(EMBEDJSON_COVERAGE AND NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  message(FATAL_ERROR "Code coverage can be computed only for Debug builds")
endif()

if(EMBEDJSON_COVERAGE)
  fw_c_flags("--coverage -O0")
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
  ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
link_libraries(m)

add_executable(ut-lexer
  lexer.h
  lexer.c
  ut_lexer.c
)

add_executable(ut-parser
  lexer.h
  lexer.c
  parser.h
  parser.c
  ut_parser.c
)

enable_testing()
add_test(NAME lexer COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ut-lexer)
add_test(NAME parser COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ut-parser)
