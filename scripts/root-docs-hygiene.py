"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            root-docs-hygiene.py                               ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 18:48:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     282                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • dbc9bfed9f  2026-04-13  Add CI/CD workflows and scripts for release management ║
    • dd319b9918  2026-04-13  Add CI/CD workflows and scripts for release management ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional

TOOL_NAME = "root-docs-hygiene"
TOOL_VERSION = "1.0.0"
SPECIAL_DOC_FILENAMES = {"REVIEW_SUMMARY.txt"}
IGNORE_FILENAMES = {
    "CMakeLists.txt",
    "copilot_instructions.md",
    "requirements-docs.txt",
    "requirements.txt",
    "sec_block.txt",
}

KEEP_ALLOWLIST = {
    "ARCHITECTURE.md",
    "AUDIT.md",
    "CHANGELOG.md",
    "CODE_OF_CONDUCT.md",
    "CONTRIBUTING.md",
    "CTEST.md",
    "FUTURE_ENHANCEMENTS.MD",
    "GOVERNANCE.md",
    "INDEX.md",
    "MAINTAINERS.md",
    "PERFORMANCE_EXPECTATIONS.md",
    "QUICKSTART.md",
    "README.md",
    "RELEASE_STRATEGY.md",
    "SECURITY.md",
    "SETUP.md",
    "SOP.md",
    "SUPPORT.md",
    "VERSIONING.md",
    "feature_enhancement.md",
    "roadmap.md",
}

ARCHIVE_PATTERNS = [
    re.compile(r"^.*SUMMARY.*\.(md|txt)$", re.IGNORECASE),
    re.compile(r"^GAP_ANALYSIS.*\.md$", re.IGNORECASE),
    re.compile(r"^.*IMPLEMENTATION_SUMMARY.*\.md$", re.IGNORECASE),
    re.compile(r"^REVIEW_SUMMARY\.(md|txt)$", re.IGNORECASE),
    re.compile(r"^CURRENT_STATUS\.md$", re.IGNORECASE),
    re.compile(r"^DEVELOPMENT_STATUS\.md$", re.IGNORECASE),
    re.compile(r"^issue_.*\.md$", re.IGNORECASE),
]

DOCS_PATTERNS = [
    re.compile(r".*GUIDE.*\.md$", re.IGNORECASE),
    re.compile(r".*PLAN.*\.md$", re.IGNORECASE),
    re.compile(r".*STATUS.*\.md$", re.IGNORECASE),
    re.compile(r".*NOTES.*\.md$", re.IGNORECASE),
    re.compile(r".*INDEX.*\.md$", re.IGNORECASE),
]


def find_repo_root() -> Path:
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


@dataclass
class RootDocItem:
    path: str
    category: str
    reason: str


@dataclass
class HygieneReport:
    keep: List[RootDocItem] = field(default_factory=list)
    docs_candidates: List[RootDocItem] = field(default_factory=list)
    archive_candidates: List[RootDocItem] = field(default_factory=list)

    @property
    def total(self) -> int:
        return len(self.keep) + len(self.docs_candidates) + len(self.archive_candidates)

    @property
    def finding_count(self) -> int:
        return len(self.docs_candidates) + len(self.archive_candidates)


def _classify_file(name: str) -> RootDocItem:
    if name in KEEP_ALLOWLIST:
        return RootDocItem(path=name, category="keep", reason="known root-level canonical document")

    for pattern in ARCHIVE_PATTERNS:
        if pattern.match(name):
            return RootDocItem(
                path=name,
                category="archive_candidate",
                reason="matches historical summary/status/archive naming pattern",
            )

    for pattern in DOCS_PATTERNS:
        if pattern.match(name):
            return RootDocItem(
                path=name,
                category="docs_candidate",
                reason="matches guide/plan/status style better suited for docs/",
            )

    return RootDocItem(
        path=name,
        category="docs_candidate",
        reason="top-level markdown file not in canonical root-doc allowlist",
    )


