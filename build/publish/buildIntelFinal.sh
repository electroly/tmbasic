set -euxo pipefail
export PUBLISHDIR=/tmp/tmbasic-publish

cd $PUBLISHDIR/tmbasic/build

export DOCKER_FLAGS="--entrypoint /bin/bash"
export TTY_FLAG=" "

export CLEAN_CMD="make clean && mkdir obj"
export BUILD_CMD="make release"

./win-x64.sh -ic "$CLEAN_CMD"
pushd $PUBLISHDIR/tmbasic/obj/resources/runners && tar xf $PUBLISHDIR/runners.tar && popd
./win-x64.sh -ic "$BUILD_CMD"
pushd $PUBLISHDIR/tmbasic/bin && zip -9 $PUBLISHDIR/tmbasic-win-x64.zip tmbasic.exe LICENSE.txt && popd

./win-x86.sh -ic "$CLEAN_CMD"
pushd $PUBLISHDIR/tmbasic/obj/resources/runners && tar xf $PUBLISHDIR/runners.tar && popd
./win-x86.sh -ic "$BUILD_CMD"
pushd $PUBLISHDIR/tmbasic/bin && zip -9 $PUBLISHDIR/tmbasic-win-x86.zip tmbasic.exe LICENSE.txt && popd

./linux-x64.sh -ic "$CLEAN_CMD"
pushd $PUBLISHDIR/tmbasic/obj/resources/runners && tar xf $PUBLISHDIR/runners.tar && popd
./linux-x64.sh -ic "$BUILD_CMD"
pushd $PUBLISHDIR/tmbasic/bin && tar zcf $PUBLISHDIR/tmbasic-linux-x64.tar.gz tmbasic LICENSE.txt && popd

./linux-x86.sh -ic "$CLEAN_CMD"
pushd $PUBLISHDIR/tmbasic/obj/resources/runners && tar xf $PUBLISHDIR/runners.tar && popd
./linux-x86.sh -ic "$BUILD_CMD"
pushd $PUBLISHDIR/tmbasic/bin && tar zcf $PUBLISHDIR/tmbasic-linux-x86.tar.gz tmbasic LICENSE.txt && popd
