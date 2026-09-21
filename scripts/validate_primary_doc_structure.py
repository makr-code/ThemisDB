#!/usr/bin/env python3
"""Validate primary module documentation structure for README/ARCHITECTURE files."""

from __future__ import annotations

import argparse
import fnmatch
import json
import re
import subprocess
import sys
import unicodedata
from dataclasses import asdict, dataclass
from pathlib import Path


HEADING_RE = re.compile(r"^\s{0,3}(#{1,6})\s+(.+?)\s*$")
MERMAID_FENCE_RE = re.compile(r"^\s*```\s*mermaid\s*$", re.IGNORECASE)
FENCE_RE = re.compile(r"^\s*```")


@dataclass
class FileResult:
    file: str
    errors: list[str]
    warnings: list[str]


@dataclass
class Report:
    verdict: str
    files_seen: int
    files_checked: int
    files_skipped: int
    files_with_errors: int
    files_with_warnings: int
    results: list[FileResult]


def load_config(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def normalize_text(value: str) -> str:
    value = unicodedata.normalize("NFKD", value)
    value = value.encode("ascii", "ignore").decode("ascii")
    value = value.lower().strip()
    value = re.sub(r"\s+", " ", value)
    return value


def extract_headings(lines: list[str]) -> list[str]:
    headings: list[str] = []
    in_code = False
    for line in lines:
        if FENCE_RE.match(line):
            in_code = not in_code
            continue
        if in_code:
            continue
        m = HEADING_RE.match(line)
        if m:
            headings.append(m.group(2).strip())
    return headings


def find_mermaid_blocks(lines: list[str]) -> list[tuple[int, int]]:
    blocks: list[tuple[int, int]] = []
    i = 0
    while i < len(lines):
        if MERMAID_FENCE_RE.match(lines[i]):
            start = i
            i += 1
            while i < len(lines) and not FENCE_RE.match(lines[i]):
                i += 1
            end = i if i < len(lines) else len(lines) - 1
            blocks.append((start, end))
        i += 1
    return blocks


def has_interpretation_after(lines: list[str], end_idx: int, keywords: list[str]) -> bool:
    max_lookahead = min(len(lines), end_idx + 8)
    window = [normalize_text(lines[j]) for j in range(end_idx + 1, max_lookahead)]
    normalized_keywords = [normalize_text(k) for k in keywords]
    for entry in window:
        if not entry:
            continue
        for keyword in normalized_keywords:
            if keyword and keyword in entry:
                return True
    return False


def validate_file(file_path: Path, rel_path: str, rule: dict) -> FileResult:
    text = file_path.read_text(encoding="utf-8")
    lines = text.splitlines()

    errors: list[str] = []
    warnings: list[str] = []

    required_headings = rule.get("required_headings", [])
    headings = extract_headings(lines)
    normalized_headings = {normalize_text(h) for h in headings}

    for required in required_headings:
        if normalize_text(required) not in normalized_headings:
            errors.append(f"missing required heading '{required}'")

    mermaid_blocks = find_mermaid_blocks(lines)
    if rule.get("require_mermaid", False) and not mermaid_blocks:
        errors.append("at least one mermaid diagram is required")

    max_blocks = int(rule.get("max_mermaid_blocks", 3))
    if len(mermaid_blocks) > max_blocks:
        warnings.append(
            f"found {len(mermaid_blocks)} mermaid blocks, recommended maximum is {max_blocks}"
        )

    if rule.get("require_interpretation_after_mermaid", False):
        keywords = rule.get("interpretation_keywords", ["Kurzinterpretation", "Interpretation"])
        for idx, (_, end) in enumerate(mermaid_blocks, start=1):
            if not has_interpretation_after(lines, end, keywords):
                errors.append(
                    f"mermaid block #{idx} has no interpretation marker after the block (expected one of: {', '.join(keywords)})"
                )

    return FileResult(file=rel_path, errors=errors, warnings=warnings)


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
        result = subprocess.run(command, cwd=repo_root, text=True, capture_output=True)
        if result.returncode == 0:
            return [line.strip().replace("\\", "/") for line in result.stdout.splitlines() if line.strip()]
    return []


def read_files_from_list(path: Path) -> list[str]:
    return [line.strip().replace("\\", "/") for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]


def rule_for_file(rel_path: str, config: dict) -> dict | None:
    name = Path(rel_path).name
    return config.get("rules", {}).get(name)


def build_report(files: list[str], repo_root: Path, config: dict) -> Report:
    results: list[FileResult] = []
    files_seen = len(files)
    files_checked = 0

    for rel_path in sorted(set(files)):
        if not is_in_scope(rel_path, config):
            continue
        rule = rule_for_file(rel_path, config)
        if not rule:
            continue

        file_path = (repo_root / rel_path).resolve()
        if not file_path.exists():
            continue

        files_checked += 1
        result = validate_file(file_path, rel_path, rule)
        if result.errors or result.warnings:
            results.append(result)

    files_with_errors = sum(1 for r in results if r.errors)
    files_with_warnings = sum(1 for r in results if r.warnings)
    files_skipped = files_seen - files_checked
    verdict = "FAIL" if files_with_errors else "PASS"

    return Report(
        verdict=verdict,
        files_seen=files_seen,
        files_checked=files_checked,
        files_skipped=files_skipped,
        files_with_errors=files_with_errors,
        files_with_warnings=files_with_warnings,
        results=results,
    )


def print_text_report(report: Report) -> None:
    if report.files_checked == 0:
        print("Primary doc structure gate: no in-scope markdown files to validate.")
        return

    print(
        f"Primary doc structure gate checked {report.files_checked} file(s) "
        f"({report.files_with_errors} with errors, {report.files_with_warnings} with warnings, {report.files_skipped} skipped)."
    )

    for result in report.results:
        if result.errors or result.warnings:
            print(f"\n{result.file}")
        for error in result.errors:
            print(f"  ERROR: {error}")
        for warning in result.warnings:
            print(f"  WARN: {warning}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate primary Markdown structure for module docs.")
    parser.add_argument("--repo-root", default=".")
    parser.add_argument("--config", default=".github/primary-doc-structure-gate.json")
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
        report_path.write_text(json.dumps(asdict(report), indent=2, ensure_ascii=False), encoding="utf-8")

    print_text_report(report)
    return 1 if report.verdict == "FAIL" else 0


if __name__ == "__main__":
    sys.exit(main())
