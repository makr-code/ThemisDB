"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_sync_issues_from_roadmap.py                   ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 18:57:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     305                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8452353dc5  2026-03-12  Add unit tests for sync-issues-from-roadmap.py ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Unit tests for scripts/sync-issues-from-roadmap.py
"""

import importlib.util
import sys
import textwrap
from pathlib import Path


_SCRIPT = Path(__file__).parent.parent / "scripts" / "sync-issues-from-roadmap.py"


def _load_module():
    spec = importlib.util.spec_from_file_location("sync_issues_from_roadmap", _SCRIPT)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


sync = _load_module()


def write_file(path: Path, content: str) -> Path:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")
    return path


def test_parse_roadmap_document_without_issue_column(tmp_path):
    roadmap = write_file(
        tmp_path / "src" / "ROADMAP.md",
        textwrap.dedent(
            """\
            ## Critical Priority

            | # | Module | Title | Target | Labels | Detail |
            |---|--------|-------|--------|--------|--------|
            | 1 | auth | LDAP DN and Filter Injection Prevention | v1.1.0 | `security`, `module:auth` | [-> Detail](auth/FUTURE_ENHANCEMENTS.md#ldap-dn-and-filter-injection-prevention) |
            """
        ),
    )

    items, tables = sync.parse_roadmap_document(roadmap)

    assert len(items) == 1
    assert len(tables) == 1
    assert items[0].module == "auth"
    assert items[0].issue_ref is None
    assert items[0].priority == "critical"
    assert items[0].source_key == "roadmap:1:auth:v1.1.0:ldap-dn-and-filter-injection-prevention"


def test_extract_acceptance_criteria_prefers_checkboxes():
    section = textwrap.dedent(
        """\
        ### Some Feature

        **Implementation Notes:**
        - [ ] First checkbox item
        - [ ] Second checkbox item
        1. Fallback numbered item
        """
    )

    criteria = sync.extract_acceptance_criteria(section)

    assert criteria == [
        "- [ ] First checkbox item",
        "- [ ] Second checkbox item",
    ]


def test_extract_acceptance_criteria_uses_numbered_tasks_when_needed():
    section = textwrap.dedent(
        """\
        ### Modular Build System

        **Tasks:**
        1. Extract core implementations to `src/themis/`
        2. Create `libthemis-base.so` / `themis-base.dll`
        3. Update CMakeLists.txt for modular builds
        """
    )

    criteria = sync.extract_acceptance_criteria(section)

    assert criteria == [
        "- [ ] Extract core implementations to `src/themis/`",
        "- [ ] Create `libthemis-base.so` / `themis-base.dll`",
        "- [ ] Update CMakeLists.txt for modular builds",
    ]


def test_find_detail_section_matches_anchor(tmp_path):
    detail_file = write_file(
        tmp_path / "src" / "auth" / "FUTURE_ENHANCEMENTS.md",
        textwrap.dedent(
            """\
            # Auth

            ### 3. LDAP DN and Filter Injection Prevention
            - [ ] Escape DN values

            ### 4. Constant-Time Comparison
            - [ ] Use constant-time compare
            """
        ),
    )

    heading, section = sync.find_detail_section(
        detail_file,
        "3-ldap-dn-and-filter-injection-prevention",
        "LDAP DN and Filter Injection Prevention",
    )

    assert heading == "3. LDAP DN and Filter Injection Prevention"
    assert "Escape DN values" in section
    assert "Constant-Time Comparison" not in section


def test_build_issue_body_contains_traceability_markers():
    item = sync.RoadmapItem(
        row_number="1",
        module="auth",
        title="LDAP DN and Filter Injection Prevention",
        target="v1.1.0",
        labels=["security", "module:auth"],
        detail_link="auth/FUTURE_ENHANCEMENTS.md#3-ldap-dn-and-filter-injection-prevention",
        detail_path="auth/FUTURE_ENHANCEMENTS.md",
        detail_anchor="3-ldap-dn-and-filter-injection-prevention",
        priority="critical",
        priority_heading="Critical Priority",
        issue_ref=None,
        source_key="roadmap:1:auth:v1.1.0:3-ldap-dn-and-filter-injection-prevention",
        line_index=10,
        table_index=0,
    )

    body = sync.build_issue_body(
        item,
        "3. LDAP DN and Filter Injection Prevention",
        "Section text",
        ["- [ ] Escape DN values"],
        ["area:auth", "type:security", "priority:critical", "status:open", "security"],
    )

    assert "### Context" in body
    assert "### Goal" in body
    assert "### Acceptance Criteria" in body
    assert "### Relationships" in body
    assert "### References" in body
    assert "- [ ] Escape DN values" in body
    assert "roadmap-source-key: roadmap:1:auth:v1.1.0:3-ldap-dn-and-filter-injection-prevention" in body


def test_build_issue_labels_maps_module_to_governance_area():
    item = sync.RoadmapItem(
        row_number="3",
        module="themis",
        title="Modular Build System",
        target="Q2 2026",
        labels=["feature", "build"],
        detail_link="themis/FUTURE_ENHANCEMENTS.md#modular-build-system",
        detail_path="themis/FUTURE_ENHANCEMENTS.md",
        detail_anchor="modular-build-system",
        priority="high",
        priority_heading="High Priority",
        issue_ref=None,
        source_key="roadmap:3:themis:Q2 2026:modular-build-system",
        line_index=20,
        table_index=1,
    )

    labels = sync.build_issue_labels(item, sync.classify_issue_kind(item))

    assert "area:core" in labels
    assert "priority:high" in labels
    assert "type:chore" in labels
    assert "status:open" in labels
    assert "build" in labels


def test_resolve_labels_for_apply_creates_governance_and_skips_unknown(monkeypatch, tmp_path):
    created = []

    def fake_create_label(repo, repo_root, label):
        created.append((repo, repo_root, label))

    monkeypatch.setattr(sync, "create_label", fake_create_label)

    applied, skipped = sync.resolve_labels_for_apply(
        "makr-code/ThemisDB",
        tmp_path,
        ["area:auth", "priority:critical", "type:security", "status:open", "thread-safety"],
        {"priority:critical", "type:security", "status:open"},
    )

    assert applied == ["area:auth", "priority:critical", "type:security", "status:open"]
    assert skipped == ["thread-safety"]
    assert created == [("makr-code/ThemisDB", tmp_path, "area:auth")]


def test_backfill_roadmap_adds_issue_column_and_values(tmp_path):
    roadmap = write_file(
        tmp_path / "src" / "ROADMAP.md",
        textwrap.dedent(
            """\
            ## Critical Priority

            | # | Module | Title | Target | Labels | Detail |
            |---|--------|-------|--------|--------|--------|
            | 1 | auth | LDAP DN and Filter Injection Prevention | v1.1.0 | `security`, `module:auth` | [-> Detail](auth/FUTURE_ENHANCEMENTS.md#ldap-dn-and-filter-injection-prevention) |
            """
        ),
    )
    write_file(
        tmp_path / "src" / "auth" / "FUTURE_ENHANCEMENTS.md",
        textwrap.dedent(
            """\
            ### LDAP DN and Filter Injection Prevention
            - [ ] Escape DN values
            """
        ),
    )

    changed = sync.backfill_roadmap(
        roadmap,
        {"roadmap:1:auth:v1.1.0:ldap-dn-and-filter-injection-prevention": 1234},
    )

    content = roadmap.read_text(encoding="utf-8")
    assert changed is True
    assert "| # | Module | Title | Target | Labels | Issue | Detail |" in content
    assert "| 1 | auth | LDAP DN and Filter Injection Prevention | v1.1.0 | `security`, `module:auth` | #1234 | [-> Detail](auth/FUTURE_ENHANCEMENTS.md#ldap-dn-and-filter-injection-prevention) |" in content


def test_compute_roadmap_summary_counts_linked_and_missing(tmp_path):
    roadmap = write_file(
        tmp_path / "src" / "ROADMAP.md",
        textwrap.dedent(
            """\
            ## Critical Priority

            | # | Module | Title | Target | Labels | Issue | Detail |
            |---|--------|-------|--------|--------|-------|--------|
            | 1 | auth | LDAP DN and Filter Injection Prevention | v1.1.0 | `security`, `module:auth` | #1234 | [-> Detail](auth/FUTURE_ENHANCEMENTS.md#ldap-dn-and-filter-injection-prevention) |
            | 2 | auth | Constant-Time Comparison | v1.1.0 | `security`, `module:auth` |  | [-> Detail](auth/FUTURE_ENHANCEMENTS.md#constant-time-comparison) |
            """
        ),
    )

    summary = sync.compute_roadmap_summary(roadmap)

    assert summary["total"] == 2
    assert summary["linked"] == 1
    assert summary["missing"] == 1
    assert summary["by_priority"] == [
        {"priority_heading": "Critical Priority", "total": 2, "linked": 1, "missing": 1}
    ]
    assert summary["missing_rows"][0]["row_number"] == "2"


def test_write_summary_artifacts_writes_json_and_markdown(tmp_path):
    preview_dir = tmp_path / "artifacts" / "roadmap-issues"
    summary = {
        "total": 2,
        "linked": 2,
        "missing": 0,
        "by_priority": [
            {"priority_heading": "Critical Priority", "total": 2, "linked": 2, "missing": 0}
        ],
        "missing_rows": [],
    }

    json_path, md_path = sync.write_summary_artifacts(preview_dir, summary)

    assert json_path.exists()
    assert md_path.exists()
    assert '"missing": 0' in json_path.read_text(encoding="utf-8")
    assert "# Roadmap Issue Summary" in md_path.read_text(encoding="utf-8")