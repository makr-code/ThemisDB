from __future__ import annotations

import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

TOOLS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS / "ci"))

import validate_dis11_evidence as dis11  # noqa: E402


class DIS11EvidenceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)

    def write_run(self, index: int, status: str = "passed", name: str = dis11.EXPECTED_TEST) -> None:
        suite = ET.Element("testsuite")
        case = ET.SubElement(suite, "testcase", name=name)
        if status == "failure":
            ET.SubElement(case, "failure")
        elif status == "skipped":
            ET.SubElement(case, "skipped")
        ET.ElementTree(suite).write(
            self.root / f"dis11-run-{index}.xml",
            encoding="utf-8",
            xml_declaration=True,
        )

    def validate(self) -> tuple[dict, bool]:
        return dis11.validate_evidence(
            self.root,
            expected_runs=3,
            commit_sha="abc123",
            workflow_run_url="https://example.invalid/runs/42",
        )

    def test_three_passing_runs_pass(self) -> None:
        for index in range(1, 4):
            self.write_run(index)
        evidence, valid = self.validate()
        self.assertTrue(valid)
        self.assertTrue(evidence["passed"])
        self.assertEqual(len(evidence["runs"]), 3)
        self.assertTrue(all(run["passed"] for run in evidence["runs"]))

    def test_missing_failure_skipped_and_wrong_name_fail(self) -> None:
        cases = (
            lambda: None,
            lambda: self.write_run(1, "failure"),
            lambda: self.write_run(1, "skipped"),
            lambda: self.write_run(1, name="WrongTest"),
        )
        for setup in cases:
            with self.subTest(setup=setup):
                for index in range(1, 4):
                    path = self.root / f"dis11-run-{index}.xml"
                    if path.exists():
                        path.unlink()
                setup()
                for index in range(2, 4):
                    if not (self.root / f"dis11-run-{index}.xml").exists():
                        self.write_run(index)
                evidence, valid = self.validate()
                self.assertFalse(valid)
                self.assertFalse(evidence["passed"])


if __name__ == "__main__":
    unittest.main()
