all: fullsdk newlib/build/x86_64-zoidberg/newlib/libc.a build/sbin/init gnu-efi-3.0.4/x86_64/lib/libefi.a build/bin/sh build/bin/mysh

export PATH := ${PWD}/sdk/usr/bin:${PATH}

gnu-efi-3.0.4/x86_64/lib/libefi.a: sdktools
	make -C gnu-efi-3.0.4 CROSS_COMPILE=x86_64-zoidberg- -i

newlib/build/x86_64-zoidberg/newlib/libc.a: sdktools
	mkdir -p newlib/build
	cd newlib/build; ../configure --target=x86_64-zoidberg
	CFLAGS="-nostdinc" make -C newlib/build -j

INCLUDES=-I${PWD}/gnu-efi-3.0.4/inc/ -I${PWD}/newlib/newlib/libc/include/

APPCFLAGS=-nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main -L${PWD}/newlib/build/x86_64-zoidberg/newlib/ ${PWD}/newlib/build/x86_64-zoidberg/newlib/libc.a

export INCLUDES
export APPCFLAGS

build/sbin/init.o: sbin/init/init.c fullsdk
	mkdir -p build/sbin
	x86_64-zoidberg-gcc -ffreestanding ${INCLUDES} -c $< -o $@

build/sbin/init: build/sbin/init.o newlib/build/x86_64-zoidberg/newlib/libc/sys/zoidberg/syscalls.o
	mkdir -p build/sbin
	x86_64-zoidberg-gcc ${APPCFLAGS} ${INCLUDES} -o $@ $^ -lgcc -lc

build/bin/sh_wordexp.o: bin/sh/sh_wordexp.c
	mkdir -p build/bin
	x86_64-zoidberg-gcc -ffreestanding ${INCLUDES} -c $< -o $@

build/bin/sh.o: bin/sh/sh.c fullsdk
	mkdir -p build/bin
	x86_64-zoidberg-gcc -ffreestanding ${INCLUDES} -c $< -o $@

build/bin/sh:  build/bin/sh.o newlib/build/x86_64-zoidberg/newlib/libc/sys/zoidberg/syscalls.o
	mkdir -p build/bin
	x86_64-zoidberg-gcc ${APPCFLAGS} ${INCLUDES} -o $@ $^ -lgcc -lc

sdk/usr/bin/%: sdk_tools/%
	ln -s ${PWD}/$< $@

build/bin/mysh:
	make -C bin/mysh
	cp bin/mysh/mysh build/bin/mysh

sdktools: sdk/usr/bin/x86_64-zoidberg-ar sdk/usr/bin/x86_64-zoidberg-as sdk/usr/bin/x86_64-zoidberg-gcc sdk/usr/bin/x86_64-zoidberg-ranlib

sdk/usr/lib/libc.a: newlib/build/x86_64-zoidberg/newlib/libc.a
	CFLAGS=-nostdinc make -C newlib/build DESTDIR=../../sdk/ install

fullsdk: sdk/usr/lib/libc.a sdktools
