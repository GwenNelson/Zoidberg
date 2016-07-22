all: newlib/build/x86_64-zoidberg/newlib/libc.a

newlib/build/x86_64-zoidberg/newlib/libc.a:
	mkdir -p newlib/build
	cd newlib/build; ../configure --target=x86_64-zoidberg
	CFLAGS=-nostdinc make -C newlib/build

