#!/bin/sh

BUILD_DIR="$1"
GENERATOR="$2"
BUILD_TYPE="$3"
shift;
shift;
shift;

echo "cmake -S . -B $BUILD_DIR -G $GENERATOR -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE $@"
cmake -S . -B $BUILD_DIR -G $GENERATOR -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE "$@"
