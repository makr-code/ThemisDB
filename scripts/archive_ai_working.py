#!/usr/bin/env python3
"""archive_ai_working.py — Governance doc freshness validation for src/ai_working/.

Two modes:

--check-freshness
    Validates that all required governance documents in src/ai_working/ are
    present and not placeholders.  Reports findings and exits non-zero on
    failures.  Used as the CI artifact freshness gate (AW-AUD-02).

--archive-stale
    Moves wave-specific stale Markdown files from src/ai_working/ (those that
    are *not* in REQUIRED_GOVERNANCE_DOCS) into docs/ARCHIVED/ai-working-history/
    with a YYYY_MM_DD date prefix.  Used for AW-AUD-01 archival automation.

Usage
-----
    python scripts/archive_ai_working.py --check-freshness
    python scripts/archive_ai_working.py --check-freshness --report-json /tmp/report.json
    python scripts/archive_ai_working.py --archive-stale --dry-run
    python scripts/archive_ai_working.py --archive-stale

Exit codes
----------
    0   All checks pass / archival completed
    1   One or more freshness failures
    2   Usage or configuration error
"""

from __future__ import annotations

import argparse
import json
import re
import shutil
import sys
from dataclasses import asdict, dataclass, field
from datetime import datetime, timezone
from pathlib import Path
from typing import List, Optional, Sequence

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

TOOL_NAME = "archive_ai_working"
TOOL_VERSION = "1.0.0"

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC_AI_WORKING = REPO_ROOT / "src" / "ai_working"
ARCHIVE_BASE = REPO_ROOT / "docs" / "ARCHIVED" / "ai-working-history"

# Required governance documents that must always be present and substantive
REQUIRED_GOVERNANCE_DOCS: set[str] = {
    "README.md",
    "ARCHITECTURE.md",
    "AUDIT.md",
    "CHANGELOG.md",
    "FUTURE_ENHANCEMENTS.md",
    "PERFORMANCE_EXPECTATIONS.md",
    "PRODUCTION_REQUIREMENTS.md",
    "ROADMAP.md",
    "SECURITY.md",
}

# Strings that indicate a file is still a placeholder and has not been filled in
PLACEHOLDER_PATTERNS: list[re.Pattern[str]] = [
    re.compile(r"^#[^\n]+\n+\s*Module placeholder for mirrored", re.IGNORECASE),
    re.compile(r"^#[^\n]+\n+\s*module placeholder for mirrored", re.IGNORECASE),
]

# Files that are always kept and never archived from src/ai_working/
PROTECTED_FILES: set[str] = REQUIRED_GOVERNANCE_DOCS | {"MODULE_GAPS.md", "CMakeLists.txt"}

# Minimum non-whitespace character count for a doc to be considered substantive
MIN_SUBSTANTIVE_CHARS = 200


# ---------------------------------------------------------------------------
# Data model
# ---------------------------------------------------------------------------

@dataclass
class Finding:
    file: str
    severity: str  # "error" | "warning" | "info"
    code: str
    message: str


@dataclass
class FreshnessReport:
    tool: str
    tool_version: str
    generated_at: str
    checked_dir: str
    required_docs: list[str]
    findings: list[Finding] = field(default_factory=list)
    missing: list[str] = field(default_factory=list)
    placeholder: list[str] = field(default_factory=list)
    too_short: list[str] = field(default_factory=list)
    passed: list[str] = field(default_factory=list)

    @property
    def has_errors(self) -> bool:
        return any(f.severity == "error" for f in self.findings)

    def summary(self) -> str:
        lines = [
            f"  Required docs checked : {len(self.required_docs)}",
            f"  Passed                : {len(self.passed)}",
            f"  Missing               : {len(self.missing)}",
            f"  Placeholder content   : {len(self.placeholder)}",
            f"  Too short             : {len(self.too_short)}",
        ]
        return "\n".join(lines)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _is_placeholder(content: str) -> bool:
    """Return True if the file content matches any placeholder pattern."""
    for pat in PLACEHOLDER_PATTERNS:
        if pat.search(content):
            return True
    return False


def _substantive_chars(content: str) -> int:
    """Count non-whitespace characters."""
    return len(content.replace(" ", "").replace("\n", "").replace("\t", ""))


# ---------------------------------------------------------------------------
# Mode 1: Freshness check
# ---------------------------------------------------------------------------

