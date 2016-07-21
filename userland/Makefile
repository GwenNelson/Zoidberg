PAWNCC=../pawn/source/compiler/build/pawncc

all: build/init

build/init.js: sbin/init/init.c
	emcc --memory-init-file 0 $< -o $@

build/init: sbin/init/init.p
	${PAWNCC} -iinclude/pawn -v2 $< -o$@
	mv $@.amx $@
