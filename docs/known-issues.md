# Known Issues

This document has two halves. The first is a list of what is currently known to
be wrong or missing. The second is the original 1995 `BUGS.TXT`, scored against
the interpreter as it stands, because a list of eighteen complaints written for
a 10 MHz machine turns out to be a good measure of what thirty years changed and
what it didn't.

## Still open

**Verbs inside noun phrases confuse the parser.** If a game has an object named
"start button" and verbs named "start" and "push", then "push the start button"
is collapsed into `push start` + `button` rather than `push` + `start button`.
The parser removes "a", "an", and "the" before parsing and has no other notion
of article or noun-phrase boundary. Still exactly as reported in 1995; the
modern interpreter merely announces it better ("I don't know how to push start
a start button"). Related: [issue #40](https://github.com/gitosaurus/archetype/issues/40).

It is worth knowing that this is a priced trade rather than an oversight. The
design constraint, set before a line of the parser was written, was that any
natural language parser for the language would have to be both semantically
forgiving and computationally cheap. The grammar-and-lexicon approach that
would resolve "the start button" correctly was neither — daunting in
complexity, expensive in computing power, and brittle. The cheap parser buys
its speed and its tolerance by not knowing where a noun phrase begins, and this
is the bill for it.

**Integers only.** No real numbers, no floating-point arithmetic. Numbers are
32-bit signed and wrap silently on overflow.

**No separate compilation.** There is no equivalent of a `.o` file: no way to
compile `intrptr.arch` once and link it with an adventure later. It is
recompiled every single time you compile your adventure, and an adventure
cannot be built and tested in independent sections.

**The cumulative assignments are not faster.** `a +:= 2` is executed exactly as
`a := a + 2`. Prefer the short form because it is clearer, not because it is
quicker; the 1995 promise that it would someday be faster has not been kept and
no longer matters.

**`&:=` propagates UNDEFINED.** `s &:= expr` leaves `s` UNDEFINED if `expr` is
UNDEFINED, rather than leaving `s` alone. This is often not what the programmer
expects, but it does make sense if you think about it: concatenation with an
undefined value is undefined, and the cumulative form is defined in terms of
the plain one.

**No local variables.** Attributes look like locals syntactically, but they are
global — anyone who knows your name knows your attributes — and they are
static. A method has no scratch space of its own.

**The compiler does not prohibit duplicate declarations.** Declaring more than
one `main` object, or the same attribute or method twice within an object, is
accepted without a word. The last declaration silently wins. It ought to be an
error, as the results may surprise the programmer.

**An unknown operator aborts the compiler.** A token that scans as an operator
but is not one — `<>`, for instance — prints "Unknown operator" and then calls
`terminate()`, so the process dies on SIGABRT with a C++ runtime message on the
user's screen. Every other compile error produces a diagnostic and exits
cleanly.

**`display` of a destroyed object crashes the interpreter.** `ObjectValue::display`
(`src/Value.cc:363`) looks up an object that no longer exists and dereferences
the null result, so `display ref` segfaults when `ref` points at an object that
`destroy` has removed. Only nameless objects — the ones `create` makes — reach
that code path, because a named object returns earlier. `write` of the same
value is unaffected. Tracked as
[issue #62](https://github.com/gitosaurus/archetype/issues/62).

**`\\` produces a spurious diagnostic.** A doubled backslash in a string
literal yields the single backslash it should, but the scanner first complains
"Unknown escape character \\" on its way there.

## The 1995 list, scored

The letter in the original list indicated the kind of problem: **I**, a
solution exists but is not implemented; **D**, several solutions but no chosen
best one; **K**, knotty, needing sweeping design changes; **N**, not really a
problem, but someday it would be nice.

| # | | The original complaint | Where it stands |
|---|---|---|---|
| 1 | I | Verbs cannot appear inside noun phrases | **Still open.** Verified; the parser still knows only "a", "an", "the" |
| 2 | D | A bare verb should say "I don't know what *verb* means all by itself" | **Resolved.** The message exists. The hollow-object case that complicated it — "leave" while sitting on a couch — remains, deliberately |
| 3 | I | Vocabulary is not saved with the game state | **Resolved.** The parser, including its assembled vocabulary, is serialized with everything else |
| 4 | K | The interpreter uses about three times the memory it needs | **Moot.** The design that traded memory for speed on an 8088 is gone, and so is the constraint that made it a bug |
| 5 | N | No lists or arrays | **Resolved in 4.0.** List literals, `@`, `head`, `tail`, and list-valued expressions |
| 6 | N | No arguments when sending messages | **Largely resolved in 4.0.** `['MOVE TO' player] -> thing` dispatches on the head and carries the rest as arguments. The experiment in dataless messages ran for thirty years and the qualified "yes" held up |
| 7 | D | No real numbers | **Still open.** Integers only |
| 8 | K | No `.o` files; no separate compilation | **Still open** |
| 9 | D | A nicer debugger, able to view and change attributes interactively | **Largely resolved.** The REPL evaluates arbitrary expressions against a loaded universe; `display` prints values inline; `--inspect` and `--sitrep` dump world state as RDF/Turtle; the `'DEBUG MESSAGES'`, `'DEBUG EXPRESSIONS'`, and `'DEBUG STATEMENTS'` toggles remain |
| 10 | I | `a +:= 2` is no faster than `a := a + 2` | **Still true, and no longer interesting** |
| 11 | N | `s &:= expr` leaves `s` UNDEFINED if `expr` is | **Still open.** Verified |
| 12 | D | Errors in a declaration are sometimes reported after the whole declaration | **Resolved.** Every diagnostic now carries file, line, column, and a caret under the offending token |
| 13 | N | No local variables | **Still open** |
| 14 | I | I/O is slower than it has to be, thanks to `SAVELOAD.PAS` | **Resolved.** That module and its per-atom I/O are gone |
| 15 | I | Include files must be in the current or the adventure's directory | **Resolved.** `--include` takes a colon-separated search path, and `ARCHETYPE_INCLUDE` supplies more |
| 16 | I | Paging assumes 24 lines (*egads*) | **Resolved.** The pager reads the terminal's actual height |
| 17 | I | Word-wrap does not recognize `\n` and friends | **Resolved.** Verified: a `\n` inside a long string breaks the line and the wrapper picks up correctly afterward |
| 18 | I | Duplicate `main`, attributes, or methods are not prohibited | **Still open.** Verified |

## What the 1995 list never imagined

Half of that document worries about memory and disk on a machine with two
floppy drives, and none of it worries about running anywhere but DOS. The
concerns have almost exactly reversed. Performance is no longer a design
constraint at this scale, while the things that now matter most — that the same
`.acx` plays identically on macOS, Linux, and in a browser under WebAssembly;
that a save file is a mutated copy of the binary and therefore fully resumable;
that compilation is deterministic, so the same source always produces the same
bytes — are not on the list at all, because in 1995 there was only one machine
to be right on.
