# Building Archetype Drivers

Archetype can be built and run on its own, or with a variety of
different drivers. This file details how to bootstrap each one.

## Building for the browser (WebAssembly)

The web driver compiles the interpreter to WebAssembly and plays the game
entirely in the page: one `archetype.wasm` the browser caches once, plus a small
`.acx` per game fetched at runtime. There is no server and no shared state — the
universe lives in the tab, which is the difference from the Cloud Run driver
below, where every player shares one blob in a bucket.

### Prerequisites

- [Emscripten](https://emscripten.org/docs/getting_started/downloads.html) on
  `PATH` (`emcc --version` should work). Version 3.1.5x or newer, for `<format>`.
- A native toolchain as well: the native interpreter is what compiles
  `games/*.arch` into the `.acx` files the page fetches.

### Building and serving

```shell
./drivers/web/build.sh
(cd build-wasm/web && python3 -m http.server 8000)
```

Then open <http://localhost:8000/?game=gorreven>. It must be served over HTTP
rather than opened as a `file://` URL, both for the `application/wasm` MIME type
and for `fetch` to work.

To add a game, add an entry to `drivers/web/games.json`; `build.sh` compiles
every `source` it names.

### How a turn works

`games/intrptr.arch` defines the native game loop as
`'START' : { while TRUE do { 'UPDATE' } }`, so one `UPDATE` message to `main` is
exactly one turn. The page never enters `'START'`; it sends one `UPDATE` per
command through `arch_turn` and gets that turn's narrative back. Because nothing
ever blocks on input, the build needs no Asyncify.

### Saves

A `.acx` save is a mutated copy of the game binary, so browser and desktop saves
are the same thing. The page keeps a save in IndexedDB after every turn and
resumes it on the next visit; **Download save** writes a real `.acx` that the
desktop interpreter picks up with `--perform=file.acx` or `--update=file.acx`,
and **Load save** takes one back.

### Known limitation

A verb that reads twice in one turn — `quit`, which asks for confirmation —
cannot get its second answer from a single-line input, so it declines and play
continues. This is a property of the one-`UPDATE`-per-turn model and the Cloud
Run driver behaves identically. Games still end normally through their own
`stop` (winning, dying), at which point the page disables input and offers a
restart.

### Deploying

`.github/workflows/pages.yml` runs `drivers/web/build.sh` and publishes
`build-wasm/web` to GitHub Pages on every push to `main`. It requires the
repository's **Settings → Pages → Source** to be set to **GitHub Actions** once,
by hand.

## Building with GCP Cloud Run

### Prerequisites

- Google Cloud SDK (`gcloud`) installed and authenticated
- Access to a GCP project with Cloud Run enabled
- A Google Cloud Storage bucket named `archetype_web_cards` containing your ACX game files

### Deploying to Cloud Run

From the **repository root directory** (not the `drivers/` directory):

```shell
# Build the image using Cloud Build
gcloud builds submit --config cloudbuild.yaml

# Deploy to Cloud Run
gcloud run deploy archetype \
  --image gcr.io/PROJECT_ID/archetype \
  --platform managed \
  --region us-west1 \
  --allow-unauthenticated
```

Replace `PROJECT_ID` with your actual GCP project ID (e.g., `my-project-123`).

**How it works:**

1. `gcloud builds submit` uses `cloudbuild.yaml` to build the Docker image using `drivers/Dockerfile`
2. `gcloud run deploy` deploys the pre-built image to Cloud Run

**Note:** The build context must be the repository root because the Dockerfile references both `src/` (the main codebase) and `drivers/cloud_run/` (the Python Flask app).

### Testing the Deployment

Once deployed, Cloud Run will provide an endpoint URL. To interact with a game:

```shell
# Set your endpoint (Cloud Run will show this after deployment)
ENDPOINT=https://archetype-HASH-REGION.a.run.app

# Send a command to update a game
curl -X POST $ENDPOINT/update/dtj-spacebits.acx -F command=look

# Other example commands
curl -X POST $ENDPOINT/update/your-game.acx -F command="take sword"
curl -X POST $ENDPOINT/update/your-game.acx -F command="go north"
```

The endpoint expects:

- Path parameter: The name of the ACX file in your `archetype_web_cards` GCS bucket
- Form parameter `command`: The game command to execute

### Building Locally (Optional)

To build and test the Docker image locally before deploying:

```shell
# From the repository root
docker build -f drivers/Dockerfile -t archetype .

# Run locally (requires GCS credentials mounted)
docker run -p 8080:8080 \
  -v ~/.config/gcloud:/root/.config/gcloud \
  -e GOOGLE_APPLICATION_CREDENTIALS=/root/.config/gcloud/application_default_credentials.json \
  archetype
```
