#!/bin/sh

export OVMFPATH=/home/gareth/edk2/Build/OvmfX64/DEBUG_GCC46/FV
export ROMPATH=/usr/lib/ipxe/qemu/efi-e1000.rom


qemu-system-x86_64 -bios ${OVMFPATH}/OVMF.fd -usb -usbdevice disk::boot.img -netdev user,id=mynet0,net=192.168.76.0/24,dhcpstart=192.168.76.9 -device e1000,netdev=mynet0,mac=DE:AD:BE:EF:FC:E6  -m 1G -net dump,file=./dump.pcap

