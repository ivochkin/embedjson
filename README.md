# embedjson
[![License MIT](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/ivochkin/embedjson/master/LICENSE)
[![Build Status](https://travis-ci.org/ivochkin/embedjson.svg?branch=master)](https://travis-ci.org/ivochkin/embedjson)
[![codecov.io](https://codecov.io/github/ivochkin/embedjson/coverage.svg?branch=master)](https://codecov.io/github/ivochkin/embedjson?branch=master)

SAX-style JSON parser, a component of jcppy code generator (https://github.com/ivochkin/jcppy)

## Disclaimer

If you are looking for a generic JSON parser/emitter, you are in the wrong place.
Embedjson is __not__ a general purpose JSON library, it is designed to be embedded into the
C/C++ object files generated by [jcppy](https://github.com/ivochkin/jcppy) to handle specific
case of JSON parsing when document structure is known (completely, or partically, as defined
by JSON Schema). In the case, JSON parsing can be optimized by throwing away code branches
that will be never reached due to the restricting document schema.

Take a look at those brilliant projects if you need a generic JSON library:
* https://github.com/lloyd/yajl
* https://github.com/miloyip/rapidjson

## Features

* UTF-8 validation, including [UTF-8 Shortest Form](http://www.unicode.org/versions/corrigendum1.html)

## Configuring embedjson

A set of `#define` definitions can be specified *before* embedding embedjson.c
into the code to configure embedjson:

| Name                        | Default   | Description
|:--------------------------- |:--------- |:--------------------------------
| EMBEDJSON_DYNAMIC_STACK     | 0         | Define to enable dynamic stack to hold parser's state. When dynamic stack is enabled, user is responsible for initializing `embedjson_parser.stack` and `embedjson_parser.stack_size` properties . By default static stack of the fixed size is used.
| EMBEDJSON_STATIC_STACK_SIZE | 16        | Size (in bytes) of the stack. Size of the stack determines maximum supported objects/arrays nesting level.
| EMBEDJSON_VALIDATE_UTF8     | 1         | Enable UTF-8 validation

## TODO
- Error handling
- UTF-16, UTF-32 support
- amalgamate.sh test
