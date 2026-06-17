# User Manual

DBYTEPAD edits text files.

It is not an IDE. It is not a browser. It is not a project system.

It opens a file, lets you change it, and saves only when you ask.

## Start

Run it directly:

```bat
build\dbytepad.exe
```

Open a file from the command line:

```bat
build\dbytepad.exe README.md
```

Open a DByte file:

```bat
build\dbytepad.exe examples\demo.dby
```

## File menu

New clears the buffer after asking about unsaved work.

Open loads a file for editing.

Open Read Only loads a file but blocks edits and saves.

Recent opens one of the last files used by this build.

Save writes the current buffer to the current path.

Save As writes the buffer to a path you choose.

Reload reads the current file from disk again. Unsaved work is checked first.

Facts shows what DBYTEPAD knows: path, bytes, line count, char count, state, and DByte facts when the file is DByte source.

Exit closes the program after checking unsaved work.

## Edit menu

Undo, Cut, Copy, Paste, and Select All are normal editor commands.

Find uses the Windows find dialog.

Find Next repeats the search.

Replace uses the Windows replace dialog.

## View menu

Word Wrap changes horizontal wrapping.

Read Only toggles edit blocking for the current buffer.

Font opens the Windows font dialog.

## Tools menu

Run DByte runs the current saved DByte file with the external DByte tool.

DByte Facts shows DByte source status, DByte tool status, lines, chars, UTF-8 bytes, and approximate tokens.

DByte Version checks the installed DByte tool.

The DByte tool is optional. DBYTEPAD still opens, edits, saves, and counts DByte source files without it.

## Settings

DBYTEPAD writes `dbytepad.ini` beside the executable.

It stores:

- window position and size
- font face and size
- word wrap
- recent files

Delete `dbytepad.ini` to reset settings.

## Text rules

DBYTEPAD reads UTF-8, UTF-8 with BOM, UTF-16LE, and falls back to the Windows ANSI code page.

DBYTEPAD saves as UTF-8.

Line endings are saved as CRLF.

## What it does not do

No autosave.

No cloud sync.

No telemetry.

No plugin loader.

No hidden network calls.
