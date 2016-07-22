all: fullsdk newlib/build/x86_64-zoidberg/newlib/libc.a build/sbin/init gnu-efi-3.0.4/x86_64/lib/libefi.a

export PATH := ${PWD}/sdk/usr/bin:${PATH}

gnu-efi-3.0.4/x86_64/lib/libefi.a: sdktools
	make -C gnu-efi-3.0.4 CROSS_COMPILE=x86_64-zoidberg- -i

newlib/build/x86_64-zoidberg/newlib/libc.a: sdktools
	mkdir -p newlib/build
	cd newlib/build; ../configure --target=x86_64-zoidberg
	CFLAGS=-nostdinc make -C newlib/build -j

INCLUDES=-Ignu-efi-3.0.4/inc/ -Inewlib/newlib/libc/include/

APPCFLAGS=-nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main 

build/sbin/init.o: sbin/init/init.c fullsdk
	mkdir -p build/sbin
	x86_64-zoidberg-gcc -ffreestanding ${INCLUDES} -c $< -o $@

build/sbin/init: newlib/build/x86_64-zoidberg/newlib/libc.a build/sbin/init.o newlib/build/x86_64-zoidberg/newlib/libc/sys/zoidberg/syscalls.o
	mkdir -p build/sbin
	x86_64-zoidberg-gcc ${APPCFLAGS} ${INCLUDES} -o $@ $^ -lgcc

sdk/usr/bin/%: sdk_tools/%
	ln -s ${PWD}/$< $@

sdktools: sdk/usr/bin/x86_64-zoidberg-ar sdk/usr/bin/x86_64-zoidberg-as sdk/usr/bin/x86_64-zoidberg-gcc sdk/usr/bin/x86_64-zoidberg-ranlib

sdk/usr/lib/libc.a: newlib/build/x86_64-zoidberg/newlib/libc.a
	CFLAGS=-nostdinc make -C newlib/build DESTDIR=../../sdk/ install

fullsdk: sdk/usr/lib/libc.a sdktools
