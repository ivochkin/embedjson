/**
 * @copyright
 * Copyright (c) 2016-2021 Stanislav Ivochkin
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
  /* Space for user-defined data, embedjson does not use this field */
  void* userdata;
} embedjson_parser;

EMBEDJSON_STATIC int embedjson_push(embedjson_parser* parser, const char* data,
    embedjson_size_t size);

EMBEDJSON_STATIC int embedjson_finalize(embedjson_parser* parser);

EMBEDJSON_STATIC int embedjson_null(embedjson_parser* parser);
EMBEDJSON_STATIC int embedjson_bool(embedjson_parser* parser, char value);
EMBEDJSON_STATIC int embedjson_int(embedjson_parser* parser, embedjson_int_t value);
EMBEDJSON_STATIC int embedjson_double(embedjson_parser* parser, double value);
EMBEDJSON_STATIC int embedjson_string_begin(embedjson_parser* parser);
EMBEDJSON_STATIC int embedjson_string_chunk(embedjson_parser* parser,
    const char* data, embedjson_size_t size);
EMBEDJSON_STATIC int embedjson_string_end(embedjson_parser* parser);
EMBEDJSON_STATIC int embedjson_object_begin(embedjson_parser* parser);
EMBEDJSON_STATIC int embedjson_object_end(embedjson_parser* parser);
EMBEDJSON_STATIC int embedjson_array_begin(embedjson_parser* parser);
EMBEDJSON_STATIC int embedjson_array_end(embedjson_parser* parser);

#if EMBEDJSON_BIGNUM
EMBEDJSON_STATIC int embedjson_bignum_begin(embedjson_parser* parser,
    embedjson_int_t initial_value);
EMBEDJSON_STATIC int embedjson_bignum_chunk(embedjson_parser* parser,
    const char* data, embedjson_size_t size);
EMBEDJSON_STATIC int embedjson_bignum_end(embedjson_parser* parser);
#endif /* EMBEDJSON_BIGNUM */

#if EMBEDJSON_DYNAMIC_STACK
/**
 * Called from embedjson_push, when parser's stack is full and more space
 * is needed.
 *
 * Implementation is expected to:
 * @li allocate new stack of size more than parser.stack_size bytes,
 * @li copy old stack's content into the new stack,
 * @li re-initialize parser.stack and parser.stack_size properties,
 * @li return 0 on success.
 *
 * Non-zero return code indicates that error has occured and parsing is aborted.
 */
EMBEDJSON_STATIC int embedjson_stack_overflow(embedjson_parser* parser);
#endif

