#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include "lexer.h"


#define SIZEOF(x) sizeof((x)) / sizeof((x)[0])

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RESET "\x1b[0m"

typedef unsigned long long ull;

typedef enum token_value_type {
  TOKEN_VALUE_TYPE_INTEGER,
  TOKEN_VALUE_TYPE_FP,
  TOKEN_VALUE_TYPE_STR,
} token_value_type;


typedef struct token_info {
  int type;
  token_value_type value_type;
  union {
    int64_t integer;
    double fp;
    struct {
      const char* data;
      size_t size;
    } str;
  } value;
} token_info;


typedef struct data_chunk {
  const char* data;
  size_t size;
} data_chunk;


typedef struct test_case {
  const char* name;
  size_t nchunks;
  data_chunk* data_chunks;
  size_t ntokens;
  token_info* tokens;
} test_case;


static test_case* itest = NULL;
static data_chunk* idata_chunk = NULL;
static token_info* itoken = NULL;


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


static void on_token(token_info ti)
{
  if (itoken == itest->tokens + itest->ntokens) {
    fail("Unexpected token of type %d, value type %d", ti.type, ti.value_type);
  }
  if (ti.type != itoken->type) {
    fail("Token type mismatch. Expected %d, got %d", itoken->type, ti.type);
  }
  if (ti.type == EMBEDJSON_TOKEN_STRING_CHUNK || ti.type == EMBEDJSON_TOKEN_NUMBER) {
    if (ti.value_type != itoken->value_type) {
      fail("Value type mismatch. Expected %d, got %d", itoken->value_type, ti.value_type);
    }
    switch(ti.value_type) {
      case TOKEN_VALUE_TYPE_FP:
        if (ti.value.fp != itoken->value.fp) {
          fail("Floating-point value mismatch. Expected %lf, got %lf",
              itoken->value.fp, ti.value.fp);
        }
        break;
      case TOKEN_VALUE_TYPE_STR:
        if (ti.value.str.size != itoken->value.str.size) {
          fail("String value size mismatch. Expected %llu bytes, got %llu",
              (ull) itoken->value.str.size, (ull) ti.value.str.size);
        }
        if (memcmp(ti.value.str.data, itoken->value.str.data, ti.value.str.size)) {
          int size = (int) ti.value.str.size;
          fail("String value mismatch. Expected \"%.*s\", got \"%.*s\"",
              size, itoken->value.str.data, size, ti.value.str.data);
        }
        break;
      case TOKEN_VALUE_TYPE_INTEGER:
        if (ti.value.integer != itoken->value.integer) {
          fail("Integer value mismatch. Expected %lld, got %lld",
              (long long) itoken->value.integer, (long long) ti.value.integer);
        }
        break;
      default:
        break;
    }
  }
  itoken = itoken + 1;
}


void embedjson_error(const char* position)
{
  fail("Lexer error near \"%.*s\"", 4, position);
}

void embedjson_token(embedjson_lexer* lexer, embedjson_tok token)
{
  on_token((token_info) {.type = token});
}


void embedjson_tokenc(embedjson_lexer* lexer, const char* data, size_t size)
{
  on_token((token_info) {
      .type = EMBEDJSON_TOKEN_STRING_CHUNK,
      .value_type = TOKEN_VALUE_TYPE_STR,
      .value = {
        .str = {.data = data, .size = size}
      }
    }
  );
}


void embedjson_tokeni(embedjson_lexer* lexer, int64_t value)
{
  on_token((token_info) {
      .type = EMBEDJSON_TOKEN_NUMBER,
      .value_type = TOKEN_VALUE_TYPE_INTEGER,
      .value = {.integer = value}
    }
  );
}


void embedjson_tokenf(embedjson_lexer* lexer, double value)
{
  on_token((token_info) {
      .type = EMBEDJSON_TOKEN_NUMBER,
      .value_type = TOKEN_VALUE_TYPE_FP,
      .value = {.fp = value}
    }
  );
}


