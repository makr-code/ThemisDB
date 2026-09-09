#!/usr/bin/env python3

import json
import sys
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPTS_DIR = REPO_ROOT / "scripts"

if str(SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_DIR))

import validate_doc_metadata as gate  # noqa: E402


class DocMetadataGateTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        (self.root / "docs").mkdir(parents=True, exist_ok=True)
        (self.root / ".github").mkdir(parents=True, exist_ok=True)
        self.config = json.loads(
            (REPO_ROOT / ".github" / "doc-metadata-gate.json").read_text(encoding="utf-8")
        )

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def test_header_section_metadata_passes(self) -> None:
        file_path = self.root / "docs" / "guide.md"
        file_path.write_text(
            "# Guide\n\n"
            "**Author:** ThemisDB Contributors\n"
            "**Created:** 2026-09-09\n"
            "**Last Updated:** 2026-09-09\n"
            "**Status:** active\n",
            encoding="utf-8",
        )

        report = gate.build_report(["docs/guide.md"], self.root, self.config)
        self.assertEqual(report.verdict, "PASS")
        self.assertEqual(report.files_checked, 1)

    def test_front_matter_metadata_passes(self) -> None:
        file_path = self.root / ".github" / "policy.md"
        file_path.write_text(
            "---\n"
            "Author: ThemisDB Contributors\n"
            "Created: 2026-09-09\n"
            "Last Updated: 2026-09-09\n"
            "Status: approved\n"
            "---\n\n"
            "# Policy\n",
            encoding="utf-8",
        )

        report = gate.build_report([".github/policy.md"], self.root, self.config)
        self.assertEqual(report.verdict, "PASS")
        self.assertEqual(report.files_checked, 1)

    def test_invalid_date_and_status_are_reported(self) -> None:
        file_path = self.root / "docs" / "broken.md"
        file_path.write_text(
            "# Broken\n\n"
            "**Author:** ThemisDB Contributors\n"
            "**Created:** 09-09-2026\n"
            "**Last Updated:** 2026-09-09\n"
            "**Status:** pending\n",
            encoding="utf-8",
        )

        report = gate.build_report(["docs/broken.md"], self.root, self.config)
        self.assertEqual(report.verdict, "FAIL")
        self.assertEqual(report.files_with_errors, 1)
        self.assertIn("field 'Created' must use YYYY-MM-DD", report.results[0].errors[0])
        self.assertIn("field 'Status' must be one of:", report.results[0].errors[1])

    def test_excluded_files_are_skipped(self) -> None:
        file_path = self.root / "docs" / "_standards" / "template.md"
        file_path.parent.mkdir(parents=True, exist_ok=True)
        file_path.write_text("# Template\n", encoding="utf-8")

        report = gate.build_report(["docs/_standards/template.md"], self.root, self.config)
        self.assertEqual(report.verdict, "PASS")
        self.assertEqual(report.files_checked, 0)


if __name__ == "__main__":
    unittest.main()
