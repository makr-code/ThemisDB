"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            doc-header-check.py                                ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 04:15:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     462                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 72315421ac  2026-03-12  docs: fix audit gaps — validate-docs.sh, Wiki/Archive str... ║
    • 1f04e03bd3  2026-03-11  docs: implement documentation system infrastructure (Phas... ║
    • cd0e6cba85  2026-03-09  Create doc-header-check.py script       ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
ThemisDB Documentation Header Checker

Validates documentation headers against docs/_standards/doc_header.schema.yml.

Usage:
  python3 scripts/doc-header-check.py [--mode changed-only|all]
                                       [--base-ref BRANCH]
                                       [--schema PATH]
                                       [--repo-root PATH]
                                       [--format text|json]
                                       [--fail-on-warnings]

Exit codes:
  0 = all checks passed
  1 = errors found (or warnings when --fail-on-warnings is set)
"""

import argparse
import fnmatch
import json
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple

try:
    import yaml
    HAS_YAML = True
except ImportError:
    HAS_YAML = False


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def find_repo_root() -> Path:
    """Locate repository root via git, falling back to script's parent."""
    try:
        result = subprocess.run(
            ["git", "rev-parse", "--show-toplevel"],
            capture_output=True,
            text=True,
            check=True,
        )
        return Path(result.stdout.strip())
    except Exception:
        return Path(__file__).resolve().parent.parent


def load_schema(schema_path: Path) -> dict:
    """Load the doc_header schema YAML file; return empty dict on failure."""
    if not schema_path.exists():
        return {}
    if not HAS_YAML:
        return {}
    try:
        with open(schema_path, "r", encoding="utf-8") as fh:
            return yaml.safe_load(fh) or {}
    except Exception:
        return {}


# ---------------------------------------------------------------------------
# File discovery
# ---------------------------------------------------------------------------

def _matches_exclude(rel_posix: str, excludes: List[str]) -> bool:
    """Return True when a relative path matches any exclude pattern.

    Converts glob-style patterns like ``docs/**/ARCHIVED/**`` into a
    substring check on the specific path component that must be present
    (everything before the trailing ``/**`` and after ``docs/**/``).
    """
    for exc in excludes:
        # Fast path: try fnmatch on the full relative path
        if fnmatch.fnmatch(rel_posix, exc):
            return True
        # Also match any *prefix* of the path against the pattern so that
        # "docs/**/ARCHIVED/**" catches "docs/de/ARCHIVED/foo.md".
        # We do this by checking whether any path segment listed in the
        # exclude pattern (the non-wildcard parts that are not "docs") are
        # present as a standalone directory component in the path.
        path_parts = rel_posix.split("/")
        exc_specific = [
            p for p in exc.replace("**", "").split("/")
            if p and p != "*" and p != "docs"
        ]
        for part in exc_specific:
            if part in path_parts:
                return True
    return False


def is_file_in_scope(file_path: Path, repo_root: Path, schema: dict) -> bool:
    """Return True when the file should be validated per the schema."""
    try:
        rel = file_path.relative_to(repo_root).as_posix()
    except ValueError:
        return False

    # Must live inside docs/
    if not rel.startswith("docs/"):
        return False

    excludes = schema.get("applies_to", {}).get("exclude", [])
    return not _matches_exclude(rel, excludes)


def get_changed_files(base_ref: str, repo_root: Path) -> List[Path]:
    """Return markdown files changed relative to *base_ref*."""
    for extra in [["HEAD"], []]:
        cmd = ["git", "diff", "--name-only", "--diff-filter=ACM", base_ref] + extra
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                cwd=str(repo_root),
            )
            if result.returncode == 0 and result.stdout.strip():
                files = []
                for line in result.stdout.strip().splitlines():
                    p = repo_root / line.strip()
                    if p.suffix == ".md" and p.exists():
                        files.append(p)
                return files
        except Exception:
            continue
    return []


def get_all_docs_files(repo_root: Path, schema: dict) -> List[Path]:
    """Return all docs/*.md files that are in scope according to the schema."""
    excludes = schema.get("applies_to", {}).get("exclude", [])
    files = []
    docs_dir = repo_root / "docs"
    if not docs_dir.exists():
        return files
    for p in sorted(docs_dir.rglob("*.md")):
        try:
            rel = p.relative_to(repo_root).as_posix()
        except ValueError:
            continue
        if not _matches_exclude(rel, excludes):
            files.append(p)
    return files


# ---------------------------------------------------------------------------
# Header validation
# ---------------------------------------------------------------------------

