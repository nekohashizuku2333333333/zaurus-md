#!/bin/sh
set -eu

REMOTE="${1:-root@192.168.122.187}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MURPHY="/home/flan/Documents/Workdir/other/murphytalk-pinyin-fix"
BASE="/tmp/zaurus-mdwriter-build"
UPLOAD="/tmp/zaurus-mdwriter-src.tar.gz"

SSH_OPTS="-F /dev/null -o StrictHostKeyChecking=no -o UserKnownHostsFile=/tmp/zaurus_md_known_hosts -o KexAlgorithms=+diffie-hellman-group14-sha1,diffie-hellman-group1-sha1 -o HostKeyAlgorithms=+ssh-rsa -o Ciphers=+aes128-cbc,3des-cbc"

ssh $SSH_OPTS "$REMOTE" "rm -rf '$BASE' && mkdir -p '$BASE/compat-lib'"
tar czf "$UPLOAD" \
  --exclude .git \
  --exclude DIST \
  --exclude IPK \
  -C "$ROOT" .
scp $SSH_OPTS "$UPLOAD" "$REMOTE:$BASE/src.tar.gz"
ssh $SSH_OPTS "$REMOTE" "cd '$BASE' && tar xzf src.tar.gz && rm -f src.tar.gz"
scp $SSH_OPTS -r "$MURPHY/lib"/. "$REMOTE:$BASE/compat-lib/"
ssh $SSH_OPTS "$REMOTE" "cd '$BASE' && ./scripts/build-remote-nomake.sh"
mkdir -p "$ROOT/DIST"
scp $SSH_OPTS "$REMOTE:$BASE/DIST/zaurusmd_0.1_arm.ipk" "$ROOT/DIST/"
rm -f "$UPLOAD"
