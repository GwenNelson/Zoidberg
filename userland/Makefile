
all: build/init build/init.js

build/init.js: sbin/init/init.c
	emcc --closure 1 -O2 -s TOTAL_MEMORY=2097152 -s TOTAL_STACK=524288 --memory-init-file 0 $< -o $@

build/init: build/init.js
	${DUK} -c $@ $<
