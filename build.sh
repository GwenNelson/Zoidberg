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
