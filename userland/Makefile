all: newlib/build/x86_64-zoidberg/newlib/libc.a build/sbin/init

newlib/build/x86_64-zoidberg/newlib/libc.a:
	mkdir -p newlib/build
	cd newlib/build; ../configure --target=x86_64-zoidberg
	CFLAGS=-nostdinc make -C newlib/build

INCLUDES=-Ignu-efi-3.0.4/inc/ -Inewlib/newlib/libc/include/

CFLAGS=-nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main 

build/sbin/init.o: sbin/init/init.c
	mkdir -p build/sbin
	sdk/x86_64-zoidberg-gcc -ffreestanding ${INCLUDES} -c $< -o $@

build/sbin/init: build/sbin/init.o newlib/build/x86_64-zoidberg/newlib/libc.a build/sbin/init.o
	mkdir -p build/sbin
	sdk/x86_64-zoidberg-gcc ${CFLAGS} ${INCLUDES} -o $@ $^ -lgcc

