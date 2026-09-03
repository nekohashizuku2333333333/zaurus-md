# Zaurus MD Writer & Reader

Qt/E 2.3.2 / Qtopia 1.x Markdown writer and reader for Sharp Zaurus.

This project follows the ABI constraints documented in:

```text
/home/flan/Documents/Workdir/other/murphytalk-pinyin-fix/doc/env.md
```

The current milestone is a Markor-like skeleton:

- file browser rooted at `/home/zaurus/Documents/Notes`
- edit/view switch
- Markdown to Qt rich text conversion
- Markdown task list toggle links in view mode
- UTF-8 file read/write with atomic save
- todo.txt parser primitives
- remote build script for the Zaurus SDK host

High-risk editor syntax highlighting is intentionally left as the next
milestone after the plain editor/viewer path is buildable and testable.

## Build On Remote SDK Host

```sh
./scripts/remote-build.sh root@192.168.122.187
```

The script uses the old SSH algorithms required by the SDK host and builds
with GCC 2.95.3, Qt/E 2.3.2 headers, and Qtopia 1.7 headers.

