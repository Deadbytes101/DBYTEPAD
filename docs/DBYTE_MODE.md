# DByte Mode

This is the bridge between DBYTEPAD and the DByte language.

DBYTEPAD stays a text editor.

DByte stays a separate command-line tool.

## File detection

DByte mode turns on for:

- `.dby`
- `.dbyte`
- `.dbyterc`
- `Dbyte.toml`

When a DByte file is open, the title and status bar show `DBYTE`.

## Optional tool

DByte mode is optional.

DBYTEPAD can open, edit, save, and count DByte source files even when the DByte tool is not installed.

Run DByte and DByte Version need the DByte tool in PATH.

If the tool is missing, DBYTEPAD says so and keeps the editor usable.

## Tools menu

Run DByte runs the saved file from disk.

DByte Facts shows path, lines, chars, UTF-8 bytes, DByte tool status, and an approximate token count.

DByte Version shows the installed DByte version when available.

## Current limits

No syntax coloring yet.

No output pane yet.

No project tree yet.

No embedded runtime yet.

The first rule is simple: open the file, inspect the bytes, run the file when the tool exists.
