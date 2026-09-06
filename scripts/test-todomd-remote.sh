#!/bin/sh
set -eu
QTSDK=/opt/Qtopia/qt-2.3.2
ARMSDK=/opt/cross/arm/2.95.3-2.15
JPEGLIB=/opt/cross/arm/3.4.6-xscale-softvfp-akita/armv5tel-cacko-linux/lib
mkdir -p test-runtime
ln -sf ../compat-lib/libqte.so test-runtime/libqte.so.2
"$ARMSDK/bin/arm-cacko-linux-gnu-g++" -shared -nostdlib \
    -Wl,-u,__udivsi3,-u,__umodsi3,-u,__divsi3,-u,__modsi3 -lgcc \
    -o test-runtime/libarmtest.so
"$ARMSDK/bin/arm-cacko-linux-gnu-g++" \
    -DQT_QWS_SL5XXX -DQT_QWS_CUSTOM -DQWS -DQT_NO_PROPERTIES -DQT_NO_DRAGANDDROP \
    -fno-exceptions -fno-rtti -Isrc -I"$QTSDK/include" \
    tests/todomd-test.cpp src/TodoMd.cpp \
    -Ltest-runtime -larmtest -Lcompat-lib -lqte -ljpeg \
    -o test-runtime/todomd-test
qemu-arm -L "$ARMSDK/arm-cacko-linux-gnu" \
    "$ARMSDK/arm-cacko-linux-gnu/lib/ld-linux.so.2" \
    --library-path "$PWD/test-runtime:$ARMSDK/arm-cacko-linux-gnu/lib:$JPEGLIB" \
    "$PWD/test-runtime/todomd-test"
