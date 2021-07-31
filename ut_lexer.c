/**
 * @copyright
 * Copyright (c) 2016-2021 Stanislav Ivochkin
 *
 * Licensed under the MIT License (see LICENSE)
 */

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
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_RESET "\x1b[0m"

typedef unsigned long long ull;

typedef enum token_value_type {
  TOKEN_VALUE_TYPE_INTEGER,
  TOKEN_VALUE_TYPE_FP,
  TOKEN_VALUE_TYPE_STR,
} token_value_type;

typedef enum additional_token_type {
  EMBEDJSON_TOKEN_NUMBER = EMBEDJSON_TOKEN_NULL + 1,
  EMBEDJSON_TOKEN_STRING_BEGIN,
  EMBEDJSON_TOKEN_STRING_CHUNK,
  EMBEDJSON_TOKEN_STRING_END,
  EMBEDJSON_TOKEN_BIGNUM_BEGIN,
  EMBEDJSON_TOKEN_BIGNUM_CHUNK,
  EMBEDJSON_TOKEN_BIGNUM_END,
  EMBEDJSON_TOKEN_ERROR
} additional_token_type;

typedef struct token_info {
  int type;
  token_value_type value_type;
  union {
    embedjson_int_t integer;
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
  int enabled;
  const char* name;
  size_t nchunks;
  data_chunk* data_chunks;
  size_t ntokens;
  token_info* tokens;
} test_case;

static test_case* itest = NULL;
static data_chunk* idata_chunk = NULL;
static token_info* itoken = NULL;

static const char* token_type_to_str(int type)
{
  switch(type) {
    case EMBEDJSON_TOKEN_OPEN_CURLY_BRACKET:
      return "EMBEDJSON_TOKEN_OPEN_CURLY_BRACKET (0)";
    case EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET:
      return "EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET (1)";
    case EMBEDJSON_TOKEN_OPEN_BRACKET:
      return "EMBEDJSON_TOKEN_OPEN_BRACKET (2)";
    case EMBEDJSON_TOKEN_CLOSE_BRACKET:
      return "EMBEDJSON_TOKEN_CLOSE_BRACKET (3)";
    case EMBEDJSON_TOKEN_COMMA:
      return "EMBEDJSON_TOKEN_COMMA (4)";
    case EMBEDJSON_TOKEN_COLON:
      return "EMBEDJSON_TOKEN_COLON (5)";
    case EMBEDJSON_TOKEN_TRUE:
      return "EMBEDJSON_TOKEN_TRUE (6)";
    case EMBEDJSON_TOKEN_FALSE:
      return "EMBEDJSON_TOKEN_FALSE (7)";
    case EMBEDJSON_TOKEN_NULL:
      return "EMBEDJSON_TOKEN_NULL (8)";
    case EMBEDJSON_TOKEN_NUMBER:
      return "EMBEDJSON_TOKEN_NUMBER (9)";
    case EMBEDJSON_TOKEN_STRING_BEGIN:
      return "EMBEDJSON_TOKEN_STRING_BEGIN (10)";
    case EMBEDJSON_TOKEN_STRING_CHUNK:
      return "EMBEDJSON_TOKEN_STRING_CHUNK (11)";
    case EMBEDJSON_TOKEN_STRING_END:
      return "EMBEDJSON_TOKEN_STRING_END (12)";
    case EMBEDJSON_TOKEN_BIGNUM_BEGIN:
      return "EMBEDJSON_TOKEN_BIGNUM_BEGIN (13)";
    case EMBEDJSON_TOKEN_BIGNUM_CHUNK:
      return "EMBEDJSON_TOKEN_BIGNUM_CHUNK (14)";
    case EMBEDJSON_TOKEN_BIGNUM_END:
      return "EMBEDJSON_TOKEN_BIGNUM_END (15)";
    case EMBEDJSON_TOKEN_ERROR:
      return "EMBEDJSON_TOKEN_ERROR (16)";
    default:
      return "Unknown token";
  };
}

static const char* token_value_type_to_str(token_value_type type)
{
  switch(type) {
    case TOKEN_VALUE_TYPE_INTEGER:
      return "TOKEN_VALUE_TYPE_INTEGER (0)";
    case TOKEN_VALUE_TYPE_FP:
      return "TOKEN_VALUE_TYPE_FP (1)";
    case TOKEN_VALUE_TYPE_STR:
      return "TOKEN_VALUE_TYPE_STR (2)";
    default:
      return "Unknown token type";
  }
}

static void fail(const char* position, const char* fmt, ...)
{
  size_t i;
  printf(ANSI_COLOR_RED "FAILED" ANSI_COLOR_RESET "\n\n");
  printf("Position: %s\n", position);
  printf("Data chunks:\n");
  for (i = 0; i < itest->nchunks; ++i) {
    data_chunk c = itest->data_chunks[i];
    printf("%llu. \"%.*s\"\n", (ull) i + 1, (int) c.size, c.data);
  }
  printf("Expected tokens:\n");
  for (i = 0; i < itest->ntokens; ++i) {
    printf("%llu. %s\n", (ull) i + 1, token_type_to_str(itest->tokens[i].type));
  }
  printf("Failed on chunk: %llu of %llu\n",
      (ull) (idata_chunk - itest->data_chunks + 1), (ull) itest->nchunks);
  printf("Failed on token: %ld of %ld\n", itoken - itest->tokens + 1, itest->ntokens);
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  printf("\n");
  va_end(args);
  exit(1);
}

static int on_token(token_info ti, const char* position)
{
  if (itoken == itest->tokens + itest->ntokens) {
    fail(position, "Unexpected token %s, value type %s", token_type_to_str(ti.type),
        token_value_type_to_str(ti.value_type));
  }
  if (ti.type != itoken->type) {
    fail(position, "Token type mismatch. Expected %s, got %s",
        token_type_to_str(itoken->type), token_type_to_str(ti.type));
  }
  if (ti.type == EMBEDJSON_TOKEN_STRING_CHUNK
      || ti.type == EMBEDJSON_TOKEN_NUMBER
      || ti.type == EMBEDJSON_TOKEN_BIGNUM_BEGIN
      || ti.type == EMBEDJSON_TOKEN_BIGNUM_CHUNK ) {
    if (ti.value_type != itoken->value_type) {
      fail(position, "Value type mismatch. Expected %s, got %s",
          token_value_type_to_str(itoken->value_type),
          token_value_type_to_str(ti.value_type));
    }
    switch(ti.value_type) {
      case TOKEN_VALUE_TYPE_FP:
        if (ti.value.fp != itoken->value.fp) {
          fail(position, "Floating-point value mismatch. Expected %lf, got %lf",
              itoken->value.fp, ti.value.fp);
        }
        break;
      case TOKEN_VALUE_TYPE_STR:
        if (ti.value.str.size != itoken->value.str.size) {
          fail(position, "String value size mismatch. Expected %llu bytes, got %llu",
              (ull) itoken->value.str.size, (ull) ti.value.str.size);
        }
        if (memcmp(ti.value.str.data, itoken->value.str.data, ti.value.str.size)) {
          int size = (int) ti.value.str.size;
          fail(position, "String value mismatch. Expected \"%.*s\", got \"%.*s\"",
              size, itoken->value.str.data, size, ti.value.str.data);
        }
        break;
      case TOKEN_VALUE_TYPE_INTEGER:
        if (ti.value.integer != itoken->value.integer) {
          fail(position, "Integer value mismatch. Expected %lld, got %lld",
              (long long) itoken->value.integer, (long long) ti.value.integer);
        }
        break;
      default:
        break;
    }
  }
  itoken = itoken + 1;
  return 0;
}

int embedjson_error(struct embedjson_parser* parser, const char* position)
{
  EMBEDJSON_UNUSED(parser);
  return on_token((token_info) {
      .type = EMBEDJSON_TOKEN_ERROR,
      .value_type = TOKEN_VALUE_TYPE_STR,
      .value = {
        .str = {.data = position, .size = 0}
      }
    },
    position
  );
}

int embedjson_token(embedjson_lexer* lexer, embedjson_tok token,
    const char* position)
{
  EMBEDJSON_UNUSED(lexer);
  EMBEDJSON_UNUSED(position);
  return on_token((token_info) {.type = token}, position);
}

int embedjson_tokenc(embedjson_lexer* lexer, const char* data,
    embedjson_size_t size)
{
  EMBEDJSON_UNUSED(lexer);
  return on_token((token_info) {
      .type = EMBEDJSON_TOKEN_STRING_CHUNK,
      .value_type = TOKEN_VALUE_TYPE_STR,
      .value = {
        .str = {.data = data, .size = size}
      }
    },
    data
  );
}

int embedjson_tokenc_begin(embedjson_lexer* lexer, const char* position)
{
  EMBEDJSON_UNUSED(lexer);
  EMBEDJSON_UNUSED(position);
  return on_token((token_info) {.type = EMBEDJSON_TOKEN_STRING_BEGIN}, position);
}

int embedjson_tokenc_end(embedjson_lexer* lexer, const char* position)
{
  EMBEDJSON_UNUSED(lexer);
  EMBEDJSON_UNUSED(position);
  return on_token((token_info) {.type = EMBEDJSON_TOKEN_STRING_END}, position);
}

int embedjson_tokeni(embedjson_lexer* lexer, embedjson_int_t value,
    const char* position)
{
  EMBEDJSON_UNUSED(lexer);
  EMBEDJSON_UNUSED(position);
  return on_token((token_info) {
      .type = EMBEDJSON_TOKEN_NUMBER,
      .value_type = TOKEN_VALUE_TYPE_INTEGER,
      .value = {.integer = value}
    },
    position
  );
}

int embedjson_tokenf(embedjson_lexer* lexer, double value, const char* position)
{
  EMBEDJSON_UNUSED(lexer);
  EMBEDJSON_UNUSED(position);
  return on_token((token_info) {
      .type = EMBEDJSON_TOKEN_NUMBER,
      .value_type = TOKEN_VALUE_TYPE_FP,
      .value = {.fp = value}
    },
    position
  );
}

#if EMBEDJSON_BIGNUM
int embedjson_tokenbn_begin(embedjson_lexer* lexer,
    const char* position, embedjson_int_t initial_value)
{
  EMBEDJSON_UNUSED(lexer);
  EMBEDJSON_UNUSED(position);
  return on_token((token_info) {
      .type = EMBEDJSON_TOKEN_BIGNUM_BEGIN,
      .value_type = TOKEN_VALUE_TYPE_INTEGER,
      .value = {.integer = initial_value}
    },
    position
  );
}

int embedjson_tokenbn(embedjson_lexer* lexer, const char* data,
    embedjson_size_t size)
{
  EMBEDJSON_UNUSED(lexer);
  return on_token((token_info) {
      .type = EMBEDJSON_TOKEN_BIGNUM_CHUNK,
      .value_type = TOKEN_VALUE_TYPE_STR,
      .value = {
        .str = {.data = data, .size = size}
      }
    },
    data
  );
}

int embedjson_tokenbn_end(embedjson_lexer* lexer,
    const char* position)
{
  EMBEDJSON_UNUSED(lexer);
  return on_token((token_info) {.type = EMBEDJSON_TOKEN_BIGNUM_END}, position);
}
#endif /* EMBEDJSON_BIGNUM */

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
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
  {
    .type = EMBEDJSON_TOKEN_STRING_CHUNK,
    .value_type = TOKEN_VALUE_TYPE_STR,
    .value = {.str = {.data = "hello,", .size = 6}}
  },
  {
    .type = EMBEDJSON_TOKEN_STRING_CHUNK,
    .value_type = TOKEN_VALUE_TYPE_STR,
    .value = {.str = {.data = " world", .size = 6}}
  },
  {.type = EMBEDJSON_TOKEN_STRING_END}
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
static char test_05_json[] = "{}[]:, {\n{\t[ \r ]\n}\r}";
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
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
  {
    .type = EMBEDJSON_TOKEN_STRING_CHUNK,
    .value_type = TOKEN_VALUE_TYPE_STR,
    .value = {.str = {.data = test_06_json + 1, .size = SIZEOF(test_06_json) - 3}}
  },
  {.type = EMBEDJSON_TOKEN_STRING_END}
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
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
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
  },
  {.type = EMBEDJSON_TOKEN_STRING_END}
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
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
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
  },
  {.type = EMBEDJSON_TOKEN_STRING_END}
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
static char test_10_json[] = "-10.0152e-2 10000.00 99.0000001";
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
  },
  {
    .type = EMBEDJSON_TOKEN_NUMBER,
    .value_type = TOKEN_VALUE_TYPE_FP,
    .value = {.fp = 99.0000001 }
  }
};

