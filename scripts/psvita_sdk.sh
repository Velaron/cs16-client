#!/bin/bash

cd $GITHUB_WORKSPACE

git clone https://github.com/vitasdk/vdpm.git --depth=1 || exit 1
pushd vdpm
./bootstrap-vitasdk.sh || exit 1
./vdpm taihen || exit 1
./vdpm taihen || exit 1
./vdpm kubridge || exit 1
./vdpm zlib || exit 1
./vdpm SceShaccCgExt || exit 1
./vdpm vitaShaRK || exit 1
./vdpm libmathneon || exit 1
popd

git clone https://github.com/fgsfdsfgs/vita-rtld.git --depth=1 || exit 1