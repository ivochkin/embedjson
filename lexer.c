#include "lexer.h"
#include "common.h"

#define EMBEDJSON_STATE_LOOKUP_TOKEN 0
#define EMBEDJSON_STATE_IN_STRING 1
#define EMBEDJSON_STATE_IN_STRING_ESCAPE 2
#define EMBEDJSON_STATE_IN_STRING_UNICODE_ESCAPE 3
#define EMBEDJSON_STATE_IN_NUMBER 4
#define EMBEDJSON_STATE_IN_TRUE 5
#define EMBEDJSON_STATE_IN_FALSE 6
#define EMBEDJSON_STATE_IN_NULL 7

/*
 * memcmp implementation taken from musl:
 * http://git.musl-libc.org/cgit/musl/tree/src/string/memcmp.c
 */
static int memcmp(const void *vl, const void *vr, size_t n)
{
  const unsigned char *l=vl, *r=vr;
  for (; n && *l == *r; n--, l++, r++);
  return n ? *l-*r : 0;
}


void embedjson_lexer_push(embedjson_lexer* lexer, const char* data, size_t size)
{
  embedjson_lexer lex = *lexer;
  embedjson_lexer orig_lex = *lexer;
  const char* string_chunk_begin = lex.state == EMBEDJSON_STATE_IN_STRING ? data : NULL;
  for (const char* end = data + size; data != end; ++data) {
    switch(lex.state) {
      case EMBEDJSON_STATE_LOOKUP_TOKEN:
        if (*data == ' ' || *data == '\n' || *data == '\r' || *data == '\t') {
          continue;
        } else if (*data == ':') {
          embedjson_on_new_token(lexer, EMBEDJSON_TOKEN_COLON);
        } else if (*data == ',') {
          embedjson_on_new_token(lexer, EMBEDJSON_TOKEN_COMMA);
        } else if (*data == '{') {
          embedjson_on_new_token(lexer, EMBEDJSON_TOKEN_OPEN_CURLY_BRACKET);
        } else if (*data == '}') {
          embedjson_on_new_token(lexer, EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET);
        } else if (*data == '[') {
          embedjson_on_new_token(lexer, EMBEDJSON_TOKEN_OPEN_BRACKET);
        } else if (*data == ']') {
          embedjson_on_new_token(lexer, EMBEDJSON_TOKEN_CLOSE_BRACKET);
        } else if (*data == '"') {
          string_chunk_begin = data + 1;
          lex.state = EMBEDJSON_STATE_IN_STRING;
          break;
        } else if (*data == 't') {
          lex.offset = 1;
          lex.state = EMBEDJSON_STATE_IN_TRUE;
        } else if (*data == 'f') {
          lex.offset = 1;
          lex.state = EMBEDJSON_STATE_IN_FALSE;
        } else if (*data == 'n') {
          lex.offset = 1;
          lex.state = EMBEDJSON_STATE_IN_NULL;
        } else if (*data == '-') {
          lex.minus = 1;
          lex.state = EMBEDJSON_STATE_IN_NUMBER;
        } else if ('0' <= *data && *data <= '9') {
          lex.int_value = *data - '0';
          lex.state = EMBEDJSON_STATE_IN_NUMBER;
        }
        break;
      case EMBEDJSON_STATE_IN_STRING:
        if (*data == '\\') {
          if (data != string_chunk_begin) {
            embedjson_on_new_tokenc(lexer, string_chunk_begin, data - string_chunk_begin);
          }
          lex.state = EMBEDJSON_STATE_IN_STRING_ESCAPE;
        } else if (*data == '"') {
          if (data != string_chunk_begin) {
            embedjson_on_new_tokenc(lexer, string_chunk_begin, data - string_chunk_begin);
          }
          lex.state = EMBEDJSON_STATE_LOOKUP_TOKEN;
        }
        break;
      case EMBEDJSON_STATE_IN_STRING_ESCAPE:
        if (*data == '"') {
          embedjson_on_new_tokenc(lexer, "\"", 1);
        } else if (*data == '\\') {
          embedjson_on_new_tokenc(lexer, "\\", 1);
        } else if (*data == '/') {
          embedjson_on_new_tokenc(lexer, "/", 1);
        } else if (*data == 'b') {
          embedjson_on_new_tokenc(lexer, "\b", 1);
        } else if (*data == 'f') {
          embedjson_on_new_tokenc(lexer, "\f", 1);
        } else if (*data == 'n') {
          embedjson_on_new_tokenc(lexer, "\n", 1);
        } else if (*data == 'r') {
          embedjson_on_new_tokenc(lexer, "\r", 1);
        } else if (*data == 't') {
          embedjson_on_new_tokenc(lexer, "\t", 1);
        } else if (*data == 'u') {
          lex.state = EMBEDJSON_STATE_IN_STRING_UNICODE_ESCAPE;
          lex.offset = 0;
          break;
        }
        string_chunk_begin = data + 1;
        lex.state = EMBEDJSON_STATE_IN_STRING;
        break;
      case EMBEDJSON_STATE_IN_STRING_UNICODE_ESCAPE: {
        char value;
        if ('0' <= *data && *data <= '9') {
          value = *data - '0';
        } else if ('a' <= *data && *data <= 'f') {
          value = 10 + *data - 'a';
        } else if ('A' <= *data && *data <= 'F') {
          value = 10 + *data - 'A';
        } else {
          embedjson_error(data);
          return;
        }
        switch(lex.offset) {
          case 0: lex.unicode_cp[0] = value << 4; break;
          case 1: lex.unicode_cp[0] |= value; break;
          case 2: lex.unicode_cp[1] = value << 4; break;
          case 3:
            lex.unicode_cp[1] |= value;
            embedjson_on_new_tokenc(lexer, lex.unicode_cp, 2);
            string_chunk_begin = data + 1;
            lex.state = EMBEDJSON_STATE_IN_STRING;
            break;
        }
        lex.offset++;
        break;
      }
      case EMBEDJSON_STATE_IN_NUMBER:
        if ('0' <= *data && *data <= '9') {
          lex.int_value = 10 * lex.int_value + *data - '0';
        } else if (*data == '.') {
          lex.is_double = 1;
        } else {
          data--;
          if (lex.minus) {
            lex.int_value = 0 - lex.int_value;
          }
          embedjson_on_new_tokeni(lexer, lex.int_value);
          lex.state = EMBEDJSON_STATE_LOOKUP_TOKEN;
        }
        break;
      case EMBEDJSON_STATE_IN_TRUE:
        if (*data != "true"[lex.offset]) {
          embedjson_error(data);
          return;
        }
        if (++lex.offset > 3) {
          embedjson_on_new_token(lexer, EMBEDJSON_TOKEN_TRUE);
          lex.state = EMBEDJSON_STATE_LOOKUP_TOKEN;
        }
        break;
      case EMBEDJSON_STATE_IN_FALSE:
        if (*data != "false"[lex.offset]) {
          embedjson_error(data);
          return;
        }
        if (++lex.offset > 4) {
          embedjson_on_new_token(lexer, EMBEDJSON_TOKEN_FALSE);
          lex.state = EMBEDJSON_STATE_LOOKUP_TOKEN;
        }
        break;
      case EMBEDJSON_STATE_IN_NULL:
        if (*data != "null"[lex.offset]) {
          embedjson_error(data);
          return;
        }
        if (++lex.offset > 3) {
          embedjson_on_new_token(lexer, EMBEDJSON_TOKEN_NULL);
          lex.state = EMBEDJSON_STATE_LOOKUP_TOKEN;
        }
        break;
    }
  }
  if (lex.state == EMBEDJSON_STATE_IN_STRING && data != string_chunk_begin) {
    embedjson_on_new_tokenc(lexer, string_chunk_begin, data - string_chunk_begin);
  }

  /*
   * Cache-friendly update lexer state only if it has changed
   */
  if (memcmp(&lex, lexer, sizeof(lex))) {
    *lexer = lex;
  }
}

void embedjson_lexer_finalize(embedjson_lexer* lexer)
{
  embedjson_lexer lex = *lexer;
  switch(lex.state) {
    case EMBEDJSON_STATE_LOOKUP_TOKEN:
      break;
    case EMBEDJSON_STATE_IN_STRING:
    case EMBEDJSON_STATE_IN_STRING_ESCAPE:
    case EMBEDJSON_STATE_IN_STRING_UNICODE_ESCAPE:
      embedjson_error(NULL);
      break;
    case EMBEDJSON_STATE_IN_NUMBER:
      if (lex.minus) {
        lex.int_value = 0 - lex.int_value;
      }
      embedjson_on_new_tokeni(lexer, lex.int_value);
      lexer->state = EMBEDJSON_STATE_LOOKUP_TOKEN;
      break;
    case EMBEDJSON_STATE_IN_TRUE:
    case EMBEDJSON_STATE_IN_FALSE:
    case EMBEDJSON_STATE_IN_NULL:
      embedjson_error(NULL);
      break;
  }
}
