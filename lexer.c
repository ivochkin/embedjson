/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 *
 * Licensed under the MIT License (see LICENSE)
 */

#ifndef EMBEDJSON_AMALGAMATE
#include "lexer.h"
#include "common.h"
#include "parser.h"
#endif /* EMBEDJSON_AMALGAMATE */

typedef enum {
  LEXER_STATE_LOOKUP_TOKEN = 0,
  LEXER_STATE_IN_STRING,
  LEXER_STATE_IN_STRING_ESCAPE,
  LEXER_STATE_IN_STRING_UNICODE_ESCAPE,
  LEXER_STATE_IN_NUMBER,
  LEXER_STATE_IN_NUMBER_FRAC,
  LEXER_STATE_IN_NUMBER_EXP_SIGN,
  LEXER_STATE_IN_NUMBER_EXP,
  LEXER_STATE_IN_TRUE,
  LEXER_STATE_IN_FALSE,
  LEXER_STATE_IN_NULL
} lexer_state;


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


/*
 * memcmp implementation taken from musl:
 * http://git.musl-libc.org/cgit/musl/tree/src/string/memcmp.c
 */
static int embedjson_memcmp(const void *vl, const void *vr, embedjson_size_t n)
{
  const unsigned char* l = (const unsigned char*) vl,
        *r = (const unsigned char*) vr;
  for (; n && *l == *r; n--, l++, r++);
  return n ? *l - *r : 0;
}


/*
 * Returns {-10}^{n}
 */
