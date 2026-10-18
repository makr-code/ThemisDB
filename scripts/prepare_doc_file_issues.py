#!/usr/bin/env python3
"""Generate per-file documentation issue bodies for failed doc gate checks."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


CANONICAL_SOURCES = [
    "DOCUMENTATION_GOVERNANCE.md",
    ".github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md",
    ".github/copilot/module-quick-rules.md",
    "src/",
    "docs/compendium/docs/",
]


def normalize_report(report) -> dict:
    if hasattr(report, "__dict__") and not isinstance(report, dict):
        report = report.__dict__
    normalized = dict(report)
    normalized["results"] = [
        item.__dict__ if hasattr(item, "__dict__") else dict(item)
        for item in normalized.get("results", [])
    ]
    return normalized


def sanitize_for_filename(value: str) -> str:
    sanitized = re.sub(r"[^a-zA-Z0-9._-]+", "-", value)
    sanitized = sanitized.strip("-")
    return sanitized or "doc-fix"


def render_problem_list(file_name: str, errors: list[str]) -> str:
    lines = ["## Problems found"]
    for error in errors:
        lines.append(f"- {error}")
    return "\n".join(lines)


def build_file_issue(file_name: str, errors: list[str], kind: str) -> tuple[str, str, str]:
    marker = f"<!-- docs-file-gap:{kind}:{file_name} -->"
    title = f"[docs] Fix {kind} issues in {file_name}"
    severity = "High" if kind == "feature" else "Medium"
    issue_kind = "module-behavior" if file_name.endswith("FEATURES.md") else "architecture-governance"
    if kind == "metadata":
        issue_kind = "architecture-governance"

    summary = (
        f"The {kind} gate found a documentation defect in `{file_name}`. This file is currently failing the repository's documentation governance checks and needs a concrete, source-backed fix."
    )

    current_state = "\n".join(f"- {error}" for error in errors)
    expected_state = (
        "The file should satisfy the active gate checks, include the required metadata or narrative structure, and remain consistent with the repository's documentation precedence."
    )

    gap_type_lines = [
        "- [ ] **Inaccurate** – Documentation describes something that no longer exists or works differently",
        "- [ ] **Missing** – Feature or module has no documentation at all",
        "- [ ] **Stale** – Documentation references outdated versions, paths, or APIs",
        "- [ ] **Broken Link** – One or more hyperlinks in the document are dead",
        "- [ ] **Incomplete** – Documentation exists but is missing important sections",
        "- [ ] **Translation** – Documentation is missing in one or more supported languages (DE/EN/FR/ES/JA)",
        "- [ ] **Tier Violation** – Secondary/Tertiary document conflicts with Primary source",
    ]
    if file_name.endswith("FEATURES.md"):
        gap_type_lines.insert(0, "- [ ] **Incomplete** – Feature page is too shallow for product-facing use")
    if kind == "metadata":
        gap_type_lines.insert(0, "- [ ] **Incomplete** – Required metadata is missing or invalid")

    acceptance_lines = [
        "- [ ] Documentation accurately reflects the current implementation",
        "- [ ] All internal links are valid",
        "- [ ] Code examples are tested and runnable",
        "- [ ] Content is reviewed by at least one other contributor",
        "- [ ] Tier precedence check passed (`Primary > Secondary > Tertiary`)",
        "- [ ] If applicable: German and English versions are in sync",
        "- [ ] Issue is assigned to the correct docs milestone",
        "- [ ] If applicable: release-gate evidence is linked for build-test or release-versioning findings",
        "- [ ] Private-plugin findings do not leak confidential repository paths, URLs, or implementation details into public docs",
    ]

    if file_name.endswith("FEATURES.md"):
        acceptance_lines.insert(0, "- [ ] The feature page explains the general problem, module role, and operational value before feature detail")

    body = f"""{marker}
## Summary

{summary}

## Audit Reference

- **Audit Version:** v1.0
- **Finding ID:** DOCS-{kind.upper()}-{sanitize_for_filename(file_name).upper()}
- **Severity:** {severity}
- **Discovered:** CI docs gate
- **Documentation Tier:** Tier 1 (Primary)
- **SOT Domain:** {issue_kind}
- **Update Interval:** Event-driven
- **Target Milestone:** DOC-WEEKLY-{sanitize_for_filename(file_name)[:8].upper()}
- **Gate Evidence:** `{kind}` validation script in CI

## Canonical Source References

- [DOCUMENTATION_GOVERNANCE.md](DOCUMENTATION_GOVERNANCE.md)
- [.github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md](.github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md)
- [.github/copilot/module-quick-rules.md](.github/copilot/module-quick-rules.md)
- [src/](src/)
- Relevant module-local source and docs for `{file_name}`

## Affected Files

- `{file_name}` – failing the docs gate because of the issues listed below

## Current State

{current_state}

## Expected State

{expected_state}

## Gap Type

{chr(10).join(gap_type_lines)}

## Acceptance Criteria

{chr(10).join(acceptance_lines)}

## Additional Context

This issue was generated automatically from the repository documentation gate. The goal is to keep the task narrow, source-backed, and directly actionable for Copilot without broad unrelated refactoring.

### Direct instructions for Copilot

1. Open `{file_name}` and read the current document before changing anything.
2. Read the relevant canonical docs and source context for the module or domain.
3. Fix the concrete issues listed under Current State without altering unrelated content.
4. Keep the wording source-aware and aligned with the repo's documentation precedence.
5. Validate the file with the same gate used in CI before closing the issue.

### Validation commands

- `python3 scripts/validate_doc_metadata.py --repo-root . --config .github/doc-metadata-gate.json --files-from /tmp/doc-metadata-files.txt`
- `python3 scripts/validate_module_features.py --repo-root . --config .github/doc-metadata-gate.json --files-from /tmp/doc-metadata-files.txt`
"""
    return title, marker, body


def build_manifest(report_path: Path, kind: str, output_dir: Path) -> list[dict]:
    if not report_path.exists():
        return []
    report = normalize_report(json.loads(report_path.read_text(encoding="utf-8")))
    issue_rows: list[dict] = []
    files_by_path: dict[str, list[str]] = {}
    for result in report.get("results", []):
        file_name = str(result.get("file", "unknown"))
        files_by_path.setdefault(file_name, []).extend(result.get("errors", []))

    for file_name in sorted(files_by_path):
        errors = files_by_path[file_name]
        title, marker, body = build_file_issue(file_name, errors, kind)
        issue_file_name = f"{sanitize_for_filename(file_name)}-{kind}.md"
        (output_dir / issue_file_name).write_text(body, encoding="utf-8")
        issue_rows.append({
            "title": title,
            "marker": marker,
            "file": file_name,
            "kind": kind,
            "output": issue_file_name,
        })

    return issue_rows


def main() -> int:
    parser = argparse.ArgumentParser(description="Create a per-file issue bundle for docs gate failures.")
    parser.add_argument("--metadata-report", default="")
    parser.add_argument("--feature-report", default="")
    parser.add_argument("--output-dir", required=True)
    args = parser.parse_args()

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    issues: list[dict] = []
    if args.metadata_report:
        issues.extend(build_manifest(Path(args.metadata_report), "metadata", output_dir))
    if args.feature_report:
        issues.extend(build_manifest(Path(args.feature_report), "feature", output_dir))

    manifest_path = output_dir / "issues_manifest.json"
    manifest_path.write_text(json.dumps({"issues": issues}, indent=2, ensure_ascii=False), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
