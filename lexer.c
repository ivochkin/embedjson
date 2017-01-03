#include "lexer.h"
#include "common.h"


#define LEXER_STATE_LOOKUP_TOKEN 0
#define LEXER_STATE_IN_STRING 1
#define LEXER_STATE_IN_STRING_ESCAPE 2
#define LEXER_STATE_IN_STRING_UNICODE_ESCAPE 3
#define LEXER_STATE_IN_NUMBER 4
#define LEXER_STATE_IN_NUMBER_FRAC 5
#define LEXER_STATE_IN_NUMBER_EXP_SIGN 6
#define LEXER_STATE_IN_NUMBER_EXP 7
#define LEXER_STATE_IN_TRUE 8
#define LEXER_STATE_IN_FALSE 9
#define LEXER_STATE_IN_NULL 10


/*
 * memcmp implementation taken from musl:
 * http://git.musl-libc.org/cgit/musl/tree/src/string/memcmp.c
 */
static int memcmp(const void *vl, const void *vr, size_t n)
{
  const unsigned char* l = vl, *r = vr;
  for (; n && *l == *r; n--, l++, r++);
  return n ? *l - *r : 0;
}


/*
 * Returns {-10}^{n}
 */
static double powm10(int n)
{
  static double e[] = {
    1e+308, 1e+307, 1e+306, 1e+305, 1e+304, 1e+303, 1e+302, 1e+301, 1e+300, 1e+299, 1e+298, 1e+297, 1e+296, 1e+295, 1e+294,
    1e+293, 1e+292, 1e+291, 1e+290, 1e+289, 1e+288, 1e+287, 1e+286, 1e+285, 1e+284, 1e+283, 1e+282, 1e+281, 1e+280, 1e+279,
    1e+278, 1e+277, 1e+276, 1e+275, 1e+274, 1e+273, 1e+272, 1e+271, 1e+270, 1e+269, 1e+268, 1e+267, 1e+266, 1e+265, 1e+264,
    1e+263, 1e+262, 1e+261, 1e+260, 1e+259, 1e+258, 1e+257, 1e+256, 1e+255, 1e+254, 1e+253, 1e+252, 1e+251, 1e+250, 1e+249,
    1e+248, 1e+247, 1e+246, 1e+245, 1e+244, 1e+243, 1e+242, 1e+241, 1e+240, 1e+239, 1e+238, 1e+237, 1e+236, 1e+235, 1e+234,
    1e+233, 1e+232, 1e+231, 1e+230, 1e+229, 1e+228, 1e+227, 1e+226, 1e+225, 1e+224, 1e+223, 1e+222, 1e+221, 1e+220, 1e+219,
    1e+218, 1e+217, 1e+216, 1e+215, 1e+214, 1e+213, 1e+212, 1e+211, 1e+210, 1e+209, 1e+208, 1e+207, 1e+206, 1e+205, 1e+204,
    1e+203, 1e+202, 1e+201, 1e+200, 1e+199, 1e+198, 1e+197, 1e+196, 1e+195, 1e+194, 1e+193, 1e+192, 1e+191, 1e+190, 1e+189,
    1e+188, 1e+187, 1e+186, 1e+185, 1e+184, 1e+183, 1e+182, 1e+181, 1e+180, 1e+179, 1e+178, 1e+177, 1e+176, 1e+175, 1e+174,
    1e+173, 1e+172, 1e+171, 1e+170, 1e+169, 1e+168, 1e+167, 1e+166, 1e+165, 1e+164, 1e+163, 1e+162, 1e+161, 1e+160, 1e+159,
    1e+158, 1e+157, 1e+156, 1e+155, 1e+154, 1e+153, 1e+152, 1e+151, 1e+150, 1e+149, 1e+148, 1e+147, 1e+146, 1e+145, 1e+144,
    1e+143, 1e+142, 1e+141, 1e+140, 1e+139, 1e+138, 1e+137, 1e+136, 1e+135, 1e+134, 1e+133, 1e+132, 1e+131, 1e+130, 1e+129,
    1e+128, 1e+127, 1e+126, 1e+125, 1e+124, 1e+123, 1e+122, 1e+121, 1e+120, 1e+119, 1e+118, 1e+117, 1e+116, 1e+115, 1e+114,
    1e+113, 1e+112, 1e+111, 1e+110, 1e+109, 1e+108, 1e+107, 1e+106, 1e+105, 1e+104, 1e+103, 1e+102, 1e+101, 1e+100, 1e+99,
    1e+98, 1e+97, 1e+96, 1e+95, 1e+94, 1e+93, 1e+92, 1e+91, 1e+90, 1e+89, 1e+88, 1e+87, 1e+86, 1e+85, 1e+84,
    1e+83, 1e+82, 1e+81, 1e+80, 1e+79, 1e+78, 1e+77, 1e+76, 1e+75, 1e+74, 1e+73, 1e+72, 1e+71, 1e+70, 1e+69,
    1e+68, 1e+67, 1e+66, 1e+65, 1e+64, 1e+63, 1e+62, 1e+61, 1e+60, 1e+59, 1e+58, 1e+57, 1e+56, 1e+55, 1e+54,
    1e+53, 1e+52, 1e+51, 1e+50, 1e+49, 1e+48, 1e+47, 1e+46, 1e+45, 1e+44, 1e+43, 1e+42, 1e+41, 1e+40, 1e+39,
    1e+38, 1e+37, 1e+36, 1e+35, 1e+34, 1e+33, 1e+32, 1e+31, 1e+30, 1e+29, 1e+28, 1e+27, 1e+26, 1e+25, 1e+24,
    1e+23, 1e+22, 1e+21, 1e+20, 1e+19, 1e+18, 1e+17, 1e+16, 1e+15, 1e+14, 1e+13, 1e+12, 1e+11, 1e+10, 1e+9,
    1e+8, 1e+7, 1e+6, 1e+5, 1e+4, 1e+3, 1e+2, 1e+1, 1.0, 1e-1, 1e-2, 1e-3, 1e-4, 1e-5, 1e-6, 1e-7,
    1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 1e-16, 1e-17, 1e-18, 1e-19, 1e-20, 1e-21, 1e-22,
    1e-23, 1e-24, 1e-25, 1e-26, 1e-27, 1e-28, 1e-29, 1e-30, 1e-31, 1e-32, 1e-33, 1e-34, 1e-35, 1e-36, 1e-37,
    1e-38, 1e-39, 1e-40, 1e-41, 1e-42, 1e-43, 1e-44, 1e-45, 1e-46, 1e-47, 1e-48, 1e-49, 1e-50, 1e-51, 1e-52,
    1e-53, 1e-54, 1e-55, 1e-56, 1e-57, 1e-58, 1e-59, 1e-60, 1e-61, 1e-62, 1e-63, 1e-64, 1e-65, 1e-66, 1e-67,
    1e-68, 1e-69, 1e-70, 1e-71, 1e-72, 1e-73, 1e-74, 1e-75, 1e-76, 1e-77, 1e-78, 1e-79, 1e-80, 1e-81, 1e-82,
    1e-83, 1e-84, 1e-85, 1e-86, 1e-87, 1e-88, 1e-89, 1e-90, 1e-91, 1e-92, 1e-93, 1e-94, 1e-95, 1e-96, 1e-97,
    1e-98, 1e-99, 1e-100, 1e-101, 1e-102, 1e-103, 1e-104, 1e-105, 1e-106, 1e-107, 1e-108, 1e-109, 1e-110, 1e-111, 1e-112,
    1e-113, 1e-114, 1e-115, 1e-116, 1e-117, 1e-118, 1e-119, 1e-120, 1e-121, 1e-122, 1e-123, 1e-124, 1e-125, 1e-126, 1e-127,
    1e-128, 1e-129, 1e-130, 1e-131, 1e-132, 1e-133, 1e-134, 1e-135, 1e-136, 1e-137, 1e-138, 1e-139, 1e-140, 1e-141, 1e-142,
    1e-143, 1e-144, 1e-145, 1e-146, 1e-147, 1e-148, 1e-149, 1e-150, 1e-151, 1e-152, 1e-153, 1e-154, 1e-155, 1e-156, 1e-157,
    1e-158, 1e-159, 1e-160, 1e-161, 1e-162, 1e-163, 1e-164, 1e-165, 1e-166, 1e-167, 1e-168, 1e-169, 1e-170, 1e-171, 1e-172,
    1e-173, 1e-174, 1e-175, 1e-176, 1e-177, 1e-178, 1e-179, 1e-180, 1e-181, 1e-182, 1e-183, 1e-184, 1e-185, 1e-186, 1e-187,
    1e-188, 1e-189, 1e-190, 1e-191, 1e-192, 1e-193, 1e-194, 1e-195, 1e-196, 1e-197, 1e-198, 1e-199, 1e-200, 1e-201, 1e-202,
    1e-203, 1e-204, 1e-205, 1e-206, 1e-207, 1e-208, 1e-209, 1e-210, 1e-211, 1e-212, 1e-213, 1e-214, 1e-215, 1e-216, 1e-217,
    1e-218, 1e-219, 1e-220, 1e-221, 1e-222, 1e-223, 1e-224, 1e-225, 1e-226, 1e-227, 1e-228, 1e-229, 1e-230, 1e-231, 1e-232,
    1e-233, 1e-234, 1e-235, 1e-236, 1e-237, 1e-238, 1e-239, 1e-240, 1e-241, 1e-242, 1e-243, 1e-244, 1e-245, 1e-246, 1e-247,
    1e-248, 1e-249, 1e-250, 1e-251, 1e-252, 1e-253, 1e-254, 1e-255, 1e-256, 1e-257, 1e-258, 1e-259, 1e-260, 1e-261, 1e-262,
    1e-263, 1e-264, 1e-265, 1e-266, 1e-267, 1e-268, 1e-269, 1e-270, 1e-271, 1e-272, 1e-273, 1e-274, 1e-275, 1e-276, 1e-277,
    1e-278, 1e-279, 1e-280, 1e-281, 1e-282, 1e-283, 1e-284, 1e-285, 1e-286, 1e-287, 1e-288, 1e-289, 1e-290, 1e-291, 1e-292,
    1e-293, 1e-294, 1e-295, 1e-296, 1e-297, 1e-298, 1e-299, 1e-300, 1e-301, 1e-302, 1e-303, 1e-304, 1e-305, 1e-306, 1e-307,
    1e-308, 1e-309, 1e-310, 1e-311, 1e-312, 1e-313, 1e-314, 1e-315, 1e-316, 1e-317, 1e-318, 1e-319, 1e-320, 1e-321, 1e-322,
    1e-323,
  };
  return e[n + 308];
}


