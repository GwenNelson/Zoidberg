#!/bin/sh
echo if this does not run, try doing "source build.sh" instead of ./build.sh


echo Configuring EDK2....

export PATH=~/bin/bin:$PWD/sdk:$PATH
export WORKSPACE=`pwd`/../edk2
export PACKAGES_PATH=$WORKSPACE:$PWD
pushd $PWD
cd $WORKSPACE
. edksetup.sh
popd

echo Configuring and compiling pawn

mkdir -p pawn/source/compiler/build
pushd pawn/source/compiler/build
cmake ..
make
popd

echo Configuring and compiling zoidberg kernel

./genversion.sh
build -a X64 -p kernel.dsc

cp -Rv $WORKSPACE/build build
cp build/zoidberg/DEBUG_GCC46/X64/kernel.efi BOOTX64.EFI

echo Building userland

make -C userland

echo Building bootable image

dd if=/dev/zero of=boot.img bs=1M count=33
/sbin/mkfs.vfat boot.img -F 32
mmd -i boot.img ::/EFI
mmd -i boot.img ::/EFI/BOOT
mmd -i boot.img ::/EFI/BOOT/sbin
mcopy -i boot.img BOOTX64.EFI ::/EFI/BOOT
mcopy -i boot.img userland/build/init ::/EFI/BOOT/sbin

