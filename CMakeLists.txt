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
  common.h
  common.c
  lexer.h
  lexer.c
  ut_lexer.c
)

add_executable(ut-parser
  common.h
  common.c
  lexer.h
  lexer.c
  parser.h
  parser.c
  ut_parser.c
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/embedjson.c"
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/amalgamate.sh"
    ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS common.h common.c lexer.h lexer.c parser.c parser.h LICENSE
)
add_custom_target(amalgamate
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/embedjson.c"
)

add_executable(embedjson-lint
  embedjson_lint.c
)
add_dependencies(embedjson-lint amalgamate)

enable_testing()
add_test(NAME lexer COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ut-lexer)
add_test(NAME parser COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ut-parser)
add_test(NAME embedjson-lint
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/tests/run.sh"
    "${CMAKE_CURRENT_BINARY_DIR}/embedjson-lint"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/cases")
