#!/usr/bin/env python3
"""
Unit tests for scripts/docs-lint.py — metadata-checking functionality.

Covers:
- _parse_frontmatter: happy path, missing delimiters, empty YAML, invalid YAML
- check_doc_metadata: valid metadata, missing status, missing doc_version,
  invalid status value, invalid validated date, missing front matter entirely
- CLI --check-metadata mode: passes for compliant files, exits 1 for missing
  or invalid metadata
"""
# ThemisDB - Hybrid Database System
# File:            test_docs_lint.py
# Version:         0.0.1
# Last Modified:   2026-04-15
# Status:          ✅ Production Ready
from __future__ import annotations

import subprocess
import sys
from pathlib import Path
from typing import List

import pytest

# ---------------------------------------------------------------------------
# Ensure scripts/ directory is importable
# ---------------------------------------------------------------------------
_SCRIPTS = Path(__file__).resolve().parent.parent  # scripts/
_REPO_ROOT = _SCRIPTS.parent

if str(_SCRIPTS) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS))

# Import the module under test by loading it as a module (the shebang line is
# fine; importlib handles the #!/usr/bin/env python3 header transparently).
import importlib.util as _ilu

_SPEC = _ilu.spec_from_file_location("docs_lint", _SCRIPTS / "docs-lint.py")
_MOD = _ilu.module_from_spec(_SPEC)
_SPEC.loader.exec_module(_MOD)

DocumentationLinter = _MOD.DocumentationLinter
VALID_DOC_STATUSES = _MOD.VALID_DOC_STATUSES


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _lines(text: str) -> List[str]:
    """Split text into lines the same way open().readlines() would."""
    return [line + "\n" for line in text.splitlines()] or [""]


def _linter() -> DocumentationLinter:
    return DocumentationLinter(base_path=_REPO_ROOT)


# ---------------------------------------------------------------------------
# _parse_frontmatter
# ---------------------------------------------------------------------------


class TestParseFrontmatter:
    def test_valid_frontmatter_returns_dict(self):
        text = "---\nstatus: current\ndoc_version: \"1.5.0\"\n---\n\n# Title\n"
        fm = _linter()._parse_frontmatter(_lines(text))
        assert fm is not None
        assert fm["status"] == "current"
        assert fm["doc_version"] == "1.5.0"

    def test_frontmatter_with_validated_field(self):
        text = (
            "---\nstatus: drifting\ndoc_version: \"1.4.x\"\n"
            "validated: \"2026-01-15\"\n---\n"
        )
        fm = _linter()._parse_frontmatter(_lines(text))
        assert fm is not None
        assert fm["validated"] == "2026-01-15"

    def test_no_frontmatter_returns_none(self):
        text = "# Just a heading\n\nNo front matter here.\n"
        assert _linter()._parse_frontmatter(_lines(text)) is None

    def test_missing_closing_delimiter_returns_none(self):
        text = "---\nstatus: current\ndoc_version: \"1.0.0\"\n"
        assert _linter()._parse_frontmatter(_lines(text)) is None

    def test_empty_file_returns_none(self):
        assert _linter()._parse_frontmatter([]) is None

    def test_frontmatter_not_at_start_returns_none(self):
        text = "\n---\nstatus: current\ndoc_version: \"1.0.0\"\n---\n"
        assert _linter()._parse_frontmatter(_lines(text)) is None


# ---------------------------------------------------------------------------
# check_doc_metadata
# ---------------------------------------------------------------------------


class TestCheckDocMetadata:
    def _check(self, text: str):
        """Run check_doc_metadata and return (errors, warnings)."""
        linter = _linter()
        linter.check_doc_metadata(Path("fake_file.md"), _lines(text))
        return linter.errors, linter.warnings

    def test_valid_metadata_produces_no_errors(self):
        text = (
            "---\nstatus: current\ndoc_version: \"1.5.0\"\n"
            "validated: \"2026-03-09\"\n---\n\n# Title\n"
        )
        errors, warnings = self._check(text)
        assert errors == []
        assert warnings == []

    def test_all_valid_status_values_accepted(self):
        for status in VALID_DOC_STATUSES:
            text = f"---\nstatus: {status}\ndoc_version: \"1.0.0\"\n---\n"
            errors, _ = self._check(text)
            assert errors == [], f"Expected no errors for status={status}"

    def test_missing_front_matter_produces_error(self):
        text = "# No front matter\n"
        errors, _ = self._check(text)
        assert any("Missing YAML front matter" in e["message"] for e in errors)

    def test_missing_status_field_produces_error(self):
        text = "---\ndoc_version: \"1.0.0\"\n---\n"
        errors, _ = self._check(text)
        assert any("'status' is missing" in e["message"] for e in errors)

    def test_invalid_status_value_produces_error(self):
        text = "---\nstatus: obsolete\ndoc_version: \"1.0.0\"\n---\n"
        errors, _ = self._check(text)
        assert any("Invalid metadata 'status'" in e["message"] for e in errors)

    def test_missing_doc_version_produces_error(self):
        text = "---\nstatus: current\n---\n"
        errors, _ = self._check(text)
        assert any("'doc_version' is missing" in e["message"] for e in errors)

    def test_invalid_validated_date_produces_warning(self):
        text = (
            "---\nstatus: current\ndoc_version: \"1.0.0\"\n"
            "validated: \"15-04-2026\"\n---\n"
        )
        _, warnings = self._check(text)
        assert any("validated" in w["message"] for w in warnings)

    def test_valid_validated_date_produces_no_warning(self):
        text = (
            "---\nstatus: current\ndoc_version: \"1.0.0\"\n"
            "validated: \"2026-04-15\"\n---\n"
        )
        errors, warnings = self._check(text)
        assert errors == []
        assert warnings == []


# ---------------------------------------------------------------------------
# CLI: --check-metadata mode
# ---------------------------------------------------------------------------


class TestCLICheckMetadata:
    """Integration-style tests that invoke the script as a subprocess."""

    def _run(self, *extra_args: str) -> subprocess.CompletedProcess:
        return subprocess.run(
            [sys.executable, str(_SCRIPTS / "docs-lint.py"), "--check-metadata",
             *extra_args],
            capture_output=True,
            text=True,
        )

    def test_passes_for_valid_doc(self, tmp_path: Path):
        doc = tmp_path / "good.md"
        doc.write_text(
            "---\nstatus: current\ndoc_version: \"1.5.0\"\n---\n\n# Title\n",
            encoding="utf-8",
        )
        result = self._run("--metadata-paths", str(doc))
        assert result.returncode == 0, result.stdout

    def test_fails_for_doc_without_front_matter(self, tmp_path: Path):
        doc = tmp_path / "bad.md"
        doc.write_text("# No metadata here\n", encoding="utf-8")
        result = self._run("--metadata-paths", str(doc))
        assert result.returncode == 1, result.stdout

    def test_fails_for_invalid_status(self, tmp_path: Path):
        doc = tmp_path / "bad_status.md"
        doc.write_text(
            "---\nstatus: unknown\ndoc_version: \"1.0.0\"\n---\n",
            encoding="utf-8",
        )
        result = self._run("--metadata-paths", str(doc))
        assert result.returncode == 1, result.stdout

    def test_passes_for_known_example_docs(self):
        """The canonical example docs checked by CI must always pass."""
        result = self._run(
            "--metadata-paths",
            "docs/de/development/DOC_METADATA.md",
            "docs/de/development/README.md",
            "docs/de/development/developers.md",
        )
        assert result.returncode == 0, result.stdout
