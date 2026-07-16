#!/usr/bin/env python3
"""Verify Doxygen documentation coverage threshold from a coverxygen summary."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


def parse_coverage_percent(text: str) -> float:
    # Accept patterns like:
    # - "Coverage: 91.3%"
    # - "Total coverage ... 91.3 %"
    match = re.search(r"(\d+(?:\.\d+)?)\s*%", text)
    if not match:
        raise ValueError("Could not find a coverage percentage in summary output.")
    return float(match.group(1))


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate Doxygen coverage threshold.")
    parser.add_argument("--summary-file", required=True, help="Path to coverxygen summary output")
    parser.add_argument("--threshold", required=True, type=float, help="Minimum required percentage")
    args = parser.parse_args()

    summary_path = Path(args.summary_file)
    if not summary_path.exists():
        print(f"ERROR: summary file not found: {summary_path}")
        return 2

    content = summary_path.read_text(encoding="utf-8", errors="replace")
    try:
        coverage = parse_coverage_percent(content)
    except ValueError as exc:
        print(f"ERROR: {exc}")
        return 3

    print(f"Detected Doxygen coverage: {coverage:.2f}%")
    print(f"Required threshold: {args.threshold:.2f}%")

    if coverage + 1e-9 < args.threshold:
        print("FAIL: Documentation coverage is below threshold.")
        return 1

    print("PASS: Documentation coverage threshold satisfied.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
