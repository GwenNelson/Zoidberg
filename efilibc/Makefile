OUTPUT = efilibc.a

EFI_CC ?= x86_64-w64-mingw32-gcc
EFI_AR ?= x86_64-w64-mingw32-ar

CFLAGS = -Iinc -Iefi -Iefi/inc -Iefi/inc/x86_64 -Iefi/inc/protocol -Wall -Wextra -Wno-long-long -g -ffreestanding

STDIO_OBJS= stdio/asprintf.o \
stdio/dprintf.o \
stdio/fprintf.o \
stdio/fwprintf.o \
stdio/printf.o \
#stdio/snprintf.o \
stdio/sprintf.o \
stdio/swprintf.o \
stdio/vasprintf.o \
stdio/vdprintf.o \
stdio/vfprintf.o \
stdio/vfwprintf.o \
stdio/vprintf.o \
stdio/vsnprintf.o \
stdio/vsprintf.o \
stdio/vswprintf.o \
stdio/vwprintf.o \
stdio/wprintf.o 

OBJS = console.o \
       errno.o \
       data.o \
       file.o \
       malloc.o \
       printf.o \
#       string.o \
       strtod.o \
       strtol.o \
       strtoll.o \
       strtoull.o \
       scanf.o \
       abort.o \
       serial.o \
       efilibc.o \
       atof.o \
       misc.o \
       strncmp.o \
       strrchr.o \
       strstr.o \
       memmove.o

all: $(OUTPUT)

%.o: %.c Makefile
	$(EFI_CC) -c -o $@ $< $(CFLAGS)
	
$(OUTPUT): $(OBJS) $(STDIO_OBJS)
	$(EFI_AR) rcs $(OUTPUT) $(OBJS) $(STDIO_OBJS)

.PHONY: clean

clean:
	rm -f *.o $(OUTPUT)

