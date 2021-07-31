#!/usr/bin/env bash
# Check compilation for all possible configurations

RED='\033[0;31m'
GREEN='\033[0;32m'
RESET='\033[0m'

configurations=()

for CMAKE_BUILD_TYPE in Release Debug; do
for EMBEDJSON_DEBUG in TRUE FALSE; do
for EMBEDJSON_DYNAMIC_STACK in TRUE FALSE; do
for EMBEDJSON_VALIDATE_UTF8 in TRUE FALSE; do
for EMBEDJSON_BIGNUM in TRUE FALSE; do
  options="
    -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE \
    -DEMBEDJSON_DEBUG=$EMBEDJSON_DEBUG \
    -DEMBEDJSON_DYNAMIC_STACK=$EMBEDJSON_DYNAMIC_STACK \
    -DEMBEDJSON_VALIDATE_UTF8=$EMBEDJSON_VALIDATE_UTF8 \
    -DEMBEDJSON_BIGNUM=$EMBEDJSON_BIGNUM \
    "
  configurations+=("$options")
done
done
done
done
done

n=${#configurations[@]}
source_dir=$(pwd)
for num in $(seq $n); do
  i=$(($num - 1))
  configuration=${configurations[$i]}
  build_dir=/tmp/embedjson-build-$(uuidgen)
  mkdir $build_dir
  cd $build_dir
  cmake $configuration $source_dir > /dev/null 2> /dev/null || (echo -e [$num/$n] $RED$configuration$RESET ; continue)
  make -j > /dev/null 2> /dev/null || (echo -e [$num/$n] $RED$configuration$RESET; continue)
  make test > /dev/null 2> /dev/null || (echo -e [$num/$n] $RED$configuration$RESET; continue)
  cd $source_dir
  rm -rf $build_dir
  echo -e [$num/$n] $GREEN$configuration$RESET
done
