/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 *
 * Licensed under the MIT License (see LICENSE)
 */

#ifndef EMBEDJSON_AMALGAMATE
#include "common.h"
#include "parser.h"
#endif /* EMBEDJSON_AMALGAMATE */

typedef enum {
  PARSER_STATE_EXPECT_VALUE = 0,
  PARSER_STATE_MAYBE_OBJECT_KEY,
  PARSER_STATE_EXPECT_OBJECT_KEY,
  PARSER_STATE_EXPECT_COLON,
  PARSER_STATE_MAYBE_OBJECT_COMMA,
  PARSER_STATE_EXPECT_OBJECT_VALUE,
  PARSER_STATE_MAYBE_ARRAY_VALUE,
  PARSER_STATE_EXPECT_ARRAY_VALUE,
  PARSER_STATE_MAYBE_ARRAY_COMMA,
  PARSER_STATE_DONE,
  PARSER_STATE_INVALID /* Should be the last enum value */
} embedjson_parser_state;

#if EMBEDJSON_DEBUG
#define EMBEDJSON_CHECK_STATE(parser) \
do { \
  if ((parser)->state == PARSER_STATE_INVALID) { \
    return embedjson_error_ex((parser), EMBEDJSON_INTERNAL_ERROR, position); \
  } \
} while (0)
#else
#define EMBEDJSON_CHECK_STATE(parser) (void) parser
#endif

typedef enum {
  STACK_VALUE_CURLY = 0,
  STACK_VALUE_SQUARE = 1
} embedjson_parser_stack_value;

#if EMBEDJSON_DYNAMIC_STACK
#define EMBEDJSON_STACK_CAPACITY(p) (p)->stack_capacity
#else
#define EMBEDJSON_STACK_CAPACITY(p) sizeof((p)->stack)
#endif

/* Returns result of expression (f) if it evaluates to non-zero */
#ifndef EMBEDJSON_RETURN_IF
#define EMBEDJSON_RETURN_IF(f) \
do { \
  int err = (f); \
  if (err) { \
    return err; \
  } \
} while (0)
#endif

/* embedjson_zero[i] contains a byte of all bits set to one except the i-th*/
static const unsigned char embedjson_zero[] = {
  0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F
};

/* embedjson_one[i] contains a byte of all bits set to zero except the i-th*/
static const unsigned char embedjson_one[] = {
  0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
};

static unsigned char stack_empty(embedjson_parser* parser)
{
  return !parser->stack_size;
}

static unsigned char stack_full(embedjson_parser* parser)
{
  const unsigned char max_size = 8 * sizeof(char) * EMBEDJSON_STACK_CAPACITY(parser);
  return parser->stack_size == max_size;
}

static int stack_push(embedjson_parser* parser, unsigned char value)
{
  if (stack_full(parser)) {
#if EMBEDJSON_DYNAMIC_STACK
    EMBEDJSON_RETURN_IF(embedjson_stack_overflow(parser));
#else
    return embedjson_error_ex(parser, EMBEDJSON_STACK_OVERFLOW, 0);
#endif
  }
  embedjson_size_t nbucket = parser->stack_size / 8;
  embedjson_size_t nbit = parser->stack_size % 8;
  if (value) {
    parser->stack[nbucket] |= embedjson_one[nbit];
  } else {
    parser->stack[nbucket] &= embedjson_zero[nbit];
  }
  parser->stack_size++;
  return 0;
}

static void stack_pop(embedjson_parser* parser)
{
  parser->stack_size--;
}

static unsigned char stack_top(embedjson_parser* parser)
{
  embedjson_size_t nbucket = (parser->stack_size - 1) / 8;
  embedjson_size_t nbit = (parser->stack_size - 1) % 8;
  return parser->stack[nbucket] & embedjson_one[nbit];
}

EMBEDJSON_STATIC int embedjson_push(embedjson_parser* parser, const char* data, embedjson_size_t size)
{
  return embedjson_lexer_push(&parser->lexer, data, size);
}

EMBEDJSON_STATIC int embedjson_finalize(embedjson_parser* parser)
{
  EMBEDJSON_RETURN_IF(embedjson_lexer_finalize(&parser->lexer));
  if (parser->state != PARSER_STATE_DONE) {
    return embedjson_error_ex(parser, EMBEDJSON_INSUFFICIENT_INPUT, 0);
  }
  return 0;
}

