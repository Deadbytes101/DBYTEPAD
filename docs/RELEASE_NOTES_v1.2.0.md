# DBYTEPAD v1.2.0

This release adds DByte mode.

DBYTEPAD is still a native Win32 text editor written in C.

No Electron. No webview. No telemetry.

## Added

- DByte file detection.
- DByte marker in title and status bar.
- DByte Facts.
- Run DByte with F6.
- DByte Version check.
- Optional DByte tool detection.
- Clear message when the DByte tool is not in PATH.
- `examples/demo.dby`.

## DByte files

DByte mode turns on for:

- `.dby`
- `.dbyte`
- `.dbyterc`
- `Dbyte.toml`

## Tool rule

DByte mode does not make DBYTEPAD depend on DByte.

Without the DByte tool, DBYTEPAD still opens, edits, saves, and counts DByte source files.

Run DByte and DByte Version need the DByte tool in PATH.

## Build facts

- Measured commit: 624d4a0
- Executable bytes: 184832
- Source lines: 1431
- Source bytes: 42963
- SHA256: 400214c526514a12ed7181618ba27a5331735f668256729771d329501d94204e

## Note

This release keeps the editor small and makes DByte support a clean bridge, not a hard dependency.
