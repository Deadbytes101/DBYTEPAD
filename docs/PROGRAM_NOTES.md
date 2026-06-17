# Program Notes

DBYTEPAD is a small program on purpose.

The operating system already has windows, menus, dialogs, text controls, file handles, and messages. DBYTEPAD uses them directly instead of building a second fake operating system inside the first one.

## Shape

One process.

One main window.

One RichEdit buffer.

One current path.

One dirty flag.

One read-only flag.

No hidden network behavior.

No background service.

No telemetry.

## File law

The file belongs to the user.

Opening a file replaces the buffer only after unsaved changes are handled.

Saving writes only when the user asks for a save.

Read-only mode blocks editing and saving.

Facts report what DBYTEPAD knows instead of pretending to know more.

## Size law

Size is measured, not guessed.

A smaller executable is good only if the editor still works.

The icon resource is part of the program cost.

A clever trick is bad if it makes file behavior harder to reason about.

## Branch law

`main` is the stable line.

Experimental work belongs on another branch.

Version 1.0.2 is the current stable release.
