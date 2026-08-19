# Archetype Documentation

Archetype is a message-passing, object-oriented programming language for
writing text-based adventure games. These are the language's own documents,
first written between 1992 and 1995 and revised for the current interpreter.
For building and running the interpreter itself, see the
[project README](../README.md).

| | |
|---|---|
| [About Archetype](about.md) | What the language is, and where it came from. |
| [How to Play an Archetype Adventure](playing.md) | For players. What to type at the prompt, and what the parser can hear. |
| [How to Quickly Write an Adventure Game](writing.md) | For authors. A tutorial that starts with a four-line adventure and ends with custom verbs. |
| [The Archetype Language Reference Manual](manual.md) | The language proper: every statement, operator, and data type, with BNF. |
| [Known Issues](known-issues.md) | What is currently wrong or missing, and the 1995 bug list scored against the interpreter of today. |

Start with [playing](playing.md) if you have never seen a text adventure, with
[writing](writing.md) if you want to make one, and with the
[manual](manual.md) if you want to know exactly what the language does.

## About these documents

The two audiences never merged, and that is on purpose. *How to Quickly Write
an Adventure Game* assumes a reader who has never met a programming language,
because Archetype was designed so that such a reader could use it — a forgiving
syntax, strongly oriented toward one domain (see [About Archetype](about.md)).
The *Reference Manual* assumes someone who wants the grammar, and gives it in
Backus-Naur form. A beginner's tutorial shipping beside a BNF grammar looks odd
until you know that both were true at once: the language had to be usable by
someone who had never programmed, and it had to be a real language.

What the revision changed is the facts. The original documents describe a pair
of DOS programs, `CREATE.EXE` and `PERFORM.EXE`, running on a machine where a
string could not exceed 256 characters and the screen was always 24 lines tall.
None of that is true any more. Where a limit has been lifted, the text says so
rather than pretending it was never there.

The originals are preserved verbatim in [`original/`](original/).
