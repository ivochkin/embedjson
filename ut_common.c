/**
 * @copyright
 * Copyright (c) 2017-2021 Stanislav Ivochkin
 *
 * Licensed under the MIT License (see LICENSE)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "common.h"

#define SIZEOF(x) sizeof((x)) / sizeof((x)[0])

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_RESET "\x1b[0m"

int embedjson_error(struct embedjson_parser* parser,
    const char* position)
{
  EMBEDJSON_UNUSED(parser);
  EMBEDJSON_UNUSED(position);
  return 0;
}

static void fail(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  printf(ANSI_COLOR_RED "FAILED" ANSI_COLOR_RESET "\n\n");
  vprintf(fmt, args);
  printf("\n");
  va_end(args);
  exit(1);
}

/**
 * Tests that all error code are covered by embejdson_strerror function
 * and all error messages are different
 */
static void test_strerror()
{
  const char* messages[EMBEDJSON_INTERNAL_ERROR + 1];
  for (int i = EMBEDJSON_OK; i <= EMBEDJSON_INTERNAL_ERROR; ++i) {
    messages[i] = embedjson_strerror(i);
    if (!messages[i][0]) {
      fail("Error code %d has empty error message\n", i);
    }
    if (!strncmp(messages[i], "Unknown error", 13)) {
      fail("Error code %d has default error message\n", i);
    }
  }
  for (size_t i = 0; i < SIZEOF(messages); ++i) {
    for (size_t j = i + 1; j < SIZEOF(messages); ++j) {
      if (!strcmp(messages[i], messages[j])) {
        fail("Error codes %d and %d have the same error message\n", i, j);
      }
    }
  }
}

int main()
{
  printf("[1/1] Run test \"strerror\" ... ");
  test_strerror();
  printf(ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET "\n");
  return 0;
}

