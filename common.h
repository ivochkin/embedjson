/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 *
 * Licensed under the MIT License (see LICENSE)
 */

#ifndef EMBEDJSON_AMALGAMATE
#pragma once
#include "config.h"
#endif /* EMBEDJSON_AMALGAMATE */

#ifndef EMBEDJSON_AMALGAMATE
#define EMBEDJSON_STATIC
#else
#define EMBEDJSON_STATIC static
#endif /* EMBEDJSON_AMALGAMATE */

#ifndef EMBEDJSON_DYNAMIC_STACK
/**
 * Enable dynamic parser's stack
 */
#define EMBEDJSON_DYNAMIC_STACK 0
#endif

#ifndef EMBEDJSON_STATIC_STACK_SIZE
#define EMBEDJSON_STATIC_STACK_SIZE 16
#endif

#ifndef EMBEDJSON_VALIDATE_UTF8
/**
 * Enable UTF-8 strings validation.
 */
#define EMBEDJSON_VALIDATE_UTF8 1
#endif

#ifndef EMBEDJSON_SIZE_T
#if defined(__i386__)
typedef unsigned long embedjson_size_t;
#elif defined(__x86_64__)
typedef unsigned long long embedjson_size_t;
#else
/*
 * Please report a bug at https://github.com/ivochkin/embedjson/issues/new
 * if an error below is triggered in your environment
 */
#error Unsupported architecture.
#endif
#else
typedef EMBEDJSON_SIZE_T embedjson_size_t;
#endif

typedef enum {
  EMBEDJSON_BAD_UTF8,
  EMBEDJSON_UTF8_TOO_LONG,
  EMBEDJSON_BAD_UNICODE_ESCAPE,
  EMBEDJSON_BAD_EXPONENT,
  EMBEDJSON_BAD_TRUE,
  EMBEDJSON_BAD_FALSE,
  EMBEDJSON_BAD_NULL,
  EMBEDJSON_EOF_IN_STRING,
  EMBEDJSON_EOF_IN_EXPONENT,
  EMBEDJSON_EOF_IN_TRUE,
  EMBEDJSON_EOF_IN_FALSE,
  EMBEDJSON_EOF_IN_NULL,
  EMBEDJSON_STACK_OVERFLOW,
  EMBEDJSON_UNEXP_CLOSE_CURLY,
  EMBEDJSON_UNEXP_CLOSE_BRACKET,
  EMBEDJSON_UNEXP_COMMA,
  EMBEDJSON_UNEXP_COLON,
  EMBEDJSON_UNEXP_STRING,
  EMBEDJSON_UNEXP_NUMBER,
  EMBEDJSON_EXP_COLON,
  EMBEDJSON_EXP_OBJECT_VALUE,
  EMBEDJSON_EXP_ARRAY_VALUE,
  EMBEDJSON_EXCESSIVE_INPUT,
  EMBEDJSON_INTERNAL_ERROR
} embedjson_error_code;

/**
 * Returns readable description of the error code
 */
EMBEDJSON_STATIC const char* embedjson_strerror(embedjson_error_code code);

struct embedjson_parser;

/**
 * A callback to handle errors
 *
 * @warning The function is deprecated and will be removed in 3.x release.
 */
EMBEDJSON_STATIC int embedjson_error(struct embedjson_parser* parser,
    const char* position);

/**
 * A callback to handle errors - proxy to embedjson_error
 *
 * @warning The function is temporary and will be renamed to embedjson_error
 * in the 3.x release.
 */
EMBEDJSON_STATIC int embedjson_error_ex(struct embedjson_parser* parser,
    embedjson_error_code code, const char* position);

