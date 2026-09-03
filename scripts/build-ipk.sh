#!/bin/sh
set -eu

APP=zaurusmd
VER=0.1
IPKDIR=IPK

rm -rf "$IPKDIR"
mkdir -p "$IPKDIR/opt/QtPalmtop/bin"
mkdir -p "$IPKDIR/opt/QtPalmtop/apps/Applications"
mkdir -p "$IPKDIR/CONTROL"

cp "DIST/$APP" "$IPKDIR/opt/QtPalmtop/bin/$APP"
chmod 755 "$IPKDIR/opt/QtPalmtop/bin/$APP"

cat > "$IPKDIR/opt/QtPalmtop/apps/Applications/$APP.desktop" <<EOF
[Desktop Entry]
Comment=Markdown writer and reader
Exec=$APP
Icon=TextEditor
Type=Application
Name=MD Writer
EOF

cat > "$IPKDIR/CONTROL/control" <<EOF
Package: zaurusmd
Priority: optional
Section: Applications
Version: $VER
Architecture: arm
Maintainer: local
Depends: libc6
Description: Markdown writer and reader for Qtopia
EOF

(
	cd "$IPKDIR"
	tar czf ../control.tar.gz -C CONTROL ./control
	tar czf ../data.tar.gz ./opt
)
printf "2.0\n" > debian-binary
mkdir -p DIST
tar czf "DIST/${APP}_${VER}_arm.ipk" ./debian-binary ./control.tar.gz ./data.tar.gz
rm -f debian-binary control.tar.gz data.tar.gz

