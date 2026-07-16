#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

_HERE = Path(__file__).resolve().parent
_TOOLS = _HERE.parent
_CI = _TOOLS / "ci"

for _p in (_TOOLS, _CI):
    if str(_p) not in sys.path:
        sys.path.insert(0, str(_p))

import check_workflow_boundaries as guard  # noqa: E402


def _write_workflow(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content.strip() + "\n", encoding="utf-8")


def test_reactivated_workflow_requires_docs_updates(tmp_path: Path):
    _write_workflow(
        tmp_path / ".github" / "workflows" / "09-pr-gates_test.yml",
        """
name: Test Guard
on:
  workflow_dispatch:
  pull_request:
    branches: [main]
    paths:
      - '.github/workflows/**'
permissions:
  contents: read
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true
jobs:
  guard:
    runs-on: ubuntu-latest
    steps:
      - run: echo ok
""",
    )
    _write_workflow(
        tmp_path / ".github" / "no_workflows" / "09-pr-gates_test.yml",
        "name: Old Guard\non: workflow_dispatch\n",
    )

    violations = guard.evaluate(
        [{
            "status": "R100",
            "old": ".github/no_workflows/09-pr-gates_test.yml",
            "new": ".github/workflows/09-pr-gates_test.yml",
        }],
        tmp_path,
    )

    assert any("WORKFLOW_GUIDELINES" in violation for violation in violations)
    assert any("WORKFLOW_REGISTRY" in violation for violation in violations)


def test_pull_request_paths_must_not_be_repo_wide(tmp_path: Path):
    workflow = tmp_path / ".github" / "workflows" / "guard.yml"
    _write_workflow(
        workflow,
        """
name: Broad Guard
on:
  pull_request:
    branches: [main]
    paths:
      - 'src/**'
permissions:
  contents: read
concurrency:
  group: test
  cancel-in-progress: true
jobs:
  guard:
    runs-on: ubuntu-latest
    steps:
      - run: echo ok
""",
    )

    violations = guard.validate_workflow_file(workflow)
    assert any("too broad" in violation for violation in violations)


def test_reactivated_workflow_with_manual_dispatch_and_docs_passes(tmp_path: Path):
    workflow = tmp_path / ".github" / "workflows" / "09-pr-gates_test.yml"
    _write_workflow(
        workflow,
        """
name: Workflow Boundary Guard
on:
  pull_request:
    branches: [main, develop]
    paths:
      - '.github/workflows/**'
      - '.github/no_workflows/**'
      - '.github/WORKFLOW_GUIDELINES.md'
      - '.github/WORKFLOW_REGISTRY.md'
      - 'tools/ci/check_workflow_boundaries.py'
      - 'tools/tests/test_check_workflow_boundaries.py'
  workflow_dispatch:
permissions:
  contents: read
concurrency:
  group: ${{ github.workflow }}-${{ github.event.pull_request.number || github.ref }}
  cancel-in-progress: true
jobs:
  guard:
    runs-on: ubuntu-latest
    steps:
      - run: echo ok
""",
    )
    _write_workflow(
        tmp_path / ".github" / "no_workflows" / "09-pr-gates_test.yml",
        "name: Old Guard\non: workflow_dispatch\n",
    )
    (tmp_path / ".github" / "WORKFLOW_GUIDELINES.md").write_text("updated\n", encoding="utf-8")
    (tmp_path / ".github" / "WORKFLOW_REGISTRY.md").write_text("updated\n", encoding="utf-8")

    violations = guard.evaluate(
        [
            {
                "status": "R100",
                "old": ".github/no_workflows/09-pr-gates_test.yml",
                "new": ".github/workflows/09-pr-gates_test.yml",
            },
            {"status": "M", "path": ".github/WORKFLOW_GUIDELINES.md"},
            {"status": "M", "path": ".github/WORKFLOW_REGISTRY.md"},
        ],
        tmp_path,
    )

    assert violations == []


def test_main_returns_zero_for_irrelevant_changes(tmp_path: Path):
    diff = tmp_path / "changes.txt"
    diff.write_text("M\tREADME.md\n", encoding="utf-8")

    rc = guard.main(["--repo-root", str(tmp_path), "--diff-file", str(diff)])
    assert rc == 0