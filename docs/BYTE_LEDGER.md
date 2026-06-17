# Byte Ledger

DBYTEPAD measures the cost of the program instead of guessing.

Record format:

Version
Commit
Executable bytes
Executable SHA256
Source lines
Source bytes
Note

The ledger should be updated from a local build after the executable is produced.

Run:

```powershell
.\scripts\measure.ps1 -Record v0.1.4
```

Entries:

No entries yet.

v0.1.4
Commit: 357ce6d
Executable bytes: 140800
Source lines: 844
Source bytes: 24621
Note: local measured build

v1.0.0
Commit: dfd4dac
Executable bytes: 140800
Source lines: 862
Source bytes: 25146
Note: local measured build
