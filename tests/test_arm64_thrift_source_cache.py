import hashlib
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

import yaml


ROOT = Path(__file__).resolve().parents[1]
PINNED_SHA512 = (
    "0570486bd6e4aa95a2eb329ad01a867a7999f7a936248e8fa3312a2dd20"
    "e247b7677d58f84a8e05e82de6bbb09647da2fe0e6aa720e825468e9f9b479831e76a"
)


class Arm64ThriftSourceCacheTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        workflow = yaml.safe_load(
            (ROOT / ".github/workflows/reusable-cmake-build.yml").read_text()
        )
        cls.steps = workflow["jobs"]["build"]["steps"]
        cls.step = next(s for s in cls.steps if s.get("name") == "Seed ARM64 Thrift source cache")
        cls.command = cls.step["run"]
        assert PINNED_SHA512 in cls.command

    def run_step(self, directory, source, expected, existing=None, curl_status=0):
        cache = directory / "download cache"
        cache.mkdir()
        archive = cache / "thrift-0.25.0.tar.gz"
        if existing is not None:
            archive.write_bytes(existing)
        fake_bin = directory / "bin"
        fake_bin.mkdir()
        curl = fake_bin / "curl"
        curl.write_text(
            '#!/bin/bash\n'
            'echo called >> "$CURL_CALLS"\n'
            'while [ "$1" != "--output" ]; do shift; done\n'
            'cp "$CURL_SOURCE" "$2"\n'
            'exit "$CURL_STATUS"\n'
        )
        curl.chmod(0o755)
        source_path = directory / "source"
        source_path.write_bytes(source)
        calls = directory / "calls"
        command = self.command.replace(PINNED_SHA512, hashlib.sha512(expected).hexdigest())
        result = subprocess.run(
            ["bash", "-euo", "pipefail", "-c", command],
            env={**os.environ, "PATH": f"{fake_bin}:{os.environ['PATH']}",
                 "VCPKG_DOWNLOADS": str(cache), "CURL_SOURCE": str(source_path),
                 "CURL_CALLS": str(calls), "CURL_STATUS": str(curl_status)},
            capture_output=True, text=True,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(list(cache.glob(".thrift-*")), [])
        return archive, calls, result.stdout

    def test_scoped_before_configure(self):
        self.assertIn("runner.os == 'Linux'", self.step["if"])
        self.assertIn("runner.arch == 'ARM64'", self.step["if"])
        self.assertIn("inputs.vcpkg_triplet == 'arm64-linux'", self.step["if"])
        self.assertLess(self.steps.index(self.step), next(
            i for i, step in enumerate(self.steps) if step.get("name") == "Configure"
        ))

    def test_download_replaces_invalid_cache_after_verification(self):
        with tempfile.TemporaryDirectory() as temp:
            archive, calls, _ = self.run_step(
                Path(temp), b"valid source", b"valid source", existing=b"invalid"
            )
            self.assertEqual(archive.read_bytes(), b"valid source")
            self.assertTrue(calls.exists())

    def test_verified_cache_avoids_download(self):
        with tempfile.TemporaryDirectory() as temp:
            archive, calls, _ = self.run_step(
                Path(temp), b"other", b"valid source", existing=b"valid source"
            )
            self.assertEqual(archive.read_bytes(), b"valid source")
            self.assertFalse(calls.exists())

    def test_bad_or_failed_download_is_not_installed(self):
        for source, status in ((b"invalid", 0), (b"valid source", 22)):
            with self.subTest(status=status), tempfile.TemporaryDirectory() as temp:
                archive, _, output = self.run_step(
                    Path(temp), source, b"valid source", curl_status=status
                )
                self.assertFalse(archive.exists())
                self.assertIn("vcpkg will use its configured source", output)

    def test_bad_download_does_not_replace_existing_cache(self):
        with tempfile.TemporaryDirectory() as temp:
            archive, _, _ = self.run_step(
                Path(temp), b"invalid mirror", b"valid source", existing=b"invalid cache"
            )
            self.assertEqual(archive.read_bytes(), b"invalid cache")


if __name__ == "__main__":
    unittest.main()
