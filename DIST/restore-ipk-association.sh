#!/bin/sh
# Standalone Sharp/Qtopia association recovery; no installed editor required.
baseline_mime() {
cat <<'ZAURUS_MIME_BASELINE'
application/activemessage
application/andrew-inset	ez
application/applefile
application/atomicmail
application/cu-seeme		csm cu
application/dca-rft
application/dec-dx
application/excel		xls
application/ghostview		
application/ipkg	ipk
application/mac-binhex40	hqx
application/mac-compactpro	cpt
application/macwriteii
application/msword		doc dot wrd
application/news-message-id
application/news-transmission
application/octet-stream	bin dms lha lzh exe class
application/oda			oda
application/pdf			pdf
application/pgp			pgp
application/pgp-signature	pgp
application/postscript		ps ai eps
application/powerpoint		ppt
application/remote-printing
application/rtf			rtf
application/slate
application/wita
application/wordperfect5.1	wp5
application/x-123		wk
application/x-Wingz		wz
application/x-bcpio		bcpio
application/x-cdlink		vcd
application/x-chess-pgn		pgn
application/x-compress		z Z
application/x-cpio		cpio
application/x-csh		csh
application/x-debian-package	deb
application/x-director		dir dcr dxr
application/x-dvi		dvi
application/x-gtar		tgz gtar
application/x-gunzip		gz
application/x-gzip		gz
application/x-hdf		hdf
application/x-httpd-php		phtml pht php
application/x-javascript	js
application/x-koan		skp skd skt skm
application/x-latex		latex
application/x-maker		frm maker frame fm fb book fbdoc
application/x-mif		mif
application/x-msdos-program	com exe bat
application/x-netcdf		nc cdf
application/x-ns-proxy-autoconfig	pac
application/x-perl		pl pm
application/x-sh		sh
application/x-shar		shar
application/x-stuffit		sit
application/x-sv4cpio		sv4cpio
application/x-sv4crc		sv4crc
application/x-tar		tar
application/x-tcl		tcl
application/x-tex		tex
application/x-texinfo		texinfo texi
application/x-troff		t tr roff
application/x-troff-man		man
application/x-troff-me		me
application/x-troff-ms		ms
application/x-ustar		ustar
application/x-wais-source	src
application/zip			zip
audio/prs.sid			sid psid
audio/basic			au snd
audio/midi			mid midi kar
audio/mpeg			mp3 mpga mp2
audio/x-aiff			aif aifc aiff
audio/x-pn-realaudio		ra ram
audio/x-pn-realaudio-plugin
audio/x-realaudio		ra
audio/x-wav			wav
chemical/x-pdb			xyz
image/gif			gif
image/ief			ief
image/jpeg			jpeg jpg jpe
image/png			png
image/tiff			tiff tif
image/x-bmp			bmp
image/x-cmu-raster		ras
image/x-portable-anymap		pnm
image/x-portable-bitmap		pbm
image/x-portable-graymap	pgm
image/x-portable-pixmap		ppm
image/x-rgb			rgb
image/x-xbitmap			xbm
image/x-xpixmap			xpm
image/x-xwindowdump		xwd
image/x-notepad			npd
message/external-body
message/news
message/partial
message/rfc822
model/iges			igs iges
model/mesh			msh mesh silo
model/vrml			vrml wrl
multipart/alternative
multipart/appledouble
multipart/digest
multipart/mixed
multipart/parallel
text/css			css
text/html			html htm
text/plain			txt asc c cc h hh cpp hpp
text/richtext			rtx
text/tab-separated-values	tsv
text/x-setext			etx
text/x-sgml			sgml sgm
text/x-vCalendar		vcs
text/x-vCard			vcf
text/x-xml-tableviewer		xmlt
text/xml			xml dtd
video/dl			dl
video/fli			fli
video/gl			gl
video/mpeg			mpeg mp2 mpe mpg
video/quicktime			mov qt
video/x-msvideo			avi
video/x-sgi-movie		movie
x-conference/x-cooltalk		ice
x-world/x-vrml			wrl vrml
ZAURUS_MIME_BASELINE
}
baseline_qinstall() {
cat <<'ZAURUS_QINSTALL_BASELINE'
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
ZAURUS_QINSTALL_BASELINE
}
baseline_slmime() {
cat <<'ZAURUS_SLMIME_BASELINE'
# mime type vs dir. for sl
Image_Files		image/
Text_Files		text/plain
Music_Files		audio/
Video_Files		video/
Web_Files		text/html
Web_Files/Pagemeno_Files	application/nf-mht
Web_Files/Bookmark_Files	application/nf-url
ZAURUS_SLMIME_BASELINE
}
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
