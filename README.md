# DBYTEPAD
![DBYTEPAD LOGO](assets/dbytepad-logo.svg)

Small Win32 text editor written in C.

No Electron. No webview. No telemetry.

![DBYTEPAD screenshots](assets/dbytepad-screens.png)

## Build

```bat
build.bat
```

Output:

```text
build\dbytepad.exe
```

## Run

```bat
build\dbytepad.exe README.md
```

## Features

- Open, save, reload.
- Read-only open.
- Recent files.
- Find and replace.
- Word Wrap.
- Font setting.
- Drag and drop file open.
- File Facts.
- Line, column, char count, and UTF-8 byte count.
- Remembers window size, font, word wrap, and recent files in `dbytepad.ini`.

## DByte mode

DBYTEPAD knows DByte source files:

- `.dby`
- `.dbyte`
- `.dbyterc`
- `Dbyte.toml`

When a DByte file is open, the title and status bar show `DBYTE`.

`Tools > DByte Facts` works without the DByte tool installed.

`Tools > Run DByte` and `Tools > DByte Version` use the external DByte command-line tool when it is in PATH.

DByte mode is optional. DBYTEPAD still opens, edits, and saves DByte source files without DByte installed.

## Docs

Start with [docs](docs/README.md).

## Release

Current release: v1.2.0

Measured build:

- exe: 184832 bytes
- source: 1431 lines
- source: 42963 bytes
- sha256: `400214c526514a12ed7181618ba27a5331735f668256729771d329501d94204e`

See [BYTE_LEDGER](docs/BYTE_LEDGER.md).

## License

MIT. See [LICENSE](LICENSE).
