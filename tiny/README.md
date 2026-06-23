# DBYTEPAD-1K

DBYTEPAD-1K is a separate tiny-build experiment for DBYTEPAD.

It is not a replacement for the normal Win32 C editor.

The normal editor stays readable, documented, and practical. The 1K build is a byte-budgeted sibling whose first job is to prove how small a native Windows editing surface can get without Electron, webviews, telemetry, background services, or decorative architecture.

## Target

Hard target:

```text
build\dbpad1k.exe <= 1024 bytes
```

Current status:

```text
experimental / unmeasured in repo
```

The project rule still applies:

```text
build first
measure second
talk third
```

## Clean-room rule

This directory must not copy RetroPad source.

RetroPad can be treated as public motivation and a size target, but DBYTEPAD-1K should stay a clean-room implementation with its own source shape, names, and constraints.

## Feature budget

The strict 1K edition starts with the smallest useful core:

```text
single native EDIT control
editable multiline buffer
normal Windows text editing behavior supplied by USER32
no menu
no status bar
no INI
no icon
no resource file
no RichEdit
no file dialogs
no DByte bridge
```

Stretch features are admitted only after the measured binary is below budget:

```text
argv file open
Ctrl+S save-back
dirty marker
small title update
```

Anything involving common dialogs, recent files, font UI, find/replace, DByte tooling, or version resources belongs to full DBYTEPAD, not to the 1K build.

## Build

Use an x86 Native Tools Command Prompt for Visual Studio.

```bat
cd tiny
build.bat
```

Outputs:

```text
..\build\dbpad1k-link.exe
..\build\dbpad1k.exe      optional, only if crinkler.exe is in PATH
```

The MS LINK build is expected to be larger than 1KB. It is kept as a correctness baseline.

The byte-fight build is the optional `crinkler.exe` path.

## Measure

```powershell
.\measure.ps1
```

Strict budget check:

```powershell
.\measure.ps1 -Strict
```

## Non-goals

Do not shrink full DBYTEPAD by making `src\dbytepad.c` obscure.

Do not hide behavior.

Do not add a fake retro skin.

Do not claim 1KB until the checked executable is measured at or below 1024 bytes.
