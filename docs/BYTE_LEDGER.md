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

For tiny builds, run:

```powershell
cd tiny
.\measure-nano.ps1
.\build.bat
.\measure.ps1
.\build-micro.bat
.\measure-micro.ps1
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

v1.2.1

- Measured commit: c26cb7f
- Executable bytes: 185856
- Source lines: 1431
- Source bytes: 42963
- Note: Windows version metadata for executable Properties

DBYTEPAD NANO experiment

- Branch: feat/dbpad-nano
- Semantic payload: 1 bit
- Stored marker: 1 byte
- Marker: tiny\dbpadnano.bit
- Score: PASS semantic payload <= 1 bit; PASS stored marker <= 1 byte
- Note: bit-level marker below the byte; not an executable and not an editor

DBYTEPAD-1K experiment

- Branch: feat/dbpad-1k
- Measured locally: 2026-06-23
- MS LINK baseline: 1296 bytes
- Crinkler executable: 454 bytes
- Budget: 1024 bytes
- Score: PASS Bronze / Silver / Gold / Black
- Note: clean-room tiny Win32 EDIT surface; beats the recorded TinyRetroPad/DTE size targets used by tiny\measure.ps1

DBYTEPAD Micro experiment

- Branch: feat/dbpad-micro
- Measured locally: 2026-06-23
- MS LINK baseline: 4608 bytes
- Crinkler executable: 1224 bytes
- Budget: 2476 bytes
- Score: PASS RetroPad full-size target
- Behavior proof: argv file open; native multiline EDIT surface; Ctrl+S save-back to original path; dirty marker in title
- Note: practical TinyRetroPad fight tier; DBYTEPAD-1K remains the 454-byte size-kill core
