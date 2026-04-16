#!/usr/bin/env python3
"""
Unit tests for tools/primary_docs_indexer.py

Covers:
- classify_doc_type() recognises all canonical doc-type patterns
- detect_status() reads status markers from file content
- fs_last_modified() returns an ISO-8601 timestamp
- scan_directory() discovers only primary docs with known extensions
- write_json() / write_yaml() produce correct output structure
- main() CLI writes a valid index file and exits 0
- main() handles --no-git, --include-root, --format yaml, --quiet
- main() returns 1 for invalid --repo-root
- print_primary_index_summary.main() renders a Markdown table
"""
from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any, Dict
from unittest.mock import patch

import pytest

# ---------------------------------------------------------------------------
# Make tools/ and tools/ci/ importable
# ---------------------------------------------------------------------------
_HERE  = Path(__file__).resolve().parent        # tools/tests/
_TOOLS = _HERE.parent                           # tools/
_CI    = _TOOLS / "ci"                          # tools/ci/

for _p in (_TOOLS, _CI):
    if str(_p) not in sys.path:
        sys.path.insert(0, str(_p))

from primary_docs_indexer import (              # noqa: E402
    classify_doc_type,
    detect_status,
    fs_last_modified,
    scan_directory,
    write_json,
    write_yaml,
    build_arg_parser,
    main,
    PrimaryIndex,
    DocEntry,
    DOC_EXTENSIONS,
    DOC_TYPE_PATTERNS,
)
from print_primary_index_summary import main as summary_main  # noqa: E402


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _make_index(entries=None) -> PrimaryIndex:
    """Create a minimal PrimaryIndex for serialisation tests."""
    return PrimaryIndex(
        generated_at="2026-01-01T00:00:00Z",
        generator="test",
        repo_root="/repo",
        scan_dirs=["src"],
        total_files=len(entries or []),
        entries=entries or [],
    )


# ---------------------------------------------------------------------------
# classify_doc_type
# ---------------------------------------------------------------------------

class TestClassifyDocType:
    """classify_doc_type must recognise all canonical doc-type names."""

    @pytest.mark.parametrize("stem,expected", [
        ("README",           "README"),
        ("readme",           "README"),
        ("Readme",           "README"),
        ("ARCHITECTURE",     "ARCHITECTURE"),
        ("architecture",     "ARCHITECTURE"),
        ("CHANGELOG",        "CHANGELOG"),
        ("CONTRIBUTING",     "CONTRIBUTING"),
        ("LICENSE",          "LICENSE"),
        ("SECURITY",         "SECURITY"),
        ("CODE_OF_CONDUCT",  "CODE_OF_CONDUCT"),
        ("code-of-conduct",  "CODE_OF_CONDUCT"),
        ("codeofconduct",    "CODE_OF_CONDUCT"),
        ("SUPPORT",          "SUPPORT"),
        ("SETUP",            "SETUP"),
        ("INSTALL",          "INSTALL"),
        ("INSTALLATION",     "INSTALL"),
        ("MAINTAINERS",      "MAINTAINERS"),
        ("MAINTAINER",       "MAINTAINERS"),
        ("AUTHORS",          "AUTHORS"),
        ("AUTHOR",           "AUTHORS"),
        ("NOTICE",           "NOTICE"),
        ("TODO",             "TODO"),
        ("ROADMAP",          "ROADMAP"),
        ("FAQ",              "FAQ"),
        ("HACKING",          "HACKING"),
        ("DEVELOPMENT",      "DEVELOPMENT"),
        ("DESIGN",           "DESIGN"),
        ("INDEX",            "INDEX"),
    ])
    def test_known_patterns(self, stem: str, expected: str):
        assert classify_doc_type(stem) == expected

    def test_unknown_stem_returns_none(self):
        assert classify_doc_type("UNKNOWN_FILE") is None

    def test_empty_stem_returns_none(self):
        assert classify_doc_type("") is None

    def test_partial_match_returns_none(self):
        # "readmefile" should NOT match the ^readme$ pattern
        assert classify_doc_type("readmefile") is None


# ---------------------------------------------------------------------------
# detect_status
# ---------------------------------------------------------------------------

