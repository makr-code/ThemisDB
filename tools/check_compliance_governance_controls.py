#!/usr/bin/env python3
"""ThemisDB Compliance/Governance gate for sourcecode, CI/CD and release controls."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


DOC_PATH = Path("docs/governance/SOURCECODE_COMPLIANCE_GOVERNANCE.md")

REQUIRED_SECTIONS = (
    "## Compliance-Control-Matrix",
    "## Governance-Policy-Set",
    "## Audit-Logik (Risk Acceptance / Exception History)",
    "## Folge-Issues pro Lücke (Gap-Backlog)",
    "## Implementation Phases",
    "## Definition of Done",
)


def _row_tokens(line: str) -> list[str]:
    parts = [p.strip() for p in line.strip().strip("|").split("|")]
    return parts


def _is_missing(value: str) -> bool:
    return value.strip() == "" or value.strip().lower() in {"tbd", "-", "n/a"}


def run(repo_root: Path) -> int:
    doc = repo_root / DOC_PATH
    if not doc.exists():
        print(f"FAIL: required governance document missing: {DOC_PATH}")
        return 1

    try:
        text = doc.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        print(f"FAIL: governance document is not valid UTF-8: {DOC_PATH}")
        return 1
    failures: list[str] = []

    for section in REQUIRED_SECTIONS:
        if section not in text:
            failures.append(f"missing section: {section}")

    critical_rows = [
        line for line in text.splitlines()
        if re.match(r"^\|\s*C-\d+\s*\|", line) and "| Critical |" in line
    ]
    if not critical_rows:
        failures.append("no Critical control rows found in control matrix")
    else:
        for line in critical_rows:
            tokens = _row_tokens(line)
            if len(tokens) < 8:
                failures.append(f"invalid critical control row format: {line}")
                continue
            owner = tokens[4]
            evidence = tokens[5]
            frequency = tokens[6]
            if _is_missing(owner) or _is_missing(evidence) or _is_missing(frequency):
                failures.append(f"incomplete critical control metadata: {line}")

    risk_rows = [
        line for line in text.splitlines()
        if re.match(r"^\|\s*RA-\d{4}-\d+\s*\|", line)
    ]
    if not risk_rows:
        failures.append("no risk acceptance rows found")
    else:
        for line in risk_rows:
            tokens = _row_tokens(line)
            if len(tokens) < 7:
                failures.append(f"invalid risk acceptance row format: {line}")
                continue
            approver = tokens[2]
            decision_date = tokens[3]
            expiry = tokens[4]
            if _is_missing(approver) or _is_missing(decision_date) or _is_missing(expiry):
                failures.append(f"incomplete risk acceptance audit metadata: {line}")

    gap_rows = [
        line for line in text.splitlines()
        if re.match(r"^\|\s*GAP-\d+\s*\|", line)
    ]
    if not gap_rows:
        failures.append("no governance gap follow-up rows found")
    else:
        for line in gap_rows:
            tokens = _row_tokens(line)
            if len(tokens) < 6:
                failures.append(f"invalid gap row format: {line}")
                continue
            severity = tokens[1]
            owner = tokens[3]
            deadline = tokens[4]
            if _is_missing(severity) or _is_missing(owner) or _is_missing(deadline):
                failures.append(f"incomplete gap row metadata: {line}")

    if failures:
        print("ThemisDB Compliance Governance Gate")
        print("=" * 42)
        print("RESULT: FAIL")
        for item in failures:
            print(f"- {item}")
        return 1

    print("ThemisDB Compliance Governance Gate")
    print("=" * 42)
    print("RESULT: PASS")
    return 0


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Validate critical compliance/governance controls documentation."
    )
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path("."),
        help="Repository root (default: current directory).",
    )
    args = parser.parse_args()
    sys.exit(run(args.repo_root.resolve()))


if __name__ == "__main__":
    main()
