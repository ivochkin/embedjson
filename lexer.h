/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 *
 * Licensed under the MIT License (see LICENSE)
 */

#ifndef EMBEDJSON_AMALGAMATE
#pragma once
#endif /* EMBEDJSON_AMALGAMATE */
#include <stdint.h> /* for int64_t */
#include <stddef.h> /* for size_t */
#ifndef EMBEDJSON_AMALGAMATE
#include "common.h"
#endif /* EMBEDJSON_AMALGAMATE */

/**
 * JSON lexer. Transforms input stream of bytes into a stream
 * of tokens defined by JSON grammar.
 *
 * For instance, a string {[:"foo"10 will be transformed into
 * a series "open curly bracket", "open bracket", "colon",
 * "string 'foo'", "integer 10".
 *
 * Lexer does not take into consideration meaning of the tokens,
 * so a string "{{{{" will be successfully handled. Syntax checks
 * are performed by a higher-level abstraction - parser.
 *
 * @note JSON strings are not accumulated by the lexer - only user
 * provided buffers are used to provide string values to the caller.
 * That's why each JSON string value is transformed into a series
 * of EMBEDJSON_TOKEN_STRING_CHUNK tokens.
 *
 * A new string chunk is created each time one of the following events occurs:
 * - a buffer provided for embedjson_lexer_push function is parsed
 *   to the end, while lexer is in the LEXER_STATE_IN_STRING state;
 * - ASCII escape sequence is found in the string;
 * - Unicode escape sequence is found in the string.
 *
 * For the user's convenience, two supplementary methods that wrap a sequence of
 * embedjson_tokenc calls are invoked by the lexer during parsing:
 * - embedjson_tokenc_begin
 * - embedjson_tokenc_end
 */
typedef struct embedjson_lexer {
  unsigned char state;
  unsigned char offset;
  char unicode_cp[2];
  char minus : 1;
  char exp_minus : 1;
  int64_t int_value;
  uint64_t frac_value;
  uint16_t frac_power;
  uint16_t exp_value;
#if EMBEDJSON_VALIDATE_UTF8
  /**
   * Number of bytes remaining to complete multibyte UTF-8 sequence
   */
  unsigned char nb;
#endif
} embedjson_lexer;

/**
 * JSON token type
 */
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
} embedjson_tok;

/**
 * Called by embedjson_push for each data chunk to parse.
 *
 * Results are returned by calling either by a famile of embedjson_token*
 * functions:
 * - embedjson_token
 * - embedjson_tokenc
 * - embedjson_tokenc_begin
 * - embedjson_tokenc_end
 * - embedjson_tokenf
 * - embedjson_tokeni
 *
 * Errors that occurs during parsing are returned via embedjson_error call.
 *
 * @note If error occurs, lexer state remain unchanged
 */
EMBEDJSON_STATIC int embedjson_lexer_push(embedjson_lexer* lexer,
    const char* data, size_t size);

/**
 * Called by embedjson_finalize to indicate that all data has been submitted to
 * lexer.
 *
 * Results are returned as in the embedjson_lexer_push function.
 */
EMBEDJSON_STATIC int embedjson_lexer_finalize(embedjson_lexer* lexer);

/**
 * Called from embedjson_lexer_push for each successfully parsed any token
 * that does not have a value.
 *
 * A list of possibly returned token types:
 * - EMBEDJSON_TOKEN_OPEN_CURLY_BRACKET,
 * - EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET,
 * - EMBEDJSON_TOKEN_OPEN_BRACKET,
 * - EMBEDJSON_TOKEN_CLOSE_BRACKET,
 * - EMBEDJSON_TOKEN_COMMA,
 * - EMBEDJSON_TOKEN_COLON,
 * - EMBEDJSON_TOKEN_TRUE,
 * - EMBEDJSON_TOKEN_FALSE,
 * - EMBEDJSON_TOKEN_NULL
 */
EMBEDJSON_STATIC int embedjson_token(embedjson_lexer* lexer,
    embedjson_tok token);

/**
 * Called from embedjson_lexer_push for each successfully parsed
 * EMBEDJSON_TOKEN_STRING_CHUNK token.
 *
 * A pointer to buffer that contains string chunk data and it's size are
 * provided to the callback
 */
EMBEDJSON_STATIC int embedjson_tokenc(embedjson_lexer* lexer, const char* data,
    size_t size);

/**
 * Called from embedjson_lexer_push for each successfully parsed
 * EMBEDJSON_TOKEN_NUMBER token and it has an integer value.
 *
 * @see embedjson_tokenf
 */
EMBEDJSON_STATIC int embedjson_tokeni(embedjson_lexer* lexer, int64_t value);

/**
 * Called from embedjson_lexer_push for each successfully parsed
 * EMBEDJSON_TOKEN_NUMBER token and it has a floating-point value.
 *
 * @see embedjson_tokeni
 */
EMBEDJSON_STATIC int embedjson_tokenf(embedjson_lexer* lexer, double value);

/**
 * Called from embedjson_lexer_push when a beginning of the string token is
 * spotted.
 *
 * @see embedjson_tokenc, embedjson_tokenc_end
 */
EMBEDJSON_STATIC int embedjson_tokenc_begin(embedjson_lexer* lexer);

/**
 * Called from embedjson_lexer_push when string parsing is complete.
 *
 * From the user's perspective, a sequence of embedjson_tokenc calls
 * will always end with a single embedjson_tokenc_end call.
 * The call indicate that all chunks of the string were parsed.
 *
 * @see embedjson_tokenc, embedjson_tokenc_begin
 */
EMBEDJSON_STATIC int embedjson_tokenc_end(embedjson_lexer* lexer);

