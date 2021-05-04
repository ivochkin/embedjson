/**
 * @copyright
 * Copyright (c) 2017-2021 Stanislav Ivochkin
 *
 * Licensed under the MIT License (see LICENSE)
 */

#ifndef EMBEDJSON_AMALGAMATE
#include "common.h"
#endif /* EMBEDJSON_AMALGAMATE */

#define EMBEDJSON_BUG_REPORT \
"Please report a bug at https://github.com/ivochkin/embedjson/issues/new"

EMBEDJSON_STATIC const char* embedjson_strerror(embedjson_error_code code)
{
  switch (code) {
    case EMBEDJSON_OK:
      return "EMBEDJSON_OK: No error (0)";
    case EMBEDJSON_BAD_UTF8:
      return "EMBEDJSON_BAD_UTF8: Malformed UTF-8 byte sequence (1)";
    case EMBEDJSON_LONG_UTF8:
      return "EMBEDJSON_LONG_UTF8: "
        "UTF-8 byte sequence more than 4 bytes long (2)";
    case EMBEDJSON_BAD_UNICODE_ESCAPE:
      return "EMBEDJSON_BAD_UNICODE_ESCAPE: "
        "Unexpected character in the unicode escape sequence (3)";
    case EMBEDJSON_BAD_EXPONENT:
      return "EMBEDJSON_BAD_EXPONENT: "
        "Unexpected character in the exponent part of "
        "the floating-point number (4)";
    case EMBEDJSON_BAD_TRUE:
      return "EMBEDJSON_BAD_TRUE: Unknown keyword, \"true\" expected (5)";
    case EMBEDJSON_BAD_FALSE:
      return "EMBEDJSON_BAD_FALSE: Unknown keyword, \"false\" expected (6)";
    case EMBEDJSON_BAD_NULL:
      return "EMBEDJSON_BAD_NULL: Unknown keyword, \"null\" expected (7)";
    case EMBEDJSON_EOF_IN_STRING:
      return "EMBEDJSON_EOF_IN_STRING: "
        "Got end-of-stream while parsing unicode escape sequence (8)";
    case EMBEDJSON_EOF_IN_EXPONENT:
      return "EMBEDJSON_EOF_IN_EXPONENT: "
        "Got end-of-stream while parsing floating-point exponent (9)";
    case EMBEDJSON_EOF_IN_TRUE:
      return "EMBEDJSON_EOF_IN_TRUE: "
        "Got end-of-stream while parsing \"true\" keyword (10)";
    case EMBEDJSON_EOF_IN_FALSE:
      return "EMBEDJSON_EOF_IN_FALSE: "
        "Got end-of-stream while parsing \"false\" keyword (11)";
    case EMBEDJSON_EOF_IN_NULL:
      return "EMBEDJSON_EOF_IN_NULL: "
        "Got end-of-stream while parsing \"null\" keyword (12)";
    case EMBEDJSON_STACK_OVERFLOW:
      return "EMBEDJSON_STACK_OVERFLOW: "
        "Object/array nesting level is too big. (13)";
    case EMBEDJSON_UNEXP_CLOSE_CURLY:
      return "EMBEDJSON_UNEXP_CLOSE_CURLY: "
        "Expected object, array, or primitive value, "
        "got closing curly bracket (14)";
    case EMBEDJSON_UNEXP_CLOSE_BRACKET:
      return "EMBEDJSON_UNEXP_CLOSE_BRACKET: "
        "Expected object, array, or primitive value, got closing bracket (15)";
    case EMBEDJSON_UNEXP_COMMA:
      return "EMBEDJSON_UNEXP_COMMA: "
        "Expected object, array, or primitive value, got comma (16)";
    case EMBEDJSON_UNEXP_COLON:
      return "EMBEDJSON_UNEXP_COLON: "
        "Expected object, array, or primitive value, got colon (17)";
    case EMBEDJSON_EXP_COLON:
      return "EMBEDJSON_EXP_COLON: "
        "Expected colon following json object's key (18)";
    case EMBEDJSON_EXP_OBJECT_KEY_OR_CLOSE_CURLY:
      return "EMBEDJSON_EXP_OBJECT_KEY_OR_CLOSE_CURLY: "
        "Expected string as object key, or close curly bracket (19)";
    case EMBEDJSON_EXP_OBJECT_KEY:
      return "EMBEDJSON_EXP_OBJECT_KEY: "
        "Expected string as object key (20)";
    case EMBEDJSON_EXP_OBJECT_VALUE:
      return "EMBEDJSON_EXP_OBJECT_VALUE: "
        "Expected object, array, or primitive value as "
        "the value of json object (21)";
    case EMBEDJSON_EXP_COMMA_OR_CLOSE_BRACKET:
      return "EMBEDJSON_EXP_COMMA_OR_CLOSE_BRACKET: "
        "Expected comma as object items separator, or close square bracket (22)";
    case EMBEDJSON_EXP_COMMA_OR_CLOSE_CURLY:
      return "EMBEDJSON_EXP_COMMA_OR_CLOSE_CURLY: "
        "Expected comma as object items separator, or close curly bracket (23)";
    case EMBEDJSON_EXP_ARRAY_VALUE:
      return "EMBEDJSON_EXP_ARRAY_VALUE: "
        "Expected object, array, or primitive value as "
        "the value of json array (24)";
    case EMBEDJSON_EXCESSIVE_INPUT:
      return "EMBEDJSON_EXCESSIVE_INPUT: "
        "Expected end-of-stream since JSON parsing is complete (25)";
    case EMBEDJSON_INSUFFICIENT_INPUT:
      return "EMBEDJSON_INSUFFICIENT_INPUT: "
        "Not enough data to complete JSON parsing (26)";
    case EMBEDJSON_UNEXP_SYMBOL:
      return "EMBEDJSON_UNEXP_SYMBOL: "
        "Expected object, array, or primitive value, "
        "got unexpected symbol (27)";
    case EMBEDJSON_LEADING_ZERO:
      return "EMBEDJSON_LEADING_ZERO: "
        "Got number starting with a leading zero (28)";
    case EMBEDJSON_LEADING_PLUS:
      return "EMBEDJSON_LEADING_PLUS: "
        "Got number starting with a leading plus (29)";
    case EMBEDJSON_EMPTY_FRAC:
      return "EMBEDJSON_EMPTY_FRAC: "
        "Got empty fractional part of the number (30)";
    case EMBEDJSON_EMPTY_EXP:
      return "EMBEDJSON_EMPTY_EXP: "
        "Got empty exponential part of the number (31)";
    case EMBEDJSON_BAD_ESCAPE:
      return "EMBEDJSON_BAD_ESCAPE: "
        "Got unexpected symbol while parsing escape sequence (32)";
    case EMBEDJSON_UNESCAPED_CONTROL_CHAR:
      return "EMBEDJSON_UNESCAPED_CONTROL_CHAR: "
        "Got unescaped ASCII control character (33)";
    case EMBEDJSON_EXPONENT_OVERFLOW:
      return "EMBEDJSON_EXPONENT_OVERFLOW: "
        "Too large exponent part of the floating-point number (34)";
    case EMBEDJSON_INTERNAL_ERROR:
      return "EMBEDJSON_INTERNAL_ERROR: "
        "Unexpected internal error (35). " EMBEDJSON_BUG_REPORT;
    default:
      return "Unknown error. " EMBEDJSON_BUG_REPORT;
  }
}

EMBEDJSON_STATIC int embedjson_error_ex(struct embedjson_parser* parser,
    embedjson_error_code code, const char* position)
{
  (void) code;
  return embedjson_error(parser, position);
}