/**
 * test 11
 *
 * Unicode Character 'CYRILLIC CAPITAL LETTER ZHE' (U+0416),
 * with corrupted second byte
 *
 * A valid sequence is \xd0\96
 */
static char test_11_json[] = "\"\xd0\x16\"";
static data_chunk test_11_data_chunks[] = {
  {.data = test_11_json, .size = sizeof(test_11_json) - 1}
};
static token_info test_11_tokens[] = {
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
  {.type = EMBEDJSON_TOKEN_ERROR}
};

/**
 * test 12
 *
 * Unicode Han Character 'right conduct, righteousness' (U+4E49),
 * with corrupted third byte
 *
 * A valid sequence is \xe4\xb9\x89
 */
static char test_12_json[] = "\"\xe4\xb9\xc9\"";
static data_chunk test_12_data_chunks[] = {
  {.data = test_12_json, .size = sizeof(test_12_json) - 1}
};
static token_info test_12_tokens[] = {
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
  {.type = EMBEDJSON_TOKEN_ERROR}
};

/**
 * test 13
 *
 * Unicode Character 'GOTHIC LETTER AHSA' (U+10330),
 * with last byte missing
 *
 * A valid sequence is \xf0\x90\x8c\xb0
 */
static char test_13_json[] = "\"\xf0\x90\x8c\"";
static data_chunk test_13_data_chunks[] = {
  {.data = test_13_json, .size = sizeof(test_13_json) - 1}
};
static token_info test_13_tokens[] = {
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
  {.type = EMBEDJSON_TOKEN_ERROR}
};