def _read_lines(file_path: Path, max_lines: int) -> Optional[List[str]]:
    try:
        with open(file_path, "r", encoding="utf-8") as fh:
            return [fh.readline() for _ in range(max_lines)]
    except Exception:
        return None


def _check_breadcrumb(lines: List[str], schema: dict) -> List[str]:
    """Validate the breadcrumb navigation line."""
    errors: List[str] = []
    bc_schema = schema.get("breadcrumb", {})
    if not bc_schema.get("required", True):
        return errors

    # Matches a line containing at least two markdown links separated by '>',
    # e.g.: [docs](../../index.md) > [de](../index.md) > [module](./index.md)
    BREADCRUMB_PATTERN = re.compile(r"\[.+?\]\(.+?\).*>.*\[.+?\]\(.+?\)")

    # Find first non-empty line
    for i, line in enumerate(lines, 1):
        stripped = line.strip()
        if not stripped:
            continue
        # Accept a line with at least two markdown links joined by >
        if BREADCRUMB_PATTERN.search(stripped):
            return errors  # valid breadcrumb
        # Also accept a single-link line that contains > (less strict fallback)
        if stripped.startswith("[") and ">" in stripped and re.search(r"\[.+?\]\(.+?\)", stripped):
            return errors
        # First non-empty line is NOT a breadcrumb
        errors.append(
            f"Line {i}: Missing breadcrumb navigation. "
            "Expected link-chain format, e.g.: "
            "[docs](../../index.md) > [de](../index.md) > [module](./index.md) > [doc_kind](./doc_kind.md)"
        )
        return errors

    errors.append("File appears to be empty or has no breadcrumb.")
    return errors


def _check_field_datum(content: str, field_schema: dict) -> List[str]:
    label = field_schema.get("label", "Datum")
    if not field_schema.get("required", True):
        return []
    pattern = re.compile(
        rf"\*\*{re.escape(label)}:\*\*\s*(\d{{4}}-\d{{2}}-\d{{2}})",
        re.IGNORECASE,
    )
    if not pattern.search(content):
        return [f"Missing required field '**{label}:**' with date in YYYY-MM-DD format."]
    return []


def _check_field_status(content: str, field_schema: dict) -> List[str]:
    label = field_schema.get("label", "Status")
    allowed = field_schema.get("allowed", ["draft", "review", "stable", "deprecated", "archived"])
    if not field_schema.get("required", True):
        return []
    # Match **Status:** <value>
    pattern = re.compile(rf"\*\*{re.escape(label)}:\*\*\s*(.+)", re.IGNORECASE)
    match = pattern.search(content)
    if not match:
        return [f"Missing required field '**{label}:**'."]
    # The value may be a pipe-separated list of options used as a template hint
    # (e.g. "draft | review | stable | deprecated | archived").
    # In that case we treat it as a template placeholder, not an error.
    raw_value = match.group(1).strip()
    if "|" in raw_value:
        # Template placeholder — warn, don't error
        return []
    if raw_value not in allowed:
        return [
            f"Field '**{label}:**' has value '{raw_value}'. "
            f"Allowed values: {', '.join(allowed)}."
        ]
    return []


def _check_field_primary(content: str, field_schema: dict) -> List[str]:
    label = field_schema.get("label", "Primary (Quelle der Wahrheit)")
    allowed_prefixes = field_schema.get("allowed_prefixes", ["src/", "include/", "examples/"])
    min_items = field_schema.get("min_items", 1)

    # Find the Primary block — try several label variants
    labels_to_try = [label, "Primary (Quelle der Wahrheit)", "Primary"]
    found_block = False
    for lbl in labels_to_try:
        if re.search(rf"\*\*{re.escape(lbl)}[^:]*:\*\*", content, re.IGNORECASE):
            found_block = True
            break
    if not found_block:
        return [
            f"Missing required field '**{label}:**'. "
            "Add at least one primary source reference (src/, include/, or examples/)."
        ]

    # Count backtick-wrapped paths that start with an allowed prefix
    prefix_pattern = "|".join(re.escape(p) for p in allowed_prefixes)
    items = re.findall(rf"`((?:{prefix_pattern})[^`]*)`", content)
    if len(items) < min_items:
        return [
            f"Field '**{label}:**' needs at least {min_items} source reference(s) "
            f"starting with one of: {', '.join(allowed_prefixes)}."
        ]
    return []


def _check_field_reference(content: str, field_schema: dict) -> List[str]:
    label = field_schema.get("label", "Bezug / Reference")
    labels_to_try = [label, "Bezug / Reference", "Bezug", "Reference", "Referenz"]
    for lbl in labels_to_try:
        if re.search(rf"\*\*{re.escape(lbl)}[^:]*:\*\*", content, re.IGNORECASE):
            return []
    return [
        f"Missing required field '**{label}:**'. "
        "Add at least one reference (Issue/PR number or link)."
    ]


