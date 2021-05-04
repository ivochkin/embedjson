/**
 * @copyright
 * Copyright (c) 2016-2021 Stanislav Ivochkin
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
#define MAGIC ((int) (0xDEADBEEF))

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RESET "\x1b[0m"

typedef unsigned long long ull;

typedef enum call_type {
  CALL_NULL,
  CALL_BOOL,
  CALL_INT,
  CALL_DOUBLE,
  CALL_STRING_BEGIN,
  CALL_STRING_CHUNK,
  CALL_STRING_END,
  CALL_BEGIN_OBJECT,
  CALL_END_OBJECT,
  CALL_BEGIN_ARRAY,
  CALL_END_ARRAY,
  CALL_ERROR
} call_type;

const char* call_type_to_str(call_type ct)
{
  switch (ct) {
    case CALL_NULL: return "CALL_NULL";
    case CALL_BOOL: return "CALL_BOOL";
    case CALL_INT: return "CALL_INT";
    case CALL_DOUBLE: return "CALL_DOUBLE";
    case CALL_STRING_BEGIN: return "CALL_STRING_BEGIN";
    case CALL_STRING_CHUNK: return "CALL_STRING_CHUNK";
    case CALL_STRING_END: return "CALL_STRING_END";
    case CALL_BEGIN_OBJECT: return "CALL_BEGIN_OBJECT";
    case CALL_END_OBJECT: return "CALL_END_OBJECT";
    case CALL_BEGIN_ARRAY: return "CALL_BEGIN_ARRAY";
    case CALL_END_ARRAY: return "CALL_END_ARRAY";
    case CALL_ERROR: return "CALL_ERROR";
    default: return "CALL_UNKNOWN";
  };
}

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
  size_t i;
  printf(ANSI_COLOR_RED "FAILED" ANSI_COLOR_RESET "\n\n");
  printf("Data chunks:\n");
  for (i = 0; i < itest->nchunks; ++i) {
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
    fail("Unexpected call %s (%d)", call_type_to_str(call), call);
  }
  if (call != *icall) {
    fail("Call type mismatch. Expected %s (%d), got %s (%d)",
        call_type_to_str(*icall), *icall, call_type_to_str(call), call);
  }
  icall = icall + 1;
  return call == CALL_ERROR ? MAGIC : 0;
}

int embedjson_error(embedjson_parser* parser, const char* position)
{
  EMBEDJSON_UNUSED(parser);
  EMBEDJSON_UNUSED(position);
  return on_call(CALL_ERROR);
}

int embedjson_null(embedjson_parser* parser)
{
  EMBEDJSON_UNUSED(parser);
  return on_call(CALL_NULL);
}

int embedjson_bool(embedjson_parser* parser, char value)
{
  EMBEDJSON_UNUSED(parser);
  EMBEDJSON_UNUSED(value);
  return on_call(CALL_BOOL);
}

int embedjson_int(embedjson_parser* parser, embedjson_int_t value)
{
  EMBEDJSON_UNUSED(parser);
  EMBEDJSON_UNUSED(value);
  return on_call(CALL_INT);
}

int embedjson_double(embedjson_parser* parser, double value)
{
  EMBEDJSON_UNUSED(parser);
  EMBEDJSON_UNUSED(value);
  return on_call(CALL_DOUBLE);
}

int embedjson_string_begin(embedjson_parser* parser)
{
  EMBEDJSON_UNUSED(parser);
  return on_call(CALL_STRING_BEGIN);
}

int embedjson_string_chunk(embedjson_parser* parser, const char* data,
    embedjson_size_t size)
{
  EMBEDJSON_UNUSED(parser);
  EMBEDJSON_UNUSED(data);
  EMBEDJSON_UNUSED(size);
  return on_call(CALL_STRING_CHUNK);
}

int embedjson_string_end(embedjson_parser* parser)
{
  EMBEDJSON_UNUSED(parser);
  return on_call(CALL_STRING_END);
}

int embedjson_object_begin(embedjson_parser* parser)
{
  EMBEDJSON_UNUSED(parser);
  return on_call(CALL_BEGIN_OBJECT);
}

int embedjson_object_end(embedjson_parser* parser)
{
  EMBEDJSON_UNUSED(parser);
  return on_call(CALL_END_OBJECT);
}

int embedjson_array_begin(embedjson_parser* parser)
{
  EMBEDJSON_UNUSED(parser);
  return on_call(CALL_BEGIN_ARRAY);
}

int embedjson_array_end(embedjson_parser* parser)
{
  EMBEDJSON_UNUSED(parser);
  return on_call(CALL_END_ARRAY);
}

#if EMBEDJSON_DYNAMIC_STACK
int embedjson_stack_overflow(embedjson_parser* parser)
{
  char* new_stack = realloc(parser->stack, 2 * parser->stack_capacity + 1);
  if (!new_stack) {
    return -1;
  }
  parser->stack = new_stack;
  parser->stack_capacity = 2 * parser->stack_capacity + 1;
  return 0;
}
#endif

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
  CALL_STRING_BEGIN,
  CALL_STRING_CHUNK,
  CALL_STRING_END,
  CALL_BEGIN_OBJECT,
  CALL_STRING_BEGIN,
  CALL_STRING_CHUNK,
  CALL_STRING_END,
  CALL_BOOL,
  CALL_STRING_BEGIN,
  CALL_STRING_CHUNK,
  CALL_STRING_END,
  CALL_BEGIN_ARRAY,
  CALL_INT,
  CALL_INT,
  CALL_END_ARRAY,
  CALL_END_OBJECT,
  CALL_STRING_BEGIN,
  CALL_STRING_CHUNK,
  CALL_STRING_END,
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
  CALL_STRING_BEGIN,
  CALL_STRING_CHUNK,
  CALL_STRING_END,
  CALL_BEGIN_ARRAY,
  CALL_BEGIN_OBJECT,
  CALL_END_OBJECT,
  CALL_END_ARRAY,
  CALL_END_OBJECT,
  CALL_END_ARRAY
};

/* test 06 */
static char test_06_json[] = "[1.0e+10, -9.1, 10]";
static data_chunk test_06_data_chunks[] = {
  {.data = test_06_json, .size = 3},
  {.data = test_06_json + 3, .size = 12},
  {.data = test_06_json + 15, SIZEOF(test_06_json) - 16}
};
static call_type test_06_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_DOUBLE,
  CALL_DOUBLE,
  CALL_INT,
  CALL_END_ARRAY
};

