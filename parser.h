#pragma once
#include <stddef.h> /* for size_t */
#include "lexer.h"

typedef struct embedjson_parser {
  /**
   * @note Should be the first embedjson_parser member to enable pointer casting
   */
  embedjson_lexer lexer;
  char* stack;
  size_t stack_size;
} embedjson_parser;

void embedjson_push(embedjson_parser* parser, const char* data, size_t size);
