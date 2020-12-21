#!/bin/bash
set -euxo pipefail

export IMAGE_NAME="tmbasic-win-x64"
export HOST_UID=$(id -u "$USER")
export HOST_GID=$(id -g "$USER")
export ARCH="x86_64"

cat files/Dockerfile.build-win | envsubst | docker build -t $IMAGE_NAME files -f-

pushd ..
docker run --rm ${TTY_FLAG:=--tty --interactive} --volume "$PWD:/code" --workdir /code --name $IMAGE_NAME ${DOCKER_FLAGS:= } $IMAGE_NAME "$@"
popd
