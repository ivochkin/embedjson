cmake_minimum_required(VERSION 2.8)
project(embedjson C)

enable_testing()
link_libraries(m)

add_executable(ut-lexer
  lexer.h
  lexer.c
  ut_lexer.c
)

add_test(NAME lexer COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ut-lexer)
