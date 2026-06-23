# DBYTEPAD Micro

Practical tiny editor tier above DBYTEPAD-1K.

DBYTEPAD-1K keeps the measured 454 byte core.

Micro target:

```text
build\dbpadmicro.exe <= 2476 bytes
```

Current measured result:

```text
MS LINK baseline: 4608 bytes
Crinkler build: 1224 bytes
Budget: 2476 bytes
Result: PASS RetroPad full-size target
Measured locally: 2026-06-23
```

Implemented behavior:

```text
argv file open
native multiline EDIT surface
save back to original path with Ctrl+S
dirty marker in title
```

Scoreboard:

```text
PASS  RetroPad full-size target <= 2476 bytes
MISS  DBYTEPAD-1K hard target <= 1024 bytes
MISS  DTE 2.x bare target <= 981 bytes
MISS  DTE 1.x aggressive target <= 890 bytes
```

Meaning:

```text
DBYTEPAD-1K remains the size-kill core.
DBYTEPAD Micro is the practical TinyRetroPad fight tier.
```