/* test 07 */
static char test_07_json[] = "[\"\",]";
static data_chunk test_07_data_chunks[] = {
  {.data = test_07_json, .size = SIZEOF(test_07_json) - 1},
};
static call_type test_07_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_STRING_BEGIN,
  CALL_STRING_END,
  CALL_ERROR
};

/* test 08 */
static char test_08_json[] = "[-]";
static data_chunk test_08_data_chunks[] = {
  {.data = test_08_json, .size = SIZEOF(test_08_json) - 1},
};
static call_type test_08_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_ERROR,
};

/* test 09 */
static char test_09_json[] = "[*]";
static data_chunk test_09_data_chunks[] = {
  {.data = test_09_json, .size = SIZEOF(test_09_json) - 1},
};
static call_type test_09_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_ERROR,
};

/* test 10 */
static char test_10_json[] = "[.123]";
static data_chunk test_10_data_chunks[] = {
  {.data = test_10_json, .size = SIZEOF(test_10_json) - 1},
};
static call_type test_10_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_ERROR,
};

/* test 11 */
static char test_11_json[] = "[012]";
static data_chunk test_11_data_chunks[] = {
  {.data = test_11_json, .size = SIZEOF(test_11_json) - 1},
};
static call_type test_11_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_ERROR,
};

/* test 12 */
static char test_12_json[] = "[1]x";
static data_chunk test_12_data_chunks[] = {
  {.data = test_12_json, .size = SIZEOF(test_12_json) - 1},
};
static call_type test_12_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_INT,
  CALL_END_ARRAY,
  CALL_ERROR,
};

/* test 13 */
static char test_13_json[] = "2@";
static data_chunk test_13_data_chunks[] = {
  {.data = test_13_json, .size = SIZEOF(test_13_json) - 1},
};
static call_type test_13_calls[] = {
  CALL_INT,
  CALL_ERROR,
};

