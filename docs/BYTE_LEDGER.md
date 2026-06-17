# Byte Ledger

DBYTEPAD measures the cost of the program instead of guessing.

Each release entry records the local build facts that matter:

- Version
- Measured commit
- Executable bytes
- Source lines
- Source bytes
- Note

Run after building:

```powershell
.\scripts\size.ps1
```

## Entries

v0.1.4

- Measured commit: 357ce6d
- Executable bytes: 140800
- Source lines: 844
- Source bytes: 24621
- Note: local measured build

v1.0.0

- Measured commit: dfd4dac
- Executable bytes: 140800
- Source lines: 862
- Source bytes: 25146
- Note: stable release baseline

v1.0.1

- Measured commit: 75d0639
- Executable bytes: 512000
- Source lines: 864
- Source bytes: 25190
- Note: embedded icon resource was too large

v1.0.2

- Measured commit: d24ccba
- Executable bytes: 174080
- Source lines: 864
- Source bytes: 25190
- Note: compact icon resource

v1.1.0

- Measured commit: 19e00a6
- Executable bytes: 180224
- Source lines: 1143
- Source bytes: 34383
- Note: recent files, ini state, font setting, replace

v1.2.0

- Measured commit: 624d4a0
- Executable bytes: 184832
- Source lines: 1431
- Source bytes: 42963
- SHA256: 400214c526514a12ed7181618ba27a5331735f668256729771d329501d94204e
- Note: optional DByte mode, DByte facts, external DByte runner
