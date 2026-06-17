# DBYTEPAD

DBYTEPAD is a small native Windows text editor built directly on Win32.

No Electron.
No webview.
No runtime bundle.
No telemetry.

The first goal is simple: create a real editor that opens fast, saves files correctly, and keeps the source small enough to understand.

## Current scope

Version 0.1 starts with a minimal native editor:

- Win32 window
- RichEdit text control
- New, Open, Save, Save As, Exit
- Undo, Cut, Copy, Paste, Select All
- Dirty marker in the title
- Line and column status
- Drag and drop file open
- Single executable build

## Build

Open a Developer Command Prompt for Visual Studio, then run:

```bat
build.bat
```

The executable is written to:

```text
build\dbytepad.exe
```

## Design rules

- Prefer direct Win32 calls over frameworks.
- Keep the program understandable before making it smaller.
- Every feature must justify its size and complexity.
- Do not hide file writes behind clever behavior.
- Make the first version boringly reliable.

## Project state

Early bootstrap. The editor is intentionally small and incomplete.
