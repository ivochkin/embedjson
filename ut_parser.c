/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 *
 * Licensed under the MIT License (see LICENSE)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include "parser.h"


#define SIZEOF(x) sizeof((x)) / sizeof((x)[0])

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RESET "\x1b[0m"


typedef unsigned long long ull;


typedef enum call_type {
  CALL_NULL,
  CALL_BOOL,
  CALL_INT,
  CALL_DOUBLE,
  CALL_STRING_CHUNK,
  CALL_BEGIN_OBJECT,
  CALL_END_OBJECT,
  CALL_BEGIN_ARRAY,
  CALL_END_ARRAY
} call_type;


typedef struct data_chunk {
  const char* data;
  size_t size;
} data_chunk;


typedef struct {
  const char* name;
  size_t nchunks;
  data_chunk* data_chunks;
  size_t ncalls;
  call_type* calls;
} test_case;


static test_case* itest = NULL;
static data_chunk* idata_chunk = NULL;
static call_type* icall = NULL;

static void fail(const char* fmt, ...)
{
  printf(ANSI_COLOR_RED "FAILED" ANSI_COLOR_RESET "\n\n");
  printf("Data chunks:\n");
  for (size_t i = 0; i < itest->nchunks; ++i) {
    data_chunk c = itest->data_chunks[i];
    printf("%llu. \"%.*s\"\n", (ull) i + 1, (int) c.size, c.data);
  }
  printf("Failed on chunk: %llu of %llu\n",
      (ull) (idata_chunk - itest->data_chunks + 1), (ull) itest->nchunks);
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  printf("\n");
  va_end(args);
  exit(1);
}


static int on_call(call_type call)
{
  if (icall == itest->calls + itest->ncalls) {
    fail("Enexpected call of type %d", call);
  }
  if (call != *icall) {
    fail("Call type mismatch. Expected %d, got %d", *icall, call);
  }
  icall = icall + 1;
  return 0;
}


int embedjson_error(const char* position)
{
  return 1;
}


int embedjson_null(embedjson_parser* parser)
{
  return on_call(CALL_NULL);
}


int embedjson_bool(embedjson_parser* parser, char value)
{
  return on_call(CALL_BOOL);
}


int embedjson_int(embedjson_parser* parser, int64_t value)
{
  return on_call(CALL_INT);
}


int embedjson_double(embedjson_parser* parser, double value)
{
  return on_call(CALL_DOUBLE);
}


int embedjson_string_chunk(embedjson_parser* parser, const char* data, size_t size)
{
  return on_call(CALL_STRING_CHUNK);
}


int embedjson_begin_object(embedjson_parser* parser)
{
  return on_call(CALL_BEGIN_OBJECT);
}


int embedjson_end_object(embedjson_parser* parser)
{
  return on_call(CALL_END_OBJECT);
}


int embedjson_begin_array(embedjson_parser* parser)
{
  return on_call(CALL_BEGIN_ARRAY);
}


int embedjson_end_array(embedjson_parser* parser)
{
  return on_call(CALL_END_ARRAY);
}


/* test 01 */
static char test_01_json[] = "{}";
static data_chunk test_01_data_chunks[] = {
  {.data = test_01_json, .size = SIZEOF(test_01_json) - 1}
};
static call_type test_01_calls[] = {
  CALL_BEGIN_OBJECT,
  CALL_END_OBJECT
};


/* test 02 */
static char test_02_json[] = "[{}, null]";
static data_chunk test_02_data_chunks[] = {
  {.data = test_02_json, .size = SIZEOF(test_02_json) - 1}
};
static call_type test_02_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_BEGIN_OBJECT,
  CALL_END_OBJECT,
  CALL_NULL,
  CALL_END_ARRAY
};


/* test 03 */
static char test_03_json[] = "{\"a\":{\"b\":true, \"c\":[1, 2]}, \"d\":null}";
static data_chunk test_03_data_chunks[] = {
  {.data = test_03_json, .size = SIZEOF(test_03_json) - 1}
};
static call_type test_03_calls[] = {
  CALL_BEGIN_OBJECT,
  CALL_STRING_CHUNK,
  CALL_BEGIN_OBJECT,
  CALL_STRING_CHUNK,
  CALL_BOOL,
  CALL_STRING_CHUNK,
  CALL_BEGIN_ARRAY,
  CALL_INT,
  CALL_INT,
  CALL_END_ARRAY,
  CALL_END_OBJECT,
  CALL_STRING_CHUNK,
  CALL_NULL,
  CALL_END_OBJECT
};


/* test 04 */
static char test_04_json[] = "[]";
static data_chunk test_04_data_chunks[] = {
  {.data = test_04_json, .size = SIZEOF(test_04_json) - 1}
};
static call_type test_04_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_END_ARRAY
};


/* test 05 */
static char test_05_json[] = "[{}, [], {\"a\":[{ }]}]";
static data_chunk test_05_data_chunks[] = {
  {.data = test_05_json, .size = SIZEOF(test_05_json) - 1}
};
static call_type test_05_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_BEGIN_OBJECT,
  CALL_END_OBJECT,
  CALL_BEGIN_ARRAY,
  CALL_END_ARRAY,
  CALL_BEGIN_OBJECT,
  CALL_STRING_CHUNK,
  CALL_BEGIN_ARRAY,
  CALL_BEGIN_OBJECT,
  CALL_END_OBJECT,
  CALL_END_ARRAY,
  CALL_END_OBJECT,
  CALL_END_ARRAY
};

#define TEST_CASE(n, description) \
{ \
  .name = (description), \
  .nchunks = SIZEOF((test_##n##_data_chunks)), \
  .data_chunks = (test_##n##_data_chunks), \
  .ncalls = SIZEOF((test_##n##_calls)), \
  .calls = (test_##n##_calls)\
}


static test_case all_tests[] = {
  TEST_CASE(01, "empty object"),
  TEST_CASE(02, "array with nested object"),
  TEST_CASE(03, "three nesting levels"),
  TEST_CASE(04, "empty array"),
  TEST_CASE(05, "nested empty arrays and objects")
};


int main()
{
  size_t ntests = SIZEOF(all_tests);
  int counter_width = 1 + (int) floor(log10(ntests));
  int err;
  for (size_t i = 0; i < ntests; ++i) {
    itest = all_tests + i;
    icall = itest->calls;
    embedjson_parser parser;
    memset(&parser, 0, sizeof(parser));
    printf("[%*d/%d] Run test \"%s\" ... ", counter_width, (int) i + 1,
        (int) ntests, itest->name);
    for (size_t j = 0; j < itest->nchunks; ++j) {
      idata_chunk = itest->data_chunks + j;
      err = embedjson_push(&parser, idata_chunk->data, idata_chunk->size);
      if (err) {
        fail("embedjson_push returned non-zero (%d)", err);
      }
    }
    err = embedjson_finalize(&parser);
    if (err) {
      fail("embedjson_finalize returned non-zero (%d)", err);
    }
    if (icall != itest->calls + itest->ncalls) {
      fail("Not enough callback calls. Expected %llu, got %llu",
          (ull) itest->ncalls, (ull) (icall - itest->calls));
    }
    printf(ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET "\n");
  }
  return 0;
}

