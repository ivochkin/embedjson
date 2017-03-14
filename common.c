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
    default:
      return "Unknown error";
  }
}

EMBEDJSON_STATIC int embedjson_error_ex(struct embedjson_parser* parser,
    embedjson_error_code code, const char* position)
{
  return embedjson_error(parser, position);
}

