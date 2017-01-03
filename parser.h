#pragma once
#include <stddef.h> /* for size_t */
#include "lexer.h"

typedef struct embedjson_parser {
  /**
   * @note Should be the first embedjson_parser member to enable pointer casting
   */
  embedjson_lexer lexer;
  unsigned char state;
  char* stack;
  size_t stack_size;
} embedjson_parser;

void embedjson_push(embedjson_parser* parser, const char* data, size_t size);

void embedjson_finalize(embedjson_parser* parser);

void embedjson_null(embedjson_parser* parser);
void embedjson_bool(embedjson_parser* parser, char value);
void embedjson_int(embedjson_parser* parser, int64_t value);
void embedjson_double(embedjson_parser* parser, double value);
void embedjson_string_chunk(embedjson_parser* parser, const char* data, size_t size);
void embedjson_begin_object(embedjson_parser* parser);
void embedjson_end_object(embedjson_parser* parser);
void embedjson_begin_array(embedjson_parser* parser);
void embedjson_end_array(embedjson_parser* parser);
