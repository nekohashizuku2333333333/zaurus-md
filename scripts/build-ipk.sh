#!/bin/sh
set -eu

APP=zaurusmd
VER=0.1
IPKDIR=IPK

rm -rf "$IPKDIR"
mkdir -p "$IPKDIR/home/QtPalmtop/bin"
mkdir -p "$IPKDIR/home/QtPalmtop/apps/Applications"
mkdir -p "$IPKDIR/home/QtPalmtop/apps/Document"
mkdir -p "$IPKDIR/home/QtPalmtop/apps/Settings"
mkdir -p "$IPKDIR/home/QtPalmtop/etc"
mkdir -p "$IPKDIR/CONTROL"

cp "DIST/$APP" "$IPKDIR/home/QtPalmtop/bin/$APP"
chmod 755 "$IPKDIR/home/QtPalmtop/bin/$APP"
cp tools/restore-file-associations.sh "$IPKDIR/home/QtPalmtop/bin/restore-file-associations"
chmod 755 "$IPKDIR/home/QtPalmtop/bin/restore-file-associations"
cp packaging/mime.types.qtopia17 "$IPKDIR/home/QtPalmtop/etc/mime.types.qtopia17"
cp packaging/slmime.types.sharp "$IPKDIR/home/QtPalmtop/etc/slmime.types.sharp"
chmod 644 "$IPKDIR/home/QtPalmtop/etc/mime.types.qtopia17" "$IPKDIR/home/QtPalmtop/etc/slmime.types.sharp"

cat > "$IPKDIR/home/QtPalmtop/apps/Applications/$APP.desktop" <<EOF
[Desktop Entry]
Comment=Markdown writer and reader
Exec=$APP %f
Icon=TextEditor
Type=Application
Name=Zaurus MDEditor
MimeType=text/x-markdown;text/markdown
CanFastload=0
EOF

cat > "$IPKDIR/home/QtPalmtop/apps/Document/$APP.desktop" <<EOF
[Desktop Entry]
Comment=Open Markdown and text files
Exec=$APP %f
Icon=TextEditor
Type=Application
Name=Zaurus MDEditor
MimeType=text/x-markdown;text/markdown
CanFastload=0
EOF

cp packaging/qinstall.desktop "$IPKDIR/home/QtPalmtop/apps/Settings/qinstall.desktop"
chmod 644 "$IPKDIR/home/QtPalmtop/apps/Settings/qinstall.desktop"

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

cat > "$IPKDIR/CONTROL/postinst" <<EOF
#!/bin/sh
if [ -x /home/QtPalmtop/bin/restore-file-associations ]; then
    /home/QtPalmtop/bin/restore-file-associations >/dev/null 2>&1 || true
fi
exit 0
EOF
chmod 755 "$IPKDIR/CONTROL/postinst"

(
	cd "$IPKDIR"
	tar --format=gnu --owner=root --group=root --mtime='2026-09-04 00:00:00' -czf ../control.tar.gz -C CONTROL ./control ./postinst
	tar --format=gnu --owner=root --group=root --mtime='2026-09-04 00:00:00' -czf ../data.tar.gz ./home
)
printf "2.0\n" > debian-binary
mkdir -p DIST
tar --format=gnu --owner=root --group=root --mtime='2026-09-04 00:00:00' -czf "DIST/${APP}_${VER}_arm.ipk" ./debian-binary ./control.tar.gz ./data.tar.gz
rm -f debian-binary control.tar.gz data.tar.gz
