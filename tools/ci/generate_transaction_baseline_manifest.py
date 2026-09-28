#!/usr/bin/env python3
"""
Generate a consolidated Wave-B transaction baseline evidence manifest.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from datetime import datetime, timezone
from pathlib import Path


IGNORED_SUFFIXES = ("_mean", "_median", "_stddev", "_cv")


def _read_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def _as_float(value: object) -> float | None:
    if value is None:
        return None
    if isinstance(value, (int, float)):
        return float(value)
    return None


def _parse_benchmarks(path: Path) -> tuple[list[dict[str, object]], dict[str, float | None]]:
    payload = _read_json(path)
    rows: list[dict[str, object]] = []

    for bench in payload.get("benchmarks", []):
        name = bench.get("name")
        if not isinstance(name, str) or name.endswith(IGNORED_SUFFIXES):
            continue

        row = {
            "name": name,
            "iterations": bench.get("iterations"),
            "time_unit": bench.get("time_unit"),
            "real_time": _as_float(bench.get("real_time")),
            "cpu_time": _as_float(bench.get("cpu_time")),
            "txns_per_sec": _as_float(bench.get("txns_per_sec")),
            "p50_ms": _as_float(bench.get("p50_ms")),
            "p95_ms": _as_float(bench.get("p95_ms")),
            "p99_ms": _as_float(bench.get("p99_ms")),
            "gate_pass": _as_float(bench.get("gate_pass")),
        }
        rows.append(row)

    def _max_metric(metric: str) -> float | None:
        values = [value for value in (_as_float(row.get(metric)) for row in rows) if value is not None]
        return max(values) if values else None

    def _min_metric(metric: str) -> float | None:
        values = [value for value in (_as_float(row.get(metric)) for row in rows) if value is not None]
        return min(values) if values else None

    summary = {
        "benchmark_count": float(len(rows)),
        "best_ops_per_sec": _max_metric("txns_per_sec"),
        "lowest_ops_per_sec": _min_metric("txns_per_sec"),
        "worst_p50_ms": _max_metric("p50_ms"),
        "worst_p95_ms": _max_metric("p95_ms"),
        "worst_p99_ms": _max_metric("p99_ms"),
    }
    return rows, summary


def _parse_chaos_log(path: Path | None) -> dict[str, object]:
    result: dict[str, object] = {
        "status": "not-provided" if path is None else "unknown",
        "path": str(path) if path is not None else None,
    }
    if path is None or not path.exists():
        return result

    text = path.read_text(encoding="utf-8", errors="replace")
    if "100% tests passed" in text:
        result["status"] = "success"
    elif "***Failed" in text or "tests failed out of" in text:
        result["status"] = "failed"

    passed_match = re.search(r"(\d+)% tests passed,\s+(\d+) tests failed out of (\d+)", text)
    if passed_match:
        result["pass_rate_percent"] = int(passed_match.group(1))
        result["failed_tests"] = int(passed_match.group(2))
        result["total_tests"] = int(passed_match.group(3))
    else:
        total_match = re.search(r"Total Test time .*", text)
        if total_match and result["status"] == "success":
            result["total_tests"] = None

    return result


def _parse_hardware_files(lscpu_path: Path | None, memory_path: Path | None, chaos_hw_path: Path | None) -> dict[str, object]:
    evidence = {
        "lscpu_path": str(lscpu_path) if lscpu_path is not None else None,
        "memory_path": str(memory_path) if memory_path is not None else None,
        "chaos_hardware_path": str(chaos_hw_path) if chaos_hw_path is not None else None,
    }

    if lscpu_path and lscpu_path.exists():
        text = lscpu_path.read_text(encoding="utf-8", errors="replace")
        for label, key in (
            ("Model name:", "cpu_model"),
            ("CPU(s):", "cpu_count"),
            ("Architecture:", "architecture"),
        ):
            for line in text.splitlines():
                if line.startswith(label):
                    evidence[key] = line.split(":", 1)[1].strip()
                    break

    if memory_path and memory_path.exists():
        text = memory_path.read_text(encoding="utf-8", errors="replace")
        for line in text.splitlines():
            if line.lower().startswith("mem:"):
                evidence["memory_summary"] = line
                break

    return evidence


def _write_markdown(path: Path, manifest: dict[str, object]) -> None:
    summary = manifest["performance_summary"]
    signoff = manifest["signoff"]
    lines = [
        "# Wave-B Transaction Baseline Evidence",
        "",
        f"- Generated at: {manifest['generated_at']}",
        f"- Workflow: `{manifest['workflow']}`",
        f"- Run ID: `{manifest['run_id']}`",
        f"- Run URL: {manifest['run_url']}",
        f"- Commit SHA: `{manifest['commit_sha']}`",
        "",
        "## Representative-hardware summary",
        "",
        f"- Benchmarks captured: {int(summary['benchmark_count'])}",
        f"- Best throughput (ops/sec): {summary['best_ops_per_sec']}",
        f"- Lowest throughput (ops/sec): {summary['lowest_ops_per_sec']}",
        f"- Worst latency p50 (ms): {summary['worst_p50_ms']}",
        f"- Worst latency p95 (ms): {summary['worst_p95_ms']}",
        f"- Worst latency p99 (ms): {summary['worst_p99_ms']}",
        "",
        "## Sign-off",
        "",
        f"- Required approver: `{signoff['required_approver']}`",
        f"- Status: `{signoff['status']}`",
        "",
    ]
    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--benchmark-json", required=True)
    parser.add_argument("--chaos-log")
    parser.add_argument("--lscpu")
    parser.add_argument("--memory")
    parser.add_argument("--chaos-hardware")
    parser.add_argument("--workflow", required=True)
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--run-url", required=True)
    parser.add_argument("--commit-sha", required=True)
    parser.add_argument("--required-approver", default="platform-release@themisdb")
    parser.add_argument("--output-json", required=True)
    parser.add_argument("--output-md")
    args = parser.parse_args()

    benchmark_path = Path(args.benchmark_json)
    if not benchmark_path.exists():
        print(f"error: benchmark JSON not found: {benchmark_path}", file=sys.stderr)
        return 1

    try:
        benchmarks, summary = _parse_benchmarks(benchmark_path)
        if not benchmarks:
            raise ValueError("benchmark JSON did not contain any non-aggregate benchmark rows")

        manifest = {
            "generated_at": datetime.now(timezone.utc).isoformat(),
            "workflow": args.workflow,
            "run_id": args.run_id,
            "run_url": args.run_url,
            "commit_sha": args.commit_sha,
            "evidence_scope": "wave-b-transaction-representative-hardware",
            "benchmark_source": str(benchmark_path),
            "performance_summary": summary,
            "benchmarks": benchmarks,
            "chaos_recovery": _parse_chaos_log(Path(args.chaos_log) if args.chaos_log else None),
            "hardware_profile": _parse_hardware_files(
                Path(args.lscpu) if args.lscpu else None,
                Path(args.memory) if args.memory else None,
                Path(args.chaos_hardware) if args.chaos_hardware else None,
            ),
            "signoff": {
                "required_approver": args.required_approver,
                "status": "pending",
                "approved_at": None,
            },
        }

        output_json = Path(args.output_json)
        output_json.parent.mkdir(parents=True, exist_ok=True)
        output_json.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

        if args.output_md:
            output_md = Path(args.output_md)
            output_md.parent.mkdir(parents=True, exist_ok=True)
            _write_markdown(output_md, manifest)
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
