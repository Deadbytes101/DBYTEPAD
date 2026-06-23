# DBYTEPAD SIZE LAB

NATIVE WINDOWS BINARIES MEASURED IN BYTES AND BITS.

NO ELECTRON. NO WEBVIEW. NO TELEMETRY.

BUILD FIRST. MEASURE SECOND. TALK THIRD.

## Current board

| Tier | Release | Asset | Bytes | Bits | Behavior |
| --- | --- | --- | ---: | ---: | --- |
| NANO VOID | `dbpadnano-v0.3.0` | `dbpadnano-3368bit.exe` | 421 | 3368 | valid Windows executable, exits immediately |
| NANO EDIT | `dbpadnano-v0.2.0` | `dbpadnano-3632bit.exe` | 454 | 3632 | native Win32 EDIT surface |
| 1K | `dbpad1k-v0.1.0` | `dbpad1k.exe` | 454 | 3632 | native Win32 EDIT surface |
| MICRO | `dbpadmicro-v0.1.0` | `dbpadmicro.exe` | 1224 | 9792 | argv open, edit, Ctrl+S save-back |
| FULL | `v1.2.1` | `dbytepad.exe` | 185856 | 1486848 | full DBYTEPAD editor |

## NANO VOID proof

```text
release: dbpadnano-v0.3.0
asset: dbpadnano-3368bit.exe
size: 421 bytes / 3368 bits
sha256: 6593e303e551dd23afff4403736ed99255e6f2cd7c481c24daacc6856e0c4e83
behavior: exits immediately
exit code: 0
```

## Score names

```text
GOLD   <= 421 B / 3368 bits
BLACK  <= 400 B / 3200 bits
DEMON  <= 384 B / 3072 bits
VOID   <= 360 B / 2880 bits
MYTH   <= 320 B / 2560 bits
```

## Rules

Do not compare unlike things without naming the tier.

NANO VOID is not an editor.

NANO EDIT and DBYTEPAD-1K are editor-core builds.

MICRO is the practical tiny editor tier.

FULL is the normal user-facing editor.

GitHub displays release asset sizes in bytes. The bit identity is carried in the asset name, release notes, and this ledger.

## Local verification

From the repository root:

```powershell
.\scripts\verify-nano.ps1 -Path .\build\dbpadnano.exe -Run -Strict
```

Expected current champion:

```text
421 bytes / 3368 bits
exit code 0
```
