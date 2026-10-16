#!/usr/bin/env python3

import importlib.util
import argparse
import json
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "extract_artifact_digests.py"


def load_module():
    spec = importlib.util.spec_from_file_location("extract_artifact_digests", SCRIPT_PATH)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


extractor = load_module()


class ExtractArtifactDigestsTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def test_extracts_archive_and_sbom_digests(self) -> None:
        archive_hash = "a" * 64
        sbom_payload = b'{"sbom":"cyclonedx"}'
        artifact_zip = self.root / "artifact.zip"
        with zipfile.ZipFile(artifact_zip, "w") as zf:
            zf.writestr(
                "release/SHA256SUMS.txt",
                f"{archive_hash}  themisdb-2.6.0-linux-amd64.zip\n",
            )
            zf.writestr("release/themisdb.sbom.json", sbom_payload)

        result = extractor.extract_digests(self.root)
        self.assertEqual(result["artifact_digest"], archive_hash)
        self.assertEqual(result["sbom_digest"], extractor.hashlib.sha256(sbom_payload).hexdigest())

    def test_returns_unknown_when_not_found(self) -> None:
        empty_zip = self.root / "empty.zip"
        with zipfile.ZipFile(empty_zip, "w") as zf:
            zf.writestr("README.txt", "no digests here")

        result = extractor.extract_digests(self.root)
        self.assertEqual(result["artifact_digest"], "unknown")
        self.assertEqual(result["sbom_digest"], "unknown")

    def test_cli_writes_json(self) -> None:
        zip_path = self.root / "art.zip"
        with zipfile.ZipFile(zip_path, "w") as zf:
            zf.writestr(
                "SHA256SUMS.txt",
                f"{'b'*64}  themisdb-2.6.0-linux-amd64.tgz\n",
            )
            zf.writestr("build/sbom.sha256", f"{'c'*64}  sbom.json\n")

        out = self.root / "digests.json"
        args = argparse.Namespace(
            artifacts_dir=str(self.root),
            output=str(out),
        )
        original_parse_args = extractor.parse_args
        extractor.parse_args = lambda: args  # type: ignore[assignment]
        try:
            exit_code = extractor.main()
        finally:
            extractor.parse_args = original_parse_args  # type: ignore[assignment]
        self.assertEqual(exit_code, 0)
        payload = json.loads(out.read_text(encoding="utf-8"))
        self.assertEqual(payload["artifact_digest"], "b" * 64)
        self.assertEqual(payload["sbom_digest"], "c" * 64)


if __name__ == "__main__":
    unittest.main()