def run_check(repo_root: Path) -> HygieneReport:
    report = HygieneReport()
    for entry in sorted(repo_root.iterdir()):
        if not entry.is_file():
            continue
        if entry.name.startswith("."):
            continue
        if entry.name in IGNORE_FILENAMES:
            continue
        if entry.suffix.lower() != ".md" and entry.name not in SPECIAL_DOC_FILENAMES:
            continue

        item = _classify_file(entry.name)
        if item.category == "keep":
            report.keep.append(item)
        elif item.category == "archive_candidate":
            report.archive_candidates.append(item)
        else:
            report.docs_candidates.append(item)
    return report


def format_text(report: HygieneReport) -> str:
    lines: List[str] = []
    lines.append("-" * 72)
    lines.append(f"  ThemisDB root docs hygiene report  ·  {TOOL_NAME} v{TOOL_VERSION}")
    lines.append("-" * 72)
    lines.append("")
    lines.append(f"  Top-level markdown/text files : {report.total:>4}")
    lines.append(f"  Keep at repo root             : {len(report.keep):>4}")
    lines.append(f"  Move-to-docs candidates       : {len(report.docs_candidates):>4}")
    lines.append(f"  Archive candidates            : {len(report.archive_candidates):>4}")
    lines.append("")

    if report.docs_candidates:
        lines.append("Move-to-docs candidates")
        lines.append("-" * 72)
        for item in report.docs_candidates:
            lines.append(f"  - {item.path}  -> {item.reason}")
        lines.append("")

    if report.archive_candidates:
        lines.append("Archive candidates")
        lines.append("-" * 72)
        for item in report.archive_candidates:
            lines.append(f"  - {item.path}  -> {item.reason}")
        lines.append("")

    if not report.finding_count:
        lines.append("  No root-doc hygiene findings.")
        lines.append("")

    return "\n".join(lines)


def format_json(report: HygieneReport) -> str:
    payload: Dict[str, object] = {
        "tool": TOOL_NAME,
        "version": TOOL_VERSION,
        "summary": {
            "total_root_docs": report.total,
            "keep": len(report.keep),
            "docs_candidates": len(report.docs_candidates),
            "archive_candidates": len(report.archive_candidates),
            "findings": report.finding_count,
        },
        "keep": [item.__dict__ for item in report.keep],
        "docs_candidates": [item.__dict__ for item in report.docs_candidates],
        "archive_candidates": [item.__dict__ for item in report.archive_candidates],
    }
    return json.dumps(payload, indent=2, ensure_ascii=False)


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog=TOOL_NAME,
        description=(
            "Classify top-level markdown/text files into canonical root docs, docs/ candidates, "
            "or archive candidates based on repository-specific hygiene rules."
        ),
    )
    parser.add_argument("--repo-root", metavar="DIR", help="Repository root (auto-detected via git if omitted).")
    parser.add_argument("--format", choices=["text", "json"], default="text", help="Output format.")
    parser.add_argument("--output", metavar="PATH", help="Write report to this file instead of stdout.")
    parser.add_argument(
        "--fail-on-findings",
        action="store_true",
        help="Exit 1 when docs/ or archive candidates are found.",
    )
    return parser


def main(argv: Optional[List[str]] = None) -> int:
    args = build_arg_parser().parse_args(argv)
    repo_root = Path(args.repo_root).resolve() if args.repo_root else find_repo_root()

    if not repo_root.is_dir():
        print(f"ERROR: repo root not found: {repo_root}", file=sys.stderr)
        return 2

    report = run_check(repo_root)
    rendered = format_json(report) if args.format == "json" else format_text(report)

    if args.output:
        try:
            Path(args.output).write_text(rendered, encoding="utf-8")
        except OSError as exc:
            print(f"ERROR: cannot write report to {args.output}: {exc}", file=sys.stderr)
            return 2
    else:
        print(rendered)

    github_output = os.environ.get("GITHUB_OUTPUT")
    if github_output:
        try:
            with open(github_output, "a", encoding="utf-8") as handle:
                handle.write(f"total_root_docs={report.total}\n")
                handle.write(f"keep={len(report.keep)}\n")
                handle.write(f"docs_candidates={len(report.docs_candidates)}\n")
                handle.write(f"archive_candidates={len(report.archive_candidates)}\n")
                handle.write(f"findings={report.finding_count}\n")
        except OSError:
            pass

    if args.fail_on_findings and report.finding_count:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