def validate_header(file_path: Path, schema: dict) -> Tuple[List[str], List[str]]:
    """
    Validate the documentation header in *file_path*.

    Returns (errors, warnings).
    """
    errors: List[str] = []
    warnings: List[str] = []

    max_lines = schema.get("enforcement", {}).get("header_must_be_within_first_n_lines", 40)
    lines = _read_lines(file_path, max_lines)
    if lines is None:
        errors.append("Could not read file.")
        return errors, warnings

    content = "".join(lines)

    errors.extend(_check_breadcrumb(lines, schema))

    fields = schema.get("header_fields", {})
    errors.extend(_check_field_datum(content, fields.get("date", {})))
    errors.extend(_check_field_status(content, fields.get("status", {})))
    errors.extend(_check_field_primary(content, fields.get("primary_sources", {})))
    errors.extend(_check_field_reference(content, fields.get("reference", {})))

    return errors, warnings


# ---------------------------------------------------------------------------
# Output helpers
# ---------------------------------------------------------------------------

Result = Dict  # {"file": str, "errors": List[str], "warnings": List[str]}


def print_text_report(
    results: List[Result],
    files_checked: int,
    mode: str,
    base_ref: str,
) -> None:
    if results:
        print(f"Doc Header Check ({mode} mode, base: {base_ref})")
        print(f"Checked {files_checked} file(s)\n")
        for res in results:
            print(f"  {res['file']}")
            for err in res["errors"]:
                print(f"    ERROR:   {err}")
            for warn in res["warnings"]:
                print(f"    WARNING: {warn}")
        total_errors = sum(len(r["errors"]) for r in results)
        total_warnings = sum(len(r["warnings"]) for r in results)
        print(f"\nTotal: {total_errors} error(s), {total_warnings} warning(s)")
    else:
        print(f"Doc Header Check: {files_checked} file(s) checked, all OK.")


def print_json_report(
    results: List[Result],
    files_checked: int,
) -> None:
    total_errors = sum(len(r["errors"]) for r in results)
    total_warnings = sum(len(r["warnings"]) for r in results)
    payload = {
        "files_checked": files_checked,
        "errors": total_errors,
        "warnings": total_warnings,
        "results": results,
    }
    print(json.dumps(payload, indent=2))


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="ThemisDB Documentation Header Checker",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--mode",
        choices=["changed-only", "all"],
        default="changed-only",
        help="Which files to validate (default: changed-only).",
    )
    parser.add_argument(
        "--base-ref",
        default="origin/develop",
        help="Base git ref for changed-only mode (default: origin/develop).",
    )
    parser.add_argument(
        "--schema",
        default="docs/_standards/doc_header.schema.yml",
        help="Path to the header schema file.",
    )
    parser.add_argument(
        "--repo-root",
        default=None,
        help="Repository root directory (auto-detected if omitted).",
    )
    parser.add_argument(
        "--format",
        choices=["text", "json"],
        default="text",
        help="Output format (default: text).",
    )
    parser.add_argument(
        "--fail-on-warnings",
        action="store_true",
        help="Exit 1 even when only warnings are present.",
    )
    args = parser.parse_args()

    repo_root = Path(args.repo_root) if args.repo_root else find_repo_root()
    schema_path = repo_root / args.schema
    schema = load_schema(schema_path)

    # Collect files
    if args.mode == "changed-only":
        candidates = get_changed_files(args.base_ref, repo_root)
        files = [f for f in candidates if is_file_in_scope(f, repo_root, schema)]
    else:
        files = get_all_docs_files(repo_root, schema)

    if not files:
        if args.format == "json":
            print_json_report([], 0)
        else:
            print("Doc Header Check: no files to check.")
        return 0

    results: List[Result] = []
    for file_path in sorted(files):
        try:
            rel = file_path.relative_to(repo_root).as_posix()
        except ValueError:
            rel = str(file_path)
        errors, warnings = validate_header(file_path, schema)
        if errors or warnings:
            results.append({"file": rel, "errors": errors, "warnings": warnings})

    if args.format == "json":
        print_json_report(results, len(files))
    else:
        print_text_report(results, len(files), args.mode, args.base_ref)

    total_errors = sum(len(r["errors"]) for r in results)
    total_warnings = sum(len(r["warnings"]) for r in results)

    if total_errors > 0:
        return 1
    if args.fail_on_warnings and total_warnings > 0:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())