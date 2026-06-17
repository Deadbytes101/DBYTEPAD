![DBYTEPAD](assets/dbytepad-logo.svg)

# DBYTEPAD

A small native Windows text editor built directly on Win32.

No Electron.
No webview.
No runtime bundle.
No telemetry.

DBYTEPAD is a file-truth editor. It opens text, tracks state, reports facts, supports read-only inspection, and measures its own build size.

A program should fit in the head.

## Version

Current stable release: v1.0.2

## What it does

- Opens and saves text files.
- Opens files from the command line.
- Opens files in read-only mode for inspection.
- Tracks dirty state in the title and status line.
- Shows line, column, character count, and UTF-8 byte count.
- Shows file facts: path, disk size, modified time, lines, chars, buffer bytes, and state.
- Provides native Find and Find Next.
- Supports Word Wrap.
- Supports drag and drop file open.
- Builds as a single Windows executable.
- Carries its own measured icon resource.

## What it refuses

- No browser hidden inside the program.
- No framework pretending to be the operating system.
- No telemetry.
- No plugin theater before the editor is solid.
- No fake cyber skin.

## Build

Open a Developer Command Prompt for Visual Studio, then run:

```bat
build.bat
```

The executable is produced in the build directory as `dbytepad.exe`.

## Run

```bat
build\dbytepad.exe README.md
```

## Release facts

v1.0.2 local measured build:

- Executable bytes: 174080
- Source lines: 864
- Source bytes: 25190

See `docs/BYTE_LEDGER.md` for the measured ledger.

## Design rules

- Use Win32 directly.
- Let RichEdit own text mechanics.
- Let DBYTEPAD own file behavior.
- Keep file writes explicit.
- Keep source understandable.
- Measure size instead of guessing.
- Preserve stable behavior before shrinking the binary.

## Stable feature set

File:

- New
- Open
- Open Read Only
- Save
- Save As
- Reload
- Facts
- Exit

Edit:

- Undo
- Cut
- Copy
- Paste
- Find
- Find Next
- Select All

View:

- Word Wrap
- Read Only

## Branches

`main` is the stable release line.

Experimental size work belongs on `tiny-build`.

## License

MIT License. See `LICENSE`.

## Status

DBYTEPAD v1.0.2 is tagged and stable. Future work should be bug fixes, release tooling, or isolated tiny-build experiments.