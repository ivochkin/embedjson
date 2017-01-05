#!/usr/bin/env bash
# Create test coverage report
#
# Prerequisite:
# * installed lcov
# * code is built with --coverage -O0
#
# Can be run in two modes:
# * default (no special environment variables set) - create local html report
# * CI ("$TRAVIS" == "true") - upload coverage report to codecov.io

lcov --directory . --capture --output-file lcov.info
lcov --remove lcov.info '*ut_*' --output lcov.info

if [ "$TRAVIS" == "true" ]; then
  # deploy
  bash <(curl -s https://codecov.io/bash)
else
  # generate html localy
  mkdir -p doc/coverage
  mv lcov.info doc/coverage/
  pushd doc/coverage
  genhtml lcov.info
  echo
  echo Open file://$(pwd)/index.html in your browser for the coverage report
  popd >/dev/null
fi
