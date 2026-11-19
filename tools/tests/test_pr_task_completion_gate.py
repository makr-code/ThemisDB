from __future__ import annotations

import json
import sys
from pathlib import Path

import pytest

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


def test_extract_checklists_preserves_source_and_completion_state():
    items = gate._extract_checklists(_context())

    assert [item["id"] for item in items] == ["PR-42-1", "PR-42-2", "ISSUE-17-1"]
    assert [item["checked"] for item in items] == [True, False, False]


def test_open_requirement_blocks_and_comment_reports_percentage(tmp_path: Path):
    context = _context()
    context["checklist_items"] = gate._extract_checklists(context)
    context_path = tmp_path / "context.json"
    response_path = tmp_path / "response.json"
    output_path = tmp_path / "report.json"
    github_output = tmp_path / "github-output"
    context_path.write_text(json.dumps(context), encoding="utf-8")
    response_path.write_text(json.dumps(_response()), encoding="utf-8")

    assert gate.evaluate(context_path, response_path, output_path, github_output) == 0

    report = json.loads(output_path.read_text(encoding="utf-8"))
    comment = github_output.read_text(encoding="utf-8")
    assert report["passed"] is False
    assert report["completion_percent"] == 67
    assert "Fulfillment: 2/3 (67%)" in comment
    assert "Add focused tests" in comment
    assert "pr-task-completion-gate" in comment


def test_all_fulfilled_requirements_pass(tmp_path: Path):
    context = _context()
    context["checklist_items"] = gate._extract_checklists(context)
    response = _response()
    for item in response["requirements"]:
        item["status"] = "fulfilled"
    context_path = tmp_path / "context.json"
    response_path = tmp_path / "response.json"
    output_path = tmp_path / "report.json"
    github_output = tmp_path / "github-output"
    context_path.write_text(json.dumps(context), encoding="utf-8")
    response_path.write_text(json.dumps(response), encoding="utf-8")

    gate.evaluate(context_path, response_path, output_path, github_output)

    assert json.loads(output_path.read_text(encoding="utf-8"))["passed"] is True


def test_omitted_checklist_item_fails_closed(tmp_path: Path):
    context = _context()
    context["checklist_items"] = gate._extract_checklists(context)
    response = _response()
    response["requirements"] = response["requirements"][:1]
    context_path = tmp_path / "context.json"
    response_path = tmp_path / "response.json"
    output_path = tmp_path / "report.json"
    github_output = tmp_path / "github-output"
    context_path.write_text(json.dumps(context), encoding="utf-8")
    response_path.write_text(json.dumps(response), encoding="utf-8")

    gate.evaluate(context_path, response_path, output_path, github_output)

    report = json.loads(output_path.read_text(encoding="utf-8"))
    assert report["passed"] is False
    assert any("omitted checklist item" in error for error in report["errors"])
    assert "Add focused tests" in github_output.read_text(encoding="utf-8")


def test_truncated_context_cannot_pass(tmp_path: Path):
    context = _context()
    context["checklist_items"] = gate._extract_checklists(context)
    context["truncated"] = True
    context_path = tmp_path / "context.json"
    response_path = tmp_path / "response.json"
    output_path = tmp_path / "report.json"
    github_output = tmp_path / "github-output"
    context_path.write_text(json.dumps(context), encoding="utf-8")
    response = _response()
    for item in response["requirements"]:
        item["status"] = "fulfilled"
    response_path.write_text(json.dumps(response), encoding="utf-8")

    gate.evaluate(context_path, response_path, output_path, github_output)

    report = json.loads(output_path.read_text(encoding="utf-8"))
    assert report["passed"] is False
    assert any("truncated" in error for error in report["errors"])


def test_invalid_response_is_reported_as_unverified(tmp_path: Path):
    context = _context()
    context["checklist_items"] = gate._extract_checklists(context)
    context_path = tmp_path / "context.json"
    response_path = tmp_path / "response.json"
    output_path = tmp_path / "report.json"
    github_output = tmp_path / "github-output"
    context_path.write_text(json.dumps(context), encoding="utf-8")
    response_path.write_text("not json", encoding="utf-8")

    gate.evaluate(context_path, response_path, output_path, github_output)

    report = json.loads(output_path.read_text(encoding="utf-8"))
    assert report["passed"] is False
    assert all(item["status"] == "unclear" for item in report["requirements"])
    assert "Add focused tests" in github_output.read_text(encoding="utf-8")


def test_markdown_special_characters_are_escaped():
    value = gate._clean_cell("[unsafe](https://example.test) | `code` <tag>", 200)

    assert "[" not in value
    assert "|" not in value
    assert "`" not in value
    assert "&lt;tag&gt;" in value
