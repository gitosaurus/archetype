# Demonstrations

Optional programs that show off something about Archetype. Nothing here is part
of the interpreter, nothing builds it, and nothing in `src/` refers to any of
it. Delete the directory and Archetype is unchanged.

## object_graph.py — drawing a universe

`archetype --inspect` writes the state of a universe as RDF, and every attribute
holding an object is an edge in a graph. `object_graph.py` restates those edges
as [GraphViz](https://graphviz.org) dot.

It needs Python 3 and nothing else — no packages to install. It writes dot to
standard output, so GraphViz itself is wanted only if you want a picture rather
than the text.

`--inspect` reads compiled `.acx` files, which are build output rather than
repository content, so each example below compiles its game first.

### Animal, whose database is its game state

`games/animal.arch` keeps a binary decision tree of questions, and adds to it
every time it guesses wrong. The tree is not stored anywhere but in the objects,
so a save file *is* the database:

```shell
./build/archetype --source=games/animal.arch --include=games --create=games/animal.acx
./demos/object_graph.py games/animal.acx \
    --attr IfYes --attr IfNo --label Q --label A | dot -Tpng -o animal.png
```

`--attr` picks which attributes to draw as edges and `--label` says which ones
to write inside a node.

Freshly compiled, that is a drawing of three nodes — the one question and two
animals `animal.arch` ships with. It is worth doing before playing, because the
interesting version is the one after: play a few rounds, keep the save, and
graph that instead. Everything new in it will be drawn in grey without a name,
which marks an object the game made while it ran rather than one written down in
source — in Animal, exactly the set of animals somebody taught it.

A game played in the browser driver can be graphed the same way. A save is a
mutated copy of the original binary, so the file that comes out of the page
needs no conversion to be read here.

### Gorreven, whose map is its object graph

Containment works the same way, so the same script draws the map:

```shell
./build/archetype --source=games/gorreven.arch --include=games --create=games/gorreven.acx
./demos/object_graph.py games/gorreven.acx --attr location --rankdir LR \
    | dot -Tpng -o gorreven.png
```

That one is a wide forest, hence `--rankdir LR`; it has to be an option here
because `dot -Grankdir=LR` only sets a default, which the drawing overrides.

### Anything else

With no `--attr` at all, every object-valued attribute is drawn, which is a
quick way to see what a game's structure actually is before deciding what is
worth looking at.

Turtle can also come in on standard input, in which case the script never needs
to find the interpreter:

```shell
./build/archetype --inspect=games/gorreven.acx | ./demos/object_graph.py -
```

### What it does not do

It reads the Turtle that `--inspect` emits, in the shape it emits it. It is not
a general Turtle parser, and it will not do anything sensible with Turtle from
somewhere else. For real queries, load the dump into a triplestore and use
SPARQL; this exists to make a picture.

## moving.arch — preconditions, and the one argument left over

```shell
./build/archetype --source=demos/moving.arch
```

It includes nothing, parses nothing, and asks for no input. The protocol is the
whole program.

The shipped protocol in `intrptr.arch` moves a thing in two steps — write the
new location into it, then send it `'MOVE'` — so the thing is told about the
move only after the move has happened. It recovers the origin from
`last_location`, kept for exactly that purpose, and a handler that wants to
refuse can only put things back afterward. `starship_types.arch` has the
canonical version: a power source moved into an occupied socket complains and
writes `location := last_location`.

Nearly all of that is fixed by asking first, and asking needs no arguments.
A precondition is a method that is ABSENT for everything with no objection —
which is almost everything — so only the rare exception mentions it at all:

```
  'CANNOT MOVE' : ABSENT
```

ABSENT is already false enough to fall through an `if`, so the common case
costs nothing and stays invisible. The vise in the demo is the exception, and
it is the entire implementation of being bolted down.

`'ADD SELF'` and `'DROP SELF'` are unchanged, and want no arguments either: the
thing being added is the thing doing the asking, so `sender` already names it.

**One thing is left over**, and it is the only list message in the file. A
precondition about the *destination* has to be given the destination, and a
thing has exactly one channel for that — `sender` — which is already saying
which thing is moving. So the destination rides in the message:

```
  'MOVE TO' : {
    dest_ := head tail message
    if dest_ = location then          TRUE
    else if 'CANNOT MOVE' -> self then   FALSE
    else if 'CANNOT ACCEPT' -> dest_ then FALSE
    else { ... }
    }
```

That is the whole case for arguments, and the whole price of them: one
attribute, `dest_`, in one handler, to give the argument a name. Everything
else in the protocol was already expressible.

Two smaller things it shows:

- **Forwarding is free.** `announced` overrides `'MOVE TO'`, and
  `message --> thing` hands the whole list to the parent, arguments and all,
  with nothing unpacked and nothing rebuilt.
- **Assembly is not movement.** `'ASSEMBLE'` in `intrptr.arch` clears
  `last_location` and calls `'MOVE'`, using the field as a "never placed" flag.
  With the guards moved ahead of the mutation there is no such field, so the
  initial placement gets its own message.

Worth noticing where the capacity check ends up. In `intrptr.arch` the mover
reaches into the destination — `if location.capacity then location.capacity -:=
size` — because it has no way to ask. Here the place answers for itself.

It is a demonstration of a protocol, not a replacement for one. The `'MOVE'`
protocol in `intrptr.arch` is subclassed by shipped games and is not going
anywhere.