/* test 01 */
static char test_01_json[] = "{}";
static data_chunk test_01_data_chunks[] = {
  {.data = test_01_json, .size = SIZEOF(test_01_json) - 1}
};
static token_info test_01_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_CURLY_BRACKET},
  {.type = EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET},
};


/* test 02 */
static char test_02_json[] = "\"hello, world\"";
static data_chunk test_02_data_chunks[] = {
  {.data = test_02_json, .size = 7},
  {.data = test_02_json + 7, .size = SIZEOF(test_02_json) - 8}
};
static token_info test_02_tokens[] = {
  {
    .type = EMBEDJSON_TOKEN_STRING_CHUNK,
    .value_type = TOKEN_VALUE_TYPE_STR,
    .value = {.str = {.data = "hello,", .size = 6}}
  },
  {
    .type = EMBEDJSON_TOKEN_STRING_CHUNK,
    .value_type = TOKEN_VALUE_TYPE_STR,
    .value = {.str = {.data = " world", .size = 6}}
  }
};


/* test 03 */
static char test_03_json[] = "true false null";
static data_chunk test_03_data_chunks[] = {
  {.data = test_03_json, .size = 3},
  {.data = test_03_json + 3, .size = 5},
  {.data = test_03_json + 8, .size = SIZEOF(test_03_json) - 9},
};
static token_info test_03_tokens[] = {
  {.type = EMBEDJSON_TOKEN_TRUE},
  {.type = EMBEDJSON_TOKEN_FALSE},
  {.type = EMBEDJSON_TOKEN_NULL}
};


/* test 04 */
static char test_04_json[] = "10 0 -99999";
static data_chunk test_04_data_chunks[] = {
  {.data = test_04_json, .size = 1},
  {.data = test_04_json + 1, .size = 2},
  {.data = test_04_json + 3, .size = 3},
  {.data = test_04_json + 6, .size = SIZEOF(test_04_json) - 7}
};
static token_info test_04_tokens[] = {
  {
    .type = EMBEDJSON_TOKEN_NUMBER,
    .value_type = TOKEN_VALUE_TYPE_INTEGER,
    .value = {.integer = 10}
  },
  {
    .type = EMBEDJSON_TOKEN_NUMBER,
    .value_type = TOKEN_VALUE_TYPE_INTEGER,
    .value = {.integer = 0}
  },
  {
    .type = EMBEDJSON_TOKEN_NUMBER,
    .value_type = TOKEN_VALUE_TYPE_INTEGER,
    .value = {.integer = -99999}
  }
};


/* test 05 */
static char test_05_json[] = "{}[]:, {\n{\t[ \r ]\b}\f}";
static data_chunk test_05_data_chunks[] = {
  {.data = test_05_json, .size = SIZEOF(test_05_json) - 1}
};
static token_info test_05_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_CURLY_BRACKET},
  {.type = EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET},
  {.type = EMBEDJSON_TOKEN_OPEN_BRACKET},
  {.type = EMBEDJSON_TOKEN_CLOSE_BRACKET},
  {.type = EMBEDJSON_TOKEN_COLON},
  {.type = EMBEDJSON_TOKEN_COMMA},
  {.type = EMBEDJSON_TOKEN_OPEN_CURLY_BRACKET},
  {.type = EMBEDJSON_TOKEN_OPEN_CURLY_BRACKET},
  {.type = EMBEDJSON_TOKEN_OPEN_BRACKET},
  {.type = EMBEDJSON_TOKEN_CLOSE_BRACKET},
  {.type = EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET},
  {.type = EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET},
};


