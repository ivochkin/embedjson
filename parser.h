/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 *
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <stddef.h> /* for size_t */
#include "lexer.h"

#define EMBEDJSON_EXTERNAL_STACK 0

typedef struct embedjson_parser {
  /**
   * @note Should be the first embedjson_parser member to enable
   * embedjson_lexer* to embedjson_parser* pointer casting.
   */
  embedjson_lexer lexer;
  unsigned char state;
  size_t stack_size;
#if EMBEDJSON_EXTERNAL_STACK
  char* stack;
  size_t stack_capacity;
#else
  char stack[16];
#endif
} embedjson_parser;

int embedjson_push(embedjson_parser* parser, const char* data, size_t size);

int embedjson_finalize(embedjson_parser* parser);

int embedjson_null(embedjson_parser* parser);
int embedjson_bool(embedjson_parser* parser, char value);
int embedjson_int(embedjson_parser* parser, int64_t value);
int embedjson_double(embedjson_parser* parser, double value);
int embedjson_string_chunk(embedjson_parser* parser,
    const char* data, size_t size);
int embedjson_begin_object(embedjson_parser* parser);
int embedjson_end_object(embedjson_parser* parser);
int embedjson_begin_array(embedjson_parser* parser);
int embedjson_end_array(embedjson_parser* parser);
