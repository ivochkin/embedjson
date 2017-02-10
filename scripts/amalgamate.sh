#!/usr/bin/env bash

cat >embedjson.c <<EOT
/**
 * This is an amalgamated version of the embedjson library.
 *
 * Regular sources of the library could be found in the
 * official repository:
 *
 *   https://github.com/ivochkin/embedjson
 *
 * "Amalgamated" means that all header and sources files
 * of the library are merged into a single embedjson.c file.
 * Due to the embedjson design, a single C file is enough,
 * since the library is considered to be inlined into each
 * object file that needs it. Code bloating is expected to
 * be eliminated by the LTO (Link-Time Optimization) compiler
 * option.
 *
 * Embedjson revision: $(git rev-parse HEAD)
 * Embedjson version: $(git describe --tags --long)
 * Generated at: $(LC_ALL=en_US.utf8 date -u)
 *
 */

/**
@copyright

EOT

cat LICENSE >> embedjson.c

cat >> embedjson.c <<EOT
 */

#define EMBEDJSON_AMALGAMATE

EOT
cat common.h | tail -n +7 >> embedjson.c
cat lexer.h | tail -n +7 >> embedjson.c
cat lexer.c | tail -n +7 >> embedjson.c
cat parser.h | tail -n +7 >> embedjson.c
cat parser.c | tail -n +7 >> embedjson.c
