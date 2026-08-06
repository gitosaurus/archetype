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

## Known open question

These goldens were generated on macOS/libc++. Whether the bytes are
*toolchain-independent* has never been tested — CI's native job is
Linux/libstdc++, and this check running there is the first real test of it.

The `.acx` half failing on Linux while the `.ttl` half passes would be the
signal: same world, different bytes, meaning the binary format has a
libc++/libstdc++ dependence worth finding. `check.sh` reports that case
specifically rather than lumping it in with ordinary drift. The fix would be to
scope the byte comparison to one platform and treat the Turtle as the
cross-platform oracle.
