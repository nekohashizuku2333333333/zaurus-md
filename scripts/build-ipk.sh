#!/bin/sh
set -eu

APP=zaurusmd
VER=0.1
IPKDIR=IPK

rm -rf "$IPKDIR"
mkdir -p "$IPKDIR/home/QtPalmtop/bin"
mkdir -p "$IPKDIR/home/QtPalmtop/apps/Applications"
mkdir -p "$IPKDIR/home/QtPalmtop/apps/Document"
mkdir -p "$IPKDIR/CONTROL"

cp "DIST/$APP" "$IPKDIR/home/QtPalmtop/bin/$APP"
chmod 755 "$IPKDIR/home/QtPalmtop/bin/$APP"

cat > "$IPKDIR/home/QtPalmtop/apps/Applications/$APP.desktop" <<EOF
[Desktop Entry]
Comment=Markdown writer and reader
Exec=$APP %f
Icon=TextEditor
Type=Application
Name=Zaurus MDEditor
MimeType=text/plain;text/x-markdown;text/markdown
CanFastload=0
EOF

cat > "$IPKDIR/home/QtPalmtop/apps/Document/$APP.desktop" <<EOF
[Desktop Entry]
Comment=Open Markdown and text files
Exec=$APP %f
Icon=TextEditor
Type=Application
Name=Zaurus MDEditor
MimeType=text/plain;text/x-markdown;text/markdown
CanFastload=0
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
	tar czf ../data.tar.gz ./home
)
printf "2.0\n" > debian-binary
mkdir -p DIST
tar czf "DIST/${APP}_${VER}_arm.ipk" ./debian-binary ./control.tar.gz ./data.tar.gz
rm -f debian-binary control.tar.gz data.tar.gz
