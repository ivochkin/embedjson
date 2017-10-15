/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 *
 * Licensed under the MIT License (see LICENSE)
 */

#ifndef EMBEDJSON_AMALGAMATE
#include "common.h"
#endif /* EMBEDJSON_AMALGAMATE */

EMBEDJSON_STATIC const char* embedjson_strerror(embedjson_error_code code)
{
  switch (code) {
#if EMBEDJSON_VALIDATE_UTF8
    case EMBEDJSON_BAD_UTF8:
      return "Malformed UTF-8 byte sequence";
    case EMBEDJSON_LONG_UTF8:
      return "UTF-8 byte sequence more than 4 bytes long";
#endif
    case EMBEDJSON_BAD_UNICODE_ESCAPE:
      return "Unexpected character in the unicode escape sequence";
    case EMBEDJSON_BAD_EXPONENT:
      return "Unexpected character in the exponent part of "
        "the floating-point number";
    case EMBEDJSON_BAD_TRUE:
      return "Unknown keyword, \"true\" expected";
    case EMBEDJSON_BAD_FALSE:
      return "Unknown keyword, \"false\" expected";
    case EMBEDJSON_BAD_NULL:
      return "Unknown keyword, \"null\" expected";
    case EMBEDJSON_EOF_IN_STRING:
      return "Got end-of-stream while parsing unicode escape sequence";
    case EMBEDJSON_EOF_IN_EXPONENT:
      return "Got end-of-stream while parsing floating-point exponent";
    case EMBEDJSON_EOF_IN_TRUE:
      return "Got end-of-stream while parsing \"true\" keyword";
    case EMBEDJSON_EOF_IN_FALSE:
      return "Got end-of-stream while parsing \"false\" keyword";
    case EMBEDJSON_EOF_IN_NULL:
      return "Got end-of-stream while parsing \"null\" keyword";
#if !EMBEDJSON_DYNAMIC_STACK
    case EMBEDJSON_STACK_OVERFLOW:
      return "Object/array nesting level is too big";
#endif
    case EMBEDJSON_UNEXP_CLOSE_CURLY:
      return "Expected object, array, or primitive value, "
        "got closing curly bracket";
    case EMBEDJSON_UNEXP_CLOSE_BRACKET:
      return "Expected object, array, or primitive value, got closing bracket";
    case EMBEDJSON_UNEXP_COMMA:
      return "Expected object, array, or primitive value, got comma";
    case EMBEDJSON_UNEXP_COLON:
      return "Expected object, array, or primitive value, got colon";
    case EMBEDJSON_EXP_COLON:
      return "Expected colon following json object's key";
    case EMBEDJSON_EXP_OBJECT_VALUE:
      return "Expected object, array, or primitive value as "
        "the value of json object";
    case EMBEDJSON_EXP_ARRAY_VALUE:
      return "Expected object, array, or primitive value as "
        "the value of json array";
    case EMBEDJSON_EXCESSIVE_INPUT:
      return "Expected end-of-stream since JSON parsing is complete";
    case EMBEDJSON_INTERNAL_ERROR:
      return "Unexpected error, please report a bug";
    default:
      return "Unknown error, please report a bug";
  }
}

EMBEDJSON_STATIC int embedjson_error_ex(struct embedjson_parser* parser,
    embedjson_error_code code, const char* position)
{
  (void) code;
  return embedjson_error(parser, position);
}

