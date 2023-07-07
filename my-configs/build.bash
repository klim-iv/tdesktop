#!/bin/bash

set -e

c_d=$(realpath $(dirname ${0}))
cd ${c_d}

TLG_VERSION=$(cat ../Telegram/build/version | grep 'AppVersionStr ' | awk '{print $2}')

API_ID="00000"
API_HASH="00000000000000000000000000000000"

if [ ! -e .unpacked ]; then
  openssl des3 -d -md sha256 -in ./secure-data.tar.des -out secure-data.tar
  tar -xvf secure-data.tar
  touch .unpacked
fi

if [ -e ./env.bash ]; then
  source ./env.bash
fi

if [ -e encode-keys.patch -a ! -e .applied ]; then
  patch -d ../Telegram/lib_ui -p1 -i ${c_d}/encode-keys.patch
  touch .applied
fi

cd ..

set -x
docker run --rm -it \
   -v $PWD:/usr/src/tdesktop \
   tdesktop:${TLG_VERSION} \
   /usr/src/tdesktop/Telegram/build/docker/centos_env/build.sh \
   -D CMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
   -D TDESKTOP_API_ID="${API_ID}" \
   -D TDESKTOP_API_HASH="${API_HASH}"
