import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
BUILDER = REPO_ROOT / "scripts" / "ci-build-doku-db.sh"


class DokuDbBuilderTests(unittest.TestCase):
    def build_index(self, text, max_tokens=8, overlap=2):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "docs").mkdir()
            (root / "docs" / "example.md").write_text(text, encoding="utf-8")
            output = root / "output"
            result = subprocess.run(
                ["bash", str(BUILDER)],
                env={
                    **os.environ,
                    "REPO_ROOT": str(root),
                    "OUTPUT_DIR": str(output),
                    "CHUNK_MAX_TOKENS": str(max_tokens),
                    "CHUNK_OVERLAP_TOKENS": str(overlap),
                    "MAX_CHUNKS": "0",
                },
                capture_output=True,
                text=True,
                timeout=15,
            )
            index = output / "doku.db.json"
            return result, json.loads(index.read_text()) if index.exists() else None

    def test_heading_transition_preserves_entire_short_section(self):
        result, index = self.build_index(
            "# First\nalpha beta gamma delta epsilon\n# Second\nzeta eta\n"
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(
            [chunk["content"] for chunk in index["chunks"]],
            ["alpha beta gamma delta epsilon", "zeta eta"],
        )
        self.assertEqual(index["chunks"][0]["source_path"], "docs/example.md")
        self.assertEqual(index["chunks"][0]["section_heading"], "# First")

    def test_long_lines_are_bounded_and_preserved_across_headings(self):
        words = [f"word{i}" for i in range(20)]
        text = "# First\n" + " ".join(words) + "\n# Second\nlast\n"
        result, index = self.build_index(text)
        self.assertEqual(result.returncode, 0, result.stderr)
        chunks = [chunk["content"].split() for chunk in index["chunks"][:-1]]
        self.assertTrue(all(len(chunk) <= 6 for chunk in chunks))
        recovered = chunks[0] + [word for chunk in chunks[1:] for word in chunk[1:]]
        self.assertEqual(recovered, words)
        repeated_result, repeated_index = self.build_index(text)
        self.assertEqual(repeated_result.returncode, 0, repeated_result.stderr)
        self.assertEqual(repeated_index, index)

    def test_invalid_windows_fail_instead_of_looping(self):
        for max_tokens, overlap in ((0, 0), (8, -1), (8, 8), (8, 20)):
            with self.subTest(max_tokens=max_tokens, overlap=overlap):
                result, index = self.build_index("# Heading\ncontent\n", max_tokens, overlap)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("CHUNK_MAX_TOKENS must be positive", result.stderr)
                self.assertIsNone(index)


if __name__ == "__main__":
    unittest.main()
