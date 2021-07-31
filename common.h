/**
 * @copyright
 * Copyright (c) 2016-2021 Stanislav Ivochkin
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
/**
 * Please report a bug at https://github.com/ivochkin/embedjson/issues/new
 * if an error below is triggered in your environment
 */
#error Unsupported architecture.
#endif
#else
typedef EMBEDJSON_SIZE_T embedjson_size_t;
#endif

#ifndef EMBEDJSON_INT_T
typedef long long embedjson_int_t;
#else
typedef EMBEDJSON_INT_T embedjson_int_t;
#endif

#define EMBEDJSON_INT_MAX ((((embedjson_int_t) 1) << (sizeof(embedjson_int_t) * 8 - 2)) - 1 + (((embedjson_int_t) 1) << (sizeof(embedjson_int_t) * 8 - 2)))
#define EMBEDJSON_INT_MIN (-(EMBEDJSON_INT_MAX - 1))

#ifndef EMBEDJSON_DEBUG
/**
 * Emit debug messages to stdout during parsing
 */
#define EMBEDJSON_DEBUG 0
#endif

#if EMBEDJSON_DEBUG
#include <string.h>
#include <stdio.h>
#define EMBEDJSON_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define EMBEDJSON_LOG(parser, ...) \
do { \
  char msg[512] = {0};\
  int printed;\
  printed = snprintf(msg, sizeof(msg), "%s (%s:%d) {%p} ", __func__, EMBEDJSON_FILENAME, __LINE__, (void*) parser);\
  printed += snprintf(msg + printed , sizeof(msg) - printed, __VA_ARGS__);\
  printf("%.*s\n", (int) printed, msg);\
} while (0)
#else
#define EMBEDJSON_LOG(...) {}
#endif

#define EMBEDJSON_UNUSED(x) (void) (x)

