# Building Archetype Drivers

Archetype can be built and run on its own, or with a variety of
different drivers. This file details how to bootstrap each one.

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
