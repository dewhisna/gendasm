#!/bin/sh

DIR=$(dirname "$0")

#if [ -d "${DIR}/build-asxv5p40" ]; then
#	rm -r "${DIR}/build-asxv5p40"
#fi

mkdir -p "${DIR}/build-asxv5p40"
cd "${DIR}/build-asxv5p40"

curl -C - https://shop-pdp.net/_ftp/asxxxx/asxs5p40.zip -o asxs5p40.zip
curl -C - https://shop-pdp.net/_ftp/asxxxx/u01540.zip -o u01540.zip

unzip -L -a -o asxs5p40.zip
unzip -L -a -o u01540.zip

patch -p1 < ../asxv5p40_6811_no_opt.patch

cd asxv5pxx/asxmak/linux/build/
make all -j 8
cp as6811 "../../../../"
cp as8051 "../../../../"
cp aslink "../../../../"
cp asavr "../../../../"
make clean

cd "../../../../"
rm -r "asxv5pxx/"
rm asxs5p40.zip
rm u01540.zip
