INCLUDES=-I../efilibc/inc -I../efilibc/efi/inc -I../efilibc/efi/inc/protocol -I../efilibc/efi/inc/x86_64 -I.
EFI_CC ?= x86_64-w64-mingw32-gcc
EFI_AR ?= x86_64-w64-mingw32-ar
CFLAGS=-ffreestanding -Wall -DPHYSFS_NO_CDROM_SUPPORT=1 -DPHYSFS_NO_THREAD_SUPPORT=1 -DPHYSFS_SUPPORTS_ZIP=1 ${INCLUDES}

ZLIB_OBJS=
PHYSFS_OBJS=physfs.o \
            physfs_byteorder.o \
            physfs_unicode.c \
            platform/zoidberg.o \
            archivers/dir.o

all: build/libphysfs.a

%.o: %.c Makefile
	${EFI_CC} -c -o build/$@ $< ${CFLAGS}

build/libphysfs.a: ${ZLIB_OBJS} ${PHYSFS_OBJS}
	${EFI_AR} rcs $@ build/*.o build/platform/*.o build/archivers/*.o


