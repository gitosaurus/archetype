# Building Archetype Drivers

Archetype can be built and run on its own, or with a variety of
different drivers.  This file details how to bootstrap each one.

## Building with GCP Cloud Run

In this directory:

```shell
gcloud run deploy --source . archetype
```

To then exercise the Cloud Run endpoint, add `/update/` and the
name of the ACX file in the `archetype_web_cards` GCS bucket.

```shell
ENDPOINT=https://archetype-d656yixu4q-uw.a.run.app
curl -X POST $ENDPOINT/update/dtj-spacebits.acx -F command=look
```