/* test 14 */
static char test_14_json[] = "{\"a\":[]}";
static data_chunk test_14_data_chunks[] = {
  {.data = test_14_json, .size = SIZEOF(test_14_json) - 1},
};
static call_type test_14_calls[] = {
  CALL_BEGIN_OBJECT,
  CALL_STRING_BEGIN,
  CALL_STRING_CHUNK,
  CALL_STRING_END,
  CALL_BEGIN_ARRAY,
  CALL_END_ARRAY,
  CALL_END_OBJECT,
};

/* test 15 */
static char test_15_json[] = "[+1]";
static data_chunk test_15_data_chunks[] = {
  {.data = test_15_json, .size = SIZEOF(test_15_json) - 1},
};
static call_type test_15_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_ERROR,
};

/* test 16 */
static char test_16_json[] = "[-2.]";
static data_chunk test_16_data_chunks[] = {
  {.data = test_16_json, .size = SIZEOF(test_16_json) - 1},
};
static call_type test_16_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_ERROR,
};

/* test 17 */
static char test_17_json[] = "[0.3e+]";
static data_chunk test_17_data_chunks[] = {
  {.data = test_17_json, .size = SIZEOF(test_17_json) - 1},
};
static call_type test_17_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_ERROR,
};

/* test 18 */
static char test_18_json[] = "[0.e1]";
static data_chunk test_18_data_chunks[] = {
  {.data = test_18_json, .size = SIZEOF(test_18_json) - 1},
};
static call_type test_18_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_ERROR,
};

/* test 19 */
static char test_19_json[] = "[1.0e+]";
static data_chunk test_19_data_chunks[] = {
  {.data = test_19_json, .size = SIZEOF(test_19_json) - 1},
};
static call_type test_19_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_ERROR,
};

/* test 20 */
static char test_20_json[] = "[1.0e-]";
static data_chunk test_20_data_chunks[] = {
  {.data = test_20_json, .size = SIZEOF(test_20_json) - 1},
};
static call_type test_20_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_ERROR,
};

/* test 21 */
static char test_21_json[] = "[2.e+3]";
static data_chunk test_21_data_chunks[] = {
  {.data = test_21_json, .size = SIZEOF(test_21_json) - 1},
};
static call_type test_21_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_ERROR,
};

/* test 22 */
static char test_22_json[] = "[2.e-3]";
static data_chunk test_22_data_chunks[] = {
  {.data = test_22_json, .size = SIZEOF(test_22_json) - 1},
};
static call_type test_22_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_ERROR,
};

/* test 23 */
static char test_23_json[] = "[2.e3]";
static data_chunk test_23_data_chunks[] = {
  {.data = test_23_json, .size = SIZEOF(test_23_json) - 1},
};
static call_type test_23_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_ERROR,
};

/* test 24 */
static char test_24_json[] = "[9.e+]";
static data_chunk test_24_data_chunks[] = {
  {.data = test_24_json, .size = SIZEOF(test_24_json) - 1},
};
static call_type test_24_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_ERROR,
};

/* test 25 */
static char test_25_json[] = "[1.]";
static data_chunk test_25_data_chunks[] = {
  {.data = test_25_json, .size = SIZEOF(test_25_json) - 1},
};
static call_type test_25_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_ERROR,
};

/* test 26 */
static char test_26_json[] = "[123e65]";
static data_chunk test_26_data_chunks[] = {
  {.data = test_26_json, .size = SIZEOF(test_26_json) - 1},
};
static call_type test_26_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_DOUBLE,
  CALL_END_ARRAY,
};

/* test 27 */
static char test_27_json[] = "[1E+2]";
static data_chunk test_27_data_chunks[] = {
  {.data = test_27_json, .size = SIZEOF(test_27_json) - 1},
};
static call_type test_27_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_DOUBLE,
  CALL_END_ARRAY,
};

/* test 28 */
static char test_28_json[] = "[1E-2]";
static data_chunk test_28_data_chunks[] = {
  {.data = test_28_json, .size = SIZEOF(test_28_json) - 1},
};
static call_type test_28_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_DOUBLE,
  CALL_END_ARRAY,
};

