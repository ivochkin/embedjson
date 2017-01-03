#include <stdio.h>
#include "parser.h"

void embedjson_push(embedjson_parser* parser, const char* data, size_t size)
{
}

void embedjson_on_new_token(embedjson_lexer* lexer, embedjson_token token, size_t size)
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
