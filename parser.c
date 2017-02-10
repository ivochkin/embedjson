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
  PARSER_STATE_EXPECT_STRING,
  PARSER_STATE_EXPECT_COLON,
  PARSER_STATE_EXPECT_OBJECT_VALUE,
  PARSER_STATE_EXPECT_ARRAY_VALUE,
  PARSER_STATE_EXPECT_ARRAY_COMMA,
  PARSER_STATE_EXPECT_OBJECT_COMMA,
  PARSER_STATE_DONE
} parser_state;


typedef enum {
  STACK_VALUE_CURLY = 0,
  STACK_VALUE_SQUARE = 1
} parser_stack_value;


#ifdef EMBEDJSON_EXTERNAL_STACK
#define STACK_CAPACITY(p) (p)->stack_capacity
#else
#define STACK_CAPACITY(p) sizeof((p)->stack)
#endif


/* Returns result of expression (f) if it evaluates to non-zero */
#ifndef RETURN_IF
#define RETURN_IF(f) \
do { \
  int err = (f); \
  if (err) { \
    return err; \
  } \
} while (0)
#endif


static unsigned char zero[] = {
  0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F
};


static unsigned char one[] = {
  0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
};


static int stack_push(embedjson_parser* parser, unsigned char value)
{
  if (parser->stack_size == 8 * sizeof(char) * STACK_CAPACITY(parser)) {
    return embedjson_error(parser, NULL);
  }
  size_t nbucket = parser->stack_size / 8;
  size_t nbit = parser->stack_size % 8;
  if (value) {
    parser->stack[nbucket] |= one[nbit];
  } else {
    parser->stack[nbucket] &= zero[nbit];
  }
  parser->stack_size++;
  return 0;
}


static void stack_pop(embedjson_parser* parser)
{
  parser->stack_size--;
}


static unsigned char stack_empty(embedjson_parser* parser)
{
  return !parser->stack_size;
}


static unsigned char stack_top(embedjson_parser* parser)
{
  size_t nbucket = (parser->stack_size - 1) / 8;
  size_t nbit = (parser->stack_size - 1) % 8;
  return parser->stack[nbucket] & one[nbit];
}


EMBEDJSON_STATIC int embedjson_push(embedjson_parser* parser, const char* data, size_t size)
{
  return embedjson_lexer_push(&parser->lexer, data, size);
}


EMBEDJSON_STATIC int embedjson_finalize(embedjson_parser* parser)
{
  RETURN_IF(embedjson_lexer_finalize(&parser->lexer));
  return parser->state != PARSER_STATE_DONE;
}


