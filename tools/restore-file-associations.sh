#!/bin/sh
# build-repair.sh prefixes this implementation with the stock data functions.
set -eu
set -f

mode=${1:-repair}
case "$mode" in repair|install|remove) ;; *) echo "Usage: $0 [repair|install|remove]" >&2; exit 1 ;; esac
root=${ZAURUSMD_ROOT:-}
qtbase="$root/home/QtPalmtop"
settings="$root/home/zaurus/Settings"
if [ ! -d "$qtbase" ]; then
    echo "QtPalmtop directory not found: $qtbase" >&2
    exit 1
fi

backup()
{
    if [ -f "$1" ] && [ ! -e "$1.zaurusmd-before-ipk-fix" ]; then
        cp -p "$1" "$1.zaurusmd-before-ipk-fix"
    fi
}

commit_file()
{
    chmod 644 "$2"
    # Replace the writable overlay entry, never overwrite a ROM symlink target.
    mv "$2" "$1"
}

remove_markdown()
{
    [ -f "$1" ] || return 0
    if grep -q '^# BEGIN zaurusmd MIME$' "$1" && grep -q '^# END zaurusmd MIME$' "$1"; then
        sed '/^# BEGIN zaurusmd MIME$/,/^# END zaurusmd MIME$/d' "$1" > "$1.zaurusmd-tmp.$$"
        commit_file "$1" "$1.zaurusmd-tmp.$$"
    fi
}

repair_mime()
{
    target=$1
    backup "$target"
    if [ ! -s "$target" ]; then
        baseline_mime > "$target.zaurusmd-tmp.$$"
        commit_file "$target" "$target.zaurusmd-tmp.$$"
    fi
    # Remove only the exact ipk extension from competing MIME entries.
    # No awk dependency: original Sharp images do not necessarily ship awk.
    (
        while IFS= read -r line || [ -n "$line" ]; do
            set -- $line
            if [ "$#" -lt 2 ]; then printf '%s\n' "$line"; continue; fi
            case "$1" in \#*|*/*) ;; *) printf '%s\n' "$line"; continue ;; esac
            case "$1" in \#*) printf '%s\n' "$line"; continue ;; esac
            rebuilt=$1
            shift
            changed=0
            comment=0
            for extension do
                case "$extension" in \#*) comment=1 ;; esac
                if [ "$comment" = 0 ] && [ "$extension" = ipk ]; then
                    changed=1
                else
                    rebuilt="$rebuilt $extension"
                fi
            done
            if [ "$changed" = 1 ]; then
                # Omit a now-empty canonical entry; it is appended once below.
                if [ "$rebuilt" != application/ipkg ]; then printf '%s\n' "$rebuilt"; fi
            else printf '%s\n' "$line"; fi
        done < "$target"
        printf 'application/ipkg\tipk\n'
    ) > "$target.zaurusmd-tmp.$$"
    commit_file "$target" "$target.zaurusmd-tmp.$$"
}

add_markdown()
{
    target=$1
    remove_markdown "$target"
    # Respect mappings supplied by other applications, even under another type.
    extensions=
    for candidate in md markdown mkd; do
        found=0
        while IFS= read -r line || [ -n "$line" ]; do
            set -- $line
            [ "$#" -gt 1 ] || continue
            case "$1" in \#*) continue ;; esac
            shift
            for extension do
                case "$extension" in \#*) break ;; esac
                if [ "$extension" = "$candidate" ]; then found=1; break; fi
            done
            [ "$found" = 0 ] || break
        done < "$target"
        if [ "$found" = 0 ]; then extensions="$extensions $candidate"; fi
    done
    if [ -n "$extensions" ]; then
        cat "$target" > "$target.zaurusmd-tmp.$$"
        printf '# BEGIN zaurusmd MIME\ntext/markdown%s\n# END zaurusmd MIME\n' "$extensions" >> "$target.zaurusmd-tmp.$$"
        commit_file "$target" "$target.zaurusmd-tmp.$$"
    fi
}

repair_editor()
{
    # Qtopia passes documents separately through setDocument(QString), not %f.
    for category in Applications Document; do
        mkdir -p "$qtbase/apps/$category"
        desktop="$qtbase/apps/$category/zaurusmd.desktop"
        backup "$desktop"
        cat > "$desktop.zaurusmd-tmp.$$" <<'EOF'
[Desktop Entry]
Comment=Markdown writer and reader
Exec=zaurusmd
Icon=TextEditor
Type=Application
Name=Zaurus MDEditor
MimeType=text/markdown;text/x-markdown
CanFastload=0
EOF
        commit_file "$desktop" "$desktop.zaurusmd-tmp.$$"
    done
    add_markdown "$qtbase/etc/mime.types"
    add_markdown "$settings/mime.types"
    echo "Restored text/markdown and text/x-markdown -> zaurusmd."
}

if [ "$mode" = remove ]; then
    remove_markdown "$qtbase/etc/mime.types"
    remove_markdown "$settings/mime.types"
else
    mkdir -p "$qtbase/apps/Settings" "$qtbase/etc" "$settings"
    desktop="$qtbase/apps/Settings/qinstall.desktop"
    backup "$desktop"
    baseline_qinstall > "$desktop.zaurusmd-tmp.$$"
    commit_file "$desktop" "$desktop.zaurusmd-tmp.$$"

    wrong="$qtbase/apps/Settings/qipkg.desktop"
    if [ -f "$wrong" ] && grep -q '^[[:space:]]*Exec[[:space:]]*=[[:space:]]*qipkg[[:space:]]*$' "$wrong"; then
        disabled="$wrong.zaurusmd.disabled"
        if [ -e "$disabled" ]; then disabled="$disabled.$$"; fi
        mv "$wrong" "$disabled"
    fi
    repair_mime "$qtbase/etc/mime.types"
    repair_mime "$settings/mime.types"
    if [ ! -s "$qtbase/etc/slmime.types" ]; then
        baseline_slmime > "$qtbase/etc/slmime.types.zaurusmd-tmp.$$"
        commit_file "$qtbase/etc/slmime.types" "$qtbase/etc/slmime.types.zaurusmd-tmp.$$"
    fi
    if [ -x "$qtbase/bin/zaurusmd" ]; then
        repair_editor
    else
        echo "Markdown editor is not installed: install the zaurusmd IPK to open Markdown files." >&2
        if [ "$mode" = install ]; then exit 1; fi
    fi
fi

rm -f "$qtbase/etc/.mimetypes.cache" "$settings/.mimetypes.cache"
if [ -z "$root" ] && [ -x "$qtbase/bin/qcop" ]; then
    "$qtbase/bin/qcop" QPE/System 'linkChanged(QString)' '' || true
fi
if [ "$mode" != remove ]; then
    echo "Restored .ipk -> application/ipkg -> qinstall. Existing MIME entries were preserved."
    if [ ! -x "$qtbase/bin/qinstall" ]; then
        echo "Warning: qinstall binary is missing; this script restores associations only." >&2
    fi
    echo "If the file manager still shows the old association, close and reopen it or restart Qtopia after saving your work."
fi