/* test 29 */
static char test_29_json[] = "[1e-2]";
static data_chunk test_29_data_chunks[] = {
  {.data = test_29_json, .size = SIZEOF(test_29_json) - 1},
};
static call_type test_29_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_DOUBLE,
  CALL_END_ARRAY,
};

/* test 30 */
static char test_30_json[] = "[1e+2]";
static data_chunk test_30_data_chunks[] = {
  {.data = test_30_json, .size = SIZEOF(test_30_json) - 1},
};
static call_type test_30_calls[] = {
  CALL_BEGIN_ARRAY,
  CALL_DOUBLE,
  CALL_END_ARRAY,
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
  TEST_CASE(05, "nested empty arrays and objects"),
  TEST_CASE(06, "array of three integers [1.0e+10, -9.1, 10]"),
  TEST_CASE(07, "JSONTestSuite.n_array_extra_comma"),
  TEST_CASE(08, "JSONTestSuite.n_array_just_minus"),
  TEST_CASE(09, "JSONTestSuite.n_array_star_inside"),
  TEST_CASE(10, "JSONTestSuite.n_number_starting_with_dot"),
  TEST_CASE(11, "JSONTestSuite.n_number_with_leading_zero"),
  TEST_CASE(12, "JSONTestSuite.n_structure_array_trailing_garbage"),
  TEST_CASE(13, "JSONTestSuite.n_structure_number_with_trailing_garbage"),
  TEST_CASE(14, "JSONTestSuite.y_object_simple"),
  TEST_CASE(15, "JSONTestSuite.n_number_+1"),
  TEST_CASE(16, "JSONTestSuite.n_number_-2."),
  TEST_CASE(17, "JSONTestSuite.n_number_0.3e+"),
  TEST_CASE(18, "JSONTestSuite.n_number_0.e1"),
  TEST_CASE(19, "JSONTestSuite.n_number_1.0e+"),
  TEST_CASE(20, "JSONTestSuite.n_number_1.0e-"),
  TEST_CASE(21, "JSONTestSuite.n_number_2.e+3"),
  TEST_CASE(22, "JSONTestSuite.n_number_2.e-3"),
  TEST_CASE(23, "JSONTestSuite.n_number_2.e3"),
  TEST_CASE(24, "JSONTestSuite.n_number_9.e+"),
  TEST_CASE(25, "JSONTestSuite.n_number_real_without_fractional_part"),
  TEST_CASE(26, "JSONTestSuite.y_number"),
  TEST_CASE(27, "JSONTestSuite.y_number_real_capital_e_neg_exp"),
  TEST_CASE(28, "JSONTestSuite.y_number_real_capital_e_pos_exp"),
  TEST_CASE(29, "JSONTestSuite.y_number_real_neg_exp"),
  TEST_CASE(30, "JSONTestSuite.y_number_real_pos_exponent"),
};

int main()
{
  size_t ntests = SIZEOF(all_tests);
  size_t i, j;
  int counter_width = 1 + (int) floor(log10(ntests));
  int err;
  for (i = 0; i < ntests; ++i) {
    itest = all_tests + i;
    icall = itest->calls;
    embedjson_parser parser;
    memset(&parser, 0, sizeof(parser));
    printf("[%*d/%d] Run test \"%s\" ... ", counter_width, (int) i + 1,
        (int) ntests, itest->name);
    for (j = 0; j < itest->nchunks; ++j) {
      idata_chunk = itest->data_chunks + j;
      err = embedjson_push(&parser, idata_chunk->data, idata_chunk->size);
      if (err == MAGIC) {
        break;
      } else if (err) {
        fail("embedjson_push returned unknown error (%d)", err);
      }
    }
    if (!err) {
      err = embedjson_finalize(&parser);
      if (err == MAGIC) {
        break;
      } else if (err) {
        fail("embedjson_finalize returned unknown error (%d)", err);
      }
    }
    if (icall != itest->calls + itest->ncalls) {
      fail("Not enough callback calls. Expected %llu, got %llu",
          (ull) itest->ncalls, (ull) (icall - itest->calls));
    }
    printf(ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET "\n");
  }
  return 0;
}

