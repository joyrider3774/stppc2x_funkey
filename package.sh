#!/bin/sh

make clean
make TARGET=funkey

mkdir -p opk
cp stppc2x opk/stppc2x
cp -r ./binaries-funkey/. opk/

mksquashfs ./opk Stppc2x.opk -all-root -noappend -no-exports -no-xattrs

rm -r opk