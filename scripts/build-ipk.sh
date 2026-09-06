#!/bin/sh
set -eu

APP=zaurusmd
VER=0.3
IPKDIR=IPK

sh scripts/build-repair.sh
rm -rf "$IPKDIR"
mkdir -p "$IPKDIR/home/QtPalmtop/bin"
mkdir -p "$IPKDIR/home/QtPalmtop/apps/Applications"
mkdir -p "$IPKDIR/home/QtPalmtop/apps/Document"
mkdir -p "$IPKDIR/home/QtPalmtop/share/zaurusmd"
mkdir -p "$IPKDIR/CONTROL"

cp "DIST/$APP" "$IPKDIR/home/QtPalmtop/bin/$APP"
cp vendor/md4c/LICENSE.md "$IPKDIR/home/QtPalmtop/share/zaurusmd/MD4C-LICENSE.txt"
chmod 755 "$IPKDIR/home/QtPalmtop/bin/$APP"
cp DIST/restore-ipk-association.sh "$IPKDIR/home/QtPalmtop/bin/restore-file-associations"
chmod 755 "$IPKDIR/home/QtPalmtop/bin/restore-file-associations"

cat > "$IPKDIR/home/QtPalmtop/apps/Applications/$APP.desktop" <<EOF
[Desktop Entry]
Comment=Markdown writer and reader
Exec=$APP
Icon=TextEditor
Type=Application
Name=Zaurus MDEditor
MimeType=text/x-markdown;text/markdown
CanFastload=0
EOF

cat > "$IPKDIR/home/QtPalmtop/apps/Document/$APP.desktop" <<EOF
[Desktop Entry]
Comment=Open Markdown and text files
Exec=$APP
Icon=TextEditor
Type=Application
Name=Zaurus MDEditor
MimeType=text/x-markdown;text/markdown
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

cat > "$IPKDIR/CONTROL/postinst" <<'EOF'
#!/bin/sh
set -eu
payload_root=${ZAURUSMD_ROOT:-${PKG_ROOT:-}}
sh "$payload_root/home/QtPalmtop/bin/restore-file-associations" install
EOF

cat > "$IPKDIR/CONTROL/prerm" <<'EOF'
#!/bin/sh
set -eu
payload_root=${ZAURUSMD_ROOT:-${PKG_ROOT:-}}
if [ -f "$payload_root/home/QtPalmtop/bin/restore-file-associations" ]; then
    sh "$payload_root/home/QtPalmtop/bin/restore-file-associations" remove
fi
EOF
cat > "$IPKDIR/CONTROL/postrm" <<'EOF'
#!/bin/sh
root=${ZAURUSMD_ROOT:-}
if [ -z "$root" ] && [ -x /home/QtPalmtop/bin/qcop ]; then
    /home/QtPalmtop/bin/qcop QPE/System 'linkChanged(QString)' '' || true
fi
exit 0
EOF
chmod 755 "$IPKDIR/CONTROL/postinst" "$IPKDIR/CONTROL/prerm" "$IPKDIR/CONTROL/postrm"

(
	cd "$IPKDIR"
	tar --format=gnu --owner=root --group=root --mtime='2026-09-04 00:00:00' -czf ../control.tar.gz -C CONTROL ./control ./postinst ./prerm ./postrm
	tar --format=gnu --owner=root --group=root --mtime='2026-09-04 00:00:00' -czf ../data.tar.gz ./home
)
printf "2.0\n" > debian-binary
mkdir -p DIST
tar --format=gnu --owner=root --group=root --mtime='2026-09-04 00:00:00' -czf "DIST/${APP}_${VER}_arm.ipk" ./debian-binary ./control.tar.gz ./data.tar.gz
rm -f debian-binary control.tar.gz data.tar.gz
