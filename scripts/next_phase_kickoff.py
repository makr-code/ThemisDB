#!/usr/bin/env python3
"""
Next-phase kickoff automation for production-readiness execution.

What this script does:
1) Verifies mandatory kickoff artifacts exist.
2) Evaluates Wave-7 gate reports (if benchmark JSON inputs are provided).
3) Captures baseline execution status (query latency, llm memory, server fault behavior).
4) Writes a machine-readable JSON and human-readable markdown summary.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT_DIR = REPO_ROOT / "ai_working"
WAVE7_REPORTER = REPO_ROOT / "benchmarks" / "wave7" / "report_variance_w7.py"
WAVE7_MANIFEST = REPO_ROOT / "benchmarks" / "wave7" / "release_gate_manifest_w7.json"


@dataclass(frozen=True)
class ArtifactCheck:
    name: str
    path: Path
    required: bool = True


def _run(cmd: list[str]) -> tuple[int, str, str]:
    proc = subprocess.run(cmd, text=True, capture_output=True)
    return proc.returncode, proc.stdout.strip(), proc.stderr.strip()


def check_artifacts() -> list[dict[str, Any]]:
    checks = [
        ArtifactCheck("Wave7 Runbook", REPO_ROOT / "benchmarks" / "wave7" / "RUNBOOK_W7.md"),
        ArtifactCheck("Wave7 Manifest", WAVE7_MANIFEST),
        ArtifactCheck("Phase3 Detailed Plan", REPO_ROOT / "ai_working" / "PHASE3_OPTIMIZATION_DETAILED_PLAN.md"),
        ArtifactCheck("Phase5 Detailed Plan", REPO_ROOT / "ai_working" / "PHASE5_HARDENING_DETAILED_PLAN.md"),
        ArtifactCheck("Next Phase Status", REPO_ROOT / "ai_working" / "NEXT_PHASE_STATUS.md"),
        ArtifactCheck("Root Roadmap", REPO_ROOT / "ROADMAP.md"),
    ]
    result: list[dict[str, Any]] = []
    for check in checks:
        exists = check.path.exists()
        result.append(
            {
                "name": check.name,
                "path": str(check.path),
                "required": check.required,
                "exists": exists,
                "status": "ok" if exists else "missing",
            }
        )
    return result


def eval_wave7_input(input_json: Path) -> dict[str, Any]:
    report_output = input_json.with_suffix(".gate_report.json")
    cmd = [
        sys.executable,
        str(WAVE7_REPORTER),
        "--input",
        str(input_json),
        "--manifest",
        str(WAVE7_MANIFEST),
        "--output",
        str(report_output),
    ]
    code, out, err = _run(cmd)
    report_data: dict[str, Any] = {}
    if report_output.exists():
        report_data = json.loads(report_output.read_text(encoding="utf-8"))

    gate_results = report_data.get("gate_results", [])
    hard_failed = [
        gate
        for gate in gate_results
        if gate.get("severity") == "blocking" and not gate.get("passed", False)
    ]
    return {
        "input": str(input_json),
        "report_file": str(report_output),
        "command": " ".join(cmd),
        "exit_code": code,
        "hard_gate_failed_count": len(hard_failed),
        "hard_gate_passed": len(hard_failed) == 0,
        "stdout": out,
        "stderr": err,
    }


def build_baseline_status(query_latency_file: str | None, llm_memory_file: str | None, server_fault_file: str | None) -> dict[str, Any]:
    def state(path_text: str | None, metric_name: str) -> dict[str, Any]:
        if not path_text:
            return {"metric": metric_name, "status": "missing_input", "path": None}
        path = Path(path_text)
        return {
            "metric": metric_name,
            "status": "available" if path.exists() else "missing_file",
            "path": str(path),
        }

    return {
        "query_latency": state(query_latency_file, "query_latency"),
        "llm_memory_profile": state(llm_memory_file, "llm_memory_profile"),
        "server_fault_behavior": state(server_fault_file, "server_fault_behavior"),
    }


def render_markdown(summary: dict[str, Any]) -> str:
    ts = summary["timestamp_utc"]
    artifacts = summary["artifacts"]
    artifact_fail = [a for a in artifacts if a["required"] and not a["exists"]]
    wave7 = summary["wave7"]
    baseline = summary["baseline"]

    lines: list[str] = [
        "# Next-Phase Kickoff Baseline Report",
        "",
        f"- Timestamp (UTC): {ts}",
        f"- Repository: {summary['repository']}",
        "",
        "## 1) Mandatory Artifacts",
        "",
    ]
    for item in artifacts:
        icon = "✅" if item["exists"] else "❌"
        lines.append(f"- {icon} {item['name']}: `{item['path']}`")

    lines.extend(
        [
            "",
            "## 2) Wave-7 Gate Re-Confirmation",
            "",
        ]
    )
    if not wave7:
        lines.append("- ⚠️ No Wave-7 benchmark JSON input provided.")
    else:
        for entry in wave7:
            icon = "✅" if entry["hard_gate_passed"] else "❌"
            lines.append(
                f"- {icon} `{entry['input']}` hard-gates-passed={entry['hard_gate_passed']} "
                f"(failed={entry['hard_gate_failed_count']})"
            )

    lines.extend(
        [
            "",
            "## 3) Kickoff Baselines",
            "",
            f"- Query latency baseline: `{baseline['query_latency']['status']}` ({baseline['query_latency']['path']})",
            f"- LLM memory baseline: `{baseline['llm_memory_profile']['status']}` ({baseline['llm_memory_profile']['path']})",
            f"- Server fault baseline: `{baseline['server_fault_behavior']['status']}` ({baseline['server_fault_behavior']['path']})",
            "",
            "## 4) Gate Decision",
            "",
        ]
    )

    gate_ok = not artifact_fail and all(entry["hard_gate_passed"] for entry in wave7) if wave7 else not artifact_fail
    lines.append(f"- Overall kickoff gate: {'✅ PASS' if gate_ok else '❌ FAIL'}")
    lines.append("")
    lines.append("## 5) Next Actions")
    lines.append("")
    lines.append("- If FAIL: close missing artifacts / failing hard gates before phase promotion.")
    lines.append("- If PASS: proceed with Block A (P3-01 + P3-02) and parallel kickoff for P5-S/P5-L.")
    return "\n".join(lines) + "\n"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run next-phase kickoff baseline checks.")
    parser.add_argument("--wave7-json", action="append", default=[], help="Wave-7 benchmark JSON file (can be repeated).")
    parser.add_argument("--query-latency-baseline", default=None, help="Path to query latency baseline artifact.")
    parser.add_argument("--llm-memory-baseline", default=None, help="Path to LLM memory baseline artifact.")
    parser.add_argument("--server-fault-baseline", default=None, help="Path to server fault baseline artifact.")
    parser.add_argument("--output-dir", default=str(DEFAULT_OUTPUT_DIR), help="Directory for report outputs.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    wave7_results = [eval_wave7_input(Path(path)) for path in args.wave7_json]
    artifacts = check_artifacts()
    baseline = build_baseline_status(
        args.query_latency_baseline,
        args.llm_memory_baseline,
        args.server_fault_baseline,
    )

    summary = {
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
        "repository": str(REPO_ROOT),
        "artifacts": artifacts,
        "wave7": wave7_results,
        "baseline": baseline,
    }

    ts = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S")
    json_path = output_dir / f"NEXT_PHASE_KICKOFF_BASELINE_{ts}.json"
    md_path = output_dir / f"NEXT_PHASE_KICKOFF_BASELINE_{ts}.md"

    json_path.write_text(json.dumps(summary, indent=2), encoding="utf-8")
    md_path.write_text(render_markdown(summary), encoding="utf-8")

    required_missing = [a for a in artifacts if a["required"] and not a["exists"]]
    hard_gate_fail = [
        entry for entry in wave7_results if not entry["hard_gate_passed"]
    ]

    print(f"Report JSON: {json_path}")
    print(f"Report MD:   {md_path}")
    if required_missing:
        print(f"Missing required artifacts: {len(required_missing)}")
    if hard_gate_fail:
        print(f"Wave-7 hard-gate failures: {len(hard_gate_fail)}")

    return 1 if required_missing or hard_gate_fail else 0


if __name__ == "__main__":
    raise SystemExit(main())