EMBEDJSON_STATIC int embedjson_token(embedjson_lexer* lexer,
    embedjson_tok token, const char* position)
{
  /*
   * See doc/syntax-parser-fsm.dot for the explanation what's
   * going on below.
   */
  embedjson_parser* parser = (embedjson_parser*) lexer;
  switch (parser->state) {
    case PARSER_STATE_EXPECT_VALUE:
      switch (token) {
        case EMBEDJSON_TOKEN_OPEN_CURLY_BRACKET:
          EMBEDJSON_RETURN_IF(stack_push(parser, STACK_VALUE_CURLY));
          EMBEDJSON_RETURN_IF(embedjson_object_begin(parser));
          parser->state = PARSER_STATE_MAYBE_OBJECT_KEY;
          break;
        case EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET:
          return embedjson_error_ex(parser, EMBEDJSON_UNEXP_CLOSE_CURLY,
              position);
        case EMBEDJSON_TOKEN_OPEN_BRACKET:
          EMBEDJSON_RETURN_IF(stack_push(parser, STACK_VALUE_SQUARE));
          EMBEDJSON_RETURN_IF(embedjson_array_begin(parser));
          parser->state = PARSER_STATE_MAYBE_ARRAY_VALUE;
          break;
        case EMBEDJSON_TOKEN_CLOSE_BRACKET:
          return embedjson_error_ex(parser, EMBEDJSON_UNEXP_CLOSE_BRACKET,
              position);
        case EMBEDJSON_TOKEN_COMMA:
          return embedjson_error_ex(parser, EMBEDJSON_UNEXP_COMMA, position);
        case EMBEDJSON_TOKEN_COLON:
          return embedjson_error_ex(parser, EMBEDJSON_UNEXP_COLON, position);
        case EMBEDJSON_TOKEN_TRUE:
          EMBEDJSON_RETURN_IF(embedjson_bool(parser, 1));
          parser->state = PARSER_STATE_DONE;
          break;
        case EMBEDJSON_TOKEN_FALSE:
          EMBEDJSON_RETURN_IF(embedjson_bool(parser, 0));
          parser->state = PARSER_STATE_DONE;
          break;
        case EMBEDJSON_TOKEN_NULL:
          EMBEDJSON_RETURN_IF(embedjson_null(parser));
          parser->state = PARSER_STATE_DONE;
          break;
        default:
          return embedjson_error_ex(parser, EMBEDJSON_INTERNAL_ERROR, position);
      }
      break;
    case PARSER_STATE_MAYBE_OBJECT_KEY:
      if (token == EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET) {
#if EMBEDJSON_DEBUG
        if (stack_empty(parser) || stack_top(parser) != STACK_VALUE_CURLY) {
          return embedjson_error_ex(parser, EMBEDJSON_INTERNAL_ERROR, position);
        }
#endif
        EMBEDJSON_RETURN_IF(embedjson_object_end(parser));
        stack_pop(parser);
        if (stack_empty(parser)) {
          parser->state = PARSER_STATE_DONE;
        } else if (stack_top(parser) == STACK_VALUE_CURLY) {
          parser->state = PARSER_STATE_MAYBE_OBJECT_COMMA;
        } else {
          parser->state = PARSER_STATE_MAYBE_ARRAY_COMMA;
        }
      } else {
        return embedjson_error_ex(parser, EMBEDJSON_EXP_OBJECT_KEY, position);
      }
      break;
    case PARSER_STATE_EXPECT_OBJECT_KEY:
      return embedjson_error_ex(parser, EMBEDJSON_EXP_OBJECT_KEY, position);
    case PARSER_STATE_EXPECT_COLON:
      if (token != EMBEDJSON_TOKEN_COLON) {
        return embedjson_error_ex(parser, EMBEDJSON_EXP_COLON, position);
      }
      parser->state = PARSER_STATE_EXPECT_OBJECT_VALUE;
      break;
    case PARSER_STATE_MAYBE_OBJECT_COMMA:
      if (token == EMBEDJSON_TOKEN_COMMA) {
        parser->state = PARSER_STATE_EXPECT_OBJECT_KEY;
      } else if (token == EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET) {
#if EMBEDJSON_DEBUG
        if (stack_empty(parser) || stack_top(parser) != STACK_VALUE_CURLY) {
          return embedjson_error_ex(parser, EMBEDJSON_INTERNAL_ERROR, position);
        }
#endif
        EMBEDJSON_RETURN_IF(embedjson_object_end(parser));
        stack_pop(parser);
        if (stack_empty(parser)) {
          parser->state = PARSER_STATE_DONE;
        } else if (stack_top(parser) == STACK_VALUE_CURLY) {
          parser->state = PARSER_STATE_MAYBE_OBJECT_COMMA;
        } else {
          parser->state = PARSER_STATE_MAYBE_ARRAY_COMMA;
        }
      } else {
        return embedjson_error_ex(parser, EMBEDJSON_EXP_COMMA_OR_CLOSE_CURLY, position);
      }
      break;
    case PARSER_STATE_EXPECT_OBJECT_VALUE:
      switch (token) {
        case EMBEDJSON_TOKEN_OPEN_CURLY_BRACKET:
          EMBEDJSON_RETURN_IF(stack_push(parser, STACK_VALUE_CURLY));
          EMBEDJSON_RETURN_IF(embedjson_object_begin(parser));
          parser->state = PARSER_STATE_MAYBE_OBJECT_KEY;
          break;
        case EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET:
          return embedjson_error_ex(parser, EMBEDJSON_UNEXP_CLOSE_CURLY, position);
        case EMBEDJSON_TOKEN_OPEN_BRACKET:
          EMBEDJSON_RETURN_IF(stack_push(parser, STACK_VALUE_SQUARE));
          EMBEDJSON_RETURN_IF(embedjson_array_begin(parser));
          parser->state = PARSER_STATE_EXPECT_ARRAY_VALUE;
          break;
        case EMBEDJSON_TOKEN_CLOSE_BRACKET:
          return embedjson_error_ex(parser, EMBEDJSON_UNEXP_CLOSE_BRACKET, position);
        case EMBEDJSON_TOKEN_COMMA:
          return embedjson_error_ex(parser, EMBEDJSON_UNEXP_COMMA, position);
        case EMBEDJSON_TOKEN_COLON:
          return embedjson_error_ex(parser, EMBEDJSON_UNEXP_COLON, position);
        case EMBEDJSON_TOKEN_TRUE:
          EMBEDJSON_RETURN_IF(embedjson_bool(parser, 1));
          parser->state = PARSER_STATE_MAYBE_OBJECT_COMMA;
          break;
        case EMBEDJSON_TOKEN_FALSE:
          EMBEDJSON_RETURN_IF(embedjson_bool(parser, 0));
          parser->state = PARSER_STATE_MAYBE_OBJECT_COMMA;
          break;
        case EMBEDJSON_TOKEN_NULL:
          EMBEDJSON_RETURN_IF(embedjson_null(parser));
          parser->state = PARSER_STATE_MAYBE_OBJECT_COMMA;
          break;
      }
      break;
    case PARSER_STATE_MAYBE_ARRAY_VALUE:
      switch (token) {
        case EMBEDJSON_TOKEN_OPEN_CURLY_BRACKET:
          EMBEDJSON_RETURN_IF(stack_push(parser, STACK_VALUE_CURLY));
          EMBEDJSON_RETURN_IF(embedjson_object_begin(parser));
          parser->state = PARSER_STATE_MAYBE_OBJECT_KEY;
          break;
        case EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET:
          return embedjson_error_ex(parser, EMBEDJSON_UNEXP_CLOSE_CURLY,
              position);
        case EMBEDJSON_TOKEN_OPEN_BRACKET:
          EMBEDJSON_RETURN_IF(stack_push(parser, STACK_VALUE_SQUARE));
          EMBEDJSON_RETURN_IF(embedjson_array_begin(parser));
          break;
        case EMBEDJSON_TOKEN_CLOSE_BRACKET:
#if EMBEDJSON_DEBUG
          if (stack_empty(parser) || stack_top(parser) == STACK_VALUE_CURLY) {
            return embedjson_error_ex(parser, EMBEDJSON_INTERNAL_ERROR,
                position);
          }
#endif
          EMBEDJSON_RETURN_IF(embedjson_array_end(parser));
          stack_pop(parser);
          if (stack_empty(parser)) {
            parser->state = PARSER_STATE_DONE;
          } else if (stack_top(parser) == STACK_VALUE_CURLY) {
            parser->state = PARSER_STATE_MAYBE_OBJECT_KEY;
          } else {
            parser->state = PARSER_STATE_MAYBE_ARRAY_COMMA;
          }
          break;
        case EMBEDJSON_TOKEN_COMMA:
          return embedjson_error_ex(parser, EMBEDJSON_UNEXP_COMMA, position);
        case EMBEDJSON_TOKEN_COLON:
          return embedjson_error_ex(parser, EMBEDJSON_UNEXP_COLON, position);
        case EMBEDJSON_TOKEN_TRUE:
          EMBEDJSON_RETURN_IF(embedjson_bool(parser, 1));
          parser->state = PARSER_STATE_MAYBE_ARRAY_COMMA;
          break;
        case EMBEDJSON_TOKEN_FALSE:
          EMBEDJSON_RETURN_IF(embedjson_bool(parser, 0));
          parser->state = PARSER_STATE_MAYBE_ARRAY_COMMA;
          break;
        case EMBEDJSON_TOKEN_NULL:
          EMBEDJSON_RETURN_IF(embedjson_null(parser));
          parser->state = PARSER_STATE_MAYBE_ARRAY_COMMA;
          break;
        default:
          return embedjson_error_ex(parser, EMBEDJSON_INTERNAL_ERROR, position);
      }
      break;
    case PARSER_STATE_EXPECT_ARRAY_VALUE:
      switch (token) {
        case EMBEDJSON_TOKEN_OPEN_CURLY_BRACKET:
          EMBEDJSON_RETURN_IF(stack_push(parser, STACK_VALUE_CURLY));
          EMBEDJSON_RETURN_IF(embedjson_object_begin(parser));
          parser->state = PARSER_STATE_MAYBE_OBJECT_KEY;
          break;
        case EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET:
          return embedjson_error_ex(parser, EMBEDJSON_UNEXP_CLOSE_CURLY,
              position);
        case EMBEDJSON_TOKEN_OPEN_BRACKET:
          EMBEDJSON_RETURN_IF(stack_push(parser, STACK_VALUE_SQUARE));
          EMBEDJSON_RETURN_IF(embedjson_array_begin(parser));
          parser->state = PARSER_STATE_MAYBE_ARRAY_VALUE;
          break;
        case EMBEDJSON_TOKEN_CLOSE_BRACKET:
          return embedjson_error_ex(parser, EMBEDJSON_UNEXP_CLOSE_BRACKET,
              position);
        case EMBEDJSON_TOKEN_COMMA:
          return embedjson_error_ex(parser, EMBEDJSON_UNEXP_COMMA, position);
        case EMBEDJSON_TOKEN_COLON:
          return embedjson_error_ex(parser, EMBEDJSON_UNEXP_COLON, position);
        case EMBEDJSON_TOKEN_TRUE:
          EMBEDJSON_RETURN_IF(embedjson_bool(parser, 1));
          parser->state = PARSER_STATE_MAYBE_ARRAY_COMMA;
          break;
        case EMBEDJSON_TOKEN_FALSE:
          EMBEDJSON_RETURN_IF(embedjson_bool(parser, 0));
          parser->state = PARSER_STATE_MAYBE_ARRAY_COMMA;
          break;
        case EMBEDJSON_TOKEN_NULL:
          EMBEDJSON_RETURN_IF(embedjson_null(parser));
          parser->state = PARSER_STATE_MAYBE_ARRAY_COMMA;
          break;
        default:
          return embedjson_error_ex(parser, EMBEDJSON_INTERNAL_ERROR, position);
      }
      break;
    case PARSER_STATE_MAYBE_ARRAY_COMMA:
      if (token == EMBEDJSON_TOKEN_COMMA) {
        parser->state = PARSER_STATE_EXPECT_ARRAY_VALUE;
      } else if (token == EMBEDJSON_TOKEN_CLOSE_BRACKET) {
#if EMBEDJSON_DEBUG
        if (stack_empty(parser) || stack_top(parser) != STACK_VALUE_SQUARE) {
          return embedjson_error_ex(parser, EMBEDJSON_INTERNAL_ERROR, position);
        }
#endif
        EMBEDJSON_RETURN_IF(embedjson_array_end(parser));
        stack_pop(parser);
        if (stack_empty(parser)) {
          parser->state = PARSER_STATE_DONE;
        } else if (stack_top(parser) == STACK_VALUE_CURLY) {
          parser->state = PARSER_STATE_MAYBE_OBJECT_COMMA;
        } else {
          parser->state = PARSER_STATE_MAYBE_ARRAY_COMMA;
        }
      } else {
        return embedjson_error_ex(parser, EMBEDJSON_EXP_COMMA_OR_CLOSE_BRACKET, position);
      }
      break;
    case PARSER_STATE_DONE:
      return embedjson_error_ex(parser, EMBEDJSON_EXCESSIVE_INPUT, position);
#if EMBEDJSON_DEBUG
    case PARSER_STATE_INVALID:
    default:
      return embedjson_error_ex(parser, EMBEDJSON_INTERNAL_ERROR, position);
#endif
  }
  return 0;
}

