#!/bin/sh
echo if this does not run, try doing "source build.sh" instead of ./build.sh

export PATH=~/bin/bin:$PWD/sdk:$PATH
export WORKSPACE=`pwd`/../edk2
export PACKAGES_PATH=$WORKSPACE:$PWD
pushd $PWD
cd $WORKSPACE
. edksetup.sh
popd

mkdir -p pawn/source/compiler/build
pushd pawn/source/compiler/build
cmake ..
make
popd
