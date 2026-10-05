#!/usr/bin/env python3
"""Heuristic triage for CTest and benchmark log output.

This utility reduces noisy CI logs to a compact, actionable summary of only the
entries that matter for human review and Copilot follow-up:
- Failed
- Not Run
- Skipped

It can be used in GitHub Actions to write a compact Markdown issue body and also
optionally emit GitHub Actions output variables via --write-github-output.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, List, Optional


@dataclass
class IssueEntry:
    category: str
    name: str
    detail: str = ""
    source: str = ""

    def to_markdown(self) -> str:
        label = self.category
        heading = f"- [{label}] {self.name}"
        if self.detail:
            return f"{heading} — {self.detail}"
        return heading


def _dedupe(entries: Iterable[IssueEntry]) -> List[IssueEntry]:
    seen: set[tuple[str, str, str]] = set()
    out: List[IssueEntry] = []
    for entry in entries:
        key = (entry.category, entry.name.strip(), entry.source)
        if key in seen:
            continue
        seen.add(key)
        out.append(entry)
    return out


def _clean_name(name: str) -> str:
    value = name.strip().strip("\"'")
    value = re.sub(r"\s+", " ", value)
    return value or "unknown"


def _normalize_category(raw: str) -> str:
    value = (raw or "").strip()
    lowered = value.lower()
    if "not run" in lowered:
        return "Not Run"
    if "skipped" in lowered:
        return "Skipped"
    if "failed" in lowered or "failure" in lowered:
        return "Failed"
    return value or "Unknown"


def extract_ctest_entries(text: str) -> List[IssueEntry]:
    if not text:
        return []

    entries: List[IssueEntry] = []
    lines = text.splitlines()
    states = {"Failed", "Not Run", "Skipped"}

    patterns = [
        re.compile(r"^\s*(?:\d+\/\d+\s+)?Test\s*#\d+:\s*(?P<name>.+?)\s*\.\.\.\s*(?P<state>Failed|Not Run|Skipped)(?:\s*(?P<detail>.*))?$", re.IGNORECASE),
        re.compile(r"^\s*\d+\)\s*(?P<name>.+?)\s*\((?P<state>Failed|Not Run|Skipped)\)\s*(?P<detail>.*)$", re.IGNORECASE),
        re.compile(r"^\s*(?P<name>.+?)\s*\.\.\.\s*(?P<state>Failed|Not Run|Skipped)\s*(?P<detail>.*)$", re.IGNORECASE),
    ]

    for line in lines:
        stripped = line.strip()
        if not stripped:
            continue

        matched = False
        for pattern in patterns:
            match = pattern.search(stripped)
            if not match:
                continue
            name = _clean_name(match.group("name"))
            state = match.group("state")
            detail = (match.group("detail") or "").strip()
            if not any(token in states for token in [state, _normalize_category(state)]):
                continue
            entries.append(IssueEntry(category=_normalize_category(state), name=name, detail=detail, source="ctest"))
            matched = True
            break
        if matched:
            continue

        no_exec_match = re.search(r"Could not find executable\s+(?P<name>[^\s]+)", stripped, re.IGNORECASE)
        if no_exec_match:
            entries.append(IssueEntry(category="Not Run", name=_clean_name(no_exec_match.group("name")), detail="Missing executable in test directory", source="ctest"))
            continue

        problem_match = re.search(r"(?P<state>Failed|Not Run|Skipped)\s*[:\-]?\s*(?P<name>.+)", stripped, re.IGNORECASE)
        if problem_match:
            state = problem_match.group("state")
            name = _clean_name(problem_match.group("name"))
            if state and name and "test project" not in name.lower():
                entries.append(IssueEntry(category=_normalize_category(state), name=name, detail="Parsed from summary output", source="ctest"))

    return _dedupe(entries)


def extract_benchmark_entries(text: str) -> List[IssueEntry]:
    if not text:
        return []

    entries: List[IssueEntry] = []
    for line in text.splitlines():
        stripped = line.strip()
        if not stripped:
            continue

        status_match = re.search(r"\b(?:FAIL(?:ED)?|❌|ERROR|REGRESSION|EXCEEDS THRESHOLD)\b", stripped, re.IGNORECASE)
        if not status_match:
            continue

        name_match = re.search(r"(?P<name>[A-Za-z0-9_./:-]+(?:\s+[A-Za-z0-9_./:-]+)*)\s*(?::|-)?\s*(?:FAIL(?:ED)?|❌|ERROR|REGRESSION|EXCEEDS THRESHOLD)", stripped, re.IGNORECASE)
        if not name_match:
            continue

        name = _clean_name(name_match.group("name"))
        category = "Failed"
        detail = stripped
        entries.append(IssueEntry(category=category, name=name, detail=detail, source="benchmark"))

    # Also catch failing lines from markdown benchmark reports.
    for line in text.splitlines():
        stripped = line.strip()
        if not stripped:
            continue
        # Example: - benchmark_name: FAIL
        md_match = re.search(r"^[-*]\s*(?P<name>.+?)\s*[:\-]\s*(?P<state>FAIL(?:ED)?|❌|ERROR)", stripped, re.IGNORECASE)
        if md_match:
            entries.append(IssueEntry(category="Failed", name=_clean_name(md_match.group("name")), detail=md_match.group("state").upper(), source="benchmark"))

    return _dedupe(entries)


def build_markdown(title: str, entries: List[IssueEntry], source_label: str) -> str:
    if not entries:
        return (
            f"<!-- ci-{source_label}:type:triage-summary -->\n"
            "## CI Triage Summary\n\n"
            "No actionable failed / not-run / skipped items were detected in the captured log output.\n"
        )

    groups: dict[str, List[IssueEntry]] = {"Failed": [], "Not Run": [], "Skipped": []}
    for entry in entries:
        groups.setdefault(entry.category, []).append(entry)

    lines = [
        f"<!-- ci-{source_label}:type:triage-summary -->",
        f"## {title}",
        "",
        f"Detected actionable CI signals: {len(entries)}",
        "",
    ]

    for category in ["Failed", "Not Run", "Skipped"]:
        items = groups.get(category, [])
        if not items:
            continue
        lines.append(f"### {category} ({len(items)})")
        for item in items:
            lines.append(item.to_markdown())
        lines.append("")

    lines.append("### Suggested next actions")
    lines.append("1. Reproduce the failing test or benchmark locally with the same filter/target.")
    lines.append("2. Inspect the first failing item for root cause before broadening the build/test scope.")
    lines.append("3. Keep the fix scoped to the failing subsystem and re-run the affected suite.")
    lines.append("")
    return "\n".join(lines) + "\n"


def write_github_output(output_path: Optional[str], summary: dict) -> None:
    if not output_path:
        return
    path = Path(output_path)
    with path.open("a", encoding="utf-8") as handle:
        handle.write(f"has_actionable_issues={'true' if summary['has_actionable_issues'] else 'false'}\n")
        handle.write(f"issue_count={summary['issue_count']}\n")
        handle.write(f"issue_types={','.join(summary['issue_types'])}\n")
        handle.write(f"source_kind={summary['source_kind']}\n")


def summarize(log_paths: List[str], source_kind: str) -> dict:
    entries: List[IssueEntry] = []
    for path in log_paths:
        candidate = Path(path)
        if not candidate.exists():
            continue
        text = candidate.read_text(encoding="utf-8", errors="replace")
        if source_kind == "ctest":
            entries.extend(extract_ctest_entries(text))
        elif source_kind == "benchmark":
            entries.extend(extract_benchmark_entries(text))
        else:
            entries.extend(extract_ctest_entries(text))
            entries.extend(extract_benchmark_entries(text))

    seen = {}
    for item in entries:
        seen.setdefault((item.category, item.name), item)
    entries = list(seen.values())
    issue_types = sorted({entry.category for entry in entries})
    summary = {
        "source_kind": source_kind,
        "issues": [entry.__dict__ for entry in entries],
        "issue_count": len(entries),
        "issue_types": issue_types,
        "has_actionable_issues": bool(entries),
    }
    return summary


def main() -> int:
    parser = argparse.ArgumentParser(description="Summarize failed/not-run/skipped ctest or benchmark results into a compact GitHub issue body.")
    parser.add_argument("--ctest-log", action="append", default=[], help="Path to a ctest log file to summarize.")
    parser.add_argument("--benchmark-log", action="append", default=[], help="Path to a benchmark console log file to summarize.")
    parser.add_argument("--benchmark-report", action="append", default=[], help="Path to a benchmark markdown report to summarize.")
    parser.add_argument("--output-markdown", default="", help="Write the compact issue body to this markdown file.")
    parser.add_argument("--output-json", default="", help="Write the structured issue summary to this JSON file.")
    parser.add_argument("--title", default="CI Triage Summary", help="Issue title header to place in the Markdown output.")
    parser.add_argument("--issue-kind", choices=["ctest", "benchmark", "mixed"], default="mixed", help="Which issue family to summarize.")
    parser.add_argument("--write-github-output", default="", help="File path for GITHUB_OUTPUT (for CI gating)")
    args = parser.parse_args()

    chosen_source = args.issue_kind
    if args.issue_kind == "ctest" and not args.ctest_log:
        print("No ctest log provided; nothing to triage.")
        return 0
    if args.issue_kind == "benchmark" and not (args.benchmark_log or args.benchmark_report):
        print("No benchmark log or report provided; nothing to triage.")
        return 0

    all_log_paths: List[str] = list(args.ctest_log) + list(args.benchmark_log) + list(args.benchmark_report)
    summary = summarize(all_log_paths, chosen_source)
    if chosen_source == "mixed":
        ctest_entries = summarize(args.ctest_log, "ctest")
        benchmark_entries = summarize(args.benchmark_log + args.benchmark_report, "benchmark")
        merged_entries = ctest_entries["issues"] + benchmark_entries["issues"]
        deduped: List[IssueEntry] = []
        seen: set[tuple[str, str]] = set()
        for item in merged_entries:
            key = (item["category"], item["name"])
            if key in seen:
                continue
            seen.add(key)
            deduped.append(IssueEntry(**item))
        issue_types = sorted({entry.category for entry in deduped})
        summary = {
            "source_kind": "mixed",
            "issues": [entry.__dict__ for entry in deduped],
            "issue_count": len(deduped),
            "issue_types": issue_types,
            "has_actionable_issues": bool(deduped),
        }

    markdown = build_markdown(args.title, [IssueEntry(**item) for item in summary["issues"]], args.issue_kind)

    if args.output_markdown:
        out_path = Path(args.output_markdown)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text(markdown, encoding="utf-8")

    if args.output_json:
        json_path = Path(args.output_json)
        json_path.parent.mkdir(parents=True, exist_ok=True)
        json_path.write_text(json.dumps(summary, indent=2, ensure_ascii=False), encoding="utf-8")

    write_github_output(args.write_github_output, summary)

    if summary["has_actionable_issues"]:
        print(f"Actionable issues found: {summary['issue_count']}")
        for item in summary["issues"]:
            print(f"- {item['category']}: {item['name']}")
        return 1

    print("No actionable failed/not-run/skipped items found in the provided logs.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