def check_freshness(report_json: Optional[Path], strict: bool) -> FreshnessReport:
    """Validate required governance docs in src/ai_working/."""
    now = datetime.now(timezone.utc).isoformat()
    report = FreshnessReport(
        tool=TOOL_NAME,
        tool_version=TOOL_VERSION,
        generated_at=now,
        checked_dir=str(SRC_AI_WORKING.relative_to(REPO_ROOT)),
        required_docs=sorted(REQUIRED_GOVERNANCE_DOCS),
    )

    if not SRC_AI_WORKING.is_dir():
        report.findings.append(Finding(
            file=str(SRC_AI_WORKING.relative_to(REPO_ROOT)),
            severity="error",
            code="AW-FRESH-001",
            message=f"Directory {SRC_AI_WORKING} does not exist.",
        ))
        return report

    for doc_name in sorted(REQUIRED_GOVERNANCE_DOCS):
        doc_path = SRC_AI_WORKING / doc_name
        rel = str(doc_path.relative_to(REPO_ROOT))

        if not doc_path.exists():
            report.missing.append(doc_name)
            report.findings.append(Finding(
                file=rel,
                severity="error",
                code="AW-FRESH-002",
                message=f"Required governance document is missing: {doc_name}",
            ))
            continue

        try:
            content = doc_path.read_text(encoding="utf-8", errors="replace")
        except OSError as exc:
            report.findings.append(Finding(
                file=rel,
                severity="error",
                code="AW-FRESH-003",
                message=f"Cannot read file: {exc}",
            ))
            continue

        if _is_placeholder(content):
            report.placeholder.append(doc_name)
            report.findings.append(Finding(
                file=rel,
                severity="error",
                code="AW-FRESH-004",
                message=f"{doc_name} still contains placeholder content and has not been filled in.",
            ))
            continue

        char_count = _substantive_chars(content)
        if char_count < MIN_SUBSTANTIVE_CHARS:
            report.too_short.append(doc_name)
            severity = "error" if strict else "warning"
            report.findings.append(Finding(
                file=rel,
                severity=severity,
                code="AW-FRESH-005",
                message=(
                    f"{doc_name} has only {char_count} non-whitespace chars "
                    f"(minimum: {MIN_SUBSTANTIVE_CHARS}). Document may be insufficient."
                ),
            ))
            continue

        report.passed.append(doc_name)

    return report


# ---------------------------------------------------------------------------
# Mode 2: Archive stale files
# ---------------------------------------------------------------------------

def archive_stale(dry_run: bool) -> int:
    """Move wave-specific stale .md files from src/ai_working/ to docs/ARCHIVED/."""
    if not SRC_AI_WORKING.is_dir():
        print(f"ERROR: {SRC_AI_WORKING} does not exist.", file=sys.stderr)
        return 2

    date_label = datetime.now(timezone.utc).strftime("%Y_%m_%d")
    archive_dir = ARCHIVE_BASE / f"stale_{date_label}"

    candidates: list[Path] = []
    for p in sorted(SRC_AI_WORKING.iterdir()):
        if not p.is_file():
            continue
        if p.suffix.lower() != ".md":
            continue
        if p.name in PROTECTED_FILES:
            continue
        candidates.append(p)

    if not candidates:
        print("[archive-stale] No stale markdown files found in src/ai_working/ — nothing to archive.")
        return 0

    if not dry_run:
        archive_dir.mkdir(parents=True, exist_ok=True)

    moved = 0
    for src in candidates:
        dest = archive_dir / src.name
        if dry_run:
            print(f"  [dry-run] would archive: {src.relative_to(REPO_ROOT)} → {dest.relative_to(REPO_ROOT)}")
        else:
            shutil.move(str(src), str(dest))
            print(f"  [archive] {src.relative_to(REPO_ROOT)} → {dest.relative_to(REPO_ROOT)}")
            moved += 1

    if dry_run:
        print(f"[archive-stale] Dry-run: {len(candidates)} file(s) would be archived to {archive_dir.relative_to(REPO_ROOT)}/")
    else:
        print(f"[archive-stale] Archived {moved} file(s) to {archive_dir.relative_to(REPO_ROOT)}/")

    return 0


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main(argv: Optional[Sequence[str]] = None) -> int:
    ap = argparse.ArgumentParser(
        description="ai_working governance doc freshness validation and archival automation.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    mode = ap.add_mutually_exclusive_group(required=True)
    mode.add_argument(
        "--check-freshness",
        action="store_true",
        help="Validate required governance docs in src/ai_working/ (CI gate mode).",
    )
    mode.add_argument(
        "--archive-stale",
        action="store_true",
        help="Archive wave-specific stale .md files from src/ai_working/ to docs/ARCHIVED/.",
    )
    ap.add_argument(
        "--report-json",
        metavar="PATH",
        type=Path,
        help="Write freshness report as JSON to this path (--check-freshness only).",
    )
    ap.add_argument(
        "--strict",
        action="store_true",
        default=False,
        help="Treat too-short documents as errors instead of warnings (--check-freshness only).",
    )
    ap.add_argument(
        "--dry-run",
        action="store_true",
        default=False,
        help="Print actions without modifying files (--archive-stale only).",
    )

    args = ap.parse_args(argv)

    if args.check_freshness:
        report = check_freshness(report_json=args.report_json, strict=args.strict)

        # Print human-readable summary
        print(f"[{TOOL_NAME}] Freshness check: {report.checked_dir}")
        print(report.summary())

        if report.findings:
            print("\nFindings:")
            for f in report.findings:
                prefix = "ERROR" if f.severity == "error" else "WARN "
                print(f"  [{prefix}] [{f.code}] {f.file}: {f.message}")
        else:
            print("\nAll required governance docs are present and substantive.")

        if args.report_json:
            args.report_json.parent.mkdir(parents=True, exist_ok=True)
            data = asdict(report)
            args.report_json.write_text(json.dumps(data, indent=2), encoding="utf-8")
            print(f"\nReport written to {args.report_json}")

        return 1 if report.has_errors else 0

    if args.archive_stale:
        return archive_stale(dry_run=args.dry_run)

    return 2  # unreachable


if __name__ == "__main__":
    sys.exit(main())
