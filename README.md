# Zaurus MD Writer & Reader

Qt/E 2.3.2 / Qtopia 1.x Markdown writer and reader for Sharp Zaurus.

This project follows the ABI constraints documented in:

```text
/home/flan/Documents/Workdir/other/murphytalk-pinyin-fix/doc/env.md
```

The current milestone is a compact Markor-style Markdown notebook:

- file browser rooted at `/home/zaurus/Documents/Notes`
- edit/view switch
- Markdown to Qt rich text conversion with headings, inline strong/emphasis,
  code, links, quotes, rules, bullet lists, numbered lists, and task links
- UTF-8 file read/write with atomic save
- new, save, save as, rename, delete, autosave
- formatting toolbar for bold, italic, code, headings, bullets, numbered lists,
  tasks, quotes, links, rules, indent/outdent, line move, date/time, undo/redo
- multi-line selection handling for line formatting, indentation, task toggles,
  priorities, and line movement
- editor keyboard helpers for Tab/Shift-Tab indentation and smart Return
  continuation for lists, tasks, quotes, and numbered lists
- todo.txt actions for done, priorities, project/context/due tags, sorting,
  hiding completed lines, moving completed lines to the end, and clearing done
- remote build script for the Zaurus SDK host

Syntax highlighting is intentionally left out for now because Qt/E 2.3 text
editing is fragile on this target; the plain editor/viewer path is the stable
base for manual device testing.

## Build On Remote SDK Host

```sh
./scripts/remote-build.sh root@192.168.122.187
```

The script uses the old SSH algorithms required by the SDK host and builds
with GCC 2.95.3, Qt/E 2.3.2 headers, and Qtopia 1.7 headers.
