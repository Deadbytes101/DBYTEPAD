# DBYTEPAD docs

This directory is for the program, not for a pitch deck.

DBYTEPAD is a small Win32 editor. It should be easy to build, easy to read, and hard to misunderstand.

## Read this first

- `USER_MANUAL.md` - how to use the editor.
- `DBYTE_MODE.md` - how the DByte bridge works.
- `PROGRAM_NOTES.md` - what the program is allowed to be.
- `HACKING.md` - how to change the code without turning it into junk.
- `BYTE_LEDGER.md` - measured size history.
- `../tiny/README.md` - DBYTEPAD-1K byte-budget experiment.
- `RELEASE_NOTES_v1.2.1.md` - current release notes.
- `RELEASE_NOTES_v1.2.0.md` - previous DByte mode release notes.
- `RELEASE_NOTES_v1.1.0.md` - older release notes.

## Current line

Current stable release: v1.2.1.

DBYTEPAD is still a text editor first.

DByte mode is a bridge. It does not turn DBYTEPAD into a heavy IDE.

Version metadata belongs in the executable. Users should not see a blank Properties page.

DBYTEPAD-1K is an experimental sibling build, not a replacement for the normal editor.

## Rules

The file belongs to the user.

A save must be explicit.

Read-only must mean read-only.

DByte support must be optional.

Measure bytes. Do not guess.

Keep the code plain enough to fix at 2 AM.
