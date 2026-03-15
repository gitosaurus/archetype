# CLAUDE.md

## Project overview

Archetype is a message-passing, object-oriented programming language for writing text-based adventure games. The interpreter is implemented in C++11. Game source files use the `.arch` extension; compiled binaries use `.acx`.

## Build

```shell
cmake -S src -B build
cmake --build build
```

## Test

```shell
./build/archetype --test
```

Always run the test suite and verify it passes before committing changes to C++ source.

## Smoke-test a game

```shell
echo "look" | ./build/archetype --source=games/gorreven.arch --include=games
```

## Repository structure

- `src/` — C++ source for the interpreter
- `games/` — Game sources (`.arch`) and compiled binaries (`.acx`)
- `drivers/` — Cloud Run driver (Python/Flask); see `drivers/README.md`
- `archetype-mode.el` — Emacs major mode for syntax highlighting

## Key conventions

- `include "file"` in Archetype source uses implicit `.arch` extension; do not include the extension explicitly.
- Compiled `.acx` files are fully resumable — a save file is a mutated copy of the original binary.
- The `--include=games` flag is needed when compiling from source so the compiler can find `standard.arch` and other shared library files.
