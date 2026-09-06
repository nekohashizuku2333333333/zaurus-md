#!/bin/sh
# Run on the SDK host. All package state and linking helpers are isolated.
set -eu
new_package=$1
old_package=$2
repair_script=$3
work="$PWD/test-runtime/legacy-ipkg-$$"
root="$work/root"
mkdir -p "$work/config" "$work/bin" "$root/home/QtPalmtop/bin" \
    "$root/home/QtPalmtop/etc" "$root/home/zaurus/Settings" \
    "$root/home/zaurus/Documents/Notes"
sed "s|IPKG_CONF_DIR=/etc|IPKG_CONF_DIR=$work/config|g;s|failure_file=/tmp/ipkg.wrongfiles|failure_file=$work/ipkg.wrongfiles|g;s|IPKG_LISTS_DIR=.*|IPKG_LISTS_DIR=$root/usr/lib/ipkg/lists|;s|IPKG_PENDING_DIR=.*|IPKG_PENDING_DIR=$root/usr/lib/ipkg/pending|" \
    /usr/bin/ipkg > "$work/ipkg"
chmod 755 "$work/ipkg"
printf 'dest root %s\n' "$root" > "$work/config/ipkg.conf"
printf '#!/bin/sh\nexit 0\n' > "$work/bin/ipkg-link"
chmod 755 "$work/bin/ipkg-link"
export PATH="$work/bin:$PATH"
export ZAURUSMD_ROOT="$root"

printf '#!/bin/sh\nexit 0\n' > "$root/home/QtPalmtop/bin/qinstall"
chmod 755 "$root/home/QtPalmtop/bin/qinstall"
printf 'application/ipkg ipk\ntext/plain txt\napplication/x-user custom\n' > "$root/home/QtPalmtop/etc/mime.types"
cp "$root/home/QtPalmtop/etc/mime.types" "$root/home/zaurus/Settings/mime.types"
printf 'keep note\n' > "$root/home/zaurus/Documents/Notes/keep.md"

verify_system()
{
    test -f "$root/home/QtPalmtop/apps/Settings/qinstall.desktop"
    grep -q '^Exec = qinstall$' "$root/home/QtPalmtop/apps/Settings/qinstall.desktop"
    grep -q '^MimeType = application/ipkg$' "$root/home/QtPalmtop/apps/Settings/qinstall.desktop"
    for mime in "$root/home/QtPalmtop/etc/mime.types" "$root/home/zaurus/Settings/mime.types"; do
        grep -q '^application/ipkg[[:space:]]*ipk$' "$mime"
        grep -q '^text/plain[[:space:]]*txt$' "$mime"
        grep -q '^application/x-user custom$' "$mime"
    done
    test "$(cat "$root/home/zaurus/Documents/Notes/keep.md")" = 'keep note'
}

# Do not run historical absolute-path maintenance scripts on any live system.
IGNORE_SCRIPTS=t "$work/ipkg" -force-depends install "$old_package"
grep -q '/apps/Settings/qinstall.desktop' "$root/usr/lib/ipkg/info/zaurusmd.list"
IGNORE_SCRIPTS=t "$work/ipkg" remove zaurusmd
test ! -e "$root/home/QtPalmtop/apps/Settings/qinstall.desktop"
echo 'PASS reproduced old package deleting qinstall on removal'

sh "$repair_script"
verify_system
echo 'PASS standalone script repairs an already-uninstalled system'

"$work/ipkg" install "$new_package"
if grep -q '/apps/Settings/\|/etc/mime.types\|/Settings/mime.types' "$root/usr/lib/ipkg/info/zaurusmd.list"; then
    echo 'FAIL package still owns system association files' >&2
    exit 1
fi
verify_system
"$work/ipkg" remove zaurusmd
verify_system
test ! -e "$root/home/QtPalmtop/bin/zaurusmd"
echo 'PASS new package install/remove retains system and TXT associations'

"$work/ipkg" install "$new_package"
verify_system
"$work/ipkg" remove zaurusmd
verify_system
echo 'PASS reinstall/remove retains system associations'

IGNORE_SCRIPTS=t "$work/ipkg" -force-depends install "$old_package"
"$work/ipkg" install "$new_package"
verify_system
"$work/ipkg" remove zaurusmd
verify_system
echo 'PASS direct upgrade from old package then removal retains associations'