/**
 * test 14
 *
 * Malformed UTF-8 sequence, that extends UTF-8 encoding principles for 5 byte
 * sequence. However, 5 byte UTF-8 sequences do not exist.
 */
static char test_14_json[] = "\"\xfb\x80\x80\x80\x80\"";
static data_chunk test_14_data_chunks[] = {
  {.data = test_14_json, .size = sizeof(test_14_json) - 1}
};
static token_info test_14_tokens[] = {
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
  {.type = EMBEDJSON_TOKEN_ERROR}
};

/**
 * test 15
 *
 * Non-shortest UTF-8 form, corner case 1 (see lexer.h for description)
 */
static char test_15_json[] = "\"\xe0\x80\x85\"";
static data_chunk test_15_data_chunks[] = {
  {.data = test_15_json, .size = sizeof(test_15_json) - 1}
};
static token_info test_15_tokens[] = {
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
  {.type = EMBEDJSON_TOKEN_ERROR}
};

/**
 * test 16
 *
 * Non-shortest UTF-8 form, corner case 2 (see lexer.h for description)
 */
static char test_16_json[] = "\"\xf0\x8f\x80\x80\"";
static data_chunk test_16_data_chunks[] = {
  {.data = test_16_json, .size = sizeof(test_16_json) - 1}
};
static token_info test_16_tokens[] = {
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
  {.type = EMBEDJSON_TOKEN_ERROR}
};

