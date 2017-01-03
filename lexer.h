#pragma once
#include <stdint.h> /* for int64_t */
#include <stddef.h> /* for size_t */

typedef struct embedjson_lexer {
  unsigned char state;
  unsigned char offset;
  char unicode_cp[2];
  char minus : 1;
  char is_double : 1;
  int64_t int_value;
  double double_value;
} embedjson_lexer;

typedef enum {
  EMBEDJSON_TOKEN_OPEN_CURLY_BRACKET,
  EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET,
  EMBEDJSON_TOKEN_OPEN_BRACKET,
  EMBEDJSON_TOKEN_CLOSE_BRACKET,
  EMBEDJSON_TOKEN_COMMA,
  EMBEDJSON_TOKEN_COLON,
  EMBEDJSON_TOKEN_STRING_CHUNK,
  EMBEDJSON_TOKEN_NUMBER,
  EMBEDJSON_TOKEN_TRUE,
  EMBEDJSON_TOKEN_FALSE,
  EMBEDJSON_TOKEN_NULL
} embedjson_token;

/**
 * Called by embedjson_push, results are returned by calling
 * either embedjson_on_new_token, or embedjson_error.
 *
 * @note If error occurs, lexer state remain unchanged
 */
void embedjson_lexer_push(embedjson_lexer* lexer,
    const char* data, size_t size);

/**
 * Called by embedjson_finalize, results are returned as in
 * the embedjson_lexer_push function.
 */
void embedjson_lexer_finalize(embedjson_lexer* lexer);

/**
 * Called from embedjson_lexer_push for each successfully parsed any token
 * that does not have a value, namely:
 * - EMBEDJSON_TOKEN_OPEN_CURLY_BRACKET,
 * - EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET,
 * - EMBEDJSON_TOKEN_OPEN_BRACKET,
 * - EMBEDJSON_TOKEN_CLOSE_BRACKET,
 * - EMBEDJSON_TOKEN_COMMA,
 * - EMBEDJSON_TOKEN_COLON,
 * - EMBEDJSON_TOKEN_STRING_CHUNK,
 * - EMBEDJSON_TOKEN_NUMBER,
 * - EMBEDJSON_TOKEN_TRUE,
 * - EMBEDJSON_TOKEN_FALSE,
 * - EMBEDJSON_TOKEN_NULL
 */
void embedjson_on_new_token(embedjson_lexer* lexer, embedjson_token token);

/**
 * Called from embedjson_lexer_push for each successfully parsed
 * EMBEDJSON_TOKEN_STRING_CHUNK token. A pointer to buffer that contains
 * string chunk data and it's size are provided to the callback
 */
void embedjson_on_new_tokenc(embedjson_lexer* lexer, const char* data, size_t size);

/**
 * Called from embedjson_lexer_push for each successfully parsed
 * EMBEDJSON_TOKEN_NUMBER token and it has an integer value.
 *
 * @see embedjson_on_new_tokenf
 */
void embedjson_on_new_tokeni(embedjson_lexer* lexer, int64_t value);

/**
 * Called from embedjson_lexer_push for each successfully parsed
 * EMBEDJSON_TOKEN_NUMBER token and it has a floating-point value.
 *
 * @see embedjson_on_new_tokeni
 */
void embedjson_on_new_tokenf(embedjson_lexer* lexer, double value);

