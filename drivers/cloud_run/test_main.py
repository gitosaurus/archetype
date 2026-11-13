"""Tests for the Cloud Run driver.

Neither Cloud Storage nor the interpreter is involved: the bucket is a
dictionary and the interpreter is a function.  Run from this directory with

    python -m unittest
"""

import unittest
from subprocess import CompletedProcess
from unittest import mock

from google.api_core.exceptions import PreconditionFailed

import main


class FakeBlob:
    def __init__(self, bucket, name):
        self.bucket = bucket
        self.name = name
        self.generation = bucket.generations[name]

    def download_to_filename(self, filename):
        with open(filename, 'wb') as f:
            f.write(self.bucket.contents[self.name])

    def upload_from_filename(self, filename, if_generation_match=None):
        current = self.bucket.generations[self.name]
        if if_generation_match is not None and if_generation_match != current:
            raise PreconditionFailed('generation mismatch')
        with open(filename, 'rb') as f:
            self.bucket.contents[self.name] = f.read()
        self.bucket.generations[self.name] = current + 1


class FakeBucket:
    def __init__(self, **contents):
        self.contents = dict(contents)
        self.generations = {name: 1 for name in contents}

    def get_blob(self, name):
        return FakeBlob(self, name) if name in self.contents else None


def interpreter(before_exit=None, returncode=0, stderr=b''):
    """A stand-in for archetype --update: appends the input to the file."""
    def run(argv, capture_output):
        options = dict(arg[2:].split('=', 1) for arg in argv[1:] if '=' in arg)
        if returncode == 0:
            with open(options['update'], 'ab') as f:
                f.write(options['input'].encode() + b';')
        if before_exit:
            before_exit()
        narrative = f"You {options['input']}.\n".encode() if returncode == 0 else b''
        return CompletedProcess(argv, returncode, narrative, stderr)
    return run


class UpdateTest(unittest.TestCase):
    def setUp(self):
        self.bucket = FakeBucket(**{'game.acx': b''})
        patcher = mock.patch.object(main, 'get_bucket', return_value=self.bucket)
        patcher.start()
        self.addCleanup(patcher.stop)
        self.client = main.app.test_client()

    def post(self, command, adventure='game.acx'):
        return self.client.post(f'/update/{adventure}', data={'command': command})

    def test_turn_is_taken_and_stored(self):
        with mock.patch.object(main.subprocess, 'run', interpreter()):
            response = self.post('look')
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.get_data(as_text=True), 'You look.\n')
        self.assertEqual(self.bucket.contents['game.acx'], b'look;')

    def test_missing_adventure_is_not_found(self):
        with mock.patch.object(main.subprocess, 'run') as run:
            response = self.post('look', adventure='no_such.acx')
        self.assertEqual(response.status_code, 404)
        run.assert_not_called()

    def test_failed_turn_is_an_error_and_stores_nothing(self):
        failing = interpreter(returncode=1, stderr=b'ERROR: Not an Archetype binary\n')
        with mock.patch.object(main.subprocess, 'run', failing):
            response = self.post('look')
        self.assertEqual(response.status_code, 500)
        # What went wrong is for the log, not for the player.
        self.assertNotIn('Not an Archetype binary', response.get_data(as_text=True))
        self.assertEqual(self.bucket.contents['game.acx'], b'')
        self.assertEqual(self.bucket.generations['game.acx'], 1)

    def test_ended_adventure_is_gone_rather_than_broken(self):
        ended = interpreter(returncode=1, stderr=b'ERROR: Universe has ended\n')
        with mock.patch.object(main.subprocess, 'run', ended):
            response = self.post('look')
        self.assertEqual(response.status_code, 410)
        self.assertEqual(self.bucket.generations['game.acx'], 1)

    def test_overlapping_turn_is_not_lost(self):
        # While this turn is with the interpreter, another finishes first.
        def somebody_else():
            self.bucket.contents['game.acx'] += b'north;'
            self.bucket.generations['game.acx'] += 1
        interruptions = iter([somebody_else])

        def interrupted_once():
            for interrupt in interruptions:
                interrupt()
                break

        with mock.patch.object(main.subprocess, 'run', interpreter(interrupted_once)):
            response = self.post('look')
        self.assertEqual(response.status_code, 200)
        # Both turns are there, in the order they were stored; the old driver
        # would have left b'look;' and nothing else.
        self.assertEqual(self.bucket.contents['game.acx'], b'north;look;')

    def test_adventure_that_stays_busy_is_a_conflict(self):
        def always_interrupted():
            self.bucket.generations['game.acx'] += 1

        with mock.patch.object(main.subprocess, 'run', interpreter(always_interrupted)):
            response = self.post('look')
        self.assertEqual(response.status_code, 409)
        self.assertEqual(self.bucket.contents['game.acx'], b'')


if __name__ == '__main__':
    unittest.main()
