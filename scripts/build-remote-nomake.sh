#!/bin/sh
set -eu

export PATH=/opt/cross/arm/2.95.3-2.15/bin:/opt/cross/arm/3.4.6-xscale-softvfp-akita/bin:$PATH
export QTDIR=/opt/Qtopia/qt-2.3.2
export QPESDK=/opt/murphytalk-sdk/qtopia-free-1.7.0

CXX=arm-cacko-linux-gnu-g++
MOC="$QTDIR/src/moc/moc"
CXXFLAGS="-pipe -DQT_QWS_SL5XXX -DQT_QWS_CUSTOM -DQWS -DQT_NO_PROPERTIES -DQT_NO_DRAGANDDROP -fno-exceptions -fno-rtti -Wall -W -O2 -DNO_DEBUG"
INCPATH="-Isrc -I$QPESDK/include -I$QTDIR/include"
LIBS="-Lcompat-lib -L$QTDIR/lib -lqpe -lqte -ljpeg"
LFLAGS=""

rm -rf DIST
mkdir -p DIST
rm -f src/*.o *.o moc_MainWindow.cpp moc_MdView.cpp

"$MOC" src/MainWindow.h -o moc_MainWindow.cpp
"$MOC" src/MdView.h -o moc_MdView.cpp

for f in \
  src/main.cpp \
  src/MainWindow.cpp \
  src/MdView.cpp \
  src/MdParser.cpp \
  src/TodoTxt.cpp \
  src/FileUtil.cpp \
  moc_MainWindow.cpp \
  moc_MdView.cpp
do
  o="${f%.cpp}.o"
  "$CXX" -c $CXXFLAGS $INCPATH -o "$o" "$f"
done

"$CXX" $LFLAGS -o DIST/zaurusmd \
  src/main.o src/MainWindow.o src/MdView.o src/MdParser.o \
  src/TodoTxt.o src/FileUtil.o moc_MainWindow.o moc_MdView.o \
  $LIBS

./scripts/build-ipk.sh
