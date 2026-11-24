"""Tests for scripts/release/resolve_release_metadata.py (GitHub release/repository metadata resolver)."""

import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RESOLVER = ROOT / "scripts/release/resolve_release_metadata.py"
REPO = "makr-code/ThemisDB"

FAKE_GH = """#!/usr/bin/env python3
import json, os, sys
with open(os.environ["GH_CALL_LOG"], "a", encoding="utf-8") as log:
    log.write(" ".join(sys.argv[1:]) + "\\n")
responses = json.load(open(os.environ["FAKE_GH_RESPONSES"], encoding="utf-8"))
path = sys.argv[-1]
if sys.argv[1] != "api" or path not in responses:
    print("gh: Not Found (HTTP 404)", file=sys.stderr)
    sys.exit(1)
print(json.dumps(responses[path]))
"""


def repository_response(**overrides):
    data = {
        "html_url": f"https://github.com/{REPO}",
        "description": "Themis Database System - High-performance C++ hybrid-database",
        "homepage": "https:\\\\www.themisdb.org",
        "license": {"key": "other", "spdx_id": "NOASSERTION"},
        "default_branch": "develop",
        "owner": {"login": "makr-code"},
        "topics": ["Database", "graph", "vector", "graph", "has space", "x" * 41],
    }
    data.update(overrides)
    return data


def release_response(tag, prerelease=False, draft=False, published_at="2026-11-20T10:00:00Z", assets=None, body="Notes"):
    if assets is None:
        assets = [{
            "name": f"ThemisDB-COMMUNITY-{tag[1:]}-windows-x64.zip",
            "browser_download_url": f"https://github.com/{REPO}/releases/download/{tag}/ThemisDB-COMMUNITY-{tag[1:]}-windows-x64.zip",
            "size": 1024,
            "digest": "sha256:" + "a" * 64,
        }, {
            "name": f"ThemisDB-COMMUNITY-{tag[1:]}-linux-amd64.tar.gz",
            "browser_download_url": f"https://github.com/{REPO}/releases/download/{tag}/linux.tar.gz",
            "size": 2048,
        }]
    return {
        "tag_name": tag,
        "html_url": f"https://github.com/{REPO}/releases/tag/{tag}",
        "prerelease": prerelease,
        "draft": draft,
        "published_at": published_at,
        "body": body,
        "assets": assets,
    }


