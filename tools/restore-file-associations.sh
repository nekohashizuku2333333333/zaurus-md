#!/bin/sh
set -eu

QTDIR=/home/QtPalmtop
SETTINGSDIR=/home/zaurus/Settings
BASE_MIME="$QTDIR/etc/mime.types.qtopia17"

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

mkdir -p "$QTDIR/apps/Settings" "$QTDIR/etc" "$SETTINGSDIR"

if [ -f "$QTDIR/apps/Settings/qipkg.desktop" ] && [ ! -f "$QTDIR/apps/Settings/qipkg.desktop.zaurusmd.bak" ]; then
    cp "$QTDIR/apps/Settings/qipkg.desktop" "$QTDIR/apps/Settings/qipkg.desktop.zaurusmd.bak"
fi

cat > "$QTDIR/apps/Settings/qipkg.desktop" <<EOF
[Desktop Entry]
Type=Application
Exec=qipkg
MimeType=application/ipkg
Icon=Ipkg
Name=Software Packages
Name[ja]=ソフトウェアパッケージ
Name[hu]=Csomag- kezelõ
Name[de]=Software
CanFastload=0
EOF

restore_mime_file "$QTDIR/etc/mime.types"
restore_mime_file "$SETTINGSDIR/mime.types"

rm -f "$QTDIR/etc/.mimetypes.cache" "$SETTINGSDIR/.mimetypes.cache"

echo "Restored full Qtopia MIME associations, then added Markdown MIME types."
