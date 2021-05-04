/**
 * @copyright
 * Copyright (c) 2016-2021 Stanislav Ivochkin
 *
 * Licensed under the MIT License (see LICENSE)
 */

#ifndef EMBEDJSON_AMALGAMATE
#pragma once
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
 * That's why each JSON string value is transformed into a possibly
 * empty series of embedjson_tokenc calls.
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
  char exp_not_empty : 1;
  embedjson_int_t int_value;
  unsigned long long frac_value;
  unsigned short frac_power;
  unsigned short exp_value;
#if EMBEDJSON_VALIDATE_UTF8
  /**
   * Number of bytes remaining to complete multibyte UTF-8 sequence
   */
  unsigned char nb;
  /**
   * Corner cases for shortest possible UTF-8 encoding issue.
   *
   * See http://www.unicode.org/versions/corrigendum1.html for detailed
   * explanation of the issue and provided solution.
   *
   * Possible values are:
   *
   * @li 1 - for code points U+0800..U+0FFF. For these code points three bytes
   * are needed for encoding. If the first byte value is \xe0 (11100000), then
   * allowed values for the second byte are not \x80..\xbf, but \xa0..\xbf.
   *
   * @li 2 - for code points U+10000..U+3FFFF. For these code points four bytes
   * are needed for encoding. If the first byte value is \xf0 (11110000), then
   * allowed values for the second byte are not \x80..\xbf, but \x90..\xbf.
   *
   * @li 3 - for code points U+100000..U+10FFFF. If the first byte value
   * is \xf4, then allowed values for the second byte are not \x80..\xbf,
   * but \x80..\x8f.
   */
  unsigned char cc;
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
  EMBEDJSON_TOKEN_TRUE,
  EMBEDJSON_TOKEN_FALSE,
  EMBEDJSON_TOKEN_NULL
} embedjson_tok;

/**
 * Called by embedjson_push for each data chunk to parse.
 *
 * Results are returned by calling a family of embedjson_token*
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
    const char* data, embedjson_size_t size);

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
    embedjson_tok token, const char* position);

/**
 * Called from embedjson_lexer_push for each successfully parsed
 * string chunk.
 *
 * A pointer to buffer that contains string chunk data and it's size are
 * provided to the callback
 *
 * @see embedjson_tokenc_begin, embedjson_tokenc_end
 */
EMBEDJSON_STATIC int embedjson_tokenc(embedjson_lexer* lexer, const char* data,
    embedjson_size_t size);

/**
 * Called from embedjson_lexer_push for each successfully parsed
 * integer value.
 *
 * @see embedjson_tokenf
 */
EMBEDJSON_STATIC int embedjson_tokeni(embedjson_lexer* lexer, embedjson_int_t value,
    const char* position);

/**
 * Called from embedjson_lexer_push for each successfully parsed
 * floating-point value.
 *
 * @see embedjson_tokeni
 */
EMBEDJSON_STATIC int embedjson_tokenf(embedjson_lexer* lexer, double value,
    const char* position);

/**
 * Called from embedjson_lexer_push when a beginning of the string token is
 * spotted.
 *
 * @see embedjson_tokenc, embedjson_tokenc_end
 */
EMBEDJSON_STATIC int embedjson_tokenc_begin(embedjson_lexer* lexer,
    const char* position);

/**
 * Called from embedjson_lexer_push when string parsing is complete.
 *
 * From the user's perspective, a sequence of embedjson_tokenc calls
 * will always end with a single embedjson_tokenc_end call.
 * The call indicate that all chunks of the string were parsed.
 *
 * @see embedjson_tokenc, embedjson_tokenc_begin
 */
EMBEDJSON_STATIC int embedjson_tokenc_end(embedjson_lexer* lexer,
    const char* position);

