#!/usr/bin/env python3
"""
validate_roadmap.py - Automated ROADMAP.md structure and quality validator.

Usage:
    python .github/scripts/validate_roadmap.py ROADMAP.md
    python .github/scripts/validate_roadmap.py --check-all-modules
    python .github/scripts/validate_roadmap.py --generate-report > roadmap_report.html
"""

import argparse
import re
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

REPO_ROOT = Path(__file__).resolve().parent.parent.parent

REQUIRED_SECTIONS = [
    "Current Status",
    "Implementation Phases",
]

# Every 'Implementation Phases' block must contain at least these phase labels
REQUIRED_PHASE_LABELS = [
    "Design",
    "Core",
    "Error",
    "Test",
    "Perf",
    "Doc",
]

VALID_CHECKBOX_STATUSES = {"[ ]", "[x]", "[~]", "[I]", "[P]", "[?]", "[!]"}

TARGET_PATTERN = re.compile(r"\(Target:\s*Q[1-4]\s+\d{4}\)")

VAGUE_PATTERNS = [
    re.compile(r"\bimprove\b", re.IGNORECASE),
    re.compile(r"\boptimize\b", re.IGNORECASE),
    re.compile(r"\benhance\b", re.IGNORECASE),
    re.compile(r"\bbetter\b", re.IGNORECASE),
    re.compile(r"\brefactor\b", re.IGNORECASE),
]

# Pattern to detect task lines (lines starting with "- [")
TASK_LINE_PATTERN = re.compile(r"^\s*-\s+(\[[^\]]\])\s+(.+)$")

# ---------------------------------------------------------------------------
# Validation helpers
# ---------------------------------------------------------------------------


def _load(path: Path) -> list[str]:
    with open(path, encoding="utf-8") as fh:
        return fh.readlines()


def validate_required_sections(lines: list[str], errors: list[str]) -> None:
    """Ensure all required top-level sections are present."""
    headings = {
        line.strip().lstrip("#").strip()
        for line in lines
        if line.startswith("#")
    }
    for section in REQUIRED_SECTIONS:
        if not any(section.lower() in h.lower() for h in headings):
            errors.append(f"Missing required section: '{section}'")


def validate_implementation_phases(lines: list[str], errors: list[str]) -> None:
    """
    Verify that the 'Implementation Phases' section exists and contains at
    least one subsection matching each required phase label keyword.
    """
    in_phases = False
    phase_labels_found: set[str] = set()

    for line in lines:
        stripped = line.strip().lstrip("#").strip()
        if "implementation phases" in stripped.lower():
            in_phases = True
            continue
        if in_phases:
            # Stop at the next same-or-higher heading
            if line.startswith("# "):
                break
            if line.startswith("## ") and "implementation phases" not in line.lower():
                break
            for label in REQUIRED_PHASE_LABELS:
                if label.lower() in stripped.lower():
                    phase_labels_found.add(label)

    if not in_phases:
        errors.append("Section 'Implementation Phases' not found.")
        return

    for label in REQUIRED_PHASE_LABELS:
        if label not in phase_labels_found:
            errors.append(
                f"Implementation Phases: missing required phase covering '{label}'"
            )


def validate_task_lines(
    lines: list[str], warnings: list[str]
) -> None:
    """
    For every task line (- [X] …):
    - Checkbox status must be in VALID_CHECKBOX_STATUSES
    - Line should carry a (Target: Q# YYYY) marker
    """
    for lineno, line in enumerate(lines, start=1):
        match = TASK_LINE_PATTERN.match(line)
        if not match:
            continue
        checkbox, rest = match.group(1), match.group(2)

        if checkbox not in VALID_CHECKBOX_STATUSES:
            warnings.append(
                f"Line {lineno}: invalid checkbox status '{checkbox}' "
                f"(allowed: {', '.join(sorted(VALID_CHECKBOX_STATUSES))})"
            )

        if not TARGET_PATTERN.search(rest):
            warnings.append(
                f"Line {lineno}: task is missing a Target quarter "
                f"'(Target: Q# YYYY)' — {rest[:60].rstrip()}"
            )


