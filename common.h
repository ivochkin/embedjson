/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 *
 * Licensed under the MIT License (see LICENSE)
 */

#ifndef EMBEDJSON_AMALGAMATE
#pragma once
#endif /* EMBEDJSON_AMALGAMATE */


#ifndef EMBEDJSON_AMALGAMATE
#define EMBEDJSON_STATIC
#else
#define EMBEDJSON_STATIC static
#endif /* EMBEDJSON_AMALGAMATE */


struct embedjson_parser;
EMBEDJSON_STATIC int embedjson_error(struct embedjson_parser* parser,
    const char* position);