class TestDetectStatus:
    """detect_status must identify editorial state from the first 10 lines."""

    def _write(self, tmp_path: Path, content: str) -> Path:
        p = tmp_path / "DOC.md"
        p.write_text(content, encoding="utf-8")
        return p

    def test_active_when_no_marker(self, tmp_path):
        p = self._write(tmp_path, "# A normal document\n\nContent here.\n")
        assert detect_status(p) == "active"

    def test_draft_marker(self, tmp_path):
        p = self._write(tmp_path, "# Title\n<!-- DRAFT -->\n")
        assert detect_status(p) == "draft"

    def test_wip_marker(self, tmp_path):
        p = self._write(tmp_path, "# Title\n> WIP\n")
        assert detect_status(p) == "wip"

    def test_deprecated_marker(self, tmp_path):
        p = self._write(tmp_path, "# Title\nDEPRECATED: use the new API.\n")
        assert detect_status(p) == "deprecated"

    def test_obsolete_marker_maps_to_deprecated(self, tmp_path):
        p = self._write(tmp_path, "OBSOLETE doc\n")
        assert detect_status(p) == "deprecated"

    def test_archived_marker(self, tmp_path):
        p = self._write(tmp_path, "ARCHIVED\n")
        assert detect_status(p) == "archived"

    def test_todo_maps_to_incomplete(self, tmp_path):
        p = self._write(tmp_path, "# Title\nTODO: finish this.\n")
        assert detect_status(p) == "incomplete"

    def test_stub_maps_to_incomplete(self, tmp_path):
        p = self._write(tmp_path, "STUB documentation placeholder\n")
        assert detect_status(p) == "incomplete"

    def test_marker_beyond_scan_window_ignored(self, tmp_path):
        # Put the marker on line 11 — outside STATUS_SCAN_LINES (10).
        lines = ["line {}\n".format(i) for i in range(10)]
        lines.append("DRAFT on line 11\n")
        p = self._write(tmp_path, "".join(lines))
        assert detect_status(p) == "active"

    def test_nonexistent_file_returns_active(self, tmp_path):
        missing = tmp_path / "nonexistent.md"
        assert detect_status(missing) == "active"


# ---------------------------------------------------------------------------
# fs_last_modified
# ---------------------------------------------------------------------------

class TestFsLastModified:
    def test_returns_iso8601_utc_string(self, tmp_path):
        p = tmp_path / "doc.md"
        p.write_text("hello")
        ts = fs_last_modified(p)
        # e.g. "2026-01-01T12:00:00Z"
        assert ts.endswith("Z")
        assert len(ts) == 20
        assert ts[4] == "-" and ts[7] == "-"
        assert ts[10] == "T"


# ---------------------------------------------------------------------------
# scan_directory
# ---------------------------------------------------------------------------

class TestScanDirectory:
    """scan_directory must discover primary docs and ignore non-primary files."""

    def _create_tree(self, root: Path) -> None:
        """Create a small synthetic directory tree."""
        # Primary docs (should be discovered)
        (root / "README.md").write_text("# Readme")
        (root / "CHANGELOG.md").write_text("# Changelog")
        sub = root / "sub"
        sub.mkdir()
        (sub / "ARCHITECTURE.md").write_text("# Architecture")
        # Non-primary doc names (should be ignored)
        (root / "my_code.md").write_text("just some markdown")
        (root / "config.txt").write_text("cfg")
        # Source file (wrong extension)
        (root / "main.cpp").write_text("int main() {}")

    def test_discovers_primary_docs(self, tmp_path):
        self._create_tree(tmp_path)
        entries = scan_directory(tmp_path, tmp_path, "src", recursive=True, use_git=False)
        paths = {e.path for e in entries}
        assert "README.md" in paths
        assert "CHANGELOG.md" in paths
        assert "sub/ARCHITECTURE.md" in paths

    def test_ignores_non_primary_names(self, tmp_path):
        self._create_tree(tmp_path)
        entries = scan_directory(tmp_path, tmp_path, "src", recursive=True, use_git=False)
        paths = {e.path for e in entries}
        assert "my_code.md" not in paths
        assert "config.txt" not in paths
        assert "main.cpp" not in paths

    def test_non_recursive_does_not_descend(self, tmp_path):
        self._create_tree(tmp_path)
        entries = scan_directory(tmp_path, tmp_path, ".", recursive=False, use_git=False)
        paths = {e.path for e in entries}
        # Root-level files should be found
        assert "README.md" in paths
        # Sub-directory files should NOT be found
        assert "sub/ARCHITECTURE.md" not in paths

    def test_missing_directory_returns_empty(self, tmp_path):
        missing = tmp_path / "no_such_dir"
        entries = scan_directory(missing, tmp_path, "src", recursive=True, use_git=False)
        assert entries == []

    def test_entry_fields_are_populated(self, tmp_path):
        (tmp_path / "README.md").write_text("# Title\n")
        entries = scan_directory(tmp_path, tmp_path, "src", recursive=False, use_git=False)
        assert len(entries) == 1
        e = entries[0]
        assert e.type == "README"
        assert e.extension == ".md"
        assert e.scan_dir == "src"
        assert e.size_bytes > 0
        assert e.last_modified is not None
        assert e.status in {"active", "draft", "wip", "deprecated", "archived", "incomplete"}


