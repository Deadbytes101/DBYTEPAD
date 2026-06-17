# DByte Mode

This is the first bridge between DBYTEPAD and the DByte language.

The editor stays an editor. DByte stays the language tool.

DBYTEPAD calls the external `dbyte` command. The compiler is not embedded in this branch.

## File detection

DByte mode turns on for `.dby`, `.dbyte`, `.dbyterc`, and `Dbyte.toml`.

When a DByte file is open, the title and status bar show `DBYTE`.

## Tools menu

`Tools > Run DByte` runs:

```text
dbyte run <file>
```

The file must be saved first. DByte runs from disk, not from the editor buffer.

`Tools > DByte Facts` shows path, lines, chars, UTF-8 bytes, and an approximate token count.

`Tools > DByte Version` runs:

```text
dbyte --version
```

## Current limits

Syntax coloring, output panes, project detection, and embedded runtime work are left for later.

The first rule is simple: open the file, inspect the bytes, run the file.