/**
 * test 17
 *
 * Non-shortest UTF-8 form, corner case 3 (see lexer.h for description)
 */
static char test_17_json[] = "\"\xf4\xbf\x80\x80\"";
static data_chunk test_17_data_chunks[] = {
  {.data = test_17_json, .size = sizeof(test_17_json) - 1}
};
static token_info test_17_tokens[] = {
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
  {.type = EMBEDJSON_TOKEN_ERROR}
};

/**
 * test 18
 *
 * Null character '\u0000' within string. Null character is a valid unicode
 * symbol and can occur in any place of the string. It breaks ASCII convention
 * that zero bytes indicates end of the string.
 *
 * However, JSON spec disallows ascii control characters in string (below 0x20)
 */
static char test_18_json[] = "\"Hello,\x00world\" null";
static data_chunk test_18_data_chunks[] = {
  {.data = test_18_json, .size = sizeof(test_18_json) - 1}
};
static token_info test_18_tokens[] = {
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
  {.type = EMBEDJSON_TOKEN_ERROR},
};

/**
 * test 19
 *
 * Test case n_array_extra_comma from the JSONTestSuite
 */
static char test_19_json[] = "[\"\",]";
static data_chunk test_19_data_chunks[] = {
  {.data = test_19_json, .size = sizeof(test_19_json) - 1}
};
static token_info test_19_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_BRACKET},
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
  {.type = EMBEDJSON_TOKEN_STRING_END},
  {.type = EMBEDJSON_TOKEN_COMMA},
  {.type = EMBEDJSON_TOKEN_CLOSE_BRACKET}
};

/**
 * test 20
 *
 * Test case n_number_with_leading_zero from the JSONTestSuite
 */
static char test_20_json[] = "[012]";
static data_chunk test_20_data_chunks[] = {
  {.data = test_20_json, .size = sizeof(test_20_json) - 1}
};
static token_info test_20_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_BRACKET},
  {.type = EMBEDJSON_TOKEN_ERROR}
};

