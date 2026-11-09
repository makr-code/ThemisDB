from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
POLICY_SCRIPT = REPOSITORY_ROOT / "tools" / "ci" / "repo_artifact_policy.py"
MAX_FILE_BYTES = 5 * 1024 * 1024


class RepoArtifactPolicyTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory()
        self.repo = Path(self.temp_dir.name)
        self._git("init", "-q")
        self._git("config", "user.email", "test@example.invalid")
        self._git("config", "user.name", "Artifact Policy Test")
        (self.repo / "base.txt").write_text("baseline\n", encoding="utf-8")
        self._git("add", "base.txt")
        self._git("commit", "-qm", "base")
        self.base_ref = self._git("rev-parse", "HEAD").stdout.strip()
        self.allowlist_path = self.repo / "allowlist.json"
        self._write_allowlist({})

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def _git(self, *args: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            ["git", "-C", str(self.repo), *args],
            check=True,
            capture_output=True,
            text=True,
        )

    def _write_allowlist(self, files: dict[str, str]) -> None:
        self.allowlist_path.write_text(json.dumps({"files": files}), encoding="utf-8")

    def _run_gate(self) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [
                sys.executable,
                str(POLICY_SCRIPT),
                "check",
                "--repo-root",
                str(self.repo),
                "--base-ref",
                self.base_ref,
                "--head-ref",
                "HEAD",
                "--allowlist",
                str(self.allowlist_path),
            ],
            capture_output=True,
            text=True,
            check=False,
        )

    def _commit_file(self, path: str, size: int) -> None:
        file_path = self.repo / path
        file_path.parent.mkdir(parents=True, exist_ok=True)
        with file_path.open("wb") as stream:
            stream.truncate(size)
        self._git("add", path)
        self._git("commit", "-qm", f"add {path}")

    def test_rejects_new_file_larger_than_five_mib(self) -> None:
        path = "artifacts/new-output.bin"
        self._commit_file(path, MAX_FILE_BYTES + 1)

        result = self._run_gate()

        self.assertEqual(result.returncode, 1)
        self.assertIn(path, result.stderr)
        self.assertIn(str(MAX_FILE_BYTES), result.stderr)

    def test_allows_exact_path_with_reviewed_rationale(self) -> None:
        path = "tests/fixtures/approved_large_fixture.bin"
        self._commit_file(path, MAX_FILE_BYTES + 1)
        self._write_allowlist({path: "Intentional deterministic fixture for integration tests."})

        result = self._run_gate()

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(f"ALLOW {path}", result.stdout)

    def test_exact_five_mib_boundary_is_allowed(self) -> None:
        self._commit_file("artifacts/at-limit.bin", MAX_FILE_BYTES)

        result = self._run_gate()

        self.assertEqual(result.returncode, 0, result.stderr)

    def test_allowlist_rejects_globs(self) -> None:
        self._write_allowlist({"tests/fixtures/*.bin": "Too broad."})

        result = self._run_gate()

        self.assertEqual(result.returncode, 2)
        self.assertIn("glob patterns are not allowed", result.stderr)


if __name__ == "__main__":
    unittest.main()
