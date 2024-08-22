#!/bin/bash

set -e
FullExecPath=$PWD
pushd `dirname $0` > /dev/null
FullScriptPath=`pwd`
popd > /dev/null


TLG_VERSION=$(cat $FullScriptPath/../../../Telegram/build/version | grep 'AppVersionStr ' | awk '{print $2}')

cd $FullScriptPath/../docker/centos_env
poetry install
poetry run gen_dockerfile | tee $FullScriptPath/../../../my-configs/Dockerfile | DOCKER_BUILDKIT=0 docker build -t tdesktop:${TLG_VERSION} -
cd $FullExecPath
