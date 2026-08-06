# Golden files

Two compiled games, pinned as they come out of the interpreter today, so that an
unintended change to the compiler or the binary format has to argue for itself
in a diff.

| file         | what it pins                                            |
|--------------|---------------------------------------------------------|
| `*.ttl`      | the world state, as RDF/Turtle — text, reviewable        |
| `*.acx`      | the compiled binary, byte for byte                       |

The Turtle is the one to read. When a check fails it diffs readably and names
the object that drifted. The `.acx` is there for the layer the Turtle cannot
see — varint encoding, field order, the exact ids handed out — where a mismatch
tells you only *that* something moved.

`cherry.arch` is 51 lines and `bare.arch` is 433; both are small enough that the
binaries do not weigh on the history when the format changes.

## Running

```shell
cmake -S src -B build && cmake --build build
./tests/golden/check.sh
```

`check.sh` compiles each game twice before it compares anything, so a compiler
that has stopped being deterministic is reported as its own failure rather than
as a stale golden.

## When a check fails

If the change was intended:

```shell
./tests/golden/regenerate.sh
git diff -- tests/golden/*.ttl        # this is the reviewable half
```

Commit the `.ttl` and the `.acx` together, and say in the message what moved.

## What is deliberately *not* pinned

Only **compiled** output. A post-turn save is not a valid golden: eight runs of
`--update --input="north"` against `gorreven` produce four distinct binaries,
because the game rolls dice. That nondeterminism is the game's, not the
serializer's — it reproduces identically on any build. Pin what the compiler
emits, never what a turn leaves behind.

## An `.acx` is toolchain-independent

This was an open question when the goldens went in, and the first CI run
answered it. The files here were generated on macOS with Clang and libc++; CI's
native job is Linux with GCC and libstdc++, and it compiled both games to bytes
identical to these. Different compiler, different standard library, same binary.

That is worth stating because nothing guarantees it in general. A serializer
that leaned on `std::map` iteration order, on a hash, or on any container whose
layout the standard leaves open would drift between the two — and it would drift
silently, because the world state would still be correct. That is the case
`check.sh` calls out on its own: same `.ttl`, different `.acx`.

So this check is now doing two jobs at once. It guards against unintended format
changes, and it stands watch over portability. If the `.acx` half ever fails on
Linux while the `.ttl` half passes, something has acquired a dependence on the
standard library's choices, and the failure names the byte offset to start from.
The fallback would be to scope the byte comparison to one platform and let the
Turtle be the cross-platform oracle — but that would be a retreat, and it is not
needed today.
