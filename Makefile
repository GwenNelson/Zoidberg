OVMFPATH=/home/gareth/edk2/Build/OvmfX64/DEBUG_GCC46/FV
INCLUDES=-Iefilibc/inc -Iefilibc/efi/inc -Iefilibc/efi/inc/protocol -Iefilibc/efi/inc/x86_64
ROMPATH=/usr/lib/ipxe/qemu/efi-e1000.rom

all: BOOTX64.EFI boot.fs

kernel.o: kernel.c
	x86_64-w64-mingw32-gcc -ffreestanding ${INCLUDES} -c $< -o $@

efilibc/efilibc.a:
	make -C efilibc

BOOTX64.EFI: efilibc/efilibc.a kernel.o
	x86_64-w64-mingw32-gcc -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main -o $@ kernel.o efilibc/efilibc.a -lgcc

boot.fs: BOOTX64.EFI
	dd if=/dev/zero of=$@ bs=1M count=33
	/sbin/mkfs.vfat $@ -F 32
	mmd -i $@ ::/EFI
	mmd -i $@ ::/EFI/BOOT
	mcopy -i $@ BOOTX64.EFI ::/EFI/BOOT

run-qemu:
	qemu-system-x86_64 -bios ${OVMFPATH}/OVMF.fd -usb -usbdevice disk::boot.fs -netdev user,id=mynet0,net=192.168.76.0/24,dhcpstart=192.168.76.9 -device virtio-net,netdev=mynet0 -nographic -serial stdio
