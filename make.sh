#!/usr/bin/env bash

./git-version-gen
autoreconf -i
./configure -q CFLAGS="-O0 -g" --enable-shared=no --enable-test --disable-docs
make
