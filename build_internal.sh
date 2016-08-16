#!/bin/sh
set -e
echo if this does not run, try doing "source build.sh" instead of ./build.sh


echo Configuring EDK2....

export PATH=~/bin/bin:$PWD/userland/sdk/usr/bin:$PATH
export WORKSPACE=`pwd`/../edk2
export PACKAGES_PATH=$WORKSPACE:$PWD
pushd $PWD
cd $WORKSPACE
. edksetup.sh
popd

echo Configuring and compiling zoidberg kernel

./genversion.sh
pushd logo
cd bin2c
gcc -o bin2c bin2c.c
cd ..
bin2c/bin2c -o ../kernel/zoidberg_logo.h Logo.bmp
popd

pushd kernel
./gen_syscalls.sh
popd

build -a X64 -p kernel.dsc

cp -Rv $WORKSPACE/build/* build/

echo Building userland

cp kernel/syscalls.inc userland/newlib/newlib/libc/sys/zoidberg
cp kernel/u_syscalls.asm userland/
pushd userland
make  -j all
popd

echo Building bootable image

dd if=/dev/zero of=boot.img bs=1M count=128
/sbin/mkfs.vfat boot.img -F 32
mmd -i boot.img ::/EFI
mmd -i boot.img ::/EFI/BOOT
mcopy -i boot.img logo/Logo.bmp ::/EFI/BOOT
mcopy -i boot.img unifont.psf   ::/EFI/BOOT
mcopy -i boot.img build/zoidberg/DEBUG_GCC46/X64/kernel.efi ::/EFI/BOOT
mcopy -i boot.img $WORKSPACE/Build/OvmfX64/DEBUG_GCC46/X64/Shell.efi ::/EFI/BOOT/BOOTX64.EFI
mcopy -i boot.img build/zoidberg/DEBUG_GCC46/X64/SimpleThread.efi ::/EFI/BOOT
mcopy -i boot.img startup.nsh ::/

echo Building initrd
dd if=/dev/zero of=initrd.img bs=1M count=1
/sbin/mkfs.vfat initrd.img 
mmd -i initrd.img ::/dev
mmd -i initrd.img ::/boot
mmd -i initrd.img ::/sbin
mmd -i initrd.img ::/bin
mcopy -i initrd.img userland/build/sbin/init ::/sbin
mcopy -i initrd.img userland/build/bin/sh ::/bin

echo Shrinking+rebuilding initrd.img
BYTESTOTAL=`du -b initrd.img | awk {'print $1'}`
BYTESFREE=`mdir -i initrd.img | grep free | sed 's/bytes free//' | sed 's/ //g'`
BYTESFINAL=$((($BYTESTOTAL-$BYTESFREE)+65536  ))

BYTESFINAL=$(($BYTESFINAL - ($BYTESFINAL % 65536)))

cp initrd.img initrd.img.bak
rm -f initrd.img
dd if=/dev/zero of=initrd.img bs=65536 count=$(( ($BYTESFINAL / 65536) ))
/sbin/mkfs.vfat initrd.img
mkdir -p initrd/
mcopy -i initrd.img.bak -spn ::/* initrd/
mcopy -i initrd.img -sp initrd/* ::/

echo Copying initrd.img to boot partition
mcopy -i boot.img initrd.img ::/EFI/BOOT