EMBEDJSON_STATIC int embedjson_token(embedjson_lexer* lexer, embedjson_tok token)
{
  /*
   * See doc/syntax-parser-fsm.dot for the explanation what's
   * going on below.
   */
  embedjson_parser* parser = (embedjson_parser*)(lexer);
  switch (parser->state) {
    case PARSER_STATE_EXPECT_VALUE:
      switch (token) {
        case EMBEDJSON_TOKEN_OPEN_CURLY_BRACKET:
          RETURN_IF(stack_push(parser, STACK_VALUE_CURLY));
          RETURN_IF(embedjson_object_begin(parser));
          parser->state = PARSER_STATE_EXPECT_STRING;
          break;
        case EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET:
          return embedjson_error(parser, NULL);
        case EMBEDJSON_TOKEN_OPEN_BRACKET:
          RETURN_IF(stack_push(parser, STACK_VALUE_SQUARE));
          RETURN_IF(embedjson_array_begin(parser));
          parser->state = PARSER_STATE_EXPECT_ARRAY_VALUE;
          break;
        case EMBEDJSON_TOKEN_CLOSE_BRACKET:
        case EMBEDJSON_TOKEN_COMMA:
        case EMBEDJSON_TOKEN_COLON:
        case EMBEDJSON_TOKEN_STRING_CHUNK:
        case EMBEDJSON_TOKEN_NUMBER:
          return embedjson_error(parser, NULL);
        case EMBEDJSON_TOKEN_TRUE:
          RETURN_IF(embedjson_bool(parser, 1));
          parser->state = PARSER_STATE_DONE;
          break;
        case EMBEDJSON_TOKEN_FALSE:
          RETURN_IF(embedjson_bool(parser, 0));
          parser->state = PARSER_STATE_DONE;
          break;
        case EMBEDJSON_TOKEN_NULL:
          RETURN_IF(embedjson_null(parser));
          parser->state = PARSER_STATE_DONE;
          break;
        default:
          return embedjson_error(parser, NULL);
      }
      break;
    case PARSER_STATE_EXPECT_STRING:
      if (token == EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET) {
        if (stack_empty(parser)
            || stack_top(parser) != STACK_VALUE_CURLY) {
          return embedjson_error(parser, NULL);
        }
        RETURN_IF(embedjson_object_end(parser));
        stack_pop(parser);
        if (stack_empty(parser)) {
          parser->state = PARSER_STATE_DONE;
        } else if (stack_top(parser) == STACK_VALUE_CURLY) {
          parser->state = PARSER_STATE_EXPECT_OBJECT_COMMA;
        } else {
          parser->state = PARSER_STATE_EXPECT_ARRAY_COMMA;
        }
      } else {
        return embedjson_error(parser, NULL);
      }
      break;
    case PARSER_STATE_EXPECT_COLON:
      if (token != EMBEDJSON_TOKEN_COLON) {
        return embedjson_error(parser, NULL);
      }
      parser->state = PARSER_STATE_EXPECT_OBJECT_VALUE;
      break;
    case PARSER_STATE_EXPECT_OBJECT_VALUE:
      switch (token) {
        case EMBEDJSON_TOKEN_OPEN_CURLY_BRACKET:
          RETURN_IF(stack_push(parser, STACK_VALUE_CURLY));
          RETURN_IF(embedjson_object_begin(parser));
          parser->state = PARSER_STATE_EXPECT_STRING;
          break;
        case EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET:
          return embedjson_error(parser, NULL);
        case EMBEDJSON_TOKEN_OPEN_BRACKET:
          RETURN_IF(stack_push(parser, STACK_VALUE_SQUARE));
          RETURN_IF(embedjson_array_begin(parser));
          parser->state = PARSER_STATE_EXPECT_ARRAY_VALUE;
          break;
        case EMBEDJSON_TOKEN_CLOSE_BRACKET:
        case EMBEDJSON_TOKEN_COMMA:
        case EMBEDJSON_TOKEN_COLON:
        case EMBEDJSON_TOKEN_STRING_CHUNK:
        case EMBEDJSON_TOKEN_NUMBER:
          return embedjson_error(parser, NULL);
        case EMBEDJSON_TOKEN_TRUE:
          RETURN_IF(embedjson_bool(parser, 1));
          parser->state = PARSER_STATE_EXPECT_OBJECT_COMMA;
          break;
        case EMBEDJSON_TOKEN_FALSE:
          RETURN_IF(embedjson_bool(parser, 0));
          parser->state = PARSER_STATE_EXPECT_OBJECT_COMMA;
          break;
        case EMBEDJSON_TOKEN_NULL:
          RETURN_IF(embedjson_null(parser));
          parser->state = PARSER_STATE_EXPECT_OBJECT_COMMA;
          break;
        default:
          return embedjson_error(parser, NULL);
      }
      break;
    case PARSER_STATE_EXPECT_ARRAY_VALUE:
      switch (token) {
        case EMBEDJSON_TOKEN_OPEN_CURLY_BRACKET:
          RETURN_IF(stack_push(parser, STACK_VALUE_CURLY));
          RETURN_IF(embedjson_object_begin(parser));
          parser->state = PARSER_STATE_EXPECT_STRING;
          break;
        case EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET:
          return embedjson_error(parser, NULL);
        case EMBEDJSON_TOKEN_OPEN_BRACKET:
          RETURN_IF(stack_push(parser, STACK_VALUE_SQUARE));
          RETURN_IF(embedjson_array_begin(parser));
          break;
        case EMBEDJSON_TOKEN_CLOSE_BRACKET:
          if (stack_empty(parser)
              || stack_top(parser) == STACK_VALUE_CURLY) {
            return embedjson_error(parser, NULL);
          }
          RETURN_IF(embedjson_array_end(parser));
          stack_pop(parser);
          if (stack_empty(parser)) {
            parser->state = PARSER_STATE_DONE;
          } else if (stack_top(parser) == STACK_VALUE_CURLY) {
            parser->state = PARSER_STATE_EXPECT_OBJECT_COMMA;
          } else {
            parser->state = PARSER_STATE_EXPECT_ARRAY_COMMA;
          }
          break;
        case EMBEDJSON_TOKEN_COMMA:
        case EMBEDJSON_TOKEN_COLON:
        case EMBEDJSON_TOKEN_STRING_CHUNK:
        case EMBEDJSON_TOKEN_NUMBER:
          return embedjson_error(parser, NULL);
        case EMBEDJSON_TOKEN_TRUE:
          RETURN_IF(embedjson_bool(parser, 1));
          parser->state = PARSER_STATE_EXPECT_ARRAY_COMMA;
          break;
        case EMBEDJSON_TOKEN_FALSE:
          RETURN_IF(embedjson_bool(parser, 0));
          parser->state = PARSER_STATE_EXPECT_ARRAY_COMMA;
          break;
        case EMBEDJSON_TOKEN_NULL:
          RETURN_IF(embedjson_null(parser));
          parser->state = PARSER_STATE_EXPECT_ARRAY_COMMA;
          break;
        default:
          return embedjson_error(parser, NULL);
      }
      break;
    case PARSER_STATE_EXPECT_ARRAY_COMMA:
      if (token == EMBEDJSON_TOKEN_COMMA) {
        parser->state = PARSER_STATE_EXPECT_ARRAY_VALUE;
      } else if (token == EMBEDJSON_TOKEN_CLOSE_BRACKET) {
        if (stack_empty(parser)
            || stack_top(parser) == STACK_VALUE_CURLY) {
          return embedjson_error(parser, NULL);
        }
        RETURN_IF(embedjson_array_end(parser));
        stack_pop(parser);
        if (stack_empty(parser)) {
          parser->state = PARSER_STATE_DONE;
        } else if (stack_top(parser) == STACK_VALUE_CURLY) {
          parser->state = PARSER_STATE_EXPECT_OBJECT_COMMA;
        } else {
          parser->state = PARSER_STATE_EXPECT_ARRAY_COMMA;
        }
      } else {
        return embedjson_error(parser, NULL);
      }
      break;
    case PARSER_STATE_EXPECT_OBJECT_COMMA:
      if (token == EMBEDJSON_TOKEN_COMMA) {
        parser->state = PARSER_STATE_EXPECT_STRING;
      } else if (token == EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET) {
        if (stack_empty(parser)
            || stack_top(parser) != STACK_VALUE_CURLY) {
          return embedjson_error(parser, NULL);
        }
        RETURN_IF(embedjson_object_end(parser));
        stack_pop(parser);
        if (stack_empty(parser)) {
          parser->state = PARSER_STATE_DONE;
        } else if (stack_top(parser) == STACK_VALUE_CURLY) {
          parser->state = PARSER_STATE_EXPECT_OBJECT_COMMA;
        } else {
          parser->state = PARSER_STATE_EXPECT_ARRAY_COMMA;
        }
      } else {
        return embedjson_error(parser, NULL);
      }
      break;
    case PARSER_STATE_DONE:
      return embedjson_error(parser, NULL);
    default:
      return embedjson_error(parser, NULL);
  }
  return 0;
}