/**
 * test 21
 *
 * Test case y_object_simple from the JSONTestSuite
 */
static char test_21_json[] = "{\"a\":[]}";
static data_chunk test_21_data_chunks[] = {
  {.data = test_21_json, .size = sizeof(test_21_json) - 1}
};
static token_info test_21_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_CURLY_BRACKET},
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
  {
    .type = EMBEDJSON_TOKEN_STRING_CHUNK,
    .value_type = TOKEN_VALUE_TYPE_STR,
    .value = {.str = {.data = "a", .size = 1}},
  },
  {.type = EMBEDJSON_TOKEN_STRING_END},
  {.type = EMBEDJSON_TOKEN_COLON},
  {.type = EMBEDJSON_TOKEN_OPEN_BRACKET},
  {.type = EMBEDJSON_TOKEN_CLOSE_BRACKET},
  {.type = EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET},
};

/**
 * test 22
 *
 * Test case n_number_+1 from the JSONTestSuite
 */
static char test_22_json[] = "[+1]";
static data_chunk test_22_data_chunks[] = {
  {.data = test_22_json, .size = sizeof(test_22_json) - 1}
};
static token_info test_22_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_BRACKET},
  {.type = EMBEDJSON_TOKEN_ERROR}
};

/**
 * test 23
 *
 * Test case y_number from the JSONTestSuite
 */
static char test_23_json[] = "[123e65]";
static data_chunk test_23_data_chunks[] = {
  {.data = test_23_json, .size = sizeof(test_23_json) - 1}
};
static token_info test_23_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_BRACKET},
  {
    .type = EMBEDJSON_TOKEN_NUMBER,
    .value_type = TOKEN_VALUE_TYPE_FP,
    .value = {.fp = 123e65}
  },
  {.type = EMBEDJSON_TOKEN_CLOSE_BRACKET},
};

/**
 * test 24
 *
 * Test case n_string_backslash_00 from the JSONTestSuite
 */
static char test_24_json[] = "[\"\\\x00\"]";
static data_chunk test_24_data_chunks[] = {
  {.data = test_24_json, .size = sizeof(test_24_json) - 1}
};
static token_info test_24_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_BRACKET},
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
  {.type = EMBEDJSON_TOKEN_ERROR},
};

/**
 * test 25
 *
 * Test case n_string_escape_x from the JSONTestSuite
 */
static char test_25_json[] = "[\"\\x00\"]";
static data_chunk test_25_data_chunks[] = {
  {.data = test_25_json, .size = sizeof(test_25_json) - 1}
};
static token_info test_25_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_BRACKET},
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
  {.type = EMBEDJSON_TOKEN_ERROR},
};

/**
 * test 26
 *
 * Test case n_string_escaped_ctrl_char_tab from the JSONTestSuite
 */
static char test_26_json[] = "[\"\\\x09\"]";
static data_chunk test_26_data_chunks[] = {
  {.data = test_26_json, .size = sizeof(test_26_json) - 1}
};
static token_info test_26_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_BRACKET},
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
  {.type = EMBEDJSON_TOKEN_ERROR},
};

/**
 * test 27
 *
 * Test case n_string_escaped_emoji from the JSONTestSuite
 */
static char test_27_json[] = "[\"\\\xF0\x9F\x8C\x80\"]";
static data_chunk test_27_data_chunks[] = {
  {.data = test_27_json, .size = sizeof(test_27_json) - 1}
};
static token_info test_27_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_BRACKET},
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
  {.type = EMBEDJSON_TOKEN_ERROR},
};

/**
 * test 28
 *
 * Test case n_string_incomplete_surrogate_escape_invalid from the JSONTestSuite
 */
static char test_28_json[] = "[\"\\uD800\\uD800\\x\"]";
static data_chunk test_28_data_chunks[] = {
  {.data = test_28_json, .size = sizeof(test_28_json) - 1}
};
static token_info test_28_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_BRACKET},
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
  {
    .type = EMBEDJSON_TOKEN_STRING_CHUNK,
    .value_type = TOKEN_VALUE_TYPE_STR,
    .value = {.str = {.data = "\xd8\x00", .size = 2}}
  },
  {
    .type = EMBEDJSON_TOKEN_STRING_CHUNK,
    .value_type = TOKEN_VALUE_TYPE_STR,
    .value = {.str = {.data = "\xd8\x00", .size = 2}}
  },
  {.type = EMBEDJSON_TOKEN_ERROR},
};

