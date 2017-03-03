/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 *
 * Licensed under the MIT License (see LICENSE)
 */

#ifndef EMBEDJSON_AMALGAMATE
#pragma once
#endif /* EMBEDJSON_AMALGAMATE */
#include <stddef.h> /* for size_t */
#ifndef EMBEDJSON_AMALGAMATE
#include "common.h"
#include "lexer.h"
#endif /* EMBEDJSON_AMALGAMATE */


typedef struct embedjson_parser {
  /**
   * @note Should be the first embedjson_parser member to enable
   * embedjson_lexer* to embedjson_parser* pointer casting.
   */
  embedjson_lexer lexer;
  unsigned char state;
  size_t stack_size;
#ifdef EMBEDJSON_DYNAMIC_STACK
  char* stack;
  size_t stack_capacity;
#else
  char stack[EMBEDJSON_STATIC_STACK_SIZE];
#endif
} embedjson_parser;

EMBEDJSON_STATIC int embedjson_push(embedjson_parser* parser, const char* data,
    size_t size);

EMBEDJSON_STATIC int embedjson_finalize(embedjson_parser* parser);

EMBEDJSON_STATIC int embedjson_null(embedjson_parser* parser);
EMBEDJSON_STATIC int embedjson_bool(embedjson_parser* parser, char value);
EMBEDJSON_STATIC int embedjson_int(embedjson_parser* parser, int64_t value);
EMBEDJSON_STATIC int embedjson_double(embedjson_parser* parser, double value);
EMBEDJSON_STATIC int embedjson_string_begin(embedjson_parser* parser);
EMBEDJSON_STATIC int embedjson_string_chunk(embedjson_parser* parser,
    const char* data, size_t size);
EMBEDJSON_STATIC int embedjson_string_end(embedjson_parser* parser);
EMBEDJSON_STATIC int embedjson_object_begin(embedjson_parser* parser);
EMBEDJSON_STATIC int embedjson_object_end(embedjson_parser* parser);
EMBEDJSON_STATIC int embedjson_array_begin(embedjson_parser* parser);
EMBEDJSON_STATIC int embedjson_array_end(embedjson_parser* parser);

