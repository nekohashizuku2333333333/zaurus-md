#!/bin/sh
set -eu

export PATH=/opt/cross/arm/2.95.3-2.15/bin:/opt/cross/arm/3.4.6-xscale-softvfp-akita/bin:$PATH
export QTDIR=/opt/Qtopia/qt-2.3.2
export QPESDK=/opt/murphytalk-sdk/qtopia-free-1.7.0

CXX=arm-cacko-linux-gnu-g++
MOC="$QTDIR/src/moc/moc"
CXXFLAGS="-pipe -DQT_QWS_SL5XXX -DQT_QWS_CUSTOM -DQWS -DQT_NO_PROPERTIES -DQT_NO_DRAGANDDROP -fno-exceptions -fno-rtti -Wall -W -O2 -DNO_DEBUG"
INCPATH="-Isrc -Ivendor/md4c -I$QPESDK/include -I$QTDIR/include"
LIBS="-Lcompat-lib -L$QTDIR/lib -lqpe -lqte -ljpeg"
LFLAGS=""

rm -rf DIST
mkdir -p DIST
rm -f src/*.o *.o moc_MainWindow.cpp moc_MdEdit.cpp moc_MdView.cpp

"$MOC" src/MainWindow.h -o moc_MainWindow.cpp
"$MOC" src/MdEdit.h -o moc_MdEdit.cpp
"$MOC" src/MdView.h -o moc_MdView.cpp

for f in \
  src/main.cpp \
  src/MainWindow.cpp \
  src/MdEdit.cpp \
  src/MdView.cpp \
  src/TextPrompt.cpp \
  src/MdParser.cpp \
  src/TodoTxt.cpp \
  src/FileUtil.cpp \
  moc_MainWindow.cpp \
  moc_MdEdit.cpp \
  moc_MdView.cpp
do
  o="${f%.cpp}.o"
  "$CXX" -c $CXXFLAGS $INCPATH -o "$o" "$f"
done

for f in src/MdRichText.c vendor/md4c/md4c.c vendor/md4c/entity.c; do
    arm-cacko-linux-gnu-gcc -O2 -Wall -Isrc -Ivendor/md4c -c "$f" -o "${f%.c}.o"
done

"$CXX" $LFLAGS -o DIST/zaurusmd \
  src/main.o src/MainWindow.o src/MdEdit.o src/MdView.o src/TextPrompt.o src/MdParser.o \
  src/TodoTxt.o src/FileUtil.o moc_MainWindow.o moc_MdEdit.o moc_MdView.o \
  src/MdRichText.o vendor/md4c/md4c.o vendor/md4c/entity.o \
  $LIBS

if [ "${BUILD_PACKAGE:-1}" = 1 ]; then
    ./scripts/build-ipk.sh
fi
