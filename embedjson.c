#include <stdlib.h>

typedef struct embedjson_parser {
  char* stack;
  size_t stack_size;
} embedjson_parser;

void embedjson_push(embedjson_parser* parser, const char* data, size_t size)
{
}

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


#define EMBEDJSON_STATE_LOOKUP_TOKEN 0
#define EMBEDJSON_STATE_IN_STRING 1
#define EMBEDJSON_STATE_IN_STRING_ESCAPE 2
#define EMBEDJSON_STATE_IN_STRING_UNICODE_ESCAPE 3
#define EMBEDJSON_STATE_IN_NUMBER 4
#define EMBEDJSON_STATE_IN_TRUE 5
#define EMBEDJSON_STATE_IN_FALSE 6
#define EMBEDJSON_STATE_IN_NULL 7


typedef struct embedjson_lexer {
  unsigned char state;
  unsigned char offset;
  unsigned char unicode_cp[2];
  char minus : 1;
  char is_double : 1;
  int64_t int_value;
  double double_value;
} embedjson_lexer;

void embedjson_new_token(embedjson_token token, size_t size)
{
  const char* token_name;
  switch(token) {
    case EMBEDJSON_TOKEN_OPEN_CURLY_BRACKET: token_name = "Open curly bracket"; break;
    case EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET: token_name = "Close curly bracket"; break;
    case EMBEDJSON_TOKEN_OPEN_BRACKET: token_name = "Open bracket"; break;
    case EMBEDJSON_TOKEN_CLOSE_BRACKET: token_name = "Close bracket"; break;
    case EMBEDJSON_TOKEN_COMMA: token_name = "Comma"; break;
    case EMBEDJSON_TOKEN_COLON: token_name = "Colon"; break;
    case EMBEDJSON_TOKEN_STRING_CHUNK: token_name = "String chunk"; break;
    case EMBEDJSON_TOKEN_NUMBER: token_name = "Number chunk"; break;
    case EMBEDJSON_TOKEN_TRUE: token_name = "True"; break;
    case EMBEDJSON_TOKEN_FALSE: token_name = "False"; break;
    case EMBEDJSON_TOKEN_NULL: token_name = "Null"; break;
    default: token_name = "Unknown"; break;
  }
  printf("token %s (%d) of size %llu\n", token_name, token, (unsigned long long) size);
}

