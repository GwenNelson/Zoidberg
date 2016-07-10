#!/bin/sh
export PATH=~/bin/bin:$PWD/sdk:$PATH
make newlib/build/x86_64-zoidberg/newlib/libc.a
make all
