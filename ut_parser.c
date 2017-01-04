#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"


int embedjson_error(const char* position)
{
  return 1;
}


int embedjson_null(embedjson_parser* parser)
{
  printf("null\n");
  return 0;
}


int embedjson_bool(embedjson_parser* parser, char value)
{
  printf("bool\n");
  return 0;
}


int embedjson_int(embedjson_parser* parser, int64_t value)
{
  printf("int\n");
  return 0;
}


int embedjson_double(embedjson_parser* parser, double value)
{
  printf("double\n");
  return 0;
}


int embedjson_string_chunk(embedjson_parser* parser, const char* data, size_t size)
{
  printf("string_chunk\n");
  return 0;
}


int embedjson_begin_object(embedjson_parser* parser)
{
  printf("begin_object\n");
  return 0;
}


int embedjson_end_object(embedjson_parser* parser)
{
  printf("end_object\n");
  return 0;
}


int embedjson_begin_array(embedjson_parser* parser)
{
  printf("begin_array\n");
  return 0;
}


int embedjson_end_array(embedjson_parser* parser)
{
  printf("end_array\n");
  return 0;
}


static char json[] =
"{\n"
"  \"description\": \"the description of the test case\",\n"
"  \"schema\": {\"the schema that should\" : \"be validated against\"},\n"
"  \"tests\": [\n"
"    {\n"
"      \"description\": \"a specific test of a valid instance\",\n"
"      \"data\": \"the instance\",\n"
"      \"valid\": true\n"
"    },\n"
"    {\n"
"      \"description\": \"another specific test this time, invalid\",\n"
"      \"data\": 15,\n"
"      \"valid\": false\n"
"    }\n"
"  ]\n"
"}\n";


int main()
{
  embedjson_parser parser;
  memset(&parser, 0, sizeof(parser));
  embedjson_push(&parser, json, sizeof(json));
  embedjson_finalize(&parser);
  return 0;
}