typedef enum {
  /**
   * No error
   *
   * EMBEDJSON_OK is a fake error value to initialize embedjson_error_type variables.
   */
  EMBEDJSON_OK = 0,
  /** Malformed UTF-8 byte sequence */
  EMBEDJSON_BAD_UTF8,
  /** UTF-8 byte sequence more than 4 bytes long */
  EMBEDJSON_LONG_UTF8,
  /**
   * Unexpected character in the unicode escape sequence
   *
   * Unicode escape sequence should be of the format \uXXXX, * where X stands
   * for hexadecimal digit. Only decimal digits [0-9] and letters [a-f], [A-F]
   * are allowed.
   */
  EMBEDJSON_BAD_UNICODE_ESCAPE,
  /**
   * Unexpected character in the exponent part of the floating-point number.
   *
   * Exponent syntax: [e|E] [-|+] [0-9]+
   */
  EMBEDJSON_BAD_EXPONENT,
  /** Unknown keyword, "true" expected */
  EMBEDJSON_BAD_TRUE,
  /** Unknown keyword, "false" expected */
  EMBEDJSON_BAD_FALSE,
  /** Unknown keyword, "null" expected */
  EMBEDJSON_BAD_NULL,
  /**
   * Got end-of-stream while parsing unicode escape sequence
   *
   * Unicode escape sequence occupies exactly 6 bytes: \uXXXX. Some of the
   * trailing bytes are missing in the input stream.
   */
  EMBEDJSON_EOF_IN_STRING,
  /**
   * Got end-of-stream while parsing floating-point exponent
   *
   * This error happens when end-of-stream comes right after the 'e' or 'E'
   * character that indicates beginning of the floating-point exponent.
   */
  EMBEDJSON_EOF_IN_EXPONENT,
  /** Got end-of-stream while parsing "true" keyword */
  EMBEDJSON_EOF_IN_TRUE,
  /** Got end-of-stream while parsing "false" keyword */
  EMBEDJSON_EOF_IN_FALSE,
  /** Got end-of-stream while parsing "null" keyword */
  EMBEDJSON_EOF_IN_NULL,
  /**
   * Object/array nesting level is too big
   *
   * Try to increase size of the stack (EMBEDJSON_STATIC_STACK_SIZE).
   */
  EMBEDJSON_STACK_OVERFLOW,
  /** Expected object, array, or primitive value, got closing curly bracket
   * @todo inspect usage - review doxygen comment
   */
  EMBEDJSON_UNEXP_CLOSE_CURLY,
  /** Expected object, array, or primitive value, got closing bracket */
  EMBEDJSON_UNEXP_CLOSE_BRACKET,
  /** Expected object, array, or primitive value, got comma */
  EMBEDJSON_UNEXP_COMMA,
  /** Expected object, array, or primitive value, got colon */
  EMBEDJSON_UNEXP_COLON,
  /** Expected colon following json object's key */
  EMBEDJSON_EXP_COLON,
  /** Expected string as object key, or close curly bracket */
  EMBEDJSON_EXP_OBJECT_KEY_OR_CLOSE_CURLY,
  /** Expected string as object key */
  EMBEDJSON_EXP_OBJECT_KEY,
  /** Expected object, array, or primitive value for the value of json object */
  EMBEDJSON_EXP_OBJECT_VALUE,
  /** Expected comma as array values separator, or close square bracket */
  EMBEDJSON_EXP_COMMA_OR_CLOSE_BRACKET,
  /** Expected comma as object items separator, or close curly bracket */
  EMBEDJSON_EXP_COMMA_OR_CLOSE_CURLY,
  /** Expected object, array, or primitive value for the value of json array */
  EMBEDJSON_EXP_ARRAY_VALUE,
  /** Expected end-of-stream since JSON parsing is complete */
  EMBEDJSON_EXCESSIVE_INPUT,
  /**
   * Not enough data to complete JSON parsing.
   */
  EMBEDJSON_INSUFFICIENT_INPUT,
  /** Unexpected symbol */
  EMBEDJSON_UNEXP_SYMBOL,
  /** Got number starting with a leading zero */
  EMBEDJSON_LEADING_ZERO,
  /** Got number starting with a leading plus */
  EMBEDJSON_LEADING_PLUS,
  /** Got empty fractional part of the number */
  EMBEDJSON_EMPTY_FRAC,
  /** Got empty exponential part of the number */
  EMBEDJSON_EMPTY_EXP,
  /**
   * Got unexpected symbol while parsing escape sequence
   *
   * Symbols to complete escape sequence: " \ / b f n r t u
   */
  EMBEDJSON_BAD_ESCAPE,
  /**
   * Got unescaped ASCII control character (code point < 0x20)
   *
   * These characters are disallowed by the JSON specification despite
   * that some of them are valid unicode codes.
   */
  EMBEDJSON_UNESCAPED_CONTROL_CHAR,
  /**
   * Too large exponent part of the floating-point number.
   *
   * The IEEE Standard for Floating-Point Arithmetic (IEEE 754)
   * allows double to represent numbers in the range
   * [4.9406564584124654 * 10^{−324}, 1.7976931348623157 * 10^{308}]
   */
  EMBEDJSON_EXPONENT_OVERFLOW,
  /**
   * Too large integer value
   *
   * Parsing integer caused signed integer type EMBEDJSON_INT_T overflow.
   * One can extend integer type capacity by redefining EMBEDJSON_INT_T
   * to 128 bit integer type.
   * Another option is to enable EMBEDJSON_BIGNUM option that allows nummber
   * values of any size.
   */
  EMBEDJSON_INT_OVERFLOW,
  /**
   * Unexpected error.
   *
   * Under normal circumstances, this error should never happen. Errors of this
   * type should be regarded as failed assertions. But since failed assertions
   * have very destructive consequences for the application, such errors are
   * reported more gently.
   *
   * Please report a bug at https://github.com/ivochkin/embedjson/issues/new
   * if you receive EMBEDJSON_INTERNAL_ERROR from the embedjson library.
   *
   * @note EMBEDJSON_INTERNAL_ERROR should the last enum member
   */
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

