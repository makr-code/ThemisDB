#!/usr/bin/env python3
"""
Check 100%-maturity hard exit criteria from roadmap and audit artifacts.
"""

from __future__ import annotations

import argparse
import json
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


@dataclass(frozen=True)
class CriterionResult:
    name: str
    passed: bool
    value: int
    target: int
    details: str


def _read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def _extract_int_metric(markdown: str, metric_name: str) -> int:
    patterns = [
        re.compile(
            rf"\|\s*\*\*{re.escape(metric_name)}\*\*\s*\|\s*\*\*(\d+)\*\*",
            re.IGNORECASE,
        ),
        re.compile(
            rf"\|\s*{re.escape(metric_name)}\s*\|\s*(\d+)\s*(?:\||$)",
            re.IGNORECASE,
        ),
    ]
    for pattern in patterns:
        match = pattern.search(markdown)
        if match:
            return int(match.group(1))
    raise ValueError(f"Metric not found: {metric_name}")


def _count_open_roadmap_items(roadmap_paths: Iterable[Path]) -> tuple[int, int]:
    open_count = 0
    in_progress_count = 0
    for path in roadmap_paths:
        text = _read_text(path)
        open_count += len(re.findall(r"^\s*-\s*\[\s\]\s+", text, flags=re.MULTILINE))
        in_progress_count += len(re.findall(r"^\s*-\s*\[~\]\s+", text, flags=re.MULTILINE))
    return open_count, in_progress_count


def _collect_placeholder_modules(maturity_report: str) -> list[str]:
    modules: list[str] = []
    for line in maturity_report.splitlines():
        if not line.startswith("| `"):
            continue
        parts = [p.strip() for p in line.strip().split("|")]
        # Expected: ["", "`module`", "LOC", "Stubs", "D1", "Tests", "D2", "Bench", "D3", "Gesamt", ""]
        if len(parts) < 10:
            continue
        module = parts[1].strip("`")
        d1 = parts[4]
        if d1.startswith("⬛"):
            modules.append(module)
    return sorted(set(modules))


def _count_compliance_gap_markers(maturity_report: str) -> int:
    section_match = re.search(
        r"## 4\..*?(?=\n##\s+[5-9]\.)",
        maturity_report,
        flags=re.DOTALL,
    )
    section = section_match.group(0) if section_match else maturity_report
    markers = re.findall(r"\b(Fehlend|fehlende|offen|ausstehend)\b", section, flags=re.IGNORECASE)
    return len(markers)


def evaluate(repo_root: Path, maturity_report_path: Path) -> dict:
    report_text = _read_text(maturity_report_path)

    technical_blockers = _extract_int_metric(report_text, "GA-Blocker (technisch)")
    governance_blockers = _extract_int_metric(report_text, "GA-Blocker (Governance)")
    modules_with_zero_tests = _extract_int_metric(report_text, "Module mit 0 Tests")
    modules_with_zero_bench = _extract_int_metric(report_text, "Module mit 0 Benchmarks")

    roadmap_paths = [repo_root / "ROADMAP.md", *sorted((repo_root / "src").glob("*/ROADMAP.md"))]
    open_items, in_progress_items = _count_open_roadmap_items(roadmap_paths)

    placeholder_modules = _collect_placeholder_modules(report_text)
    compliance_markers = _count_compliance_gap_markers(report_text)

    checks = [
        CriterionResult(
            name="ga_blockers_technical",
            passed=technical_blockers == 0,
            value=technical_blockers,
            target=0,
            details="GA-Blocker (technisch) must be 0.",
        ),
        CriterionResult(
            name="ga_blockers_governance",
            passed=governance_blockers == 0,
            value=governance_blockers,
            target=0,
            details="GA-Blocker (Governance) must be 0, including §9 sign-off closure.",
        ),
        CriterionResult(
            name="modules_with_zero_tests",
            passed=modules_with_zero_tests == 0,
            value=modules_with_zero_tests,
            target=0,
            details="All modules require automated tests.",
        ),
        CriterionResult(
            name="modules_with_zero_benchmarks",
            passed=modules_with_zero_bench == 0,
            value=modules_with_zero_bench,
            target=0,
            details="All modules require benchmark coverage.",
        ),
        CriterionResult(
            name="placeholder_scaffold_modules",
            passed=len(placeholder_modules) == 0,
            value=len(placeholder_modules),
            target=0,
            details="No module may remain in D1 placeholder/scaffold status.",
        ),
        CriterionResult(
            name="roadmap_open_items",
            passed=open_items == 0,
            value=open_items,
            target=0,
            details="Root and src/*/ROADMAP.md must not contain open [ ] items.",
        ),
        CriterionResult(
            name="roadmap_in_progress_items",
            passed=in_progress_items == 0,
            value=in_progress_items,
            target=0,
            details="Root and src/*/ROADMAP.md must not contain in-progress [~] items.",
        ),
        CriterionResult(
            name="compliance_gap_markers",
            passed=compliance_markers == 0,
            value=compliance_markers,
            target=0,
            details="Compliance section should not contain open/missing markers.",
        ),
    ]

    return {
        "repo_root": str(repo_root),
        "maturity_report": str(maturity_report_path),
        "pass": all(c.passed for c in checks),
        "checks": [c.__dict__ for c in checks],
        "details": {
            "placeholder_modules": placeholder_modules,
            "roadmap_files_checked": [str(p) for p in roadmap_paths],
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Check ThemisDB 100%-maturity hard exit criteria.")
    parser.add_argument(
        "--repo-root",
        default="/home/runner/work/ThemisDB/ThemisDB",
        help="Absolute path to repository root.",
    )
    parser.add_argument(
        "--maturity-report",
        default="audit/MATURITY_REPORT_2026-08.md",
        help="Path to maturity report file (absolute or relative to repo root).",
    )
    parser.add_argument(
        "--output-json",
        default="artifacts/maturity_exit_criteria.json",
        help="Output JSON path (absolute or relative to repo root).",
    )
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    maturity_report_path = Path(args.maturity_report)
    if not maturity_report_path.is_absolute():
        maturity_report_path = repo_root / maturity_report_path

    output_json_path = Path(args.output_json)
    if not output_json_path.is_absolute():
        output_json_path = repo_root / output_json_path
    output_json_path.parent.mkdir(parents=True, exist_ok=True)

    result = evaluate(repo_root=repo_root, maturity_report_path=maturity_report_path)
    output_json_path.write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    print(f"Result: {'PASS' if result['pass'] else 'FAIL'}")
    print(f"Artifact: {output_json_path}")
    for check in result["checks"]:
        status = "PASS" if check["passed"] else "FAIL"
        print(f"- {status} {check['name']}: {check['value']} (target {check['target']})")
    if result["details"]["placeholder_modules"]:
        print("Placeholder modules:", ", ".join(result["details"]["placeholder_modules"]))

    return 0 if result["pass"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