EMBEDJSON_STATIC int embedjson_tokenc(embedjson_lexer* lexer, const char* data,
    size_t size)
{
  embedjson_parser* parser = (embedjson_parser*)(lexer);
  switch (parser->state) {
    case PARSER_STATE_EXPECT_VALUE:
    case PARSER_STATE_EXPECT_STRING:
      RETURN_IF(embedjson_string_chunk(parser, data, size));
      break;
    case PARSER_STATE_EXPECT_COLON:
      return embedjson_error(parser, NULL);
    case PARSER_STATE_EXPECT_OBJECT_VALUE:
    case PARSER_STATE_EXPECT_ARRAY_VALUE:
      RETURN_IF(embedjson_string_chunk(parser, data, size));
      break;
    case PARSER_STATE_EXPECT_ARRAY_COMMA:
    case PARSER_STATE_EXPECT_OBJECT_COMMA:
    case PARSER_STATE_DONE:
      return embedjson_error(parser, NULL);
    default:
      return embedjson_error(parser, NULL);
  }
  return 0;
}


EMBEDJSON_STATIC int embedjson_tokeni(embedjson_lexer* lexer, int64_t value)
{
  embedjson_parser* parser = (embedjson_parser*)(lexer);
  switch (parser->state) {
    case PARSER_STATE_EXPECT_VALUE:
      RETURN_IF(embedjson_int(parser, value));
      parser->state = PARSER_STATE_DONE;
      break;
    case PARSER_STATE_EXPECT_STRING:
    case PARSER_STATE_EXPECT_COLON:
      return embedjson_error(parser, NULL);
    case PARSER_STATE_EXPECT_OBJECT_VALUE:
      RETURN_IF(embedjson_int(parser, value));
      parser->state = PARSER_STATE_EXPECT_OBJECT_COMMA;
      break;
    case PARSER_STATE_EXPECT_ARRAY_VALUE:
      RETURN_IF(embedjson_int(parser, value));
      parser->state = PARSER_STATE_EXPECT_ARRAY_COMMA;
      break;
    case PARSER_STATE_EXPECT_ARRAY_COMMA:
    case PARSER_STATE_EXPECT_OBJECT_COMMA:
    case PARSER_STATE_DONE:
      return embedjson_error(parser, NULL);
    default:
      return embedjson_error(parser, NULL);
  }
  return 0;
}


