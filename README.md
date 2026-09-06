# Zaurus MD Writer & Reader

Qt/E 2.3.2 / Qtopia 1.x Markdown writer and reader for Sharp Zaurus.

This project follows the ABI constraints documented in:

```text
/home/flan/Documents/Workdir/other/murphytalk-pinyin-fix/doc/env.md
```

The current milestone is a compact Markor-style Markdown notebook:

- file browser rooted at `/home/zaurus/Documents/Notes`
- edit/view switch
- separate browser/document button bars so file management stays out of the
  editor, with compact Zaurus-sized buttons
- document bar uses full `View` and `Save` labels, and the title is
  `Zaurus MDEditor` or `Zaurus MDEditor - <file>`
- document save state is shown on the right side of the document bar as
  `Saved` or `Modified`
- leaving a document for the file browser, or closing the app, prompts to save
  unsaved edits; autosave no longer writes behind the prompt
- desktop files install to both Applications and Document with `%f` file
  arguments and Markdown MIME declarations
- package 0.2 restores `.ipk` to Sharp `qinstall` without owning the system
  launcher or MIME files; removal retains system associations and only removes
  Markdown entries explicitly added by this package
- recovery preserves existing MIME mappings and category customizations;
  complete defaults are used only for missing/empty files
- file manager launches are handled through Qtopia's document app path:
  `showMainDocumentWidget()` plus `setDocument(const QString&)`
- paged editing toolbar with no scroll bar; `More` rotates the labels/actions
  of fixed buttons instead of destroying widgets at runtime
- bottom toolbar uses ten fixed 63px buttons to fill the 640px landscape
  screen, and the file browser lists real directory entries once
- toolbar buttons keep stable signal connections and relayout from the current
  window width, which is safer under Qtopia magnified display mode
- explicit `song` QPF font selection for the app, editor, preview, browser,
  and toolbar controls
- editor soft wrapping follows the available widget width, including font size
  and line-number margin changes; saved text keeps its original newlines
- the document bar's `Lines` toggle shows source line numbers; wrapped
  continuation rows do not receive new numbers, and commands use source positions
- fenced code is rendered as one preformatted block without visible fences;
  backtick/tilde fences, CRLF, blank lines, indentation, unfinished fences,
  escaped punctuation and multi-backtick inline code are handled
- MD4C 0.5.3 CommonMark/GFM parsing with a Qt2 rich text adapter: Setext/ATX
  headings, soft/hard breaks, nested lists and quotes, reference links,
  tables with alignment, entities, nested emphasis and task links
- literal raw HTML, blocked application-command URLs from source links, and
  source-accurate task toggles (including nested tasks and CRLF)
- relative preview images resolve against the current Markdown file; code
  and inline code explicitly retain the device's `song` font family
- UTF-8 file read/write with atomic save
- new file, new folder, save, save as, rename, delete, autosave
- formatting toolbar for bold, italic, inline code, code blocks, headings,
  bullets, numbered lists, tasks, quotes, links, images, tables, rules,
  indent/outdent, line move, date/time, undo/redo
- multi-line selection handling for line formatting, indentation, task toggles,
  priorities, and line movement
- copy, cut, paste, select all, and duplicate line/selection block actions
- editor keyboard helpers for Tab/Shift-Tab indentation and smart Return
  continuation for lists, tasks, quotes, and numbered lists
- todo.txt actions for done, priorities, project/context/due tags, sorting,
  hiding completed lines, project/context preview filtering, moving completed
  lines to the end, clearing done, and batch open/done state changes
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
The executable is downloaded and packaged locally because the SDK host's tar
does not support the reproducible timestamp option. IPKs remain gzip-compressed
GNU tar archives without PAX headers.

On the SDK host, run `sh scripts/test-remote.sh` from the source directory for
ARM/QEMU parser regression tests. The test-only runtime helper library is not
included in the application package. Visual line-number alignment and touch
interaction still require a device check at 640x480 and in magnified mode.

Run `sh scripts/test-corpus.sh` locally for sanitizer-enabled coverage of the
user-supplied corpus. Its recorded outputs can also be checked on the SDK host
with `sh scripts/test-arm-corpus.sh`. See [corpus coverage](doc/corpus-coverage.md)
for implemented syntax, unsupported extensions, and device validation limits.
The vendored parser's MIT license is included in the IPK.

## Recover IPK Associations

Old 0.1 packages incorrectly owned the system `qinstall.desktop` (and earlier
ones owned `mime.types`). Removing those packages could remove the system's
file associations. Package 0.2 no longer lists those files in its data archive.

`DIST/restore-ipk-association.sh` is standalone and works even after the editor
has been uninstalled. On the Zaurus, as root, run it from the directory holding
the file:

```sh
sh restore-ipk-association.sh
```

Install `DIST/zaurusmd_0.2_arm.ipk` afterward, or install it directly using
`ipkg install` in the terminal (which does not need a working file association).
It can upgrade 0.1 in place; uninstalling first is unnecessary.
The script requests Qtopia link refresh. If a file manager retains an old
association, reopen it or restart Qtopia after saving documents.

Validation: `python3 tests/test-associations.py` checks isolated repair and
package lifecycle behavior. `scripts/test-legacy-ipkg.sh` exercises the SDK's
actual old ipkg with redirected state/config paths and a no-op link helper.
See [repair details](doc/association-repair.md).
