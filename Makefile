OVMFPATH=/home/gareth/edk2/Build/OvmfX64/DEBUG_GCC46/FV
INCLUDES=-I. -Inewlib/newlib/libc/include -Iefilibc/efi/inc -Iefilibc/efi/inc/protocol -Iefilibc/efi/inc/x86_64 -Ilibnet/src -Ipawn/source/amx/
ROMPATH=/usr/lib/ipxe/qemu/efi-e1000.rom

CC=x86_64-w64-mingw32-gcc
CFLAGS=-ffreestanding ${INCLUDES}


all: BOOTX64.EFI boot.img

LIBNET_OBJS=libnet/build/libnet_init.o\
            libnet/build/libnet_build_ip.o\
            libnet/build/libnet_build_dhcp.o\
            libnet/build/libnet_build_udp.o\
            libnet/build/libnet_link_none.o\
            libnet/build/libnet_pblock.o\
            libnet/build/libnet_checksum.o

libnet/build/%.o: libnet/src/%.c
	x86_64-w64-mingw32-gcc -ffreestanding ${INCLUDES} -c $< -o $@

libnet/libnet.a: ${LIBNET_OBJS}
	x86_64-w64-mingw32-ar rcs $@ libnet/build/*.o

genversion:
	./genversion.sh

amx.o: pawn/source/amx/amx.c
	x86_64-w64-mingw32-gcc -DPAWN_CELL_SIZE=64 -ffreestanding ${INCLUDES} -c $< -o $@

vm/vm_pawn.o: vm/vm_pawn.c
	x86_64-w64-mingw32-gcc -ffreestanding ${INCLUDES} -c $< -o $@

k_thread.o: k_thread.c cr.c cr.h
	x86_64-w64-mingw32-gcc -ffreestanding ${INCLUDES} -c $< -o $@

k_network.o: k_network.c
	x86_64-w64-mingw32-gcc -ffreestanding ${INCLUDES} -c $< -o $@

k_heap.o: k_heap.c
	x86_64-w64-mingw32-gcc -ffreestanding ${INCLUDES} -c $< -o $@

k_main.o: k_main.c genversion
	x86_64-w64-mingw32-gcc -ffreestanding ${INCLUDES} -c $< -o $@

kmsg.o: kmsg.c
	x86_64-w64-mingw32-gcc -ffreestanding ${INCLUDES} -c $< -o $@

efilibc/efilibc.a:
	make -C efilibc

newlib/build/x86_64-zoidberg/newlib/libc.a:
	mkdir -p newlib/build
	cd newlib/build; ../configure --target=x86_64-zoidberg
	CFLAGS=-nostdinc make -C newlib/build

BOOTX64.EFI:newlib/build/x86_64-zoidberg/newlib/libc.a  k_main.o kmsg.o k_heap.o k_network.o k_thread.o amx.o vm/vm_pawn.o libnet/libnet.a
	x86_64-w64-mingw32-gcc -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main -o $@ kmsg.o k_thread.o k_heap.o k_network.o k_main.o amx.o vm/vm_pawn.o libnet/libnet.a newlib/build/x86_64-zoidberg/newlib/libc.a -lgcc

boot.img: BOOTX64.EFI
	dd if=/dev/zero of=$@ bs=1M count=33
	/sbin/mkfs.vfat $@ -F 32
	mmd -i $@ ::/EFI
	mmd -i $@ ::/EFI/BOOT
	mcopy -i $@ BOOTX64.EFI ::/EFI/BOOT

boot_hdd.img: boot.img
	mkgpt -o boot_hdd.img --image-size 4096 --part $^ --type system

boot.iso: boot.img
	cp $^ iso
	xorriso -as mkisofs -R -f -e boot.img -no-emul-boot -o boot.iso iso

run-qemu:
	qemu-system-x86_64 -bios ${OVMFPATH}/OVMF.fd -usb -usbdevice disk::boot.img -netdev user,id=mynet0,net=192.168.76.0/24,dhcpstart=192.168.76.9 -device e1000,netdev=mynet0,mac=DE:AD:BE:EF:FC:E6  -m 1G -net dump,file=./dump.pcap -nographic -serial stdio 
