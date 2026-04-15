"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_sync_milestones.py                            ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-04-15 18:57:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     280                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Unit tests for scripts/sync-milestones-from-roadmap.py

Run with:  python3 -m pytest tests/test_sync_milestones.py -v
"""

import importlib.util
import sys
import textwrap
from pathlib import Path

import pytest

# ---------------------------------------------------------------------------
# Import the module under test
# ---------------------------------------------------------------------------

_SCRIPT = Path(__file__).parent.parent / "scripts" / "sync-milestones-from-roadmap.py"


def _load_module():
    spec = importlib.util.spec_from_file_location("sync_milestones", _SCRIPT)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


sync = _load_module()


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def make_roadmap(tmp_path: Path, subdir: str, content: str) -> Path:
    """Create a fake src/<subdir>/ROADMAP.md file."""
    d = tmp_path / "src" / subdir
    d.mkdir(parents=True, exist_ok=True)
    p = d / "ROADMAP.md"
    p.write_text(content, encoding="utf-8")
    return p


# ---------------------------------------------------------------------------
# parse_roadmaps
# ---------------------------------------------------------------------------


class TestParseRoadmaps:

    def test_basic_mapping(self, tmp_path):
        make_roadmap(
            tmp_path,
            "mymod",
            textwrap.dedent("""\
                # My Module Roadmap
                - [P] Implement thing (Target: Q2 2026) (Issue: #42)
            """),
        )
        result = sync.parse_roadmaps(tmp_path / "src")
        assert 42 in result
        assert result[42]["milestone"] == "Q2 2026"
        assert "mymod" in result[42]["roadmap"]

    def test_multiple_issues_on_one_line(self, tmp_path):
        make_roadmap(
            tmp_path,
            "alpha",
            "- [I] Some thing (Target: Q3 2026) (Issue: #10) (Issue: #11)\n",
        )
        result = sync.parse_roadmaps(tmp_path / "src")
        assert result[10]["milestone"] == "Q3 2026"
        assert result[11]["milestone"] == "Q3 2026"

    def test_issue_without_target_excluded(self, tmp_path):
        make_roadmap(
            tmp_path,
            "beta",
            "- [I] No target here (Issue: #99)\n",
        )
        result = sync.parse_roadmaps(tmp_path / "src")
        assert 99 not in result

    def test_multiple_roadmaps(self, tmp_path):
        make_roadmap(
            tmp_path,
            "mod_a",
            "- [P] Feature A (Target: Q2 2026) (Issue: #1)\n",
        )
        make_roadmap(
            tmp_path,
            "mod_b",
            "- [P] Feature B (Target: Q4 2026) (Issue: #2)\n",
        )
        result = sync.parse_roadmaps(tmp_path / "src")
        assert result[1]["milestone"] == "Q2 2026"
        assert result[2]["milestone"] == "Q4 2026"

    def test_first_occurrence_wins(self, tmp_path):
        """If the same issue appears in two roadmaps, first roadmap wins."""
        make_roadmap(
            tmp_path,
            "first",
            "- [P] Feature (Target: Q2 2026) (Issue: #7)\n",
        )
        make_roadmap(
            tmp_path,
            "second",
            "- [P] Feature (Target: Q4 2026) (Issue: #7)\n",
        )
        result = sync.parse_roadmaps(tmp_path / "src")
        # Both map to #7; first alphabetically wins (first/ < second/)
        assert result[7]["milestone"] in ("Q2 2026", "Q4 2026")

    def test_relative_path_in_result(self, tmp_path):
        make_roadmap(
            tmp_path,
            "mymod",
            "- [P] Feature (Target: Q3 2026) (Issue: #50)\n",
        )
        result = sync.parse_roadmaps(tmp_path / "src")
        assert not Path(result[50]["roadmap"]).is_absolute()


# ---------------------------------------------------------------------------
# parse_all_issue_refs
# ---------------------------------------------------------------------------


class TestParseAllIssueRefs:

    def test_includes_issues_without_target(self, tmp_path):
        make_roadmap(
            tmp_path,
            "mod",
            textwrap.dedent("""\
                - [I] No target (Issue: #100)
                - [P] With target (Target: Q2 2026) (Issue: #101)
            """),
        )
        refs = sync.parse_all_issue_refs(tmp_path / "src")
        assert 100 in refs
        assert refs[100]["milestone"] is None
        assert 101 in refs
        assert refs[101]["milestone"] == "Q2 2026"


# ---------------------------------------------------------------------------
# _module_from_path
# ---------------------------------------------------------------------------


class TestModuleFromPath:

    def test_relative_unix_path(self):
        assert sync._module_from_path("src/acceleration/ROADMAP.md") == "acceleration"

    def test_relative_windows_path(self):
        assert sync._module_from_path("src\\query\\ROADMAP.md") == "query"

    def test_absolute_path(self):
        assert sync._module_from_path("/home/user/project/src/geo/ROADMAP.md") == "geo"

    def test_unknown_when_no_src(self):
        assert sync._module_from_path("no_src/mod/ROADMAP.md") == "unknown"


# ---------------------------------------------------------------------------
# QUARTER_DUE_DATES sanity checks
# ---------------------------------------------------------------------------


class TestQuarterDueDates:

    def test_all_four_quarters_present(self):
        for q in ("Q2 2026", "Q3 2026", "Q4 2026", "Q1 2027"):
            assert q in sync.QUARTER_DUE_DATES

    def test_due_dates_are_iso_strings(self):
        for key, val in sync.QUARTER_DUE_DATES.items():
            assert "T" in val, f"Expected ISO datetime for {key}: {val}"


# ---------------------------------------------------------------------------
# _quarter_due_date (dynamic calculation)
# ---------------------------------------------------------------------------


class TestQuarterDueDateDynamic:

    def test_known_quarter_uses_table(self):
        result = sync._quarter_due_date("Q2 2026")
        assert result == "2026-06-30T23:59:59Z"

    def test_unknown_future_quarter(self):
        result = sync._quarter_due_date("Q3 2030")
        assert result == "2030-09-30T23:59:59Z"

    def test_q1(self):
        assert sync._quarter_due_date("Q1 2028") == "2028-03-31T23:59:59Z"

    def test_q4(self):
        assert sync._quarter_due_date("Q4 2028") == "2028-12-31T23:59:59Z"

    def test_invalid_returns_none(self):
        assert sync._quarter_due_date("v1.5.0") is None

    def test_none_for_empty_string(self):
        assert sync._quarter_due_date("") is None


# ---------------------------------------------------------------------------
# generate_audit_report
# ---------------------------------------------------------------------------


class TestGenerateAuditReport:

    def test_creates_file(self, tmp_path):
        src_root = tmp_path / "src"
        src_root.mkdir()
        out = tmp_path / "audit.md"
        mapped = {
            42: {"milestone": "Q2 2026", "roadmap": "src/mod/ROADMAP.md", "line": "- line"},
        }
        all_refs = {
            42: {"milestone": "Q2 2026", "roadmap": "src/mod/ROADMAP.md", "line": "- line"},
            99: {"milestone": None, "roadmap": "src/mod/ROADMAP.md", "line": "- no target"},
        }
        sync.generate_audit_report(src_root, all_refs, mapped, [], out)
        assert out.exists()
        content = out.read_text()
        assert "#42" in content
        assert "#99" in content
        assert "Q2 2026" in content

    def test_milestone_marked_missing_when_not_in_existing(self, tmp_path):
        src_root = tmp_path / "src"
        src_root.mkdir()
        out = tmp_path / "audit.md"
        mapped = {1: {"milestone": "Q3 2026", "roadmap": "src/m/ROADMAP.md", "line": ""}}
        all_refs = {1: {"milestone": "Q3 2026", "roadmap": "src/m/ROADMAP.md", "line": ""}}
        sync.generate_audit_report(src_root, all_refs, mapped, [], out)
        content = out.read_text()
        assert "missing" in content

    def test_milestone_marked_exists_when_in_existing(self, tmp_path):
        src_root = tmp_path / "src"
        src_root.mkdir()
        out = tmp_path / "audit.md"
        mapped = {1: {"milestone": "Q3 2026", "roadmap": "src/m/ROADMAP.md", "line": ""}}
        all_refs = {1: {"milestone": "Q3 2026", "roadmap": "src/m/ROADMAP.md", "line": ""}}
        existing = [{"title": "Q3 2026", "number": 5}]
        sync.generate_audit_report(src_root, all_refs, mapped, existing, out)
        content = out.read_text()
        assert "exists" in content