EMBEDJSON_STATIC int embedjson_tokenc(embedjson_lexer* lexer, const char* data,
    embedjson_size_t size)
{
  embedjson_parser* parser = (embedjson_parser*) lexer;
  EMBEDJSON_CHECK_STATE(parser);
  return embedjson_string_chunk(parser, data, size);
}

EMBEDJSON_STATIC int embedjson_tokeni(embedjson_lexer* lexer, long long value,
    const char* position)
{
  embedjson_parser* parser = (embedjson_parser*) lexer;
  switch (parser->state) {
    case PARSER_STATE_EXPECT_VALUE:
      parser->state = PARSER_STATE_DONE;
      return embedjson_int(parser, value);
    case PARSER_STATE_MAYBE_OBJECT_KEY:
      return embedjson_error_ex(parser, EMBEDJSON_EXP_OBJECT_KEY_OR_CLOSE_CURLY, position);
    case PARSER_STATE_EXPECT_OBJECT_KEY:
      return embedjson_error_ex(parser, EMBEDJSON_EXP_OBJECT_KEY, position);
    case PARSER_STATE_EXPECT_COLON:
      return embedjson_error_ex(parser, EMBEDJSON_EXP_COLON, position);
    case PARSER_STATE_MAYBE_OBJECT_COMMA:
      return embedjson_error_ex(parser, EMBEDJSON_EXP_COMMA_OR_CLOSE_BRACKET, position);
    case PARSER_STATE_EXPECT_OBJECT_VALUE:
      parser->state = PARSER_STATE_MAYBE_OBJECT_COMMA;
      return embedjson_int(parser, value);
    case PARSER_STATE_MAYBE_ARRAY_VALUE:
    case PARSER_STATE_EXPECT_ARRAY_VALUE:
      parser->state = PARSER_STATE_MAYBE_ARRAY_COMMA;
      return embedjson_int(parser, value);
    case PARSER_STATE_MAYBE_ARRAY_COMMA:
      return embedjson_error_ex(parser, EMBEDJSON_EXP_COMMA_OR_CLOSE_CURLY, position);
    case PARSER_STATE_DONE:
      return embedjson_error_ex(parser, EMBEDJSON_EXCESSIVE_INPUT, position);
#if EMBEDJSON_DEBUG
    case PARSER_STATE_INVALID:
    default:
      return embedjson_error_ex(parser, EMBEDJSON_INTERNAL_ERROR, position);
#endif
  }
  return 0;
}