/**
 * test 29
 *
 * Test case n_string_invalid_backslash_esc from the JSONTestSuite
 */
static char test_29_json[] = "[\"\\a\"]";
static data_chunk test_29_data_chunks[] = {
  {.data = test_29_json, .size = sizeof(test_29_json) - 1}
};
static token_info test_29_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_BRACKET},
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
  {.type = EMBEDJSON_TOKEN_ERROR},
};

/**
 * test 30
 *
 * Test case n_string_unicode_CapitalU from the JSONTestSuite
 */
static char test_30_json[] = "\"\\UA66D\"";
static data_chunk test_30_data_chunks[] = {
  {.data = test_30_json, .size = sizeof(test_30_json) - 1}
};
static token_info test_30_tokens[] = {
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
  {.type = EMBEDJSON_TOKEN_ERROR},
};

/**
 * test 31
 *
 * Test case n_string_invalid_utf8_after_escape from the JSONTestSuite
 */
static char test_31_json[] = "[\"\\\xE5\"]";
static data_chunk test_31_data_chunks[] = {
  {.data = test_31_json, .size = sizeof(test_31_json) - 1}
};
static token_info test_31_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_BRACKET},
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
  {.type = EMBEDJSON_TOKEN_ERROR},
};

/**
 * test 32
 *
 * Test case n_string_unescaped_ctrl_char from the JSONTestSuite
 */
static char test_32_json[] = "[\"a\x00a\"]";
static data_chunk test_32_data_chunks[] = {
  {.data = test_32_json, .size = sizeof(test_32_json) - 1}
};
static token_info test_32_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_BRACKET},
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
  {.type = EMBEDJSON_TOKEN_ERROR},
};

/**
 * test 33
 *
 * Test case n_string_unescaped_tab from the JSONTestSuite
 */
static char test_33_json[] = "[\"\x09\"]";
static data_chunk test_33_data_chunks[] = {
  {.data = test_33_json, .size = sizeof(test_33_json) - 1}
};
static token_info test_33_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_BRACKET},
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
  {.type = EMBEDJSON_TOKEN_ERROR},
};

/**
 * test 34
 *
 * Test case n_string_unescaped_newline from the JSONTestSuite
 */
static char test_34_json[] = "[\"new\x0Aline\"]";
static data_chunk test_34_data_chunks[] = {
  {.data = test_34_json, .size = sizeof(test_34_json) - 1}
};
static token_info test_34_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_BRACKET},
  {.type = EMBEDJSON_TOKEN_STRING_BEGIN},
  {.type = EMBEDJSON_TOKEN_ERROR},
};

/**
 * test 35
 *
 * Test case i_number_huge_exp from the JSONTestSuite
 */
static char test_35_json[] =
    "[0.4e006699999999999999999999999999999999999999999999999999999999999999999999999999"
    "99999999999999999999999999999999999999999969999999006]";
static data_chunk test_35_data_chunks[] = {
  {.data = test_35_json, .size = sizeof(test_35_json) - 1}
};
static token_info test_35_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_BRACKET},
  {.type = EMBEDJSON_TOKEN_ERROR},
};

/**
 * test 36
 *
 * Test case i_number_neg_int_huge_exp from the JSONTestSuite
 */
static char test_36_json[] = "[-1e+9999]";
static data_chunk test_36_data_chunks[] = {
  {.data = test_36_json, .size = sizeof(test_36_json) - 1}
};
static token_info test_36_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_BRACKET},
  {.type = EMBEDJSON_TOKEN_ERROR},
};

/**
 * test 37
 *
 * Test case i_number_pos_double_huge_exp from the JSONTestSuite
 */
static char test_37_json[] = "[1.5e+9999]";
static data_chunk test_37_data_chunks[] = {
  {.data = test_37_json, .size = sizeof(test_37_json) - 1}
};
static token_info test_37_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_BRACKET},
  {.type = EMBEDJSON_TOKEN_ERROR},
};

/**
 * test 38
 *
 * Test case i_number_real_neg_overflow from the JSONTestSuite
 */
static char test_38_json[] = "[-123123e100000]";
static data_chunk test_38_data_chunks[] = {
  {.data = test_38_json, .size = sizeof(test_38_json) - 1}
};
static token_info test_38_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_BRACKET},
  {.type = EMBEDJSON_TOKEN_ERROR},
};

/**
 * test 39
 *
 * Test case i_number_real_pos_overflow from the JSONTestSuite
 */
