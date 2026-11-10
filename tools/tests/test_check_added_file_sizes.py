from __future__ import annotations

import subprocess
import tempfile
import unittest
from pathlib import Path

from tools.ci.check_added_file_sizes import MAX_FILE_SIZE_BYTES, find_oversized_additions


def _git(root: Path, *args: str) -> None:
    subprocess.run(["git", *args], cwd=root, check=True, capture_output=True)


def _commit_file(root: Path, commit_message: str, name: str, content: bytes) -> str:
    path = root / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(content)
    _git(root, "add", name)
    _git(root, "commit", "-m", commit_message)
    return subprocess.run(
        ["git", "rev-parse", "HEAD"], cwd=root, check=True, capture_output=True, text=True
    ).stdout.strip()


class AddedFileSizeTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary_directory.cleanup)
        self.root = Path(self.temporary_directory.name) / "repo"
        self.root.mkdir()
        _git(self.root, "init", "-q")
        _git(self.root, "config", "user.name", "Test")
        _git(self.root, "config", "user.email", "test@example.invalid")
        self.base = _commit_file(self.root, "base", "tracked.txt", b"base\n")

    def test_exactly_five_mib_is_allowed(self) -> None:
        head = _commit_file(
            self.root, "exact limit", "exact.bin", b"x" * MAX_FILE_SIZE_BYTES
        )

        self.assertEqual(find_oversized_additions(self.base, head, cwd=self.root), [])

    def test_new_file_over_five_mib_is_rejected(self) -> None:
        head = _commit_file(
            self.root,
            "oversized addition",
            "large.bin",
            b"x" * (MAX_FILE_SIZE_BYTES + 1),
        )

        self.assertEqual(
            find_oversized_additions(self.base, head, cwd=self.root),
            [("large.bin", MAX_FILE_SIZE_BYTES + 1)],
        )

    def test_small_file_enlarged_over_limit_is_rejected(self) -> None:
        head = _commit_file(
            self.root,
            "enlarge file",
            "tracked.txt",
            b"x" * (MAX_FILE_SIZE_BYTES + 1),
        )

        self.assertEqual(
            find_oversized_additions(self.base, head, cwd=self.root),
            [("tracked.txt", MAX_FILE_SIZE_BYTES + 1)],
        )

    def test_exact_path_allowlist_exempts_file(self) -> None:
        head = _commit_file(
            self.root,
            "allowed fixture",
            "fixtures/large.bin",
            b"x" * (MAX_FILE_SIZE_BYTES + 1),
        )

        self.assertEqual(
            find_oversized_additions(
                self.base,
                head,
                cwd=self.root,
                allowlist={"fixtures/large.bin"},
            ),
            [],
        )

    def test_renamed_large_file_is_rejected(self) -> None:
        large_path = self.root / "large.bin"
        large_path.write_bytes(b"x" * (MAX_FILE_SIZE_BYTES + 1))
        _git(self.root, "add", "large.bin")
        _git(self.root, "commit", "-m", "add large file")
        base_with_large_file = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            cwd=self.root,
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()
        large_path.rename(self.root / "renamed.bin")
        _git(self.root, "add", "-A")
        _git(self.root, "commit", "-m", "rename large file")
        head = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            cwd=self.root,
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()

        self.assertNotEqual(self.base, base_with_large_file)
        self.assertEqual(
            find_oversized_additions(base_with_large_file, head, cwd=self.root),
            [("renamed.bin", MAX_FILE_SIZE_BYTES + 1)],
        )

    def test_existing_large_file_can_be_updated_without_crossing_limit_again(self) -> None:
        large_path = self.root / "large.bin"
        large_path.write_bytes(b"x" * (MAX_FILE_SIZE_BYTES + 1))
        _git(self.root, "add", "large.bin")
        _git(self.root, "commit", "-m", "existing large file")
        base = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            cwd=self.root,
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()
        large_path.write_bytes(b"y" * (MAX_FILE_SIZE_BYTES + 1))
        _git(self.root, "add", "large.bin")
        _git(self.root, "commit", "-m", "update existing large file")
        head = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            cwd=self.root,
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()

        self.assertEqual(find_oversized_additions(base, head, cwd=self.root), [])


if __name__ == "__main__":
    unittest.main()
