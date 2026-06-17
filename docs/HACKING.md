# Hacking DBYTEPAD

This file is for changing the program without making it soft.

Keep it native.

Keep it readable.

Keep the file rules exact.

## Build

Use MSVC:

```bat
build.bat
```

The output is:

```text
build\dbytepad.exe
```

The build uses `/W4 /WX`. Warnings are treated as failures.

## Source shape

Main source:

```text
src\dbytepad.c
```

Resource files:

```text
src\dbytepad.rc
src\resource.h
assets\dbytepad.ico
```

The program is one Win32 process with one main window and one RichEdit buffer.

Do not add a second UI layer unless there is a real reason.

## External behavior

Do not add network access.

Do not add background services.

Do not add telemetry.

Do not add an updater.

Do not add a plugin system before the editor itself needs one.

## File behavior

Before replacing the buffer, check unsaved work.

Before closing, check unsaved work.

Read-only mode must block editing and saving.

Saving must be user-visible and explicit.

Facts must report what the program actually knows.

## Size rule

Build first.

Measure second.

Talk third.

Use:

```powershell
.\scripts\size.ps1
```

Record release builds with:

```powershell
.\scripts\size.ps1 -Record vX.Y.Z
```

Do not make the program smaller by making the code stupid.

## Version rule

Release builds must update `APP_VERSION` in `src\dbytepad.c`.

Tags should point at the same commit as `main` for that release.

Check:

```powershell
git diff vX.Y.Z main
```

No diff means the tag and main agree.

## Good changes

A good change makes the editor more useful without hiding behavior.

Examples:

- better file handling
- better status facts
- better read-only behavior
- better search behavior
- smaller binary with the same behavior

## Bad changes

A bad change makes the program look bigger than it is.

Examples:

- skins
- fake terminals
- webviews
- startup splash screens
- background work the user did not ask for
- decorative architecture

The code should look like it was written to be fixed, not admired.