EMBEDJSON_STATIC int embedjson_tokenf(embedjson_lexer* lexer, double value,
    const char* position)
{
  embedjson_parser* parser = (embedjson_parser*) lexer;
  switch (parser->state) {
    case PARSER_STATE_EXPECT_VALUE:
      parser->state = PARSER_STATE_DONE;
      return embedjson_double(parser, value);
    case PARSER_STATE_MAYBE_OBJECT_KEY:
      return embedjson_error_ex(parser, EMBEDJSON_EXP_OBJECT_KEY_OR_CLOSE_CURLY, position);
    case PARSER_STATE_EXPECT_OBJECT_KEY:
      return embedjson_error_ex(parser, EMBEDJSON_EXP_OBJECT_KEY, position);
    case PARSER_STATE_EXPECT_COLON:
      return embedjson_error_ex(parser, EMBEDJSON_EXP_COLON, position);
    case PARSER_STATE_MAYBE_OBJECT_COMMA:
      return embedjson_error_ex(parser, EMBEDJSON_EXP_COMMA_OR_CLOSE_BRACKET, position);
    case PARSER_STATE_EXPECT_OBJECT_VALUE:
      parser->state = PARSER_STATE_MAYBE_OBJECT_COMMA;
      return embedjson_double(parser, value);
    case PARSER_STATE_MAYBE_ARRAY_VALUE:
    case PARSER_STATE_EXPECT_ARRAY_VALUE:
      parser->state = PARSER_STATE_MAYBE_ARRAY_COMMA;
      return embedjson_double(parser, value);
    case PARSER_STATE_MAYBE_ARRAY_COMMA:
      return embedjson_error_ex(parser, EMBEDJSON_EXP_COMMA_OR_CLOSE_CURLY, position);
    case PARSER_STATE_DONE:
      return embedjson_error_ex(parser, EMBEDJSON_EXCESSIVE_INPUT, position);
#if EMBEDJSON_DEBUG
    case PARSER_STATE_INVALID:
    default:
      return embedjson_error_ex(parser, EMBEDJSON_INTERNAL_ERROR, position);
#endif
  }
  return 0;
}