# ---------------------------------------------------------------------------
# write_json / write_yaml
# ---------------------------------------------------------------------------

class TestWriteJson:
    def test_creates_file_with_correct_structure(self, tmp_path):
        out = tmp_path / "out.json"
        idx = _make_index()
        write_json(idx, out, pretty=True)
        assert out.exists()
        data = json.loads(out.read_text())
        assert data["generated_at"] == "2026-01-01T00:00:00Z"
        assert data["total_files"] == 0
        assert isinstance(data["entries"], list)

    def test_creates_parent_directory(self, tmp_path):
        out = tmp_path / "nested" / "dir" / "out.json"
        write_json(_make_index(), out, pretty=True)
        assert out.exists()

    def test_compact_output_no_indentation(self, tmp_path):
        entry = DocEntry(
            path="src/README.md", type="README", extension=".md",
            scan_dir="src", last_modified=None, status="active", size_bytes=10,
        )
        out = tmp_path / "out.json"
        write_json(_make_index([entry]), out, pretty=False)
        raw = out.read_text()
        # Compact JSON should not start with "{\n  "
        assert "\n  " not in raw

    def test_pretty_output_has_indentation(self, tmp_path):
        out = tmp_path / "out.json"
        write_json(_make_index(), out, pretty=True)
        raw = out.read_text()
        assert "\n" in raw


class TestWriteYaml:
    def test_creates_file(self, tmp_path):
        pytest.importorskip("yaml")
        out = tmp_path / "out.yaml"
        write_yaml(_make_index(), out)
        assert out.exists()
        import yaml  # type: ignore[import]
        data = yaml.safe_load(out.read_text())
        assert data["generated_at"] == "2026-01-01T00:00:00Z"


# ---------------------------------------------------------------------------
# main() CLI
# ---------------------------------------------------------------------------

