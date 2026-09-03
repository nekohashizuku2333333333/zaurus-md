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
- package restores the full Qtopia 1.7 `mime.types` baseline first, restores
  the Sharp `slmime.types` category table, and maps `.ipk` back to
  `qinstall.desktop` rather than the incompatible SDK `qipkg.desktop`
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
- Markdown to Qt rich text conversion with headings, inline strong/emphasis,
  code, links, images, strikeout, quotes, rules, bullet lists, numbered lists,
  and task links
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
