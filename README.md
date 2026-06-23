# DBYTEPAD
![DBYTEPAD LOGO](assets/dbytepad-logo.svg)

Small Win32 text editor written in C.

No Electron. No webview. No telemetry.

![DBYTEPAD screenshots](assets/dbytepad-screens.png)

## Tiny line

The tiny builds are live in [Releases](https://github.com/Deadbytes101/DBYTEPAD/releases).

| Build | Size | Role |
| --- | ---: | --- |
| DBYTEPAD NANO v0.3.0 | 421 B / 3368 bits | VOID executable, exits immediately |
| DBYTEPAD-1K v0.1.0 | 454 B | size-kill Win32 EDIT core |
| DBYTEPAD Micro v0.1.0 | 1224 B | practical tiny editor: argv open, edit, Ctrl+S save-back |

SIZE LAB: [docs/SIZE_LAB.md](docs/SIZE_LAB.md)

Build first. Measure second. Talk third.

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

## Release

Current release: v1.2.1

Measured build:

- commit: c26cb7f
- exe: 185856 bytes
- source: 1431 lines
- source: 42963 bytes
- sha256: `e45cfbc8997fedf2f6d248144bde683349b873bebd80db594b43c3a174b5432a`

## Docs

Start with [docs](docs/README.md).

## License

MIT. See [LICENSE](LICENSE).