EMBEDJSON_STATIC int embedjson_tokenf(embedjson_lexer* lexer, double value)
{
  embedjson_parser* parser = (embedjson_parser*)(lexer);
  switch (parser->state) {
    case PARSER_STATE_EXPECT_VALUE:
      RETURN_IF(embedjson_double(parser, value));
      parser->state = PARSER_STATE_DONE;
      break;
    case PARSER_STATE_EXPECT_STRING:
    case PARSER_STATE_EXPECT_COLON:
      return embedjson_error(parser, NULL);
    case PARSER_STATE_EXPECT_OBJECT_VALUE:
      RETURN_IF(embedjson_double(parser, value));
      parser->state = PARSER_STATE_EXPECT_OBJECT_COMMA;
      break;
    case PARSER_STATE_EXPECT_ARRAY_VALUE:
      RETURN_IF(embedjson_double(parser, value));
      parser->state = PARSER_STATE_EXPECT_ARRAY_COMMA;
      break;
    case PARSER_STATE_EXPECT_ARRAY_COMMA:
    case PARSER_STATE_EXPECT_OBJECT_COMMA:
    case PARSER_STATE_DONE:
      return embedjson_error(parser, NULL);
    default:
      return embedjson_error(parser, NULL);
  }
  return 0;
}


EMBEDJSON_STATIC int embedjson_tokenc_begin(embedjson_lexer* lexer)
{
  embedjson_parser* parser = (embedjson_parser*)(lexer);
  embedjson_string_begin(parser);
  return 0;
}


EMBEDJSON_STATIC int embedjson_tokenc_end(embedjson_lexer* lexer)
{
  embedjson_parser* parser = (embedjson_parser*)(lexer);
  switch (parser->state) {
    case PARSER_STATE_EXPECT_VALUE:
      parser->state = PARSER_STATE_DONE;
      break;
    case PARSER_STATE_EXPECT_STRING:
      parser->state = PARSER_STATE_EXPECT_COLON;
      break;
    case PARSER_STATE_EXPECT_COLON:
      return embedjson_error(parser, NULL);
    case PARSER_STATE_EXPECT_OBJECT_VALUE:
      parser->state = PARSER_STATE_EXPECT_OBJECT_COMMA;
      break;
    case PARSER_STATE_EXPECT_ARRAY_VALUE:
      parser->state = PARSER_STATE_EXPECT_ARRAY_COMMA;
      break;
    case PARSER_STATE_EXPECT_ARRAY_COMMA:
    case PARSER_STATE_EXPECT_OBJECT_COMMA:
    case PARSER_STATE_DONE:
      return embedjson_error(parser, NULL);
    default:
      return embedjson_error(parser, NULL);
  }
  embedjson_string_end(parser);
  return 0;
}

