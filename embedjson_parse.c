#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <embedjson.c>

static const char* error_position = NULL;

static int embedjson_error(embedjson_parser* parser, const char* position)
{
  error_position = position;
  return 1;
}

static int embedjson_null(embedjson_parser* parser)
{
  printf("null\n");
  return 0;
}

static int embedjson_bool(embedjson_parser* parser, char value)
{
  printf("bool %s\n", value ? "true" : "false");
  return 0;
}

static int embedjson_int(embedjson_parser* parser, int64_t value)
{
  printf("int %llu\n", (long long) value);
  return 0;
}

static int embedjson_double(embedjson_parser* parser, double value)
{
  printf("double %lf\n", value);
  return 0;
}

static int embedjson_string_begin(embedjson_parser* parser)
{
  printf("begin string\n");
  return 0;
}

static int embedjson_string_chunk(embedjson_parser* parser,
    const char* data, size_t size)
{
  printf("string %.*s\n", (int) size, data);
  return 0;
}

static int embedjson_string_end(embedjson_parser* parser)
{
  printf("end string\n");
  return 0;
}

static int embedjson_object_begin(embedjson_parser* parser)
{
  printf("begin object\n");
  return 0;
}

static int embedjson_object_end(embedjson_parser* parser)
{
  printf("end object\n");
  return 0;
}

static int embedjson_array_begin(embedjson_parser* parser)
{
  printf("begin array\n");
  return 0;
}

static int embedjson_array_end(embedjson_parser* parser)
{
  printf("end array\n");
  return 0;
}

int main()
{
  embedjson_parser parser;
  char ch;
  memset(&parser, 0, sizeof(parser));
  while (read(STDIN_FILENO, &ch, 1) > 0) {
    int err = embedjson_push(&parser, &ch, 1);
    if (err < 0) {
      fprintf(stderr, "error parsing json near symbol '%c'\n", *error_position);
      return err;
    }
  }
  int err = embedjson_finalize(&parser);
  if (err < 0) {
    fprintf(stderr, "error parsing json near symbol '%c'\n", *error_position);
  }
  return 0;
}

