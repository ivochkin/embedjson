#include <stdio.h>
#include "parser.h"


#define PARSER_STATE_EXPECT_VALUE 0
#define PARSER_STATE_EXPECT_STRING 1
#define PARSER_STATE_EXPECT_COLON 2
#define PARSER_STATE_EXPECT_OBJECT_VALUE 3
#define PARSER_STATE_EXPECT_ARRAY_VALUE 4
#define PARSER_STATE_EXPECT_ARRAY_COMMA 5
#define PARSER_STATE_EXPECT_OBJECT_COMMA 6
#define PARSER_STATE_DONE 7


void embedjson_push(embedjson_parser* parser, const char* data, size_t size)
{
  embedjson_lexer_push(&parser->lexer, data, size);
}


void embedjson_token(embedjson_lexer* lexer, embedjson_tok token)
{
}


void embedjson_tokenc(embedjson_lexer* lexer, const char* data, size_t size)
{
}


void embedjson_tokeni(embedjson_lexer* lexer, int64_t value)
{
}


void embedjson_tokenf(embedjson_lexer* lexer, double value)
{
}


