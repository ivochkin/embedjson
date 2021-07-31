/**
 * @copyright
 * Copyright (c) 2017-2021 Stanislav Ivochkin
 *
 * Licensed under the MIT License (see LICENSE)
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <embedjson.c>

/**
 * Command-line arguments
 */
static int verbose = 0;
static const char* input_file = NULL;

static const char* error_position = NULL;
static embedjson_error_code error_code = EMBEDJSON_OK;

#define EMBEDJSON_PRINTF(...) \
{ \
  if (verbose) { \
    printf(__VA_ARGS__); \
  } \
}

static int embedjson_error(embedjson_parser* parser, const char* position)
{
  EMBEDJSON_UNUSED(parser);
  error_position = position;
  /** @todo When 3.0 is released, real erro code will be reported here */
  error_code = EMBEDJSON_INTERNAL_ERROR;
  return 1;
}

static int embedjson_null(embedjson_parser* parser)
{
  EMBEDJSON_UNUSED(parser);
  EMBEDJSON_PRINTF("null\n");
  return 0;
}

static int embedjson_bool(embedjson_parser* parser, char value)
{
  EMBEDJSON_UNUSED(parser);
  EMBEDJSON_PRINTF("bool %s\n", value ? "true" : "false");
  return 0;
}

static int embedjson_int(embedjson_parser* parser, embedjson_int_t value)
{
  EMBEDJSON_UNUSED(parser);
  EMBEDJSON_PRINTF("int %lld\n", value);
  return 0;
}

static int embedjson_double(embedjson_parser* parser, double value)
{
  EMBEDJSON_UNUSED(parser);
  EMBEDJSON_PRINTF("double %lf\n", value);
  return 0;
}

static int embedjson_string_begin(embedjson_parser* parser)
{
  EMBEDJSON_UNUSED(parser);
  EMBEDJSON_PRINTF("begin string\n");
  return 0;
}

static int embedjson_string_chunk(embedjson_parser* parser,
    const char* data, embedjson_size_t size)
{
  EMBEDJSON_UNUSED(parser);
  EMBEDJSON_PRINTF("string %.*s\n", (int) size, data);
  return 0;
}

static int embedjson_string_end(embedjson_parser* parser)
{
  EMBEDJSON_UNUSED(parser);
  EMBEDJSON_PRINTF("end string\n");
  return 0;
}

static int embedjson_object_begin(embedjson_parser* parser)
{
  EMBEDJSON_UNUSED(parser);
  EMBEDJSON_PRINTF("begin object\n");
  return 0;
}

static int embedjson_object_end(embedjson_parser* parser)
{
  EMBEDJSON_UNUSED(parser);
  EMBEDJSON_PRINTF("end object\n");
  return 0;
}

static int embedjson_array_begin(embedjson_parser* parser)
{
  EMBEDJSON_UNUSED(parser);
  EMBEDJSON_PRINTF("begin array\n");
  return 0;
}

static int embedjson_array_end(embedjson_parser* parser)
{
  EMBEDJSON_UNUSED(parser);
  EMBEDJSON_PRINTF("end array\n");
  return 0;
}

#if EMBEDJSON_BIGNUM
static int embedjson_bignum_begin(embedjson_parser* parser,
    embedjson_int_t initial_value)
{
  EMBEDJSON_UNUSED(parser);
  EMBEDJSON_PRINTF("begin big number, initial value %lld\n", initial_value);
  return 0;
}

static int embedjson_bignum_chunk(embedjson_parser* parser,
    const char* data, embedjson_size_t size)
{
  EMBEDJSON_UNUSED(parser);
  EMBEDJSON_PRINTF("bignum %.*s\n", (int) size, data);
  return 0;
}

static int embedjson_bignum_end(embedjson_parser* parser)
{
  EMBEDJSON_UNUSED(parser);
  EMBEDJSON_PRINTF("end bignum\n");
  return 0;
}
#endif /* EMBEDJSON_BIGNUM */

#if EMBEDJSON_DYNAMIC_STACK
static int embedjson_stack_overflow(embedjson_parser* parser)
{
  size_t new_stack_capacity = 2 * parser->stack_capacity + 1;
  char* new_stack = realloc(parser->stack, new_stack_capacity);
  if (!new_stack) {
    return -1;
  }
  parser->stack = new_stack;
  parser->stack_capacity = new_stack_capacity;
  return 0;
}
#endif /* EMBEDJSON_DYNAMIC_STACK */

int main(int argc, char* argv[])
{
  embedjson_parser parser;
  char ch;
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "-v")) {
      verbose = 1;
    } else if (!strcmp(argv[i], "--verbose")) {
      verbose = 1;
    } else {
      input_file = argv[i];
    }
  }
  memset(&parser, 0, sizeof(parser));
  int fd = STDIN_FILENO;
  if (input_file) {
    fd = open(input_file, O_RDONLY);
  }
  while (read(fd, &ch, 1) > 0) {
    int err = embedjson_push(&parser, &ch, 1);
    if (err) {
      fprintf(stderr, "error parsing json near symbol '%c': %s\n",
          error_position ? *error_position : '?',
          embedjson_strerror(error_code));
      return err;
    }
  }
  int err = embedjson_finalize(&parser);
  if (err) {
    fprintf(stderr, "error parsing json near symbol '%c': %s\n",
        error_position ? *error_position : '?',
        embedjson_strerror(error_code));
    return err;
  }
  return 0;
}

