# Byte Ledger

DBYTEPAD measures the cost of the program instead of guessing.

Each release entry records the local build facts that matter:

- Version
- Commit
- Executable bytes
- Source lines
- Source bytes
- Note

Run after building:

```powershell
.\scripts\size.ps1 -Record v1.0.0
```

## Entries

v0.1.4

- Commit: 357ce6d
- Executable bytes: 140800
- Source lines: 844
- Source bytes: 24621
- Note: local measured build

v1.0.0

- Commit: dfd4dac
- Executable bytes: 140800
- Source lines: 862
- Source bytes: 25146
- Note: local measured build
