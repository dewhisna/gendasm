#!/bin/sh

DIR=$(dirname "$0")

mkdir -p "${DIR}/build-avra"
cd "${DIR}/build-avra"

git clone 'git://github.com/Ro5bert/avra' avra-git
cd avra-git
git checkout de5272f53771e54112245b2aa72aeeb09a19147d

make all -j 8
mv src/avra ..
cd ..
rm -rf avra-git
