# Archetype

**Archetype** is a programming language designed originally for writing text-based adventure games. Its syntax is object-oriented and message-passing, making it natural to model the rooms, objects, and actors of an interactive fiction world. A brief introduction and historical notes can be found at [derektjones.net/archetype](https://www.derektjones.net/archetype/archetype.html).

The language was originally created in the early 1990s in Turbo Pascal and has been fully reimplemented in modern C++. Compiled game files (`.acx`) are fully resumable, so that a save file is simply a mutated copy of the original binary, which makes it straightforward to deploy an adventure as a stateless cloud service (see [Cloud deployment](#cloud-deployment) below).

---

## Building

The preferred build method uses CMake:

```shell
cmake -S src -B build
cmake --build build
```

This produces a `build/archetype` binary. The source is also simple enough that a direct compile works:

```shell
c++ src/*.cc -o archetype
```

### Language standard

Archetype requires **C++20**. The interpreter uses concepts, `std::span`, `std::source_location`, `std::format`, designated initializers and scoped enumerations throughout.

### A note on modules

C++20 modules are deliberately *not* used, neither for Archetype's own code nor by way of `import std;`. This is a build-tooling limitation rather than an architectural one — the header graph is a clean DAG with natural module boundaries, and `import std;` does compile against the module source that libc++ ships at `/usr/share/libc++/v1/std.cppm`.

The obstacle is that CMake's `import std` support needs a `modules.json` manifest describing where that module source lives and how to build it, and Apple's libc++ does not ship one. Without it, using modules means building and tracking `.pcm` files outside the build system, keeping them in sync by hand, and pinning every developer to the same toolchain version — a `.pcm` is invalidated by a compiler update.

The usual reward for all of that is compile-time scaling, and Archetype does not have a compile-time problem: a clean parallel build of its 42 translation units takes about three seconds. The trade will be worth revisiting once CMake ships `modules.json` support for Apple libc++ out of the box, at which point adopting `import std;` becomes a build-system setting plus a mechanical sweep of `#include` lines.

---

## Running a game

Games are in the `games/` directory and are written in `.arch` source files.

### Compile and run from source

```shell
./build/archetype --source=games/gorreven.arch --include=games
```

`--include` tells the compiler where to find `standard.arch` and other shared library files.

An option that always needs a value takes it either way, so `--source games/gorreven.arch` works as well as `--source=games/gorreven.arch`. The exceptions are the options that can stand alone — `--create`, `--autosave`, and `--repl` — where a following word cannot be told apart from an unrelated argument, so `=` is the only way to give them one. Run `--help` for the full listing.

### Compile to a binary, then run

This is the preferred method if you are intending to share your game as a single file.

Compile once to a `.acx` binary:

```shell
./build/archetype --source=games/gorreven.arch --include=games --create
# Creates: gorreven.acx
```

Then run the compiled binary:

```shell
./build/archetype --perform=gorreven.acx
```

### Autosave

`--autosave` keeps the state of a game on disk as it is played, rather than only at the moments a player remembers to type `save`:

```shell
./build/archetype --perform=games/gorreven.acx --autosave
```

Without a filename, the state is written *alongside* the game rather than over it: `gorreven.acx` becomes `gorreven.save.acx`, and the original binary is left untouched. This matters because a `.acx` holds the program and the state together — autosaving over the distributed binary would leave you with no way to start a fresh game. Resuming a save keeps updating that same save:

```shell
./build/archetype --perform=games/gorreven.save.acx --autosave
```

Give a filename to choose the target yourself. Naming the file being played is allowed, and is then a deliberate act:

```shell
./build/archetype --perform=mygame.acx --autosave=mygame.acx
```

By default a checkpoint is written after every completed turn and again on the way out, whether the player types `quit` or presses `^D`. Per-turn is what makes the save survive a `^C`, a closed terminal, or a crash, none of which give the interpreter a chance to run any exit code. Use `--autosave-at=exit` to checkpoint only when exiting cleanly:

```shell
./build/archetype --perform=games/gorreven.acx --autosave --autosave-at=exit
```

Each checkpoint rotates the previous one to `<file>.bak`, so the state as of the turn before last is always recoverable — copy it back over the save. That is one turn of undo, not an undo stack; the `save` command remains the way to keep a checkpoint you can return to later.

Saves are written to a temporary file and renamed into place, so an interrupted write cannot leave a half-written game behind. If a checkpoint fails, the interpreter says so on stderr and turns autosave off for the rest of the session rather than failing silently every turn.

### Resume / update (stateless step-by-step)

`--update` loads a `.acx` file, processes one command, and writes the mutated state back to the same file. This is the mechanism underlying the cloud driver:

```shell
./build/archetype --update=gorreven.acx --input="look"
```

The game's response is written to stdout, and the file is updated in place. Running `--update` again from the same file continues exactly where you left off. Add `--width=N` to wrap that output at something other than 80 columns.

A universe draws its random seed the first time it is played and stores it in the binary, so an unseeded run is different every time. `--seed=N` forces it, which makes a whole scripted run repeatable — seed the first turn and the rest follow from the stored state:

```shell
./build/archetype --update=gorreven.acx --input="look" --seed=42
./build/archetype --update=gorreven.acx --input="north"
```

`--seed` applies to whichever mode plays the game — `--source`, `--perform`, `--repl`, or `--update`. It is rejected with `--create`, which never plays and so has no generator to seed.

---

## The REPL

Archetype includes an interactive Read-Eval-Print Loop for general exploration:

```shell
./build/archetype --repl
```

From the REPL you can load a binary, inspect state, and send messages directly to objects:

```
> "gorreven.acx" -> system
> 'UPDATE' -> main
```

---

## Running the tests

```shell
./build/archetype --test
```

---

## Cloud deployment

Archetype's `--update` mode powers a GCP Cloud Run driver that lets a player step through a game one command at a time via HTTP. The game state lives in a Google Cloud Storage bucket as a `.acx` file that is updated on every request.

Full build and deployment instructions are in [`drivers/README.md`](drivers/README.md). A quick taste:

```shell
# Deploy
gcloud builds submit --config cloudbuild.yaml
gcloud run deploy archetype --image gcr.io/PROJECT_ID/archetype \
  --platform managed --region us-west1 --allow-unauthenticated

# Play via curl
ENDPOINT=https://archetype-HASH-REGION.a.run.app
curl -X POST $ENDPOINT/update/gorreven.acx -F command=look
curl -X POST $ENDPOINT/update/gorreven.acx -F command="go north"
```

---

## Repository layout

| Path | Contents |
|---|---|
| `src/` | C++ source for the Archetype interpreter |
| `games/` | Adventure game sources (`.arch`) and compiled binaries (`.acx`) |
| `drivers/` | Cloud Run driver (Python/Flask) and its `README.md` |
| `demos/` | Optional demonstrations; nothing here is needed to build or play |

---

## License

See [LICENSE](LICENSE).
