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

import module_docs_issue_reporter as reporter  # noqa: E402


def test_module_work_package_name_for_include_only_module():
    info = {"files": ["include/document/README.md", "include/document/ROADMAP.md"]}
    assert reporter._module_work_package_name("document", info) == "include_document"


def test_module_work_package_name_for_src_or_mixed_module():
    info = {"files": ["src/chaos/README.md", "include/chaos/README.md"]}
    assert reporter._module_work_package_name("chaos", info) == "chaos"


def test_module_issue_title_matches_harmonized_format():
    assert reporter._module_issue_title("chaos", {"files": ["src/chaos/README.md"]}) == "[MODULE] chaos"
    assert reporter._module_issue_title("document", {"files": ["include/document/README.md"]}) == "[MODULE] include_document"


def test_module_issue_body_contains_required_sections():
    body = reporter._module_issue_body(
        "maintenance",
        {
            "files": ["src/maintenance/README.md", "src/maintenance/ROADMAP.md"],
            "de_human_authored": 0,
        },
    )
    assert "## Scope (Primary/Secondary)" in body
    assert "## Nicht-Ziele" in body
    assert "## Tasks 1–4 (verbindlich)" in body
    assert "Task 1 — Reality-Check gegen Sourcecode" in body
    assert "Task 2 — ROADMAP/FUTURE_ENHANCEMENTS-Verifikation" in body
    assert "Task 3 — Research-Hinweise" in body
    assert "Task 4 — Missing-Implementations-Report" in body
    assert "## Definition of Done (DoD)" in body
    assert "## Abschlussformat für Online-Agenten" in body
    assert "v1.8.0" in body


def test_create_issue_appends_milestone(monkeypatch):
    called = {}

    def _fake_run(args, timeout=30):
        called["args"] = args
        return 0, "ok", ""

    monkeypatch.setattr(reporter, "_run", _fake_run)
    ok = reporter._create_issue(
        repo="makr-code/ThemisDB",
        title="[MODULE] chaos",
        labels=["type:documentation"],
        body="x",
        dry_run=False,
        milestone="v1.8.0",
    )

    assert ok is True
    assert "--milestone" in called["args"]
    assert "v1.8.0" in called["args"]