void embedjson_lexer_push(embedjson_lexer* lexer, const char* data, size_t size)
{
  const char* string_chunk_begin = NULL;
  embedjson_lexer lex = *lexer;
  embedjson_lexer orig_lex = *lexer;
  for (const char* end = data + size; data != end; ++data) {
    switch(lex.state) {
      case EMBEDJSON_STATE_LOOKUP_TOKEN:
        if (*data == ' ' || *data == '\n' || *data == '\r' || *data == '\t') {
          continue;
        } else if (*data == ':') {
          embedjson_new_token(EMBEDJSON_TOKEN_COLON, 0);
        } else if (*data == ',') {
          embedjson_new_token(EMBEDJSON_TOKEN_COMMA, 0);
        } else if (*data == '{') {
          embedjson_new_token(EMBEDJSON_TOKEN_OPEN_CURLY_BRACKET, 0);
        } else if (*data == '}') {
          embedjson_new_token(EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET, 0);
        } else if (*data == '[') {
          embedjson_new_token(EMBEDJSON_TOKEN_OPEN_BRACKET, 0);
        } else if (*data == ']') {
          embedjson_new_token(EMBEDJSON_TOKEN_CLOSE_BRACKET, 0);
        } else if (*data == '"') {
          string_chunk_begin = data;
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
          embedjson_new_token(EMBEDJSON_TOKEN_STRING_CHUNK, data - string_chunk_begin - 1);
          lex.state = EMBEDJSON_STATE_IN_STRING_ESCAPE;
        } else if (*data == '"') {
          embedjson_new_token(EMBEDJSON_TOKEN_STRING_CHUNK, data - string_chunk_begin - 1);
          lex.state = EMBEDJSON_STATE_LOOKUP_TOKEN;
        }
        break;
      case EMBEDJSON_STATE_IN_STRING_ESCAPE:
        if (*data == '"') {
          embedjson_new_token(EMBEDJSON_TOKEN_STRING_CHUNK, 1);
        } else if (*data == '\\') {
          embedjson_new_token(EMBEDJSON_TOKEN_STRING_CHUNK, 1);
        } else if (*data == '/') {
          embedjson_new_token(EMBEDJSON_TOKEN_STRING_CHUNK, 1);
        } else if (*data == 'b') {
          embedjson_new_token(EMBEDJSON_TOKEN_STRING_CHUNK, 1);
        } else if (*data == 'f') {
          embedjson_new_token(EMBEDJSON_TOKEN_STRING_CHUNK, 1);
        } else if (*data == 'n') {
          embedjson_new_token(EMBEDJSON_TOKEN_STRING_CHUNK, 1);
        } else if (*data == 'r') {
          embedjson_new_token(EMBEDJSON_TOKEN_STRING_CHUNK, 1);
        } else if (*data == 't') {
          embedjson_new_token(EMBEDJSON_TOKEN_STRING_CHUNK, 1);
        } else if (*data == 'u') {
          lex.state = EMBEDJSON_STATE_IN_STRING_UNICODE_ESCAPE;
          lex.offset = 0;
          break;
        }
        string_chunk_begin = data;
        lex.state = EMBEDJSON_STATE_IN_STRING;
        break;
      case EMBEDJSON_STATE_IN_STRING_UNICODE_ESCAPE: {
        unsigned char value;
        if ('0' <= *data && *data <= '9') {
          value = *data - '0';
        } else if ('a' <= *data && *data <= 'f') {
          value = 10 + *data - 'a';
        } else if ('A' <= *data && *data <= 'F') {
          value = 10 + *data - 'A';
        } else {
          abort();
        }
        switch(lex.offset) {
          case 0: lex.unicode_cp[0] = value << 4; break;
          case 1: lex.unicode_cp[0] |= value; break;
          case 2: lex.unicode_cp[1] = value << 4; break;
          case 3:
            lex.unicode_cp[1] |= value;
            embedjson_new_token(EMBEDJSON_TOKEN_STRING_CHUNK, 2);
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
          embedjson_new_token(EMBEDJSON_TOKEN_NUMBER, (size_t) lex.int_value);
          lex.state = EMBEDJSON_STATE_LOOKUP_TOKEN;
        }
        break;
      case EMBEDJSON_STATE_IN_TRUE:
        if (*data != "true"[lex.offset]) {
          abort();
        }
        if (++lex.offset > 3) {
          embedjson_new_token(EMBEDJSON_TOKEN_TRUE, 0);
          lex.state = EMBEDJSON_STATE_LOOKUP_TOKEN;
        }
        break;
      case EMBEDJSON_STATE_IN_FALSE:
        if (*data != "false"[lex.offset]) {
          abort();
        }
        if (++lex.offset > 4) {
          embedjson_new_token(EMBEDJSON_TOKEN_FALSE, 0);
          lex.state = EMBEDJSON_STATE_LOOKUP_TOKEN;
        }
        break;
      case EMBEDJSON_STATE_IN_NULL:
        if (*data != "null"[lex.offset]) {
          abort();
        }
        if (++lex.offset > 3) {
          embedjson_new_token(EMBEDJSON_TOKEN_NULL, 0);
          lex.state = EMBEDJSON_STATE_LOOKUP_TOKEN;
        }
        break;
    }
  }
  if (lex.state == EMBEDJSON_STATE_IN_STRING) {
    embedjson_new_token(EMBEDJSON_TOKEN_STRING_CHUNK, data - string_chunk_begin - 1);
  }

  if (lex.state != orig_lex.state
      || lex.offset != orig_lex.offset
      || lex.unicode_cp != orig_lex.unicode_cp) {
    *lexer = lex;
  }
}


int main()
{
  size_t stack_size = 1024;
  char* stack = malloc(stack_size);

  const char json[] = "\
{\n\
    \"id\": 1,\n\
    \"name\": \"A gr\\u0432 een [] door\\n with \\t\",\n\
    \"price\": 12.50,\n\
    \"tags\": [\"home\", \"green\"],\n\
    \"foo\": null,\n\
    \"bar\": true,\n\
    \"qux\":false,\n\
    \"minux\": -18,\n\
    \"minus_double\": -123.00e+02\n\
}\n";

  printf("%s\n", json);
  embedjson_lexer lexer;
  lexer.state = EMBEDJSON_STATE_LOOKUP_TOKEN;
  embedjson_lexer_push(&lexer, json, sizeof(json));

//  embedjson_parser parser;
//  parser.stack = stack;
//  parser.stack_size = stack_size;
//  embedjson_push(&parser, json, sizeof(json));
}