static char test_39_json[] = "[123123e100000]";
static data_chunk test_39_data_chunks[] = {
  {.data = test_39_json, .size = sizeof(test_39_json) - 1}
};
static token_info test_39_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_BRACKET},
  {.type = EMBEDJSON_TOKEN_ERROR},
};

/**
 * test 40
 *
 * Test case i_number_real_underflow from the JSONTestSuite
 */
static char test_40_json[] = "[123e-10000000]";
static data_chunk test_40_data_chunks[] = {
  {.data = test_40_json, .size = sizeof(test_40_json) - 1}
};
static token_info test_40_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_BRACKET},
  {.type = EMBEDJSON_TOKEN_ERROR},
};

/**
 * test 41
 *
 * Test case n_object_non_string_key_but_huge_number_instead from the JSONTestSuite
 */
static char test_41_json[] = "{9999E9999:1}";
static data_chunk test_41_data_chunks[] = {
  {.data = test_41_json, .size = sizeof(test_41_json) - 1}
};
static token_info test_41_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_CURLY_BRACKET},
  {.type = EMBEDJSON_TOKEN_ERROR},
};

/**
 * test 42
 *
 * valid big integer
 */
static char test_42_json[] = "{1000000000000000000000000000000000}";
static data_chunk test_42_data_chunks[] = {
  {.data = test_42_json, .size = sizeof(test_42_json) - 1}
};
static token_info test_42_tokens[] = {
  {.type = EMBEDJSON_TOKEN_OPEN_CURLY_BRACKET},
  {
    .type = EMBEDJSON_TOKEN_BIGNUM_BEGIN,
    .value_type = TOKEN_VALUE_TYPE_INTEGER,
    .value = {.integer = 1000000000000000000},
  },
  {
    .type = EMBEDJSON_TOKEN_BIGNUM_CHUNK,
    .value_type = TOKEN_VALUE_TYPE_STR,
    .value = {.str = {.data = "00000000000000", .size = 14}}
  },
  {.type = EMBEDJSON_TOKEN_BIGNUM_END},
  {.type = EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET},
};

/**
 * test 43
 *
 * just big integer
 */
static char test_43_json[] = "1000000000000000000000000000000000";
static data_chunk test_43_data_chunks[] = {
  {.data = test_43_json, .size = sizeof(test_43_json) - 1}
};
static token_info test_43_tokens[] = {
  {
    .type = EMBEDJSON_TOKEN_BIGNUM_BEGIN,
    .value_type = TOKEN_VALUE_TYPE_INTEGER,
    .value = {.integer = 1000000000000000000},
  },
  {
    .type = EMBEDJSON_TOKEN_BIGNUM_CHUNK,
    .value_type = TOKEN_VALUE_TYPE_STR,
    .value = {.str = {.data = "00000000000000", .size = 14}}
  },
  {.type = EMBEDJSON_TOKEN_BIGNUM_END},
};


