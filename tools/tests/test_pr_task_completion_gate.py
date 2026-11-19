from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS_CI = Path(__file__).resolve().parents[1] / "ci"
if str(TOOLS_CI) not in sys.path:
    sys.path.insert(0, str(TOOLS_CI))

import pr_task_completion_gate as gate  # noqa: E402


def _context() -> dict:
    return {
        "pull_request": {
            "number": 42,
            "title": "Implement task gate",
            "body": "## Tasks\n- [x] Add a reusable workflow\n- [ ] Add focused tests",
        },
        "issues": [{
            "number": 17,
            "title": "Task requirements",
            "body": "## Acceptance criteria\n- [ ] Publish a completion score",
        }],
        "changed_files": [{"path": "tools/ci/example.py", "patch": "+pass"}],
        "truncated": False,
    }


def _response() -> dict:
    return {
        "requirements": [
            {
                "id": "PR-42-1",
                "source": "PR #42",
                "requirement": "Add a reusable workflow",
                "status": "fulfilled",
                "evidence": "The caller uses the reusable workflow.",
            },
            {
                "id": "PR-42-2",
                "source": "PR #42",
                "requirement": "Add focused tests",
                "status": "open",
                "evidence": "No test changes are shown.",
            },
            {
                "id": "ISSUE-17-1",
                "source": "Issue #17",
                "requirement": "Publish a completion score",
                "status": "fulfilled",
                "evidence": "The workflow comment includes a percentage.",
            },
        ]
    }


class PRTaskCompletionGateTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tempdir = tempfile.TemporaryDirectory()
        self.addCleanup(self.tempdir.cleanup)
        self.root = Path(self.tempdir.name)
        self.context_path = self.root / "context.json"
        self.response_path = self.root / "response.json"
        self.output_path = self.root / "report.json"
        self.github_output_path = self.root / "github-output"

    def _write_inputs(self, context: dict, response: object) -> None:
        context["checklist_items"] = gate._extract_checklists(context)
        self.context_path.write_text(json.dumps(context), encoding="utf-8")
        self.response_path.write_text(json.dumps(response), encoding="utf-8")

    def _evaluate(self) -> tuple[dict, str]:
        self.assertEqual(
            gate.evaluate(
                self.context_path,
                self.response_path,
                self.output_path,
                self.github_output_path,
            ),
            0,
        )
        return (
            json.loads(self.output_path.read_text(encoding="utf-8")),
            self.github_output_path.read_text(encoding="utf-8"),
        )

    def test_extract_checklists_preserves_source_and_completion_state(self) -> None:
        items = gate._extract_checklists(_context())

        self.assertEqual(
            [item["id"] for item in items],
            ["PR-42-1", "PR-42-2", "ISSUE-17-1"],
        )
        self.assertEqual([item["checked"] for item in items], [True, False, False])

    def test_open_requirement_blocks_and_comment_reports_percentage(self) -> None:
        context = _context()
        context["issues"][0]["body"] = context["issues"][0]["body"].replace("[ ]", "[x]")
        self._write_inputs(context, _response())

        report, comment = self._evaluate()

        self.assertFalse(report["passed"])
        self.assertEqual(report["completion_percent"], 67)
        self.assertIn("Fulfillment: 2/3 (67%)", comment)
        self.assertIn("Add focused tests", comment)
        self.assertIn("pr-task-completion-gate", comment)

    def test_all_fulfilled_requirements_pass(self) -> None:
        context = _context()
        context["pull_request"]["body"] = context["pull_request"]["body"].replace("[ ]", "[x]")
        context["issues"][0]["body"] = context["issues"][0]["body"].replace("[ ]", "[x]")
        response = _response()
        for item in response["requirements"]:
            item["status"] = "fulfilled"
        self._write_inputs(context, response)

        report, _ = self._evaluate()

        self.assertTrue(report["passed"])

    def test_omitted_checklist_item_fails_closed(self) -> None:
        response = _response()
        response["requirements"] = response["requirements"][:1]
        self._write_inputs(_context(), response)

        report, comment = self._evaluate()

        self.assertFalse(report["passed"])
        self.assertTrue(any("omitted checklist item" in error for error in report["errors"]))
        self.assertIn("Add focused tests", comment)

    def test_truncated_context_cannot_pass(self) -> None:
        context = _context()
        context["truncated"] = True
        response = _response()
        for item in response["requirements"]:
            item["status"] = "fulfilled"
        self._write_inputs(context, response)

        report, _ = self._evaluate()

        self.assertFalse(report["passed"])
        self.assertTrue(any("truncated" in error for error in report["errors"]))

    def test_invalid_response_is_reported_as_unverified(self) -> None:
        context = _context()
        context["checklist_items"] = gate._extract_checklists(context)
        self.context_path.write_text(json.dumps(context), encoding="utf-8")
        self.response_path.write_text("not json", encoding="utf-8")

        report, comment = self._evaluate()

        self.assertFalse(report["passed"])
        self.assertTrue(all(item["status"] == "unclear" for item in report["requirements"]))
        self.assertIn("Add focused tests", comment)

    def test_markdown_special_characters_are_escaped(self) -> None:
        value = gate._clean_cell("[unsafe](https://example.test) | `code` <tag>", 200)

        self.assertNotIn("[", value)
        self.assertNotIn("|", value)
        self.assertNotIn("`", value)
        self.assertIn("&lt;tag&gt;", value)

    def test_no_assessable_requirements_fails_closed(self) -> None:
        context = _context()
        context["pull_request"]["body"] = "No checklist was supplied."
        context["issues"] = []
        self._write_inputs(context, {"requirements": []})

        report, comment = self._evaluate()

        self.assertFalse(report["passed"])
        self.assertEqual(report["completion_percent"], 0)
        self.assertIn("No actionable requirements were returned", comment)

    def test_not_applicable_items_are_excluded_from_percentage(self) -> None:
        context = _context()
        context["pull_request"]["body"] = context["pull_request"]["body"].replace("[ ]", "[x]")
        context["issues"][0]["body"] = context["issues"][0]["body"].replace("[ ]", "[x]")
        response = _response()
        response["requirements"][0]["status"] = "fulfilled"
        response["requirements"][1]["status"] = "fulfilled"
        response["requirements"][2]["status"] = "not_applicable"
        self._write_inputs(context, response)

        report, comment = self._evaluate()

        self.assertTrue(report["passed"])
        self.assertIn("Fulfillment: 2/2 (100%)", comment)

    def test_unchecked_checklist_cannot_be_marked_fulfilled_by_copilot(self) -> None:
        response = _response()
        for item in response["requirements"]:
            item["status"] = "fulfilled"
        self._write_inputs(_context(), response)

        report, comment = self._evaluate()

        self.assertFalse(report["passed"])
        self.assertIn("still unchecked", comment)


if __name__ == "__main__":
    unittest.main()
