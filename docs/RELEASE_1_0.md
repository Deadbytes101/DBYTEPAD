# DBYTEPAD 1.0.0 Release Plan

Version 1.0.0 is a stability release.

Feature work stops here unless it fixes correctness.

Included:

- Native Win32 window.
- RichEdit text buffer.
- New, Open, Open Read Only, Save, Save As, Reload, Exit.
- Undo, Cut, Copy, Paste, Select All.
- Find and Find Next.
- Word Wrap.
- Read Only mode.
- File Facts.
- Command-line file open.
- Dirty title marker.
- Line, column, char count, UTF-8 byte count.
- Byte Ledger measurement.

Release rule:

A build can be called 1.0.0 only after build.bat succeeds, release-check.ps1 runs, and the manual checks pass.

Manual checks:

- Open README.md from command line.
- Edit text and confirm modified state.
- Save and confirm saved state.
- Save As to a new file.
- Open Read Only and confirm typing is blocked.
- Toggle Read Only off and confirm typing works.
- Use Find and F3.
- Use File Facts before and after editing.
- Reload with unsaved changes and confirm the save prompt appears.
- Close with unsaved changes and confirm the save prompt appears.

Tag:

```powershell
git tag -a v1.0.0 -m "DBYTEPAD v1.0.0"
git push origin v1.0.0
```
