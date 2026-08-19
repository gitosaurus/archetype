# About Archetype

Archetype is a simple, stripped-down object-oriented programming language
designed for the writing of text adventure games. The interpreter has a partial
natural language parser, and the language's `write` statement pauses when it
has filled the screen, so that long pieces of text do not get lost off the top.

However, Archetype does not have everything necessary for quickly creating an
adventure game, because the language is more general-purpose than that. There
are about a thousand lines of code in the files `intrptr.arch`, `lexicon.arch`,
and `cardinal.arch`, all of which are included when your adventure includes
`standard.arch`. Thus much of Archetype is itself written in Archetype.

The language is a translator and an interpreter in one program. The interpreter
expects a file that has been syntactically and semantically verified by the
translator and turned into a binary format. This half-a-compiler/interpreter
pair design is found in other languages, such as Icon and certain versions of
LISP. It is not a full interpreter, such as BASIC, nor a full compiler, such as
Pascal.

```shell
archetype --source=mygame.arch --include=games --create   # translate
archetype --perform=mygame.acx                            # interpret
```

## Where it came from

My own computer, a TI-99/4A, had a sprite library: an abstraction under which a
shape animated itself, kept moving once dispatched, and noticed when it
overlapped another. Using it taught me the extraordinary value of having the
right abstractions for a given problem space. What I wanted for text adventures
was something like sprites, but for objects which could be described by text
and addressed with natural language. The clue arrived in a one-hour lecture on
Smalltalk in a Comparative Programming Languages course: I understood the basic
premise right away, the way it turned conventional programming inside-out.
Instead of being verb-oriented, it was noun-oriented. Of course that's how
you'd write a text adventure.

I settled on the design right before graduating in late 1990 and began building
it. Around the same time I found out about a text adventure authoring system
called AdvSys, written in LISP by David Betz, who had also written XLISP 2.0.
It was, no surprises, also object-oriented. Upon this discovery my crest was
felled and a great deal of wind left my sails. But LISP is a powerful language,
not an easy one, and I had designed Archetype to be so simple that even a
non-programmer could use it, with a forgiving syntax that was strongly oriented
toward the text adventure domain. The world could still use it. Somewhere.

I did not know then that Infocom had solved the same problem years earlier with
an internally developed language called ZIL. How could I have known? It was the
closely guarded trade secret of a company making ten million dollars a year at
its peak, long before the open source ethos had proven itself in the global
marketplace.

## A little history

Archetype was developed using Turbo Pascal 5.5 on an IBM-compatible laptop with
an 8088 processor, two disk drives (no hard drive), and a 10 MHz clock. Because
of this, the compiler and interpreter were designed to minimize disk use,
getting things into memory as quickly as possible and working with them there.
As a result it made tremendous use of dynamic memory. On a higher-end machine,
the disk may actually be faster than dynamically allocating memory, so that
architecture did not run as much faster as you might expect on a faster
machine.

Archetype 1.0 was distributed as shareware. Archetype 1.01 was distributed
public-domain, along with its Turbo Pascal source code and the Archetype source
code for the adventures.

Version 3.0 was a complete reimplementation in modern C++, and is the one you
are reading about here. The language it runs is the same language, and the
adventures written for the Turbo Pascal version still play; but the interpreter
is no longer fighting for memory on a machine with two floppy drives, and a
number of the old limits went away with the machine that imposed them. Version
4.0 added lists and list-headed message dispatch. The whole thing is now on
GitHub under the MIT license.

## An invitation

I'd like to know what you think of both the games in this archive and the
language itself. If you would like to try writing an adventure game, I would
really like to know how easy or difficult you found it using Archetype and the
`standard.arch` include files.

But what I would like to know even more is if you can think of an application
for Archetype other than writing adventure games. Somebody once said that the
best tool is one which is used for a purpose other than the one for which it
was designed. The file `animal.arch` in this archive shows one different way to
use Archetype.

Enjoy!

Derek T. Jones
