/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 *
 * Licensed under the MIT License (see LICENSE)
 */

#ifndef EMBEDJSON_AMALGAMATE
#pragma once
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
  embedjson_size_t stack_size;
#if EMBEDJSON_DYNAMIC_STACK
  char* stack;
  embedjson_size_t stack_capacity;
#else
  char stack[EMBEDJSON_STATIC_STACK_SIZE];
#endif
} embedjson_parser;

EMBEDJSON_STATIC int embedjson_push(embedjson_parser* parser, const char* data,
    embedjson_size_t size);

EMBEDJSON_STATIC int embedjson_finalize(embedjson_parser* parser);

EMBEDJSON_STATIC int embedjson_null(embedjson_parser* parser);
EMBEDJSON_STATIC int embedjson_bool(embedjson_parser* parser, char value);
EMBEDJSON_STATIC int embedjson_int(embedjson_parser* parser, long long value);
EMBEDJSON_STATIC int embedjson_double(embedjson_parser* parser, double value);
EMBEDJSON_STATIC int embedjson_string_begin(embedjson_parser* parser);
EMBEDJSON_STATIC int embedjson_string_chunk(embedjson_parser* parser,
    const char* data, embedjson_size_t size);
EMBEDJSON_STATIC int embedjson_string_end(embedjson_parser* parser);
EMBEDJSON_STATIC int embedjson_object_begin(embedjson_parser* parser);
EMBEDJSON_STATIC int embedjson_object_end(embedjson_parser* parser);
EMBEDJSON_STATIC int embedjson_array_begin(embedjson_parser* parser);
EMBEDJSON_STATIC int embedjson_array_end(embedjson_parser* parser);

