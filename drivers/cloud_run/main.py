import os
import subprocess
from tempfile import TemporaryDirectory

from flask import Flask, request
from flask_cors import CORS
from google.api_core.exceptions import PreconditionFailed
from google.cloud import storage

BUCKET = 'archetype_web_cards'
ARCHETYPE = '/usr/local/bin/archetype'

# How many times a turn is taken again from the top when somebody else's
# turn got to the adventure first.
ATTEMPTS = 3

app = Flask(__name__)
CORS(app)

_client = None


def get_bucket():
    # Made on first use rather than at import, so that importing this module
    # (as a test does) needs no credentials.
    global _client
    if _client is None:
        _client = storage.Client()
    return _client.get_bucket(BUCKET)


@app.route('/update/<adventure>', methods=['POST'])
def update(adventure: str):
    command = request.form.get('command', 'wait')
    bucket = get_bucket()
    for _ in range(ATTEMPTS):
        blob = bucket.get_blob(adventure)
        if blob is None:
            return f'There is no adventure named {adventure}.\n', 404
        with TemporaryDirectory() as temp_dir:
            local_data = os.path.join(temp_dir, 'local.acx')
            blob.download_to_filename(local_data)
            run = subprocess.run([
                ARCHETYPE,
                '--width=80', '--silent',
                f'--update={local_data}',
                f'--input={command}'], capture_output=True)
            if run.returncode != 0:
                # The interpreter leaves the file alone when a turn fails, and
                # so the stored adventure is left alone too.
                complaint = run.stderr.decode(errors='replace').strip()
                if 'Universe has ended' in complaint:
                    return 'This adventure has ended.\n', 410
                app.logger.error('archetype exited %d on %s: %s',
                                 run.returncode, adventure, complaint)
                return 'The adventure could not take that turn.\n', 500
            try:
                # Only if the adventure is still the one this turn started
                # from.  Two turns that overlap would otherwise both begin at
                # the same state, and the second to finish would erase the
                # first.
                blob.upload_from_filename(
                    local_data, if_generation_match=blob.generation)
            except PreconditionFailed:
                continue
            return run.stdout.decode()
    return 'The adventure is busy; try that again.\n', 409


if __name__ == "__main__":
    app.run(debug=True, host="0.0.0.0", port=int(os.environ.get("PORT", 8080)))
