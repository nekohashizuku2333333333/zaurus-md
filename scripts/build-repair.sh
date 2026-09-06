#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p DIST
output=DIST/restore-ipk-association.sh
{
    printf '#!/bin/sh\n# Standalone Sharp/Qtopia association recovery; no installed editor required.\n'
    printf "baseline_mime() {\ncat <<'ZAURUS_MIME_BASELINE'\n"
    cat packaging/mime.types.qtopia17
    printf 'ZAURUS_MIME_BASELINE\n}\n'
    printf "baseline_qinstall() {\ncat <<'ZAURUS_QINSTALL_BASELINE'\n"
    cat packaging/qinstall.desktop
    printf 'ZAURUS_QINSTALL_BASELINE\n}\n'
    printf "baseline_slmime() {\ncat <<'ZAURUS_SLMIME_BASELINE'\n"
    cat packaging/slmime.types.sharp
    printf 'ZAURUS_SLMIME_BASELINE\n}\n'
    cat tools/restore-file-associations.sh
} > "$output"
chmod 755 "$output"
