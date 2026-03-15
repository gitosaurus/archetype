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

---

## Running a game

Games are in the `games/` directory and are written in `.arch` source files.

### Compile and run from source

```shell
./build/archetype --source=games/gorreven.arch --include=games
```

`--include` tells the compiler where to find `standard.arch` and other shared library files.

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

### Resume / update (stateless step-by-step)

`--update` loads a `.acx` file, processes one command, and writes the mutated state back to the same file. This is the mechanism underlying the cloud driver:

```shell
./build/archetype --update=gorreven.acx --input="look"
```

The game's response is written to stdout, and the file is updated in place. Running `--update` again from the same file continues exactly where you left off.

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

---

## License

See [LICENSE](LICENSE).
