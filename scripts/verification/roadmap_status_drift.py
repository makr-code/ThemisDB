#!/usr/bin/env python3
"""
Detect potential ROADMAP checkbox drift for in-progress [~] items.

Heuristic:
- Scan src/*/ROADMAP.md for checklist lines marked [~]
- Flag entries that contain completion-like wording (e.g. implemented, green,
  complete, passing, sign-off) which often indicates stale status markers.
"""

from __future__ import annotations

import argparse
import json
import re
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path


SIGNALS: tuple[tuple[str, str], ...] = (
    ("implemented", r"\bimplemented\b"),
    ("complete", r"\bcomplete(?:d)?\b"),
    ("green", r"\bgreen\b"),
    ("passing", r"\bpassing\b"),
    ("sign_off", r"\bsign[- ]?off\b"),
    ("closure", r"\bclosure\b"),
    ("deployed", r"\bdeployed?\b"),
    ("ready", r"\bready\b"),
    ("verified", r"\bverified\b"),
)


@dataclass(frozen=True)
class Finding:
    line: int
    text: str
    signals: list[str]


def collect_findings(roadmap_path: Path) -> tuple[int, list[Finding]]:
    text = roadmap_path.read_text(encoding="utf-8", errors="replace")
    in_progress_count = 0
    findings: list[Finding] = []

    for line_no, line in enumerate(text.splitlines(), start=1):
        match = re.match(r"^\s*-\s*\[~\]\s+(.+)$", line)
        if not match:
            continue
        in_progress_count += 1
        item = match.group(1).strip()
        item_lower = item.lower()
        matched = [name for name, pattern in SIGNALS if re.search(pattern, item_lower)]
        if matched:
            findings.append(Finding(line=line_no, text=item, signals=matched))
    return in_progress_count, findings


def build_report(repo_root: Path) -> dict:
    modules: list[dict] = []
    total_in_progress = 0
    total_suspected = 0

    for roadmap in sorted((repo_root / "src").glob("*/ROADMAP.md")):
        module = roadmap.parent.name
        in_progress_count, findings = collect_findings(roadmap)
        total_in_progress += in_progress_count
        total_suspected += len(findings)

        if not findings:
            continue

        modules.append(
            {
                "module": module,
                "roadmap": str(roadmap.relative_to(repo_root)),
                "in_progress_count": in_progress_count,
                "suspected_drift_count": len(findings),
                "suspected_items": [
                    {"line": f.line, "text": f.text, "signals": f.signals} for f in findings
                ],
            }
        )

    return {
        "kind": "roadmap_progress_drift",
        "generated_at": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "summary": {
            "modules_scanned": len(list((repo_root / "src").glob("*/ROADMAP.md"))),
            "in_progress_items": total_in_progress,
            "suspected_drift_items": total_suspected,
            "modules_with_suspected_drift": len(modules),
        },
        "modules": modules,
    }


def render_markdown(report: dict) -> str:
    summary = report.get("summary", {})
    modules = report.get("modules", [])
    lines = [
        "# ROADMAP [~] Drift Report",
        "",
        f"- Scanned modules: **{summary.get('modules_scanned', 0)}**",
        f"- Total `[~]` items: **{summary.get('in_progress_items', 0)}**",
        f"- Suspected stale `[~]` items: **{summary.get('suspected_drift_items', 0)}**",
        f"- Modules with suspected drift: **{summary.get('modules_with_suspected_drift', 0)}**",
        "",
    ]

    if not modules:
        lines.extend(["✅ No suspected `[~]` drift items detected.", ""])
        return "\n".join(lines)

    lines.extend(
        [
            "| Module | ROADMAP | `[~]` | Suspected Drift |",
            "|---|---|---:|---:|",
        ]
    )
    for mod in modules:
        lines.append(
            f"| `{mod['module']}` | `{mod['roadmap']}` | {mod['in_progress_count']} | {mod['suspected_drift_count']} |"
        )
    lines.append("")

    for mod in modules:
        lines.append(f"## {mod['module']}")
        for item in mod["suspected_items"]:
            sig = ", ".join(item["signals"])
            lines.append(
                f"- L{item['line']}: {item['text']} _(signals: {sig})_"
            )
        lines.append("")

    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Detect potential stale [~] ROADMAP items.")
    parser.add_argument(
        "--repo-root",
        default=".",
        help="Repository root path (default: current directory).",
    )
    parser.add_argument(
        "--output-json",
        default="artifacts/roadmap_status_drift.json",
        help="Output JSON file path (relative to repo root when not absolute).",
    )
    parser.add_argument(
        "--output-md",
        default="artifacts/roadmap_status_drift.md",
        help="Output markdown report path (relative to repo root when not absolute).",
    )
    parser.add_argument(
        "--fail-on-findings",
        action="store_true",
        help="Exit 1 when suspected drift items are detected.",
    )
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    output_json = Path(args.output_json)
    output_md = Path(args.output_md)
    if not output_json.is_absolute():
        output_json = repo_root / output_json
    if not output_md.is_absolute():
        output_md = repo_root / output_md

    report = build_report(repo_root)
    output_json.parent.mkdir(parents=True, exist_ok=True)
    output_md.parent.mkdir(parents=True, exist_ok=True)
    output_json.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    output_md.write_text(render_markdown(report) + "\n", encoding="utf-8")

    summary = report["summary"]
    print("ROADMAP [~] drift scan complete")
    print(f"JSON: {output_json}")
    print(f"MD  : {output_md}")
    print(f"Scanned modules           : {summary['modules_scanned']}")
    print(f"Total [~] items           : {summary['in_progress_items']}")
    print(f"Suspected stale [~] items : {summary['suspected_drift_items']}")
    print(f"Modules with drift        : {summary['modules_with_suspected_drift']}")

    if args.fail_on_findings and summary["suspected_drift_items"] > 0:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