EMBEDJSON_STATIC int embedjson_tokenc_begin(embedjson_lexer* lexer,
    const char* position)
{
  embedjson_parser* parser = (embedjson_parser*) lexer;
  switch (parser->state) {
    case PARSER_STATE_EXPECT_VALUE:
    case PARSER_STATE_MAYBE_OBJECT_KEY:
    case PARSER_STATE_EXPECT_OBJECT_KEY:
      return embedjson_string_begin(parser);
    case PARSER_STATE_EXPECT_COLON:
      return embedjson_error_ex(parser, EMBEDJSON_EXP_COLON, position);
    case PARSER_STATE_MAYBE_OBJECT_COMMA:
      return embedjson_error_ex(parser, EMBEDJSON_EXP_COMMA_OR_CLOSE_CURLY, position);
    case PARSER_STATE_EXPECT_OBJECT_VALUE:
    case PARSER_STATE_MAYBE_ARRAY_VALUE:
    case PARSER_STATE_EXPECT_ARRAY_VALUE:
      return embedjson_string_begin(parser);
    case PARSER_STATE_MAYBE_ARRAY_COMMA:
      return embedjson_error_ex(parser, EMBEDJSON_EXP_COMMA_OR_CLOSE_BRACKET, position);
    case PARSER_STATE_DONE:
      return embedjson_error_ex(parser, EMBEDJSON_EXCESSIVE_INPUT, position);
#if EMBEDJSON_DEBUG
    case PARSER_STATE_INVALID:
    default:
      return embedjson_error_ex(parser, EMBEDJSON_INTERNAL_ERROR, position);
#endif
  }
  return 0;
}

