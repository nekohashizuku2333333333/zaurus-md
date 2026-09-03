#!/bin/sh
set -eu

QTDIR=/home/QtPalmtop
SETTINGSDIR=/home/zaurus/Settings
BASE_MIME="$QTDIR/etc/mime.types.qtopia17"
BASE_SLMIME="$QTDIR/etc/slmime.types.sharp"

restore_mime_file()
{
    target="$1"
    dir=`dirname "$target"`
    mkdir -p "$dir"

    if [ -f "$target" ] && [ ! -f "$target.zaurusmd.bak" ]; then
        cp "$target" "$target.zaurusmd.bak"
    fi

    if [ -f "$BASE_MIME" ]; then
        cp "$BASE_MIME" "$target"
    else
        touch "$target"
    fi

    if ! grep -q '^text/x-markdown[ 	].*md' "$target"; then
        echo 'text/x-markdown	md markdown mkd' >> "$target"
    fi
    if ! grep -q '^text/markdown[ 	].*md' "$target"; then
        echo 'text/markdown	md markdown mkd' >> "$target"
    fi
}

mkdir -p "$QTDIR/apps/Settings" "$QTDIR/apps/Applications" "$QTDIR/apps/Document" "$QTDIR/etc" "$SETTINGSDIR"

if [ -f "$QTDIR/apps/Settings/qinstall.desktop" ] && [ ! -f "$QTDIR/apps/Settings/qinstall.desktop.zaurusmd.bak" ]; then
    cp "$QTDIR/apps/Settings/qinstall.desktop" "$QTDIR/apps/Settings/qinstall.desktop.zaurusmd.bak"
fi

cat > "$QTDIR/apps/Settings/qinstall.desktop" <<EOF
[Desktop Entry]
CanFastload = 0
Display = 640x480/144dpi,480x640/144dpi
Exec = qinstall
HidePrivilege = 1
Icon = qinstall_icn.png
MimeType = application/ipkg
Name = Add/Remove Software
Name[de] = Software
Name[ja] = ソフトウェアの追加/削除
Name[zh_CN] = 添加/删除
Type = Application
Type[de] = Anwendung
EOF

if [ -f "$QTDIR/apps/Settings/qipkg.desktop" ]; then
    if grep -q '^Exec=qipkg' "$QTDIR/apps/Settings/qipkg.desktop" || grep -q '^Exec = qipkg' "$QTDIR/apps/Settings/qipkg.desktop"; then
        mv "$QTDIR/apps/Settings/qipkg.desktop" "$QTDIR/apps/Settings/qipkg.desktop.zaurusmd.disabled"
    fi
fi

restore_mime_file "$QTDIR/etc/mime.types"
restore_mime_file "$SETTINGSDIR/mime.types"

if [ -f "$BASE_SLMIME" ]; then
    if [ -f "$QTDIR/etc/slmime.types" ] && [ ! -f "$QTDIR/etc/slmime.types.zaurusmd.bak" ]; then
        cp "$QTDIR/etc/slmime.types" "$QTDIR/etc/slmime.types.zaurusmd.bak"
    fi
    cp "$BASE_SLMIME" "$QTDIR/etc/slmime.types"
fi

for desktop in "$QTDIR/apps/Applications/zaurusmd.desktop" "$QTDIR/apps/Document/zaurusmd.desktop"; do
    if [ -f "$desktop" ]; then
        sed 's/;application\/octet-stream//g;s/application\/octet-stream;//g;s/application\/octet-stream//g' "$desktop" > "$desktop.tmp"
        mv "$desktop.tmp" "$desktop"
    fi
done

rm -f "$QTDIR/etc/.mimetypes.cache" "$SETTINGSDIR/.mimetypes.cache"

echo "Restored Qtopia MIME associations, Sharp MIME categories, and qinstall IPK handling."