class TestMain:
    """Integration tests exercising the main() entry point."""

    def test_default_run_creates_json(self, tmp_path):
        out = tmp_path / "index.json"
        rc = main([
            "--repo-root", str(tmp_path),
            "--output", str(out),
            "--scan-dirs", "",
            "--no-git",
            "--quiet",
        ])
        assert rc == 0
        assert out.exists()
        data = json.loads(out.read_text())
        assert "generated_at" in data
        assert "entries" in data

    def test_yaml_format(self, tmp_path):
        pytest.importorskip("yaml")
        out = tmp_path / "index.yaml"
        rc = main([
            "--repo-root", str(tmp_path),
            "--output", str(out),
            "--format", "yaml",
            "--scan-dirs", "",
            "--no-git",
            "--quiet",
        ])
        assert rc == 0
        assert out.exists()

    def test_include_root_scans_repo_root(self, tmp_path):
        (tmp_path / "README.md").write_text("# Root readme")
        out = tmp_path / "index.json"
        rc = main([
            "--repo-root", str(tmp_path),
            "--output", str(out),
            "--scan-dirs", "",
            "--include-root",
            "--no-git",
            "--quiet",
        ])
        assert rc == 0
        data = json.loads(out.read_text())
        paths = [e["path"] for e in data["entries"]]
        assert "README.md" in paths

    def test_no_git_flag_skips_git(self, tmp_path):
        """With --no-git the script should not invoke git and still succeed."""
        out = tmp_path / "index.json"
        with patch("primary_docs_indexer.git_last_modified") as mock_git:
            rc = main([
                "--repo-root", str(tmp_path),
                "--output", str(out),
                "--scan-dirs", "",
                "--no-git",
                "--quiet",
            ])
        assert rc == 0
        mock_git.assert_not_called()

    def test_invalid_repo_root_returns_1(self, tmp_path):
        rc = main(["--repo-root", str(tmp_path / "no_such_dir")])
        assert rc == 1

    def test_scan_finds_docs_in_subdir(self, tmp_path):
        src = tmp_path / "src"
        src.mkdir()
        (src / "README.md").write_text("# Readme")
        out = tmp_path / "index.json"
        rc = main([
            "--repo-root", str(tmp_path),
            "--output", str(out),
            "--scan-dirs", "src",
            "--no-git",
            "--quiet",
        ])
        assert rc == 0
        data = json.loads(out.read_text())
        assert data["total_files"] >= 1
        assert any("README" in e["type"] for e in data["entries"])

    def test_quiet_suppresses_stdout(self, tmp_path, capsys):
        out = tmp_path / "index.json"
        main([
            "--repo-root", str(tmp_path),
            "--output", str(out),
            "--scan-dirs", "",
            "--no-git",
            "--quiet",
        ])
        captured = capsys.readouterr()
        assert captured.out == ""

    def test_verbose_prints_summary(self, tmp_path, capsys):
        src = tmp_path / "src"
        src.mkdir()
        (src / "README.md").write_text("# Readme")
        out = tmp_path / "index.json"
        main([
            "--repo-root", str(tmp_path),
            "--output", str(out),
            "--scan-dirs", "src",
            "--no-git",
        ])
        captured = capsys.readouterr()
        assert "Primary docs index written to" in captured.out
        assert "Total entries" in captured.out


# ---------------------------------------------------------------------------
# print_primary_index_summary.main()
# ---------------------------------------------------------------------------

class TestPrintPrimaryIndexSummary:
    def test_prints_markdown_table(self, tmp_path, capsys):
        data: Dict[str, Any] = {
            "generated_at": "2026-01-01T00:00:00Z",
            "total_files": 2,
            "scan_dirs": ["src", "include"],
            "entries": [
                {"type": "README",       "path": "src/README.md"},
                {"type": "ARCHITECTURE", "path": "src/ARCHITECTURE.md"},
            ],
        }
        index_file = tmp_path / "primary_index.json"
        index_file.write_text(json.dumps(data))
        rc = summary_main.__wrapped__(index_file) if hasattr(summary_main, "__wrapped__") \
            else _run_summary_main(index_file, capsys)
        out = capsys.readouterr().out
        assert "README" in out
        assert "ARCHITECTURE" in out
        assert "2026-01-01T00:00:00Z" in out

    def test_missing_file_returns_0(self, tmp_path, capsys):
        missing = tmp_path / "nonexistent.json"
        rc = _run_summary_main(missing, capsys)
        assert rc == 0

    def test_entries_with_multiple_types(self, tmp_path, capsys):
        data: Dict[str, Any] = {
            "generated_at": "2026-01-02T00:00:00Z",
            "total_files": 3,
            "scan_dirs": ["src"],
            "entries": [
                {"type": "README",    "path": "src/README.md"},
                {"type": "README",    "path": "include/README.md"},
                {"type": "CHANGELOG", "path": "src/CHANGELOG.md"},
            ],
        }
        index_file = tmp_path / "primary_index.json"
        index_file.write_text(json.dumps(data))
        _run_summary_main(index_file, capsys)
        out = capsys.readouterr().out
        assert "README" in out
        assert "CHANGELOG" in out
        # Two READMEs → count "2"
        assert "2" in out

    def test_empty_entries_prints_no_files_message(self, tmp_path, capsys):
        data: Dict[str, Any] = {
            "generated_at": "2026-01-01T00:00:00Z",
            "total_files": 0,
            "scan_dirs": ["src"],
            "entries": [],
        }
        index_file = tmp_path / "primary_index.json"
        index_file.write_text(json.dumps(data))
        _run_summary_main(index_file, capsys)
        out = capsys.readouterr().out
        assert "No primary documentation files" in out


def _run_summary_main(index_file: Path, capsys) -> int:
    """Helper to call summary_main() with a positional path argument."""
    with patch("sys.argv", ["print_primary_index_summary.py", str(index_file)]):
        return summary_main()