/* test 06 */
static char test_06_json[] = "\""
    "\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82\x20\xE5\x97\xA8\x20\xD9\x85\xD8"
    "\xB1\xD8\xAD\xD8\xA8\xD8\xA7\x20\xE0\xA8\x85\xE0\xA8\xA7\xE0\xA8\xBF\xE0\xA8\x95"
    "\xE0\xA8\xA4\xE0\xA8\xAE\x20\xEC\x95\x88\xEB\x85\x95\x20\xDB\x81\xDB\x8C\xD9\x84"
    "\xD9\x88\x20\xD1\x81\xD3\x99\xD0\xBB\xD0\xB0\xD0\xBC\x20\xE0\xB8\xAA\xE0\xB8\xA7"
    "\xE0\xB8\xB1\xE0\xB8\xAA\xE0\xB8\x94\xE0\xB8\xB5\x20\xE0\xA4\xB9\xE0\xA4\xBE\xE0"
    "\xA4\xAF\x20\x68\x69"
    "\"";
static data_chunk test_06_data_chunks[] = {
  {.data = test_06_json, .size = SIZEOF(test_06_json) - 1}
};
static token_info test_06_tokens[] = {
  {
    .type = EMBEDJSON_TOKEN_STRING_CHUNK,
    .value_type = TOKEN_VALUE_TYPE_STR,
    .value = {.str = {.data = test_06_json + 1, .size = SIZEOF(test_06_json) - 3}}
  }
};


/* test 07 */
static char test_07_json[] = "\"\\r\\n\\t\\b\\f\\\"\"";
static data_chunk test_07_data_chunks[] = {
  {.data = test_07_json, .size = 4},
  {.data = test_07_json + 4, .size = 3},
  {.data = test_07_json + 7, .size = 5},
  {.data = test_07_json + 12, .size = SIZEOF(test_07_json) - 13}
};
static token_info test_07_tokens[] = {
  {
    .type = EMBEDJSON_TOKEN_STRING_CHUNK,
    .value_type = TOKEN_VALUE_TYPE_STR,
    .value = {.str = {.data = "\r", .size = 1}}
  },
  {
    .type = EMBEDJSON_TOKEN_STRING_CHUNK,
    .value_type = TOKEN_VALUE_TYPE_STR,
    .value = {.str = {.data = "\n", .size = 1}}
  },
  {
    .type = EMBEDJSON_TOKEN_STRING_CHUNK,
    .value_type = TOKEN_VALUE_TYPE_STR,
    .value = {.str = {.data = "\t", .size = 1}}
  },
  {
    .type = EMBEDJSON_TOKEN_STRING_CHUNK,
    .value_type = TOKEN_VALUE_TYPE_STR,
    .value = {.str = {.data = "\b", .size = 1}}
  },
  {
    .type = EMBEDJSON_TOKEN_STRING_CHUNK,
    .value_type = TOKEN_VALUE_TYPE_STR,
    .value = {.str = {.data = "\f", .size = 1}}
  },
  {
    .type = EMBEDJSON_TOKEN_STRING_CHUNK,
    .value_type = TOKEN_VALUE_TYPE_STR,
    .value = {.str = {.data = "\"", .size = 1}}
  }
};


/* test 08 */
static char test_08_json[] = "\"\\uD0BF\\ud180\\uD0B8\\ud0B2\\uD0b5\\uD182\"";
static data_chunk test_08_data_chunks[] = {
  {.data = test_08_json, .size = 6},
  {.data = test_08_json + 6, .size = 3},
  {.data = test_08_json + 9, .size = 3},
  {.data = test_08_json + 12, .size = 14},
  {.data = test_08_json + 26, .size = 5},
  {.data = test_08_json + 31, .size = SIZEOF(test_08_json) - 32},
};
static token_info test_08_tokens[] = {
  {
    .type = EMBEDJSON_TOKEN_STRING_CHUNK,
    .value_type = TOKEN_VALUE_TYPE_STR,
    .value = {.str = {.data = "п", .size = 2}}
  },
  {
    .type = EMBEDJSON_TOKEN_STRING_CHUNK,
    .value_type = TOKEN_VALUE_TYPE_STR,
    .value = {.str = {.data = "р", .size = 2}}
  },
  {
    .type = EMBEDJSON_TOKEN_STRING_CHUNK,
    .value_type = TOKEN_VALUE_TYPE_STR,
    .value = {.str = {.data = "и", .size = 2}}
  },
  {
    .type = EMBEDJSON_TOKEN_STRING_CHUNK,
    .value_type = TOKEN_VALUE_TYPE_STR,
    .value = {.str = {.data = "в", .size = 2}}
  },
  {
    .type = EMBEDJSON_TOKEN_STRING_CHUNK,
    .value_type = TOKEN_VALUE_TYPE_STR,
    .value = {.str = {.data = "е", .size = 2}}
  },
  {
    .type = EMBEDJSON_TOKEN_STRING_CHUNK,
    .value_type = TOKEN_VALUE_TYPE_STR,
    .value = {.str = {.data = "т", .size = 2}}
  }
};


