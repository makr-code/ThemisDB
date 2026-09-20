#!/usr/bin/env python3
"""Compare two module Doxygen manifest JSON files and report structural drift.

Inputs:
- baseline manifest JSON
- current manifest JSON

Outputs:
- JSON report with per-module deltas
- Markdown summary for CI/job documentation
"""

from __future__ import annotations

import argparse
import json
import time
from pathlib import Path
from typing import Any


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Compare module Doxygen manifests")
    p.add_argument("--baseline", required=True, help="Baseline manifest JSON")
    p.add_argument("--current", required=True, help="Current manifest JSON")
    p.add_argument("--report-json", required=True, help="Output JSON report")
    p.add_argument("--summary-md", required=True, help="Output Markdown summary")
    p.add_argument(
        "--warn-delta-threshold",
        type=int,
        default=10,
        help="Absolute class/struct delta threshold for warning flag",
    )
    return p.parse_args()


def load_manifest(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def index_modules(manifest: dict[str, Any]) -> dict[str, dict[str, Any]]:
    modules: dict[str, dict[str, Any]] = {}
    for item in manifest.get("modules", []):
        name = item.get("module")
        if isinstance(name, str) and name:
            modules[name] = item
    return modules


def metric(item: dict[str, Any], key: str) -> int:
    val = item.get(key, 0)
    return int(val) if isinstance(val, (int, float, str)) and str(val).strip("-").isdigit() else 0


def build_report(baseline: dict[str, Any], current: dict[str, Any], warn_delta_threshold: int) -> dict[str, Any]:
    b_idx = index_modules(baseline)
    c_idx = index_modules(current)

    all_modules = sorted(set(b_idx.keys()) | set(c_idx.keys()))
    rows: list[dict[str, Any]] = []

    severe_count = 0
    changed_count = 0

    for mod in all_modules:
        b = b_idx.get(mod)
        c = c_idx.get(mod)

        if b is None:
            rows.append({
                "module": mod,
                "state": "ADDED",
                "baseline_status": "N/A",
                "current_status": c.get("status", "N/A"),
                "delta_compound_total": metric(c, "compound_total"),
                "delta_class_struct_total": metric(c, "class_struct_total"),
                "delta_namespace_total": metric(c, "namespace_total"),
                "warning": False,
            })
            changed_count += 1
            continue

        if c is None:
            rows.append({
                "module": mod,
                "state": "REMOVED",
                "baseline_status": b.get("status", "N/A"),
                "current_status": "N/A",
                "delta_compound_total": -metric(b, "compound_total"),
                "delta_class_struct_total": -metric(b, "class_struct_total"),
                "delta_namespace_total": -metric(b, "namespace_total"),
                "warning": True,
            })
            changed_count += 1
            severe_count += 1
            continue

        d_compound = metric(c, "compound_total") - metric(b, "compound_total")
        d_class = metric(c, "class_struct_total") - metric(b, "class_struct_total")
        d_ns = metric(c, "namespace_total") - metric(b, "namespace_total")

        status_changed = b.get("status") != c.get("status")
        any_delta = d_compound != 0 or d_class != 0 or d_ns != 0 or status_changed
        warning = abs(d_class) >= warn_delta_threshold or (b.get("status") == "PASS" and c.get("status") == "FAIL")

        if any_delta:
            changed_count += 1
        if warning:
            severe_count += 1

        rows.append({
            "module": mod,
            "state": "CHANGED" if any_delta else "UNCHANGED",
            "baseline_status": b.get("status", "N/A"),
            "current_status": c.get("status", "N/A"),
            "delta_compound_total": d_compound,
            "delta_class_struct_total": d_class,
            "delta_namespace_total": d_ns,
            "warning": warning,
        })

    return {
        "tool": "compare_module_doxygen_manifests.py",
        "generated_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "baseline_generated_at": baseline.get("generated_at"),
        "current_generated_at": current.get("generated_at"),
        "warn_delta_threshold": warn_delta_threshold,
        "totals": {
            "modules": len(all_modules),
            "changed": changed_count,
            "warnings": severe_count,
        },
        "rows": rows,
    }


def write_summary(path: Path, report: dict[str, Any]) -> None:
    lines: list[str] = []
    lines.append("# Module Doxygen Drift Report")
    lines.append("")
    lines.append(f"- Baseline Generated At: {report.get('baseline_generated_at')}")
    lines.append(f"- Current Generated At: {report.get('current_generated_at')}")
    lines.append(f"- Modules: {report['totals']['modules']}")
    lines.append(f"- Changed: {report['totals']['changed']}")
    lines.append(f"- Warnings: {report['totals']['warnings']}")
    lines.append("")
    lines.append("| Module | State | Baseline | Current | dCompounds | dClasses | dNamespaces | Warning |")
    lines.append("|---|---|---|---|---:|---:|---:|---|")

    def key_fn(r: dict[str, Any]) -> tuple[int, str]:
        return (0 if r.get("warning") else 1, r.get("module", ""))

    for row in sorted(report.get("rows", []), key=key_fn):
        lines.append(
            "| {module} | {state} | {baseline_status} | {current_status} | {delta_compound_total} | {delta_class_struct_total} | {delta_namespace_total} | {warning} |".format(
                module=row.get("module", ""),
                state=row.get("state", ""),
                baseline_status=row.get("baseline_status", ""),
                current_status=row.get("current_status", ""),
                delta_compound_total=row.get("delta_compound_total", 0),
                delta_class_struct_total=row.get("delta_class_struct_total", 0),
                delta_namespace_total=row.get("delta_namespace_total", 0),
                warning="yes" if row.get("warning") else "no",
            )
        )

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    args = parse_args()
    baseline = load_manifest(Path(args.baseline))
    current = load_manifest(Path(args.current))

    report = build_report(baseline, current, args.warn_delta_threshold)

    report_path = Path(args.report_json)
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")

    write_summary(Path(args.summary_md), report)

    print(
        f"Drift analysis completed: modules={report['totals']['modules']} changed={report['totals']['changed']} warnings={report['totals']['warnings']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
