# DBYTEPAD
![DBYTEPAD LOGO](assets/dbytepad-logo.svg)

Small Win32 text editor written in C.

No Electron. No webview. No telemetry.

![DBYTEPAD screenshot](assets/dbytepad-screens.png)

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

## Release

Current release: v1.1.0

Measured build:

- exe: 180224 bytes
- source: 1143 lines
- source: 34383 bytes

See [BYTE_LEDGER](docs/BYTE_LEDGER.md)

## License

MIT. See [LICENSE](LICENSE).
