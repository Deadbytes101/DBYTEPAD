# Program Notes

DBYTEPAD is small on purpose.

Windows already has windows, menus, dialogs, text controls, file handles, and messages. DBYTEPAD uses them directly.

## Shape

One process.

One main window.

One RichEdit buffer.

One current path.

One dirty flag.

One read-only flag.

One ini file beside the executable.

## File law

The file belongs to the user.

Opening a file checks unsaved work first.

Saving writes only when the user asks for a save.

Read-only mode blocks editing and saving.

Reload reads from disk again.

Facts report what DBYTEPAD knows.

## Memory law

DBYTEPAD may remember simple local state.

Window size is state.

Font is state.

Word wrap is state.

Recent files are state.

That state lives in `dbytepad.ini` beside the executable.

## Size law

Size is measured, not guessed.

A smaller executable is good only if the editor still works.

The icon resource is part of the program cost.

A clever trick is bad if it makes file behavior harder to reason about.

## Branch law

`main` is the stable line.

Experimental work belongs on another branch.

Version 1.1.0 is the current stable release.
