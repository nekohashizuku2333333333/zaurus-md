# Device Checks

Use a disposable UTF-8 note with a source line longer than 640 pixels,
including Chinese text, a URL with no spaces, and several short lines after it.

1. Open the note at 640x480. Text should wrap inside the editor; the bottom
   action buttons should remain visible. Test again with magnified display.
2. Toggle Lines. Only the beginning of each source line receives a number.
   Continuation rows remain unnumbered. Scroll down and back up.
3. Change the font size with A+ and A-. Wrapping and line-number positions
   should adapt immediately, including when Lines is turned off again.
4. With the cursor on a wrapped continuation row, indent, outdent, duplicate,
   and move the source line. Undo each edit and compare with the original.
5. Search for a word beyond the first visual row. Replace it, select a range
   across two source lines, format it, and undo. Check that no extra physical
   newline is added merely by resizing or toggling Lines.
6. Preview backtick and tilde fences, indented code, blank code lines, HTML
   characters, inline backticks, and literal checkbox text inside code.
   Fences should not be visible. Code should not gain task links or emphasis.
7. Return to the file browser after real edits and check the save prompt.
   Reopen the saved file and verify its contents.

Parser automation: on the remote SDK host, run `sh scripts/test-remote.sh`.
These checks use the actual Qt2 library under ARM QEMU, but do not exercise
the physical device's font rendering, touchscreen, or framebuffer.
