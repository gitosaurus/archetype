# CLAUDE.md

## Project overview

Archetype is a message-passing, object-oriented programming language for writing text-based adventure games. The interpreter is implemented in C++20. Game source files use the `.arch` extension; compiled binaries use `.acx`.

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

## Build the browser version

Requires Emscripten on `PATH`. Builds the wasm interpreter, compiles the games
with the native one, and assembles everything into `build-wasm/web`:

```shell
./drivers/web/build.sh
(cd build-wasm/web && python3 -m http.server 8000)
```

## Compile a game to binary

```shell
./build/archetype --source=games/gorreven.arch --include=games --create=games/gorreven.acx
```

## Smoke-test a game

```shell
echo "look" | ./build/archetype --source=games/gorreven.arch --include=games
```

## Repository structure

- `src/` — C++ source for the interpreter
- `games/` — Game sources (`.arch`) and compiled binaries (`.acx`)
- `drivers/` — Cloud Run driver (Python/Flask) and the WebAssembly web driver
  (`drivers/web/`); see `drivers/README.md`
- `demos/` — Optional demonstrations, standard library only; nothing in `src/`
  refers to them and nothing builds them
- `archetype-mode.el` — Emacs major mode for syntax highlighting

## Key conventions

- `include "file"` in Archetype source uses implicit `.arch` extension; do not include the extension explicitly.
- Compiled `.acx` files are fully resumable — a save file is a mutated copy of the original binary.
- The `--include=games` flag is needed when compiling from source so the compiler can find `standard.arch` and other shared library files.