#define TEST_CASE(n, description) \
{ \
  .enabled = 1, \
  .name = (description), \
  .nchunks = SIZEOF((test_##n##_data_chunks)), \
  .data_chunks = (test_##n##_data_chunks), \
  .ntokens = SIZEOF((test_##n##_tokens)), \
  .tokens = (test_##n##_tokens) \
}

#if EMBEDJSON_VALIDATE_UTF8
#define TEST_CASE_IF_VALIDATE_UTF8(n, description) TEST_CASE(n, description)
#else
#define TEST_CASE_IF_VALIDATE_UTF8(n, description) \
{ \
  .enabled = 0, \
  .name = (description), \
  .nchunks = SIZEOF((test_##n##_data_chunks)), \
  .data_chunks = (test_##n##_data_chunks), \
  .ntokens = SIZEOF((test_##n##_tokens)), \
  .tokens = (test_##n##_tokens) \
}
#endif

#if EMBEDJSON_BIGNUM
#define TEST_CASE_IF_BIGNUM(n, description) TEST_CASE(n, description)
#else
#define TEST_CASE_IF_BIGNUM(n, description) \
{ \
  .enabled = 0, \
  .name = (description), \
  .nchunks = SIZEOF((test_##n##_data_chunks)), \
  .data_chunks = (test_##n##_data_chunks), \
  .ntokens = SIZEOF((test_##n##_tokens)), \
  .tokens = (test_##n##_tokens) \
}
#endif

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
  TEST_CASE(10, "reset state after parsing double, -10.0152e-2 10000.00"),
  TEST_CASE_IF_VALIDATE_UTF8(11, "bad second byte in two-byte utf-8 sequence"),
  TEST_CASE_IF_VALIDATE_UTF8(12, "bad third byte in three-byte utf-8 sequence"),
  TEST_CASE_IF_VALIDATE_UTF8(13,
      "missing last byte in four-byte utf-8 sequence"),
  TEST_CASE_IF_VALIDATE_UTF8(14, "5 byte utf-8 sequence"),
  TEST_CASE_IF_VALIDATE_UTF8(15, "UTF-8 non-shortest form, case 1"),
  TEST_CASE_IF_VALIDATE_UTF8(16, "UTF-8 non-shortest form, case 2"),
  TEST_CASE_IF_VALIDATE_UTF8(17, "UTF-8 non-shortest form, case 3"),
  TEST_CASE_IF_VALIDATE_UTF8(18, "Null character within UTF-8 string"),
  TEST_CASE(19, "JSONTestSuite.n_array_extra_comma"),
  TEST_CASE(20, "JSONTestSuite.n_number_with_leading_zero"),
  TEST_CASE(21, "JSONTestSuite.y_object_simple"),
  TEST_CASE(22, "JSONTestSuite.n_number_+1"),
  TEST_CASE(23, "JSONTestSuite.y_number"),
  TEST_CASE(24, "JSONTestSuite.n_string_backslash_00"),
  TEST_CASE(25, "JSONTestSuite.n_string_escape_x"),
  TEST_CASE(26, "JSONTestSuite.n_string_escaped_ctrl_char_tab"),
  TEST_CASE(27, "JSONTestSuite.n_string_escaped_emoji"),
  TEST_CASE(28, "JSONTestSuite.n_string_incomplete_surrogate_escape_invalid"),
  TEST_CASE(29, "JSONTestSuite.n_string_invalid_backslash_esc"),
  TEST_CASE(30, "JSONTestSuite.n_string_invalid_utf8_after_escape"),
  TEST_CASE(31, "JSONTestSuite.n_string_unicode_CapitalU"),
  TEST_CASE_IF_VALIDATE_UTF8(32, "JSONTestSuite.n_string_unescaped_ctrl_char"),
  TEST_CASE_IF_VALIDATE_UTF8(33, "JSONTestSuite.n_string_unescaped_newline"),
  TEST_CASE_IF_VALIDATE_UTF8(34, "JSONTestSuite.n_string_unescaped_tab"),
  TEST_CASE(35, "JSONTestSuite.i_number_huge_exp"),
  TEST_CASE(36, "JSONTestSuite.i_number_neg_int_huge_exp"),
  TEST_CASE(37, "JSONTestSuite.i_number_pos_double_huge_exp"),
  TEST_CASE(38, "JSONTestSuite.i_number_real_neg_overflow"),
  TEST_CASE(39, "JSONTestSuite.i_number_real_pos_overflow"),
  TEST_CASE(40, "JSONTestSuite.i_number_real_underflow"),
  TEST_CASE(41, "JSONTestSuite.n_object_non_string_key_but_huge_number_instead"),
  TEST_CASE_IF_BIGNUM(42, "valid big integer"),
  TEST_CASE_IF_BIGNUM(43, "just big integer"),
};

int main()
{
  size_t ntests = SIZEOF(all_tests);
  size_t i, j;
  int counter_width = 1 + (int) floor(log10(ntests));
  for (i = 0; i < ntests; ++i) {
    itest = all_tests + i;
    printf("[%*d/%d] Run test \"%s\" ... ", counter_width, (int) i + 1,
        (int) ntests, itest->name);
    if (!itest->enabled) {
      printf(ANSI_COLOR_YELLOW "SKIPPED" ANSI_COLOR_RESET "\n");
      continue;
    }
    itoken = itest->tokens;
    embedjson_lexer lexer;
    memset(&lexer, 0, sizeof(lexer));
    for (j = 0; j < itest->nchunks; ++j) {
      idata_chunk = itest->data_chunks + j;
      embedjson_lexer_push(&lexer, idata_chunk->data, idata_chunk->size);
    }
    embedjson_lexer_finalize(&lexer);
    if (itoken != itest->tokens + itest->ntokens) {
      fail("EOF", "Not enough tokens. Expected %llu, got %llu",
          (ull) itest->ntokens, (ull) (itoken - itest->tokens));
    }
    printf(ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET "\n");
  }
  return 0;
}