EMBEDJSON_STATIC int embedjson_tokenc_end(embedjson_lexer* lexer,
    const char* position)
{
  static const unsigned char next_state[PARSER_STATE_INVALID] = {
    /* PARSER_STATE_EXPECT_VALUE        -> */ PARSER_STATE_DONE,
    /* PARSER_STATE_MAYBE_OBJECT_KEY    -> */ PARSER_STATE_EXPECT_COLON,
    /* PARSER_STATE_EXPECT_OBJECT_KEY   -> */ PARSER_STATE_EXPECT_COLON,
    /* PARSER_STATE_EXPECT_COLON        -> */ PARSER_STATE_INVALID,
    /* PARSER_STATE_MAYBE_OBJECT_COMMA  -> */ PARSER_STATE_INVALID,
    /* PARSER_STATE_EXPECT_OBJECT_VALUE -> */ PARSER_STATE_MAYBE_OBJECT_COMMA,
    /* PARSER_STATE_MAYBE_ARRAY_VALUE   -> */ PARSER_STATE_MAYBE_ARRAY_COMMA,
    /* PARSER_STATE_EXPECT_ARRAY_VALUE  -> */ PARSER_STATE_MAYBE_ARRAY_COMMA,
    /* PARSER_STATE_MAYBE_ARRAY_COMMA   -> */ PARSER_STATE_INVALID,
    /* PARSER_STATE_DONE                -> */ PARSER_STATE_INVALID,
  };
  embedjson_parser* parser = (embedjson_parser*) lexer;
  EMBEDJSON_UNUSED(position);
  EMBEDJSON_CHECK_STATE(parser);
  parser->state = next_state[parser->state];
  EMBEDJSON_CHECK_STATE(parser);
  return embedjson_string_end(parser);
}

