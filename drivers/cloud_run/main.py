import os

from flask import Flask, request
from flask_cors import CORS
from google.cloud import storage
from tempfile import TemporaryDirectory
import subprocess

app = Flask(__name__)
CORS(app)
client = storage.Client()


@app.route('/update/<adventure>', methods=['POST'])
def update(adventure: str):
    command = request.form.get('command', 'wait')
    bucket = client.get_bucket('archetype_web_cards')
    blob = bucket.get_blob(adventure)
    with TemporaryDirectory() as temp_dir:
        local_data = temp_dir + '/local.acx'
        blob.download_to_filename(local_data)
        run = subprocess.run([
            '/usr/local/bin/archetype',
            '--width=80', '--silent',
            f'--update={local_data}',
            '--input={}'.format(command)], capture_output=True)
        narrative = run.stdout.decode()
        blob.upload_from_filename(local_data)
    return narrative


if __name__ == "__main__":
    app.run(debug=True, host="0.0.0.0", port=int(os.environ.get("PORT", 8080)))
