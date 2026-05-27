"""Compute delta between two gap scanner aggregate JSON files.

Usage:
  python -m tools.scanners.delta_report --baseline ai_working/gap_scan_v3_aggregate.json --current ai_working/gap_scan_v3_aggregate.json
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Dict, Tuple


SEVERITY_KEYS = ("severity_critical", "severity_high", "severity_medium")


def _load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def _sum_categories(aggregate: dict) -> Dict[str, int]:
    totals: Dict[str, int] = {}
    for module_data in aggregate.values():
        if not isinstance(module_data, dict):
            continue
        by_category = module_data.get("by_category", {})
        if not isinstance(by_category, dict):
            continue
        for key, value in by_category.items():
            totals[key] = totals.get(key, 0) + int(value)
    return totals


def _sum_severities(aggregate: dict) -> Dict[str, int]:
    totals = {key: 0 for key in SEVERITY_KEYS}
    for module_data in aggregate.values():
        if not isinstance(module_data, dict):
            continue
        for key in SEVERITY_KEYS:
            totals[key] += int(module_data.get(key, 0))
    return totals


def _sum_total(aggregate: dict) -> int:
    total = 0
    for module_data in aggregate.values():
        if not isinstance(module_data, dict):
            continue
        total += int(module_data.get("total", 0))
    return total


def _delta_dict(current: Dict[str, int], baseline: Dict[str, int]) -> Dict[str, int]:
    all_keys = set(current) | set(baseline)
    return {k: int(current.get(k, 0)) - int(baseline.get(k, 0)) for k in sorted(all_keys)}


def _top_changes(delta: Dict[str, int], limit: int = 15) -> Tuple[list, list]:
    increased = [(k, v) for k, v in delta.items() if v > 0]
    decreased = [(k, v) for k, v in delta.items() if v < 0]
    increased.sort(key=lambda x: x[1], reverse=True)
    decreased.sort(key=lambda x: x[1])
    return increased[:limit], decreased[:limit]


def build_report(baseline: dict, current: dict) -> dict:
    base_total = _sum_total(baseline)
    cur_total = _sum_total(current)

    base_sev = _sum_severities(baseline)
    cur_sev = _sum_severities(current)
    sev_delta = _delta_dict(cur_sev, base_sev)

    base_cat = _sum_categories(baseline)
    cur_cat = _sum_categories(current)
    cat_delta = _delta_dict(cur_cat, base_cat)

    inc, dec = _top_changes(cat_delta)

    return {
        "total": {
            "baseline": base_total,
            "current": cur_total,
            "delta": cur_total - base_total,
        },
        "severity": {
            "baseline": base_sev,
            "current": cur_sev,
            "delta": sev_delta,
        },
        "category": {
            "baseline": base_cat,
            "current": cur_cat,
            "delta": cat_delta,
            "top_increased": inc,
            "top_decreased": dec,
        },
    }


def _render_markdown(report: dict, baseline_path: Path, current_path: Path) -> str:
    lines = [
        "# Gap Scanner Delta Report",
        "",
        f"- Baseline: `{baseline_path.as_posix()}`",
        f"- Current: `{current_path.as_posix()}`",
        "",
        "## Total",
        "",
        "| Metric | Value |",
        "|---|---:|",
        f"| Baseline | {report['total']['baseline']} |",
        f"| Current | {report['total']['current']} |",
        f"| Delta | {report['total']['delta']} |",
        "",
        "## Severity Delta",
        "",
        "| Severity | Baseline | Current | Delta |",
        "|---|---:|---:|---:|",
    ]

    for key in SEVERITY_KEYS:
        lines.append(
            f"| {key} | {report['severity']['baseline'].get(key, 0)} | "
            f"{report['severity']['current'].get(key, 0)} | "
            f"{report['severity']['delta'].get(key, 0)} |"
        )

    lines.extend([
        "",
        "## Top Increased Categories",
        "",
        "| Category | Delta |",
        "|---|---:|",
    ])
    for key, value in report["category"]["top_increased"]:
        lines.append(f"| {key} | +{value} |")

    lines.extend([
        "",
        "## Top Decreased Categories",
        "",
        "| Category | Delta |",
        "|---|---:|",
    ])
    for key, value in report["category"]["top_decreased"]:
        lines.append(f"| {key} | {value} |")

    return "\n".join(lines) + "\n"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build delta report for gap scanner aggregate JSON files.")
    parser.add_argument("--baseline", required=True, help="Path to baseline aggregate JSON")
    parser.add_argument("--current", required=True, help="Path to current aggregate JSON")
    parser.add_argument(
        "--out-json",
        default="",
        help="Optional output path for structured delta JSON",
    )
    parser.add_argument(
        "--out-md",
        default="",
        help="Optional output path for markdown report",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    baseline_path = Path(args.baseline)
    current_path = Path(args.current)

    baseline = _load_json(baseline_path)
    current = _load_json(current_path)

    report = build_report(baseline, current)

    if args.out_json:
        out_json_path = Path(args.out_json)
        out_json_path.parent.mkdir(parents=True, exist_ok=True)
        out_json_path.write_text(json.dumps(report, indent=2), encoding="utf-8")

    md = _render_markdown(report, baseline_path, current_path)
    if args.out_md:
        out_md_path = Path(args.out_md)
        out_md_path.parent.mkdir(parents=True, exist_ok=True)
        out_md_path.write_text(md, encoding="utf-8")

    print(json.dumps(report["total"], indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