/* test 09 */
static char test_09_json[] = "-10.0152e+02";
static data_chunk test_09_data_chunks[] = {
  {.data = test_09_json, .size = SIZEOF(test_09_json) - 1}
};
static token_info test_09_tokens[] = {
  {
    .type = EMBEDJSON_TOKEN_NUMBER,
    .value_type = TOKEN_VALUE_TYPE_FP,
    .value = {.fp = -1001.52 }
  }
};


/* test 10 */
static char test_10_json[] = "-10.0152e-2 10000.00";
static data_chunk test_10_data_chunks[] = {
  {.data = test_10_json, .size = 6},
  {.data = test_10_json + 6, .size = 9},
  {.data = test_10_json + 15, .size = SIZEOF(test_10_json) - 16}
};
static token_info test_10_tokens[] = {
  {
    .type = EMBEDJSON_TOKEN_NUMBER,
    .value_type = TOKEN_VALUE_TYPE_FP,
    .value = {.fp = -0.100152 }
  },
  {
    .type = EMBEDJSON_TOKEN_NUMBER,
    .value_type = TOKEN_VALUE_TYPE_FP,
    .value = {.fp = 10000. }
  }
};


#define TEST_CASE(n, description) \
{ \
  .name = (description), \
  .nchunks = SIZEOF((test_##n##_data_chunks)), \
  .data_chunks = (test_##n##_data_chunks), \
  .ntokens = SIZEOF((test_##n##_tokens)), \
  .tokens = (test_##n##_tokens)\
}


static test_case all_tests[] = {
  TEST_CASE(01, "empty object"),
  TEST_CASE(02, "string split into two chunks"),
  TEST_CASE(03, "string literals: true false null"),
  TEST_CASE(04, "integers: 10 0 -99999"),
  TEST_CASE(05, "brackets, comma and colon separated by escaped wss"),
  TEST_CASE(06, "raw unicode string"),
  TEST_CASE(07, "ascii escaping"),
  TEST_CASE(08, "unicode escaping"),
  TEST_CASE(09, "double -10.0152+e02"),
  TEST_CASE(10, "reset state after parsing double, -10.0152e-2 10000.00")
};


int main()
{
  size_t ntests = SIZEOF(all_tests);
  int counter_width = 1 + (int) floor(log10(ntests));
  for (size_t i = 0; i < ntests; ++i) {
    itest = all_tests + i;
    itoken = itest->tokens;
    embedjson_lexer lexer;
    memset(&lexer, 0, sizeof(lexer));
    printf("[%*d/%d] Run test \"%s\" ... ", counter_width, (int) i + 1,
        (int) ntests, itest->name);
    for (size_t j = 0; j < itest->nchunks; ++j) {
      idata_chunk = itest->data_chunks + j;
      embedjson_lexer_push(&lexer, idata_chunk->data, idata_chunk->size);
    }
    embedjson_lexer_finalize(&lexer);
    if (itoken != itest->tokens + itest->ntokens) {
      fail("Not enough tokens. Expected %llu, got %llu",
          (ull) itest->ntokens, (ull) (itoken - itest->tokens));
    }
    printf(ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET "\n");
  }
  return 0;
}

