PAWNCC=../pawn/source/compiler/build/pawncc
DUK=~/duktape/duk

all: build/init build/init.js

build/init.js: sbin/init/init.c
	emcc --memory-init-file 0 $< -o $@

build/init: build/init.js
	${DUK} -c $@ $<
