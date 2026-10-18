#!/usr/bin/env python3
"""Generate an issue body for failing module feature pages."""

from __future__ import annotations

import argparse
import json
from dataclasses import asdict
from pathlib import Path


def normalize_report(report):
    if hasattr(report, "__dict__") and not isinstance(report, dict):
        report = asdict(report)
    normalized = dict(report)
    normalized["results"] = [
        asdict(item) if hasattr(item, "__dict__") else dict(item)
        for item in normalized.get("results", [])
    ]
    return normalized


def build_issue_body(report: dict) -> str:
    results = report.get("results", [])
    if not results:
        return ""

    bullet_lines = []
    for result in results:
        file_name = result.get("file", "unknown")
        errors = "; ".join(result.get("errors", []))
        bullet_lines.append(f"- `{file_name}` — {errors}")

    body = """<!-- module-feature-gate -->
## Summary

The module feature gate detected weak or incomplete `src/**/FEATURES.md` pages. These pages must explain the module in ThemisDB context, why it matters operationally, and how it fits the platform story before they are acceptable for merge.

## Affected files

{files}

## Canonical source references

- [DOCUMENTATION_GOVERNANCE.md](DOCUMENTATION_GOVERNANCE.md)
- [.github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md](.github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md)
- [.github/copilot/module-quick-rules.md](.github/copilot/module-quick-rules.md)
- [src/](src/)

## What needs fixing

- Add a clear ThemisDB framing near the start of each page.
- Explain the general problem and operational value before feature detail.
- Add a source-backed explanation and relevant module links.
- Expand shallow pages so they are suitable as a marketing-facing feature summary and WordPress content base.

## Validation evidence

- Gate: `scripts/validate_module_features.py`
- Trigger: `gate-pr-doc-metadata.yml`
- Verdict: **FAIL**
""".format(files="\n".join(bullet_lines))

    return body


def main() -> int:
    parser = argparse.ArgumentParser(description="Prepare a docs issue for failing module feature pages.")
    parser.add_argument("--report-json", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    report_path = Path(args.report_json)
    if not report_path.exists():
        return 0

    raw_report = json.loads(report_path.read_text(encoding="utf-8"))
    report = normalize_report(raw_report)
    issue_body = build_issue_body(report)
    output_path = Path(args.output)
    output_path.write_text(issue_body, encoding="utf-8")
    return 0 if issue_body else 0


if __name__ == "__main__":
    raise SystemExit(main())
