#!/usr/bin/env bash

cat >embedjson.c <<EOT
/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 *
 * Licensed under the MIT License (see LICENSE)
 */

#define EMBEDJSON_AMALGAMATE

EOT
cat common.h | tail -n +7 >> embedjson.c
cat lexer.h | tail -n +7 >> embedjson.c
cat lexer.c | tail -n +7 >> embedjson.c
cat parser.h | tail -n +7 >> embedjson.c
cat parser.c | tail -n +7 >> embedjson.c
