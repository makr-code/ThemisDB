#!/usr/bin/env python3
"""Validate module feature pages for structure, metadata, and narrative depth."""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import asdict, dataclass
from pathlib import Path

import validate_doc_metadata as base_gate


@dataclass
class FileResult:
    file: str
    errors: list[str]


@dataclass
class Report:
    verdict: str
    files_seen: int
    files_checked: int
    files_skipped: int
    files_with_errors: int
    results: list[FileResult]


def is_feature_path(rel_path: str) -> bool:
    return rel_path.endswith("FEATURES.md") and rel_path.startswith("src/")


def validate_module_feature(file_path: Path, rel_path: str, config: dict) -> FileResult:
    text = file_path.read_text(encoding="utf-8")
    metadata = base_gate.collect_metadata(file_path, int(config.get("max_scan_lines", 40)))
    errors: list[str] = []

    for logical_name, aliases in config["required_fields"].items():
        value = base_gate.resolve_field(metadata, aliases)
        label = aliases[0]
        if not value:
            errors.append(f"missing required field '{label}'")
            continue
        if logical_name in {"created", "last_updated"} and not base_gate.is_valid_date(value):
            errors.append(f"field '{label}' must use YYYY-MM-DD, got '{value}'")
        elif logical_name == "status" and value.lower() not in {status.lower() for status in config["allowed_statuses"]}:
            errors.append(
                "field 'Status' must be one of: "
                + ", ".join(sorted(status.lower() for status in config["allowed_statuses"]))
                + f" (got '{value}')"
            )

    if re.search(r"^#\s+.+", text, flags=re.MULTILINE) is None:
        errors.append("FEATURES.md must start with a top-level H1 heading.")

    if len(text.strip()) < 400:
        errors.append("FEATURES.md is too short to explain the module's purpose, value, and operational role.")

    if not re.search(r"\[[^\]]+\]\([^\)]+\)", text):
        errors.append("FEATURES.md should include at least one relevant source or module reference.")

    if not re.search(r"\bThemisDB\b", text, flags=re.IGNORECASE):
        errors.append("FEATURES.md should frame the module in ThemisDB context.")

    if not re.search(
        r"(?i)(the core problem|why this matters|why it matters|for teams|for customers|production load|operational value|runtime behavior|workloads|in practice|in the platform|ThemisDB)",
        text,
    ):
        errors.append("FEATURES.md should explain why the module matters in production and operational terms.")

    return FileResult(file=rel_path, errors=errors)


def list_requested_files(files: list[str]) -> list[str]:
    return [item for item in files if is_feature_path(item)]


def build_report(files: list[str], repo_root: Path, config: dict) -> Report:
    candidate_files = list_requested_files(files)
    results: list[FileResult] = []
    files_seen = len(files)
    files_checked = 0
    for rel_path in sorted(set(candidate_files)):
        file_path = (repo_root / rel_path).resolve()
        if not file_path.exists():
            continue
        files_checked += 1
        result = validate_module_feature(file_path, rel_path, config)
        if result.errors:
            results.append(result)

    verdict = "FAIL" if results else "PASS"
    files_skipped = max(files_seen - files_checked, 0)
    return Report(
        verdict=verdict,
        files_seen=files_seen,
        files_checked=files_checked,
        files_skipped=files_skipped,
        files_with_errors=len(results),
        results=results,
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate module feature pages.")
    parser.add_argument("--repo-root", default=".")
    parser.add_argument("--config", default=".github/doc-metadata-gate.json")
    parser.add_argument("--files-from", help="Path to newline-separated changed files list")
    parser.add_argument("--report-json", help="Write JSON report to this path")
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    config = json.loads((repo_root / args.config).read_text(encoding="utf-8"))

    if args.files_from:
        files = [line.strip().replace("\\", "/") for line in Path(args.files_from).read_text(encoding="utf-8").splitlines() if line.strip()]
    else:
        files = []

    report = build_report(files, repo_root, config)

    if args.report_json:
        output = Path(args.report_json).resolve()
        output.write_text(json.dumps(asdict(report), indent=2, ensure_ascii=False), encoding="utf-8")

    if report.files_checked == 0:
        print("Module feature validation: no src/**/FEATURES.md files to validate.")
        return 0

    print(
        f"Module feature validation checked {report.files_checked} file(s) "
        f"({report.files_with_errors} with errors)."
    )
    for result in report.results:
        print(f"\n{result.file}")
        for error in result.errors:
            print(f"  ERROR: {error}")

    return 1 if report.verdict == "FAIL" else 0


if __name__ == "__main__":
    sys.exit(main())
