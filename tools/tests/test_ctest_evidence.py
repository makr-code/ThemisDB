from __future__ import annotations

import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path
from unittest.mock import patch

TOOLS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS / "ci"))

import summarize_ctest_evidence as evidence  # noqa: E402


class CTestEvidenceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary_directory.name)
        self.junit = self.root / "junit.xml"

    def tearDown(self) -> None:
        self.temporary_directory.cleanup()

    def _write_junit(self, statuses: dict[str, str]) -> None:
        suite = ET.Element("testsuite")
        for name, status in statuses.items():
            case = ET.SubElement(suite, "testcase", name=name)
            if status == "failed":
                ET.SubElement(case, "failure")
            elif status == "skipped":
                ET.SubElement(case, "skipped")
        ET.ElementTree(suite).write(self.junit, encoding="utf-8", xml_declaration=True)

    def _summarize(self, expected: list[dict]) -> tuple[str, bool]:
        return evidence._summarize(
            expected,
            self.junit,
            {"auth", "storage"},
            platform="Linux",
            commit_sha="deadbeef",
            workflow_run_url="https://example.invalid/runs/42",
        )

    @patch("summarize_ctest_evidence.subprocess.run")
    def test_ctest_preset_inventory_uses_the_same_preset_and_repository_root(self, run) -> None:
        run.return_value.stdout = '{"tests": [{"name": "module_auth_test_auth_focused"}]}'

        tests = evidence._read_expected_tests(
            self.root / "build",
            ctest_preset="windows-release",
            repo_root=self.root,
        )

        self.assertEqual(tests, [{"name": "module_auth_test_auth_focused"}])
        run.assert_called_once_with(
            ["ctest", "--preset", "windows-release", "--show-only=json-v1"],
            check=True,
            capture_output=True,
            text=True,
            cwd=self.root,
        )

    def test_complete_run_groups_results_by_module(self) -> None:
        self._write_junit({"module_auth_test_auth_focused": "passed", "module_storage_test_storage": "passed"})
        summary, valid = self._summarize([
            {
                "name": "module_auth_test_auth_focused",
                "properties": [{"name": "LABELS", "value": ["module:auth"]}],
            },
            {
                "name": "module_storage_test_storage",
                "properties": [{"name": "LABELS", "value": ["module:storage"]}],
            },
        ])

        self.assertTrue(valid)
        self.assertIn("| `auth` | 1 | 0 | 0 | 0 |", summary)
        self.assertIn("| `storage` | 1 | 0 | 0 | 0 |", summary)
        self.assertIn("deadbeef", summary)

    def test_skipped_test_is_not_reported_as_pass(self) -> None:
        self._write_junit({"module_auth_test_auth_focused": "skipped"})
        summary, valid = self._summarize([
            {
                "name": "module_auth_test_auth_focused",
                "properties": [{"name": "LABELS", "value": ["auth"]}],
            },
        ])

        self.assertFalse(valid)
        self.assertIn("1 test(s) were skipped", summary)
        self.assertIn("| `auth` | 0 | 0 | 1 | 0 |", summary)

    def test_partial_run_is_not_reported_as_pass(self) -> None:
        self._write_junit({"module_auth_test_auth_focused": "passed"})
        summary, valid = self._summarize([
            {"name": "module_auth_test_auth_focused", "properties": []},
            {"name": "module_storage_test_storage", "properties": []},
        ])

        self.assertFalse(valid)
        self.assertIn("JUnit is missing 1 registered test(s)", summary)

    def test_missing_junit_is_a_failure(self) -> None:
        summary, valid = evidence._summarize(
            [],
            self.root / "missing.xml",
            set(),
            platform="Windows",
            commit_sha="deadbeef",
            workflow_run_url="https://example.invalid/runs/42",
        )

        self.assertFalse(valid)
        self.assertIn("Unable to read complete CTest JUnit output", summary)


if __name__ == "__main__":
    unittest.main()