static double powm10(int n)
{
  static double e[] = {
    1e+308, 1e+307, 1e+306, 1e+305, 1e+304, 1e+303, 1e+302, 1e+301,
    1e+300, 1e+299, 1e+298, 1e+297, 1e+296, 1e+295, 1e+294, 1e+293,
    1e+292, 1e+291, 1e+290, 1e+289, 1e+288, 1e+287, 1e+286, 1e+285,
    1e+284, 1e+283, 1e+282, 1e+281, 1e+280, 1e+279, 1e+278, 1e+277,
    1e+276, 1e+275, 1e+274, 1e+273, 1e+272, 1e+271, 1e+270, 1e+269,
    1e+268, 1e+267, 1e+266, 1e+265, 1e+264, 1e+263, 1e+262, 1e+261,
    1e+260, 1e+259, 1e+258, 1e+257, 1e+256, 1e+255, 1e+254, 1e+253,
    1e+252, 1e+251, 1e+250, 1e+249, 1e+248, 1e+247, 1e+246, 1e+245,
    1e+244, 1e+243, 1e+242, 1e+241, 1e+240, 1e+239, 1e+238, 1e+237,
    1e+236, 1e+235, 1e+234, 1e+233, 1e+232, 1e+231, 1e+230, 1e+229,
    1e+228, 1e+227, 1e+226, 1e+225, 1e+224, 1e+223, 1e+222, 1e+221,
    1e+220, 1e+219, 1e+218, 1e+217, 1e+216, 1e+215, 1e+214, 1e+213,
    1e+212, 1e+211, 1e+210, 1e+209, 1e+208, 1e+207, 1e+206, 1e+205,
    1e+204, 1e+203, 1e+202, 1e+201, 1e+200, 1e+199, 1e+198, 1e+197,
    1e+196, 1e+195, 1e+194, 1e+193, 1e+192, 1e+191, 1e+190, 1e+189,
    1e+188, 1e+187, 1e+186, 1e+185, 1e+184, 1e+183, 1e+182, 1e+181,
    1e+180, 1e+179, 1e+178, 1e+177, 1e+176, 1e+175, 1e+174, 1e+173,
    1e+172, 1e+171, 1e+170, 1e+169, 1e+168, 1e+167, 1e+166, 1e+165,
    1e+164, 1e+163, 1e+162, 1e+161, 1e+160, 1e+159, 1e+158, 1e+157,
    1e+156, 1e+155, 1e+154, 1e+153, 1e+152, 1e+151, 1e+150, 1e+149,
    1e+148, 1e+147, 1e+146, 1e+145, 1e+144, 1e+143, 1e+142, 1e+141,
    1e+140, 1e+139, 1e+138, 1e+137, 1e+136, 1e+135, 1e+134, 1e+133,
    1e+132, 1e+131, 1e+130, 1e+129, 1e+128, 1e+127, 1e+126, 1e+125,
    1e+124, 1e+123, 1e+122, 1e+121, 1e+120, 1e+119, 1e+118, 1e+117,
    1e+116, 1e+115, 1e+114, 1e+113, 1e+112, 1e+111, 1e+110, 1e+109,
    1e+108, 1e+107, 1e+106, 1e+105, 1e+104, 1e+103, 1e+102, 1e+101,
    1e+100, 1e+099, 1e+098, 1e+097, 1e+096, 1e+095, 1e+094, 1e+093,
    1e+092, 1e+091, 1e+090, 1e+089, 1e+088, 1e+087, 1e+086, 1e+085,
    1e+084, 1e+083, 1e+082, 1e+081, 1e+080, 1e+079, 1e+078, 1e+077,
    1e+076, 1e+075, 1e+074, 1e+073, 1e+072, 1e+071, 1e+070, 1e+069,
    1e+068, 1e+067, 1e+066, 1e+065, 1e+064, 1e+063, 1e+062, 1e+061,
    1e+060, 1e+059, 1e+058, 1e+057, 1e+056, 1e+055, 1e+054, 1e+053,
    1e+052, 1e+051, 1e+050, 1e+049, 1e+048, 1e+047, 1e+046, 1e+045,
    1e+044, 1e+043, 1e+042, 1e+041, 1e+040, 1e+039, 1e+038, 1e+037,
    1e+036, 1e+035, 1e+034, 1e+033, 1e+032, 1e+031, 1e+030, 1e+029,
    1e+028, 1e+027, 1e+026, 1e+025, 1e+024, 1e+023, 1e+022, 1e+021,
    1e+020, 1e+019, 1e+018, 1e+017, 1e+016, 1e+015, 1e+014, 1e+013,
    1e+012, 1e+011, 1e+010, 1e+009, 1e+008, 1e+007, 1e+006, 1e+005,
    1e+004, 1e+003, 1e+002, 1e+001, 1.0000, 1e-001, 1e-002, 1e-003,
    1e-004, 1e-005, 1e-006, 1e-007, 1e-008, 1e-009, 1e-010, 1e-011,
    1e-012, 1e-013, 1e-014, 1e-015, 1e-016, 1e-017, 1e-018, 1e-019,
    1e-020, 1e-021, 1e-022, 1e-023, 1e-024, 1e-025, 1e-026, 1e-027,
    1e-028, 1e-029, 1e-030, 1e-031, 1e-032, 1e-033, 1e-034, 1e-035,
    1e-036, 1e-037, 1e-038, 1e-039, 1e-040, 1e-041, 1e-042, 1e-043,
    1e-044, 1e-045, 1e-046, 1e-047, 1e-048, 1e-049, 1e-050, 1e-051,
    1e-052, 1e-053, 1e-054, 1e-055, 1e-056, 1e-057, 1e-058, 1e-059,
    1e-060, 1e-061, 1e-062, 1e-063, 1e-064, 1e-065, 1e-066, 1e-067,
    1e-068, 1e-069, 1e-070, 1e-071, 1e-072, 1e-073, 1e-074, 1e-075,
    1e-076, 1e-077, 1e-078, 1e-079, 1e-080, 1e-081, 1e-082, 1e-083,
    1e-084, 1e-085, 1e-086, 1e-087, 1e-088, 1e-089, 1e-090, 1e-091,
    1e-092, 1e-093, 1e-094, 1e-095, 1e-096, 1e-097, 1e-098, 1e-099,
    1e-100, 1e-101, 1e-102, 1e-103, 1e-104, 1e-105, 1e-106, 1e-107,
    1e-108, 1e-109, 1e-110, 1e-111, 1e-112, 1e-113, 1e-114, 1e-115,
    1e-116, 1e-117, 1e-118, 1e-119, 1e-120, 1e-121, 1e-122, 1e-123,
    1e-124, 1e-125, 1e-126, 1e-127, 1e-128, 1e-129, 1e-130, 1e-131,
    1e-132, 1e-133, 1e-134, 1e-135, 1e-136, 1e-137, 1e-138, 1e-139,
    1e-140, 1e-141, 1e-142, 1e-143, 1e-144, 1e-145, 1e-146, 1e-147,
    1e-148, 1e-149, 1e-150, 1e-151, 1e-152, 1e-153, 1e-154, 1e-155,
    1e-156, 1e-157, 1e-158, 1e-159, 1e-160, 1e-161, 1e-162, 1e-163,
    1e-164, 1e-165, 1e-166, 1e-167, 1e-168, 1e-169, 1e-170, 1e-171,
    1e-172, 1e-173, 1e-174, 1e-175, 1e-176, 1e-177, 1e-178, 1e-179,
    1e-180, 1e-181, 1e-182, 1e-183, 1e-184, 1e-185, 1e-186, 1e-187,
    1e-188, 1e-189, 1e-190, 1e-191, 1e-192, 1e-193, 1e-194, 1e-195,
    1e-196, 1e-197, 1e-198, 1e-199, 1e-200, 1e-201, 1e-202, 1e-203,
    1e-204, 1e-205, 1e-206, 1e-207, 1e-208, 1e-209, 1e-210, 1e-211,
    1e-212, 1e-213, 1e-214, 1e-215, 1e-216, 1e-217, 1e-218, 1e-219,
    1e-220, 1e-221, 1e-222, 1e-223, 1e-224, 1e-225, 1e-226, 1e-227,
    1e-228, 1e-229, 1e-230, 1e-231, 1e-232, 1e-233, 1e-234, 1e-235,
    1e-236, 1e-237, 1e-238, 1e-239, 1e-240, 1e-241, 1e-242, 1e-243,
    1e-244, 1e-245, 1e-246, 1e-247, 1e-248, 1e-249, 1e-250, 1e-251,
    1e-252, 1e-253, 1e-254, 1e-255, 1e-256, 1e-257, 1e-258, 1e-259,
    1e-260, 1e-261, 1e-262, 1e-263, 1e-264, 1e-265, 1e-266, 1e-267,
    1e-268, 1e-269, 1e-270, 1e-271, 1e-272, 1e-273, 1e-274, 1e-275,
    1e-276, 1e-277, 1e-278, 1e-279, 1e-280, 1e-281, 1e-282, 1e-283,
    1e-284, 1e-285, 1e-286, 1e-287, 1e-288, 1e-289, 1e-290, 1e-291,
    1e-292, 1e-293, 1e-294, 1e-295, 1e-296, 1e-297, 1e-298, 1e-299,
    1e-300, 1e-301, 1e-302, 1e-303, 1e-304, 1e-305, 1e-306, 1e-307,
    1e-308, 1e-309, 1e-310, 1e-311, 1e-312, 1e-313, 1e-314, 1e-315,
    1e-316, 1e-317, 1e-318, 1e-319, 1e-320, 1e-321, 1e-322, 1e-323,
  };
  return e[n + 308];
}


