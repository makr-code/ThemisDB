#!/usr/bin/env python3
"""Validate minimum metadata for in-scope Markdown files."""

from __future__ import annotations

import argparse
import fnmatch
import json
import re
import subprocess
import sys
from dataclasses import asdict, dataclass
from datetime import datetime
from pathlib import Path


DATE_FIELDS = {"created", "last_updated"}
DATE_PATTERN = "%Y-%m-%d"


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


def load_config(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def normalize_key(value: str) -> str:
    return re.sub(r"[^a-z0-9]+", "", value.lower())


def strip_wrapping_quotes(value: str) -> str:
    value = value.strip()
    if len(value) >= 2 and value[0] == value[-1] and value[0] in {"'", '"'}:
        return value[1:-1].strip()
    return value


def parse_front_matter(lines: list[str]) -> dict[str, str]:
    if not lines or lines[0].strip() != "---":
        return {}

    end_index = None
    for index in range(1, len(lines)):
        if lines[index].strip() == "---":
            end_index = index
            break

    if end_index is None:
        return {}

    metadata: dict[str, str] = {}
    for raw_line in lines[1:end_index]:
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        match = re.match(r"^([A-Za-z][A-Za-z0-9 _()/.-]*)\s*:\s*(.+?)\s*$", line)
        if not match:
            continue
        metadata[normalize_key(match.group(1))] = strip_wrapping_quotes(match.group(2))
    return metadata


def parse_header_metadata(lines: list[str], max_scan_lines: int) -> dict[str, str]:
    metadata: dict[str, str] = {}
    bold_pattern = re.compile(r"^(?:>\s*)?(?:[-*]\s+)?\*\*([^*]+?):\*\*\s*(.+?)\s*$")
    plain_pattern = re.compile(r"^(?:>\s*)?(?:[-*]\s+)?([^:]+?)\s*:\s*(.+?)\s*$")

    for raw_line in lines[:max_scan_lines]:
        line = raw_line.strip()
        if not line:
            continue
        match = bold_pattern.match(line) or plain_pattern.match(line)
        if match is None:
            continue
        key = normalize_key(match.group(1))
        metadata.setdefault(key, strip_wrapping_quotes(match.group(2)))
    return metadata


def collect_metadata(file_path: Path, max_scan_lines: int) -> dict[str, str]:
    lines = file_path.read_text(encoding="utf-8").splitlines()
    metadata = parse_header_metadata(lines, max_scan_lines)
    front_matter = parse_front_matter(lines)
    metadata.update(front_matter)
    return metadata


def is_valid_date(value: str) -> bool:
    try:
        datetime.strptime(value, DATE_PATTERN)
    except ValueError:
        return False
    return True


def resolve_field(metadata: dict[str, str], aliases: list[str]) -> str | None:
    for alias in aliases:
        value = metadata.get(normalize_key(alias))
        if value:
            return value.strip()
    return None


def validate_file(file_path: Path, rel_path: str, config: dict) -> FileResult:
    metadata = collect_metadata(file_path, int(config.get("max_scan_lines", 40)))
    required_fields = config["required_fields"]
    allowed_statuses = {status.lower() for status in config["allowed_statuses"]}
    errors: list[str] = []

    for logical_name, aliases in required_fields.items():
        value = resolve_field(metadata, aliases)
        label = aliases[0]
        if not value:
            errors.append(f"missing required field '{label}'")
            continue
        if logical_name in DATE_FIELDS and not is_valid_date(value):
            errors.append(
                f"field '{label}' must use YYYY-MM-DD, got '{value}'"
            )
        elif logical_name == "status" and value.lower() not in allowed_statuses:
            errors.append(
                "field 'Status' must be one of: "
                + ", ".join(sorted(allowed_statuses))
                + f" (got '{value}')"
            )

    return FileResult(file=rel_path, errors=errors)


def matches_any(path: str, patterns: list[str]) -> bool:
    for pattern in patterns:
        if fnmatch.fnmatch(path, pattern):
            return True
        if "/**/" in pattern and fnmatch.fnmatch(path, pattern.replace("/**/", "/")):
            return True
    return False


def is_in_scope(path: str, config: dict) -> bool:
    return path.endswith(".md") and matches_any(path, config["include"]) and not matches_any(path, config["exclude"])


def list_changed_files(repo_root: Path, base_ref: str) -> list[str]:
    commands = [
        ["git", "diff", "--name-only", "--diff-filter=ACMR", f"origin/{base_ref}...HEAD"],
        ["git", "diff", "--name-only", "--diff-filter=ACMR", f"{base_ref}...HEAD"],
        ["git", "diff", "--name-only", "--diff-filter=ACMR", base_ref],
    ]
    for command in commands:
        result = subprocess.run(
            command,
            cwd=repo_root,
            text=True,
            capture_output=True,
        )
        if result.returncode == 0:
            return [line.strip().replace("\\", "/") for line in result.stdout.splitlines() if line.strip()]
    return []


def read_files_from_list(path: Path) -> list[str]:
    return [line.strip().replace("\\", "/") for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]


def build_report(files: list[str], repo_root: Path, config: dict) -> Report:
    results: list[FileResult] = []
    files_seen = len(files)
    files_checked = 0

    for rel_path in sorted(set(files)):
        if not is_in_scope(rel_path, config):
            continue
        file_path = (repo_root / rel_path).resolve()
        if not file_path.exists():
            continue
        files_checked += 1
        result = validate_file(file_path, rel_path, config)
        if result.errors:
            results.append(result)

    files_with_errors = len(results)
    files_skipped = files_seen - files_checked
    verdict = "FAIL" if files_with_errors else "PASS"
    return Report(
        verdict=verdict,
        files_seen=files_seen,
        files_checked=files_checked,
        files_skipped=files_skipped,
        files_with_errors=files_with_errors,
        results=results,
    )


def print_text_report(report: Report) -> None:
    if report.files_checked == 0:
        print("Doc metadata gate: no in-scope markdown files to validate.")
        return

    print(
        f"Doc metadata gate checked {report.files_checked} file(s) "
        f"({report.files_with_errors} with errors, {report.files_skipped} skipped)."
    )
    for result in report.results:
        print(f"\n{result.file}")
        for error in result.errors:
            print(f"  ERROR: {error}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate minimum metadata for Markdown files.")
    parser.add_argument("--repo-root", default=".")
    parser.add_argument("--config", default=".github/doc-metadata-gate.json")
    parser.add_argument("--base-ref", default="develop")
    parser.add_argument("--files-from", help="Path to newline-separated changed files list")
    parser.add_argument("--report-json", help="Write JSON report to this path")
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    config = load_config((repo_root / args.config).resolve())

    if args.files_from:
        files = read_files_from_list(Path(args.files_from).resolve())
    else:
        files = list_changed_files(repo_root, args.base_ref)

    report = build_report(files, repo_root, config)

    if args.report_json:
        report_path = Path(args.report_json).resolve()
        report_path.write_text(
            json.dumps(asdict(report), indent=2, ensure_ascii=False),
            encoding="utf-8",
        )

    print_text_report(report)
    return 1 if report.verdict == "FAIL" else 0


if __name__ == "__main__":
    sys.exit(main())