class ReleaseMetadataResolverTests(unittest.TestCase):
    def setUp(self):
        self._temp = tempfile.TemporaryDirectory()
        self.directory = Path(self._temp.name)
        binaries = self.directory / "bin"
        binaries.mkdir()
        gh = binaries / "gh"
        gh.write_text(FAKE_GH)
        gh.chmod(0o755)
        self.path = f"{binaries}{os.pathsep}{os.environ['PATH']}"
        (self.directory / "LICENSE").write_text(
            "MIT License with Government Clause\n\nCopyright (c) 2025 The ThemisDB Authors\n"
        )
        (self.directory / "VERSION").write_text("2.4.0\n")
        self.responses = {f"repos/{REPO}": repository_response()}

    def tearDown(self):
        self._temp.cleanup()

    def run_resolver(self, *args, repo=REPO):
        (self.directory / "responses.json").write_text(json.dumps(self.responses))
        output = self.directory / "github-output"
        output.write_text("")
        metadata_path = self.directory / "metadata.json"
        if metadata_path.exists():
            metadata_path.unlink()
        result = subprocess.run(
            [sys.executable, str(RESOLVER), "--repo", repo, "--repo-root", str(self.directory),
             "--output", str(metadata_path), "--github-output", str(output), *args],
            cwd=self.directory,
            env={**os.environ, "PATH": self.path, "FAKE_GH_RESPONSES": str(self.directory / "responses.json"),
                 "GH_CALL_LOG": str(self.directory / "gh-calls")},
            capture_output=True, text=True,
        )
        outputs = dict(line.split("=", 1) for line in output.read_text().splitlines() if "=" in line)
        metadata = json.loads(metadata_path.read_text()) if metadata_path.exists() else None
        return result, metadata, outputs

    def set_releases(self, *releases):
        self.responses = {key: value for key, value in self.responses.items() if "/releases" not in key}
        self.responses[f"repos/{REPO}/releases?per_page=100"] = list(releases)
        for release in releases:
            self.responses[f"repos/{REPO}/releases/tags/{release['tag_name']}"] = release

    def test_repository_metadata_falls_back_to_license_file_and_repository_url(self):
        self.set_releases(release_response("v2.4.0"))
        result, metadata, outputs = self.run_resolver("--version", "2.4.0")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(metadata["license"], "MIT License with Government Clause")
        self.assertEqual(metadata["license_spdx"], "")
        self.assertEqual(metadata["publisher"], "The ThemisDB Authors")
        self.assertEqual(metadata["copyright"], "Copyright (c) 2025 The ThemisDB Authors")
        self.assertEqual(metadata["homepage"], f"https://github.com/{REPO}")
        self.assertIn("is not a valid http(s) URL", result.stderr)
        self.assertEqual(metadata["publisher_support_url"], f"https://github.com/{REPO}/issues")
        self.assertEqual(metadata["license_url"], f"https://github.com/{REPO}/blob/v2.4.0/LICENSE")
        self.assertEqual(metadata["tags"], ["database", "graph", "vector"])
        self.assertEqual(outputs["license"], "MIT License with Government Clause")
        self.assertEqual(outputs["source"], "explicit")

    def test_spdx_license_and_valid_homepage_are_used(self):
        self.responses[f"repos/{REPO}"] = repository_response(
            license={"spdx_id": "MIT"}, homepage="https://www.themisdb.org")
        self.set_releases(release_response("v2.4.0"))
        result, metadata, _ = self.run_resolver("--version", "v2.4.0")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(metadata["license"], "MIT")
        self.assertEqual(metadata["homepage"], "https://www.themisdb.org")

    def test_missing_description_or_license_fails_with_clear_error(self):
        self.set_releases(release_response("v2.4.0"))
        self.responses[f"repos/{REPO}"] = repository_response(description="  ")
        result, metadata, _ = self.run_resolver("--version", "2.4.0")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("no GitHub description", result.stderr)
        self.assertIsNone(metadata)

        self.responses[f"repos/{REPO}"] = repository_response(license=None)
        (self.directory / "LICENSE").unlink()
        result, _, _ = self.run_resolver("--version", "2.4.0")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("License for", result.stderr)

    def test_long_description_and_release_notes_are_truncated(self):
        self.responses[f"repos/{REPO}"] = repository_response(description="d" * 400)
        self.set_releases(release_response("v2.4.0", body="n" * 20000))
        result, metadata, _ = self.run_resolver("--version", "2.4.0")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertLessEqual(len(metadata["description"]), 256)
        self.assertLessEqual(len(metadata["release_notes"]), 10000)
        self.assertIn("truncated", result.stderr)

    def test_auto_resolution_selects_latest_published_stable_release(self):
        self.set_releases(
            release_response("v2.3.0", published_at="2026-10-01T00:00:00Z"),
            release_response("v2.4.0", published_at="2026-11-01T00:00:00Z"),
            release_response("v2.5.0-alpha", prerelease=True, published_at="2026-11-10T00:00:00Z"),
            release_response("v2.5.0-rc1", published_at="2026-11-11T00:00:00Z"),
            release_response("v2.6.0", draft=True, published_at=None),
            release_response("v2.4.1-nightly.20261112", published_at="2026-11-12T00:00:00Z"),
            release_response("minimal-v2.9.0", published_at="2026-11-13T00:00:00Z"),
        )
        result, metadata, outputs = self.run_resolver()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(metadata["version"], "2.4.0")
        self.assertEqual(outputs["source"], "latest-published")
        self.assertEqual(outputs["is-prerelease"], "false")

        result, metadata, _ = self.run_resolver("--allow-prerelease")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(metadata["version"], "2.5.0-rc1")
        self.assertTrue(metadata["is_prerelease"])

        result, metadata, _ = self.run_resolver("--allow-prerelease", "--include-nightly")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(metadata["version"], "2.4.1-nightly.20261112")

    def test_auto_resolution_without_candidates_fails(self):
        self.set_releases(release_response("v2.5.0-alpha", prerelease=True))
        result, _, _ = self.run_resolver()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("No published stable release", result.stderr)

    def test_explicit_release_must_exist_and_be_published(self):
        for overrides in ({"draft": True}, {"published_at": None}, {"published_at": ""}, {"published_at": "today"}):
            with self.subTest(**overrides):
                self.set_releases({**release_response("v2.4.0"), **overrides})
                result, _, _ = self.run_resolver("--version", "2.4.0")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("not published", result.stderr)
        self.set_releases()
        result, _, _ = self.run_resolver("--version", "2.4.0")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("not found", result.stderr)

    def test_required_assets_must_match_exactly_one_non_empty_file(self):
        self.set_releases(release_response("v2.4.0"))
        result, metadata, outputs = self.run_resolver(
            "--version", "2.4.0", "--require-asset", r"windows-zip=windows.*(x64|amd64).*\.zip$")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(outputs["asset-windows-zip"], "ThemisDB-COMMUNITY-2.4.0-windows-x64.zip")
        windows = next(asset for asset in metadata["assets"] if asset["name"].endswith(".zip"))
        self.assertEqual(windows["sha256"], "a" * 64)
        self.assertTrue(windows["url"].startswith("https://github.com/"))

        for name, assets, message in [
            ("missing", [], "has no uploaded assets"),
            ("none", [{"name": "linux.tar.gz", "size": 1}], "found 0"),
            ("duplicate", [{"name": "a-windows-x64.zip", "size": 1}, {"name": "b-windows-x64.zip", "size": 1}], "found 2"),
            ("empty", [{"name": "a-windows-x64.zip", "size": 0}], "is empty"),
        ]:
            with self.subTest(name):
                self.set_releases(release_response("v2.4.0", assets=assets))
                result, _, _ = self.run_resolver(
                    "--version", "2.4.0", "--require-asset", r"windows-zip=windows.*(x64|amd64).*\.zip$")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(message, result.stderr)

    def test_invalid_inputs_fail_before_contacting_github(self):
        for args, repo, message in [
            (("--version", "2.4.0/../../bad"), REPO, "Invalid release version"),
            ((), "not-a-repo", "Invalid or missing repository"),
            (("--require-asset", "Bad=.*"), REPO, "Invalid --require-asset"),
            (("--require-asset", "zip=("), REPO, "Invalid regex"),
        ]:
            with self.subTest(args=args, repo=repo):
                calls = self.directory / "gh-calls"
                if calls.exists():
                    calls.unlink()
                result, _, _ = self.run_resolver(*args, repo=repo)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(message, result.stderr)

    def test_repository_only_mode_uses_version_file_without_release_lookup(self):
        result, metadata, outputs = self.run_resolver("--repository-only")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(metadata["version"], "2.4.0")
        self.assertEqual(metadata["source"], "repository")
        self.assertEqual(metadata["license_url"], f"https://github.com/{REPO}/blob/develop/LICENSE")
        self.assertNotIn("releases", (self.directory / "gh-calls").read_text())
        self.assertEqual(outputs["version"], "2.4.0")

        result, _, _ = self.run_resolver("--repository-only", "--require-asset", "zip=x")
        self.assertNotEqual(result.returncode, 0)


if __name__ == "__main__":
    unittest.main()
