cmake_minimum_required(VERSION 3.0)
project(embedjson C)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(fw)
include(CheckTypeSize)

set(EMBEDJSON_DYNAMIC_STACK FALSE CACHE BOOL "")
set(EMBEDJSON_STATIC_STACK_SIZE 16 CACHE STRING "")
set(EMBEDJSON_VALIDATE_UTF8 TRUE CACHE BOOL "")
set(EMBEDJSON_COVERAGE FALSE CACHE BOOL "")
set(EMBEDJSON_ENABLE_INT128 FALSE CACHE BOOL "")

if(EMBEDJSON_COVERAGE AND NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  message(FATAL_ERROR "Code coverage can be computed only for Debug builds")
endif()

if(EMBEDJSON_COVERAGE)
  fw_c_flags("--coverage -O0")
endif()

if(EMBEDJSON_ENABLE_INT128)
  check_type_size(__int128_t EMBEDJSON_SIZEOF_INT128_T)
  check_type_size(__int128 EMBEDJSON_SIZEOF_INT128)
  if(EMBEDJSON_SIZEOF_INT128_T STREQUAL 16)
    set(EMBEDJSON_INT_T __int128_t)
  elseif(EMBEDJSON_SIZEOF_INT128 STREQUAL 16)
    set(EMBEDJSON_INT_T __int128)
  else()
    message(FATAL_ERROR "No suitable 128-bit integer type found. __int128, __int128_t probed.")
  endif()
else()
  set(EMBEDJSON_INT_T "long long")
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
  ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
link_libraries(m)

fw_c99()
fw_c_flags("-Wall -Wextra -Wpedantic")

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

add_executable(ut-common
  common.h
  common.c
  ut_common.c
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
add_test(NAME common COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ut-common)
add_test(NAME embedjson-lint
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/tests/run.sh"
    "${CMAKE_CURRENT_BINARY_DIR}/embedjson-lint"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/cases")
