# embedjson
[![License MIT](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/ivochkin/embedjson/master/LICENSE)
[![Build Status](https://travis-ci.org/ivochkin/embedjson.svg?branch=master)](https://travis-ci.org/ivochkin/embedjson)
[![codecov.io](https://codecov.io/github/ivochkin/embedjson/coverage.svg?branch=master)](https://codecov.io/github/ivochkin/embedjson?branch=master)

SAX-style JSON parser, a component of jcppy code generator (https://github.com/ivochkin/jcppy)

## Configuring embedjson

A set of `#define` definitions can be specified *before* embedding embedjson.c
into the code to configure embedjson:

| Name                        | Default   | Description
|:--------------------------- |:--------- |:--------------------------------
| EMBEDJSON_DYNAMIC_STACK     | undefined | Define to enable dynamic stack to hold parser's state. When dynamic stack is enabled, user is responsible for initializing `embedjson_parser.stack` and `embedjson_parser.stack_size` properties manually. By default static stack of the fixed size is used.
| EMBEDJSON_STATIC_STACK_SIZE | 16        | Size (in bytes) of the stack. Size of the stack determines maximum supported objects/arrays nesting level.

## TODO
- Error handling
- Unicode validation
- UTF-16, UTF-32 support
