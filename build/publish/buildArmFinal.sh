set -euxo pipefail
export PUBLISHDIR=/tmp/tmbasic-publish

cd $PUBLISHDIR/tmbasic/build

export DOCKER_FLAGS="--entrypoint /bin/bash"
export TTY_FLAG=" "

export CLEAN_CMD="make clean && mkdir obj"
export BUILD_CMD="make release"

./linux-arm64.sh -ic "$CLEAN_CMD"
pushd $PUBLISHDIR/tmbasic/obj/resources/runners && tar xf $PUBLISHDIR/runners.tar && popd
./linux-arm64.sh -ic "$BUILD_CMD"
pushd $PUBLISHDIR/tmbasic/bin && tar zcf $PUBLISHDIR/tmbasic-linux-arm64.tar.gz tmbasic LICENSE.txt && popd

./linux-arm32.sh -ic "$CLEAN_CMD"
pushd $PUBLISHDIR/tmbasic/obj/resources/runners && tar xf $PUBLISHDIR/runners.tar && popd
./linux-arm32.sh -ic "$BUILD_CMD"
pushd $PUBLISHDIR/tmbasic/bin && tar zcf $PUBLISHDIR/tmbasic-linux-arm32.tar.gz tmbasic LICENSE.txt && popd
