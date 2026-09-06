#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p test-runtime
${CC:-cc} -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined \
    -Wall -Wextra -Isrc -Ivendor/md4c \
    tests/render-cli.c src/MdRichText.c vendor/md4c/md4c.c vendor/md4c/entity.c \
    -o test-runtime/render-cli
python3 tests/test-corpus.py test-runtime/render-cli test-runtime/corpus-cases
