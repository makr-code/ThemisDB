#!/usr/bin/env python3
"""Validate High-finding exception completeness in PR descriptions."""

from __future__ import annotations

import argparse
import os
import re
import sys
from pathlib import Path

SECTION_HEADING = "High-Finding Exception Record (only if High is accepted)"
ACTIVATION_LABEL = "High-finding exception claimed in this PR"
REQUIRED_FIELDS = [
    "Finding reference",
    "Maintainer approver",
    "Mitigation in current release",
    "Target fix milestone",
    "Tracking issue",
    "Validation evidence",
]


def load_body(args: argparse.Namespace) -> str:
    if args.body_file:
        return Path(args.body_file).read_text(encoding="utf-8")
    return os.environ.get("PR_BODY", "")


def extract_section(markdown: str, heading: str) -> str:
    pattern = re.compile(
        rf"(?ims)^##\s+{re.escape(heading)}\s*$\n(.*?)(?=^##\s+|\Z)"
    )
    match = pattern.search(markdown)
    return match.group(1) if match else ""


def checkbox_checked(section: str, label: str) -> bool:
    pattern = re.compile(rf"(?im)^\s*-\s*\[(x|X)\]\s*{re.escape(label)}\s*$")
    return bool(pattern.search(section))


def parse_field_value(section: str, field_name: str) -> str:
    pattern = re.compile(rf"(?im)^\s*-\s*{re.escape(field_name)}:\s*(.*)$")
    match = pattern.search(section)
    if not match:
        return ""
    return match.group(1).strip()


def is_empty_value(value: str) -> bool:
    if not value:
        return True
    stripped = value.strip()
    return stripped in {"N/A", "n/a", "-", "<fill>", "<to fill>", "TBD", "tbd"}


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Validate High-finding exception record completeness in PR body"
    )
    parser.add_argument(
        "--body-file",
        help="Path to a markdown file containing PR description",
    )
    args = parser.parse_args()

    body = load_body(args)
    if not body.strip():
        print("PASS: PR description is empty or unavailable; nothing to validate.")
        return 0

    section = extract_section(body, SECTION_HEADING)
    if not section:
        print("PASS: High-finding exception section not found; nothing to validate.")
        return 0

    if not checkbox_checked(section, ACTIVATION_LABEL):
        print("PASS: High-finding exception not activated in PR section.")
        return 0

    missing = []
    for field in REQUIRED_FIELDS:
        value = parse_field_value(section, field)
        if is_empty_value(value):
            missing.append(field)

    policy_present = "REVIEW_SEVERITY_POLICY.md" in body
    if not policy_present:
        missing.append("Severity policy reference (REVIEW_SEVERITY_POLICY.md)")

    if missing:
        print("FAIL: High-finding exception record is incomplete.")
        for item in missing:
            print(f"- Missing: {item}")
        return 1

    print("PASS: High-finding exception record is complete.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
