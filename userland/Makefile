all: newlib/build/x86_64-zoidberg/newlib/libc.a build/sbin/init

newlib/build/x86_64-zoidberg/newlib/libc.a:
	mkdir -p newlib/build
	cd newlib/build; ../configure --target=x86_64-zoidberg
	CFLAGS=-nostdinc make -C newlib/build

CFLAGS=-nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main

build/sbin/init: sbin/init/init.c
	sdk/x86_64-zoidberg-gcc -o $@ $< newlib/build/x86_64-zoidberg/newlib/libc.a -lgcc

