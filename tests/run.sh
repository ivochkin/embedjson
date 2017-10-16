#!/usr/bin/env bash

embedjson_lint="$1"
cases_dir="$2"

for i in $cases_dir/*; do
  echo -n "Run test $i ... "
  tmpfile=/tmp/embedjson.$RANDOM
  cat "$i/in.json" | $embedjson_lint --verbose > $tmpfile
  [[ $? != 0 ]] && exit 1
  if [ "$(cat $tmpfile)" == "$(cat $i/out.txt)" ]; then
    rm $tmpfile
    echo OK
  else
    echo FAIL
    echo Expected content of $i/out.txt
    echo Got content of $tmpfile
    exit 1
  fi
done