void embedjson_lexer_push(embedjson_lexer* lexer, const char* data, size_t size)
{
  embedjson_lexer lex = *lexer;
  embedjson_lexer orig_lex = *lexer;
  const char* string_chunk_begin = lex.state == LEXER_STATE_IN_STRING ? data : NULL;
  for (const char* end = data + size; data != end; ++data) {
    switch(lex.state) {
      case LEXER_STATE_LOOKUP_TOKEN:
        if (*data == ' ' || *data == '\n' || *data == '\r' || *data == '\t') {
          continue;
        } else if (*data == ':') {
          embedjson_token(lexer, EMBEDJSON_TOKEN_COLON);
        } else if (*data == ',') {
          embedjson_token(lexer, EMBEDJSON_TOKEN_COMMA);
        } else if (*data == '{') {
          embedjson_token(lexer, EMBEDJSON_TOKEN_OPEN_CURLY_BRACKET);
        } else if (*data == '}') {
          embedjson_token(lexer, EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET);
        } else if (*data == '[') {
          embedjson_token(lexer, EMBEDJSON_TOKEN_OPEN_BRACKET);
        } else if (*data == ']') {
          embedjson_token(lexer, EMBEDJSON_TOKEN_CLOSE_BRACKET);
        } else if (*data == '"') {
          string_chunk_begin = data + 1;
          lex.state = LEXER_STATE_IN_STRING;
          break;
        } else if (*data == 't') {
          lex.offset = 1;
          lex.state = LEXER_STATE_IN_TRUE;
        } else if (*data == 'f') {
          lex.offset = 1;
          lex.state = LEXER_STATE_IN_FALSE;
        } else if (*data == 'n') {
          lex.offset = 1;
          lex.state = LEXER_STATE_IN_NULL;
        } else if (*data == '-') {
          lex.minus = 1;
          lex.state = LEXER_STATE_IN_NUMBER;
        } else if ('0' <= *data && *data <= '9') {
          lex.int_value = *data - '0';
          lex.state = LEXER_STATE_IN_NUMBER;
        }
        break;
      case LEXER_STATE_IN_STRING:
        if (*data == '\\') {
          if (data != string_chunk_begin) {
            embedjson_tokenc(lexer, string_chunk_begin, data - string_chunk_begin);
          }
          lex.state = LEXER_STATE_IN_STRING_ESCAPE;
        } else if (*data == '"') {
          if (data != string_chunk_begin) {
            embedjson_tokenc(lexer, string_chunk_begin, data - string_chunk_begin);
          }
          lex.state = LEXER_STATE_LOOKUP_TOKEN;
        }
        break;
      case LEXER_STATE_IN_STRING_ESCAPE:
        if (*data == '"') {
          embedjson_tokenc(lexer, "\"", 1);
        } else if (*data == '\\') {
          embedjson_tokenc(lexer, "\\", 1);
        } else if (*data == '/') {
          embedjson_tokenc(lexer, "/", 1);
        } else if (*data == 'b') {
          embedjson_tokenc(lexer, "\b", 1);
        } else if (*data == 'f') {
          embedjson_tokenc(lexer, "\f", 1);
        } else if (*data == 'n') {
          embedjson_tokenc(lexer, "\n", 1);
        } else if (*data == 'r') {
          embedjson_tokenc(lexer, "\r", 1);
        } else if (*data == 't') {
          embedjson_tokenc(lexer, "\t", 1);
        } else if (*data == 'u') {
          lex.state = LEXER_STATE_IN_STRING_UNICODE_ESCAPE;
          lex.offset = 0;
          break;
        }
        string_chunk_begin = data + 1;
        lex.state = LEXER_STATE_IN_STRING;
        break;
      case LEXER_STATE_IN_STRING_UNICODE_ESCAPE: {
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
            embedjson_tokenc(lexer, lex.unicode_cp, 2);
            string_chunk_begin = data + 1;
            lex.state = LEXER_STATE_IN_STRING;
            break;
        }
        lex.offset++;
        break;
      }
      case LEXER_STATE_IN_NUMBER:
        if ('0' <= *data && *data <= '9') {
          lex.int_value = 10 * lex.int_value + *data - '0';
        } else if (*data == '.') {
          lex.state = LEXER_STATE_IN_NUMBER_FRAC;
        } else {
          data--;
          if (lex.minus) {
            lex.int_value = 0 - lex.int_value;
          }
          embedjson_tokeni(lexer, lex.int_value);
          lex.state = LEXER_STATE_LOOKUP_TOKEN;
          lex.int_value = 0;
          lex.minus = 0;
          lex.state = LEXER_STATE_LOOKUP_TOKEN;
        }
        break;
      case LEXER_STATE_IN_NUMBER_FRAC:
        if ('0' <= *data && *data <= '9') {
          lex.frac_value = 10 * lex.frac_value + *data - '0';
          lex.frac_power++;
        } else if (*data == 'e' || *data == 'E') {
          lex.state = LEXER_STATE_IN_NUMBER_EXP_SIGN;
        } else {
          data--;
          double value = lex.frac_value * powm10(lex.frac_power) + lex.int_value;
          if (lex.minus) {
            value = 0 - value;
          }
          embedjson_tokenf(lexer, value);
          lex.int_value = 0;
          lex.frac_power = 0;
          lex.frac_value = 0;
          lex.minus = 0;
          lex.state = LEXER_STATE_LOOKUP_TOKEN;
        }
        break;
      case LEXER_STATE_IN_NUMBER_EXP_SIGN:
        if (*data == '-') {
          lex.exp_minus = 1;
        } else if ('0' <= *data && *data <= '9') {
          lex.exp_value = *data - '0';
        } else if (*data != '+') {
          embedjson_error(data);
        }
        lex.state = LEXER_STATE_IN_NUMBER_EXP;
        break;
      case LEXER_STATE_IN_NUMBER_EXP:
        if ('0' <= *data && *data <= '9') {
          lex.exp_value = 10 * lex.exp_value + *data - '0';
        } else {
          data--;
          double value = lex.frac_value * powm10(lex.frac_power) + lex.int_value;
          value *= powm10(lex.exp_minus ? lex.exp_value : 0 - lex.exp_value);
          if (lex.minus) {
            value = 0 - value;
          }
          embedjson_tokenf(lexer, value);
          lex.int_value = 0;
          lex.frac_power = 0;
          lex.frac_value = 0;
          lex.minus = 0;
          lex.exp_value = 0;
          lex.exp_minus = 0;
          lex.state = LEXER_STATE_LOOKUP_TOKEN;
        }
        break;
      case LEXER_STATE_IN_TRUE:
        if (*data != "true"[lex.offset]) {
          embedjson_error(data);
          return;
        }
        if (++lex.offset > 3) {
          embedjson_token(lexer, EMBEDJSON_TOKEN_TRUE);
          lex.state = LEXER_STATE_LOOKUP_TOKEN;
        }
        break;
      case LEXER_STATE_IN_FALSE:
        if (*data != "false"[lex.offset]) {
          embedjson_error(data);
          return;
        }
        if (++lex.offset > 4) {
          embedjson_token(lexer, EMBEDJSON_TOKEN_FALSE);
          lex.state = LEXER_STATE_LOOKUP_TOKEN;
        }
        break;
      case LEXER_STATE_IN_NULL:
        if (*data != "null"[lex.offset]) {
          embedjson_error(data);
          return;
        }
        if (++lex.offset > 3) {
          embedjson_token(lexer, EMBEDJSON_TOKEN_NULL);
          lex.state = LEXER_STATE_LOOKUP_TOKEN;
        }
        break;
    }
  }
  if (lex.state == LEXER_STATE_IN_STRING && data != string_chunk_begin) {
    embedjson_tokenc(lexer, string_chunk_begin, data - string_chunk_begin);
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
    case LEXER_STATE_LOOKUP_TOKEN:
      break;
    case LEXER_STATE_IN_STRING:
    case LEXER_STATE_IN_STRING_ESCAPE:
    case LEXER_STATE_IN_STRING_UNICODE_ESCAPE:
      embedjson_error(NULL);
      break;
    case LEXER_STATE_IN_NUMBER:
      if (lex.minus) {
        lex.int_value = 0 - lex.int_value;
      }
      embedjson_tokeni(lexer, lex.int_value);
      break;
    case LEXER_STATE_IN_NUMBER_FRAC: {
      double value = lex.frac_value * powm10(lex.frac_power) + lex.int_value;
      if (lex.minus) {
        value = 0 - value;
      }
      embedjson_tokenf(lexer, value);
      break;
    }
    case LEXER_STATE_IN_NUMBER_EXP_SIGN:
      embedjson_error(NULL);
      break;
    case LEXER_STATE_IN_NUMBER_EXP: {
      double value = lex.frac_value * powm10(lex.frac_power) + lex.int_value;
      value *= powm10(lex.exp_minus ? lex.exp_value : 0 - lex.exp_value);
      if (lex.minus) {
        value = 0 - value;
      }
      embedjson_tokenf(lexer, value);
      break;
    }
    case LEXER_STATE_IN_TRUE:
    case LEXER_STATE_IN_FALSE:
    case LEXER_STATE_IN_NULL:
      embedjson_error(NULL);
      break;
  }
}