EMBEDJSON_STATIC int embedjson_lexer_push(embedjson_lexer* lexer,
    const char* data, embedjson_size_t size)
{
  embedjson_lexer lex = *lexer;
  const char* string_chunk_begin =
    (lex.state == LEXER_STATE_IN_STRING) ? data : 0;
  const char* end = data + size;
  for (; data != end; ++data) {
    switch(lex.state) {
      case LEXER_STATE_LOOKUP_TOKEN:
        if (*data == ' ' || *data == '\n' || *data == '\r' || *data == '\t') {
          continue;
        } else if (*data == ':') {
          RETURN_IF(embedjson_token(lexer, EMBEDJSON_TOKEN_COLON, data));
        } else if (*data == ',') {
          RETURN_IF(embedjson_token(lexer, EMBEDJSON_TOKEN_COMMA, data));
        } else if (*data == '{') {
          RETURN_IF(embedjson_token(lexer, EMBEDJSON_TOKEN_OPEN_CURLY_BRACKET, data));
        } else if (*data == '}') {
          RETURN_IF(embedjson_token(lexer,
                EMBEDJSON_TOKEN_CLOSE_CURLY_BRACKET, data));
        } else if (*data == '[') {
          RETURN_IF(embedjson_token(lexer, EMBEDJSON_TOKEN_OPEN_BRACKET, data));
        } else if (*data == ']') {
          RETURN_IF(embedjson_token(lexer, EMBEDJSON_TOKEN_CLOSE_BRACKET, data));
        } else if (*data == '"') {
          string_chunk_begin = data + 1;
          lex.state = LEXER_STATE_IN_STRING;
          RETURN_IF(embedjson_tokenc_begin(lexer, data));
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
#if EMBEDJSON_VALIDATE_UTF8
        if (lex.nb) {
          if (lex.nb == 2 && lex.cc == 1) {
            /*
             * '\xa0' <= *data <= '\xbf'
             * Or, in binary representation:
             * b10100000 <= *data <= b10111111
             * Therefore, *data & b11100000 should be equal to b10100000
             */
            if ((*data & 0xe0) != 0xa0) {
              return embedjson_error_ex((embedjson_parser*) lexer,
                  EMBEDJSON_BAD_UTF8, data);
            }
            lex.cc = 0;
          } else if (lex.nb == 3) {
            if (lex.cc == 2) {
              /*
               * '\x90' <= *data <= '\xbf'
               * Or, in binary representation:
               * b10010000 <= *data <= b10111111
               * Therefore, *data & b11010000 should be equal to b10010000
               */
              if ((*data & 0xd0) != 0x90) {
                return embedjson_error_ex((embedjson_parser*) lexer,
                    EMBEDJSON_BAD_UTF8, data);
              }
              lex.cc = 0;
            } else if (lex.cc == 3) {
              /*
               * '\x80' <= *data <= '\x8f'
               * Or, in binary representation:
               * b10000000 <= *data <= b10001111
               * Therefore, *data & b11110000 should be equal to b10000000
               */
              if ((*data & 0xf0) != 0x80) {
                return embedjson_error_ex((embedjson_parser*) lexer,
                    EMBEDJSON_BAD_UTF8, data);
              }
              lex.cc = 0;
            }
          } else if ((*data & 0xc0) != 0x80) {
            /*
             * '\x80' <= *data <= '\xbf'
             * Or, in binary representation:
             * b10000000 <= *data <= b10111111
             * Therefore, *data & b11000000 should be equal to b10000000
             */
            return embedjson_error_ex((embedjson_parser*) lexer,
                EMBEDJSON_BAD_UTF8, data);
          }
          lex.nb--;
        }
        if ((*data & 0xe0) == 0xc0) {
          lex.nb = 1;
        } else if ((*data & 0xf0) == 0xe0) {
          if (*data == '\xe0') {
            lex.cc = 1;
          }
          lex.nb = 2;
        } else if ((*data & 0xf8) == 0xf0) {
          if (*data == '\xf0') {
            lex.cc = 2;
          } else if (*data == '\xf4') {
            lex.cc = 3;
          }
          lex.nb = 3;
        } else if ((*data & 0xf8) == 0xf8) {
          /**
           * According to RFC 3629 "UTF-8, a transformation format of ISO 10646"
           * maximum length of the UTF-8 byte sequence is 4.
           *
           * See RFC 3629 Section 3 and Section 4 for details.
           */
          return embedjson_error_ex((embedjson_parser*) lexer,
              EMBEDJSON_LONG_UTF8, data);
        } else {
#else
        {
#endif
          if (*data == '\\') {
            if (data != string_chunk_begin) {
              RETURN_IF(embedjson_tokenc(lexer, string_chunk_begin,
                    data - string_chunk_begin));
            }
            lex.state = LEXER_STATE_IN_STRING_ESCAPE;
          } else if (*data == '"') {
            if (data != string_chunk_begin) {
              RETURN_IF(embedjson_tokenc(lexer, string_chunk_begin,
                    data - string_chunk_begin));
            }
            RETURN_IF(embedjson_tokenc_end(lexer, data));
            lex.state = LEXER_STATE_LOOKUP_TOKEN;
          }
        }
        break;
      case LEXER_STATE_IN_STRING_ESCAPE:
        if (*data == '"') {
          RETURN_IF(embedjson_tokenc(lexer, "\"", 1));
        } else if (*data == '\\') {
          RETURN_IF(embedjson_tokenc(lexer, "\\", 1));
        } else if (*data == '/') {
          RETURN_IF(embedjson_tokenc(lexer, "/", 1));
        } else if (*data == 'b') {
          RETURN_IF(embedjson_tokenc(lexer, "\b", 1));
        } else if (*data == 'f') {
          RETURN_IF(embedjson_tokenc(lexer, "\f", 1));
        } else if (*data == 'n') {
          RETURN_IF(embedjson_tokenc(lexer, "\n", 1));
        } else if (*data == 'r') {
          RETURN_IF(embedjson_tokenc(lexer, "\r", 1));
        } else if (*data == 't') {
          RETURN_IF(embedjson_tokenc(lexer, "\t", 1));
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
          return embedjson_error_ex((embedjson_parser*) lexer,
              EMBEDJSON_BAD_UNICODE_ESCAPE, data);
        }
        switch(lex.offset) {
          case 0: lex.unicode_cp[0] = value << 4; break;
          case 1: lex.unicode_cp[0] |= value; break;
          case 2: lex.unicode_cp[1] = value << 4; break;
          case 3:
            lex.unicode_cp[1] |= value;
            RETURN_IF(embedjson_tokenc(lexer, lex.unicode_cp, 2));
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
          RETURN_IF(embedjson_tokeni(lexer, lex.int_value, data));
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
          double value = lex.int_value +
            lex.frac_value * powm10(lex.frac_power);
          if (lex.minus) {
            value = 0 - value;
          }
          RETURN_IF(embedjson_tokenf(lexer, value, data));
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
          return embedjson_error_ex((embedjson_parser*) lexer,
              EMBEDJSON_BAD_EXPONENT, data);
        }
        lex.state = LEXER_STATE_IN_NUMBER_EXP;
        break;
      case LEXER_STATE_IN_NUMBER_EXP:
        if ('0' <= *data && *data <= '9') {
          lex.exp_value = 10 * lex.exp_value + *data - '0';
        } else {
          data--;
          double value = lex.int_value
            + lex.frac_value * powm10(lex.frac_power);
          value *= powm10(lex.exp_minus ? lex.exp_value : 0 - lex.exp_value);
          if (lex.minus) {
            value = 0 - value;
          }
          RETURN_IF(embedjson_tokenf(lexer, value, data));
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
          return embedjson_error_ex((embedjson_parser*) lexer,
              EMBEDJSON_BAD_TRUE, data);
        }
        if (++lex.offset > 3) {
          RETURN_IF(embedjson_token(lexer, EMBEDJSON_TOKEN_TRUE, data));
          lex.state = LEXER_STATE_LOOKUP_TOKEN;
        }
        break;
      case LEXER_STATE_IN_FALSE:
        if (*data != "false"[lex.offset]) {
          return embedjson_error_ex((embedjson_parser*) lexer,
              EMBEDJSON_BAD_FALSE, data);
        }
        if (++lex.offset > 4) {
          RETURN_IF(embedjson_token(lexer, EMBEDJSON_TOKEN_FALSE, data));
          lex.state = LEXER_STATE_LOOKUP_TOKEN;
        }
        break;
      case LEXER_STATE_IN_NULL:
        if (*data != "null"[lex.offset]) {
          return embedjson_error_ex((embedjson_parser*) lexer,
              EMBEDJSON_BAD_NULL, data);
        }
        if (++lex.offset > 3) {
          RETURN_IF(embedjson_token(lexer, EMBEDJSON_TOKEN_NULL, data));
          lex.state = LEXER_STATE_LOOKUP_TOKEN;
        }
        break;
    }
  }
  if (lex.state == LEXER_STATE_IN_STRING && data != string_chunk_begin) {
    RETURN_IF(embedjson_tokenc(lexer, string_chunk_begin,
          data - string_chunk_begin));
  }

  /*
   * Cache-friendly update lexer state only if it has changed
   */
  if (embedjson_memcmp(&lex, lexer, sizeof(lex))) {
    *lexer = lex;
  }
  return 0;
}


EMBEDJSON_STATIC int embedjson_lexer_finalize(embedjson_lexer* lexer)
{
  embedjson_lexer lex = *lexer;
  switch(lex.state) {
    case LEXER_STATE_LOOKUP_TOKEN:
      break;
    case LEXER_STATE_IN_STRING:
    case LEXER_STATE_IN_STRING_ESCAPE:
    case LEXER_STATE_IN_STRING_UNICODE_ESCAPE:
      return embedjson_error_ex((embedjson_parser*) lexer,
          EMBEDJSON_EOF_IN_STRING, 0);
    case LEXER_STATE_IN_NUMBER:
      if (lex.minus) {
        lex.int_value = 0 - lex.int_value;
      }
      RETURN_IF(embedjson_tokeni(lexer, lex.int_value, 0));
      break;
    case LEXER_STATE_IN_NUMBER_FRAC: {
      double value = lex.frac_value * powm10(lex.frac_power) + lex.int_value;
      if (lex.minus) {
        value = 0 - value;
      }
      RETURN_IF(embedjson_tokenf(lexer, value, 0));
      break;
    }
    case LEXER_STATE_IN_NUMBER_EXP_SIGN:
      return embedjson_error_ex((embedjson_parser*) lexer,
          EMBEDJSON_EOF_IN_EXPONENT, 0);
    case LEXER_STATE_IN_NUMBER_EXP: {
      double value = lex.frac_value * powm10(lex.frac_power) + lex.int_value;
      value *= powm10(lex.exp_minus ? lex.exp_value : 0 - lex.exp_value);
      if (lex.minus) {
        value = 0 - value;
      }
      RETURN_IF(embedjson_tokenf(lexer, value, 0));
      break;
    }
    case LEXER_STATE_IN_TRUE:
      return embedjson_error_ex((embedjson_parser*) lexer,
          EMBEDJSON_EOF_IN_TRUE, 0);
    case LEXER_STATE_IN_FALSE:
      return embedjson_error_ex((embedjson_parser*) lexer,
          EMBEDJSON_EOF_IN_FALSE, 0);
    case LEXER_STATE_IN_NULL:
      return embedjson_error_ex((embedjson_parser*) lexer,
          EMBEDJSON_EOF_IN_NULL, 0);
  }
  return 0;
}