def validate_vagueness(lines: list[str], warnings: list[str]) -> None:
    """Flag task lines that use vague verbs without a measurable goal."""
    for lineno, line in enumerate(lines, start=1):
        if not TASK_LINE_PATTERN.match(line):
            continue
        for pattern in VAGUE_PATTERNS:
            if pattern.search(line):
                warnings.append(
                    term = re.sub(r"\\b", "", pattern.pattern).lower()
                warnings.append(
                    f"Line {lineno}: vague term '{term}' "
                    f"detected — add a measurable goal. '{line.rstrip()[:80]}'"
                )
                break  # one warning per line


# ---------------------------------------------------------------------------
# Main validation entry point
# ---------------------------------------------------------------------------


def validate_roadmap(path: Path) -> tuple[list[str], list[str]]:
    """
    Validate a single ROADMAP file.

    Returns
    -------
    errors   : list of blocking issues (exit code 1 if non-empty)
    warnings : list of advisory issues (exit code 0, printed but not blocking)
    """
    errors: list[str] = []
    warnings: list[str] = []

    if not path.exists():
        errors.append(f"File not found: {path}")
        return errors, warnings

    lines = _load(path)

    validate_required_sections(lines, errors)
    validate_implementation_phases(lines, errors)
    validate_task_lines(lines, warnings)
    validate_vagueness(lines, warnings)

    return errors, warnings


def check_all_modules(repo_root: Path) -> dict[Path, tuple[list[str], list[str]]]:
    """Find and validate every *roadmap*.md / *ROADMAP*.md in the repo."""
    results: dict[Path, tuple[list[str], list[str]]] = {}
    for candidate in sorted(repo_root.rglob("*[Rr][Oo][Aa][Dd][Mm][Aa][Pp]*.md")):
        # Skip files inside .git
        if ".git" in candidate.parts:
            continue
        results[candidate] = validate_roadmap(candidate)
    return results


# ---------------------------------------------------------------------------
# Report generation
# ---------------------------------------------------------------------------


def _html_escape(text: str) -> str:
    return (
        text.replace("&", "&amp;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
        .replace('"', "&quot;")
    )


def generate_html_report(
    results: dict[Path, tuple[list[str], list[str]]], repo_root: Path
) -> str:
    rows: list[str] = []
    for path, (errors, warnings) in results.items():
        rel = path.relative_to(repo_root)
        status = "✅ PASS" if not errors else "❌ FAIL"
        color = "#d4edda" if not errors else "#f8d7da"
        issues = "".join(
            f"<li style='color:red'>{_html_escape(e)}</li>" for e in errors
        ) + "".join(
            f"<li style='color:orange'>{_html_escape(w)}</li>" for w in warnings
        )
        rows.append(
            f"<tr style='background:{color}'>"
            f"<td>{_html_escape(str(rel))}</td>"
            f"<td>{status}</td>"
            f"<td><ul>{issues}</ul></td>"
            f"</tr>"
        )

    table = "\n".join(rows)
    return f"""<!DOCTYPE html>
<html lang="en">
<head><meta charset="UTF-8"><title>Roadmap Validation Report</title>
<style>body{{font-family:sans-serif;margin:2rem}}
table{{border-collapse:collapse;width:100%}}
th,td{{border:1px solid #ccc;padding:.5rem;text-align:left;vertical-align:top}}
th{{background:#343a40;color:#fff}}</style>
</head>
<body>
<h1>ThemisDB — Roadmap Validation Report</h1>
<table>
<thead><tr><th>File</th><th>Status</th><th>Issues / Warnings</th></tr></thead>
<tbody>{table}</tbody>
</table>
</body></html>
"""


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def _print_result(
    label: str, errors: list[str], warnings: list[str]
) -> None:
    if errors or warnings:
        print(f"\n{'─'*60}")
        print(f"📄 {label}")
    for err in errors:
        print(f"  ❌  {err}")
    for warn in warnings:
        print(f"  ⚠️   {warn}")
    if not errors and not warnings:
        print(f"  ✅  {label}: no issues found")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Validate ROADMAP.md structure and quality."
    )
    parser.add_argument(
        "roadmap",
        nargs="?",
        default=None,
        help="Path to a single roadmap file to validate.",
    )
    parser.add_argument(
        "--check-all-modules",
        action="store_true",
        help="Scan the entire repository for roadmap files and validate all.",
    )
    parser.add_argument(
        "--generate-report",
        action="store_true",
        help="Output an HTML report to stdout (use with --check-all-modules).",
    )
    args = parser.parse_args(argv)

    if args.check_all_modules:
        all_results = check_all_modules(REPO_ROOT)
        if not all_results:
            print("No roadmap files found.")
            return 0

        if args.generate_report:
            print(generate_html_report(all_results, REPO_ROOT))
            return 0

        overall_errors = 0
        for path, (errors, warnings) in all_results.items():
            rel = path.relative_to(REPO_ROOT)
            _print_result(str(rel), errors, warnings)
            overall_errors += len(errors)

        print(f"\n{'='*60}")
        print(f"Validated {len(all_results)} roadmap file(s).")
        if overall_errors:
            print(f"❌ {overall_errors} blocking error(s) found.")
            return 1
        print("✅ All roadmap files passed validation.")
        return 0

    if args.roadmap:
        path = Path(args.roadmap).resolve()
        errors, warnings = validate_roadmap(path)
        _print_result(str(path), errors, warnings)
        return 1 if errors else 0

    parser.print_help()
    return 0


if __name__ == "__main__":
    sys.exit(main())
