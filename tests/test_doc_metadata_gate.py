#!/usr/bin/env python3

import json
import re
import sys
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPTS_DIR = REPO_ROOT / "scripts"
GITHUB_SCRIPTS_DIR = REPO_ROOT / ".github" / "scripts"

if str(SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_DIR))
if str(GITHUB_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(GITHUB_SCRIPTS_DIR))

import analyze_code_maturity as maturity_gate  # noqa: E402
import prepare_doc_file_issues as doc_issue_gate  # noqa: E402
import validate_doc_metadata as gate  # noqa: E402
import validate_module_features as feature_gate  # noqa: E402


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

    def test_skill_front_matter_metadata_passes(self) -> None:
        skill_dir = self.root / ".github" / "skills" / "code-review"
        skill_dir.mkdir(parents=True, exist_ok=True)
        skill_path = skill_dir / "SKILL.md"
        skill_path.write_text(
            "---\n"
            "name: \"Code Review\"\n"
            "description: \"Review pull requests...\"\n"
            "Author: \"ThemisDB Team\"\n"
            "Created: \"2026-10-07\"\n"
            "Last Updated: \"2026-10-08\"\n"
            "Status: \"active\"\n"
            "---\n\n"
            "# Code Review\n",
            encoding="utf-8",
        )

        report = gate.build_report([".github/skills/code-review/SKILL.md"], self.root, self.config)
        self.assertEqual(report.verdict, "PASS")
        self.assertEqual(report.files_checked, 1)

    def test_excluded_files_are_skipped(self) -> None:
        file_path = self.root / "docs" / "_standards" / "template.md"
        file_path.parent.mkdir(parents=True, exist_ok=True)
        file_path.write_text("# Template\n", encoding="utf-8")

        report = gate.build_report(["docs/_standards/template.md"], self.root, self.config)
        self.assertEqual(report.verdict, "PASS")
        self.assertEqual(report.files_checked, 0)

    def test_ai_context_files_are_skipped(self) -> None:
        file_path = self.root / "ai_context" / "README.md"
        file_path.parent.mkdir(parents=True, exist_ok=True)
        file_path.write_text("# AI Context\n\nNo metadata here.\n", encoding="utf-8")

        report = gate.build_report(["ai_context/README.md"], self.root, self.config)
        self.assertEqual(report.verdict, "PASS")
        self.assertEqual(report.files_checked, 0)

    def test_src_module_docs_are_skipped(self) -> None:
        file_path = self.root / "src" / "query" / "ARCHITECTURE.md"
        file_path.parent.mkdir(parents=True, exist_ok=True)
        file_path.write_text("# Architecture\n", encoding="utf-8")

        report = gate.build_report(["src/query/ARCHITECTURE.md"], self.root, self.config)
        self.assertEqual(report.verdict, "PASS")
        self.assertEqual(report.files_checked, 0)

    def test_gemini_exports_are_skipped(self) -> None:
        file_path = self.root / "ai_context" / "research" / "gemini_exports" / "report.md"
        file_path.parent.mkdir(parents=True, exist_ok=True)
        file_path.write_text("# Exported Report\n", encoding="utf-8")

        report = gate.build_report(
            ["ai_context/research/gemini_exports/report.md"], self.root, self.config
        )
        self.assertEqual(report.verdict, "PASS")
        self.assertEqual(report.files_checked, 0)

    def test_src_feature_pages_are_included(self) -> None:
        file_path = self.root / "src" / "query" / "FEATURES.md"
        file_path.parent.mkdir(parents=True, exist_ok=True)
        file_path.write_text(
            "---\n"
            "Author: ThemisDB Contributors\n"
            "Created: 2026-10-18\n"
            "Last Updated: 2026-10-18\n"
            "Status: active\n"
            "---\n\n"
            "# Query in ThemisDB\n\n"
            "A modern database needs a query layer that turns natural user intent into reproducible execution plans. "
            "ThemisDB combines SQL-like semantics, planner guidance, and hybrid execution so teams can ask for data, "
            "insights, and operational views without stitching together separate systems.\n\n"
            "This module is the connective tissue between clients, storage, and orchestration. It keeps behaviour explainable, "
            "observable, and safe under production load.\n",
            encoding="utf-8",
        )

        report = gate.build_report(["src/query/FEATURES.md"], self.root, self.config)
        self.assertEqual(report.verdict, "PASS")
        self.assertEqual(report.files_checked, 1)

    def test_module_feature_gate_rejects_shallow_pages(self) -> None:
        file_path = self.root / "src" / "query" / "FEATURES.md"
        file_path.parent.mkdir(parents=True, exist_ok=True)
        file_path.write_text(
            "---\n"
            "Author: ThemisDB Contributors\n"
            "Created: 2026-10-18\n"
            "Last Updated: 2026-10-18\n"
            "Status: active\n"
            "---\n\n"
            "# Query\n\n"
            "This page is too short and does not explain the module in ThemisDB context.\n",
            encoding="utf-8",
        )

        report = feature_gate.build_report(["src/query/FEATURES.md"], self.root, self.config)
        self.assertEqual(report.verdict, "FAIL")
        self.assertTrue(report.results[0].errors)

    def test_per_file_issue_bundle_is_created(self) -> None:
        failing_metadata = {
            "verdict": "FAIL",
            "files_seen": 1,
            "files_checked": 1,
            "files_skipped": 0,
            "files_with_errors": 1,
            "results": [{
                "file": "docs/guide.md",
                "errors": ["missing required field 'Created'", "field 'Status' must be one of: active, approved"],
            }],
        }
        failing_feature = {
            "verdict": "FAIL",
            "files_seen": 1,
            "files_checked": 1,
            "files_skipped": 0,
            "files_with_errors": 1,
            "results": [{
                "file": "src/query/FEATURES.md",
                "errors": ["FEATURES.md should explain why the module matters in production and operational terms."],
            }],
        }
        output_dir = self.root / "tmp-doc-issues"
        metadata_report = output_dir / "metadata.json"
        feature_report = output_dir / "feature.json"
        metadata_report.parent.mkdir(parents=True, exist_ok=True)
        metadata_report.write_text(json.dumps(failing_metadata), encoding="utf-8")
        feature_report.write_text(json.dumps(failing_feature), encoding="utf-8")

        manifest = doc_issue_gate.build_manifest(metadata_report, "metadata", output_dir)
        manifest.extend(doc_issue_gate.build_manifest(feature_report, "feature", output_dir))

        self.assertEqual(len(manifest), 2)
        self.assertTrue((output_dir / "docs-guide.md-metadata.md").exists())
        self.assertTrue((output_dir / "src-query-FEATURES.md-feature.md").exists())

        metadata_issue = (output_dir / "docs-guide.md-metadata.md").read_text(encoding="utf-8")
        self.assertIn("## Summary", metadata_issue)
        self.assertIn("## Acceptance Criteria", metadata_issue)
        self.assertIn("## Direct instructions for Copilot", metadata_issue)


if __name__ == "__main__":
    unittest.main()
