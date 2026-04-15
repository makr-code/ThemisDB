"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_coverage_report.py                           ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-15 18:19:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     361                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 02fd81c194  2026-04-13  feat(perf): add nightly-bench-sweep CMake preset, workflo... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

# ---------------------------------------------------------------------------
# Module definitions (modules 2–33 as per issue specification)
# ---------------------------------------------------------------------------

# Maps module number → human-readable name and a list of bench binary name
# fragments that belong to it.  A bench binary is "owned" by the first module
# whose fragment list produces a match.
MODULE_MAP: dict[int, dict[str, Any]] = {
    2:  {"name": "Storage",         "patterns": ["storage", "rocksdb", "lsm", "blob", "delta"]},
    3:  {"name": "Index",           "patterns": ["index", "inverted", "vector_search", "ann", "hnsw", "ivf"]},
    4:  {"name": "Query",           "patterns": ["query", "aql", "join", "scan", "filter", "where"]},
    5:  {"name": "Graph",           "patterns": ["graph", "traversal", "bfs", "dfs", "shortest"]},
    6:  {"name": "Transactions",    "patterns": ["transaction", "tx", "mvcc", "commit", "rollback"]},
    7:  {"name": "CDC / Realtime",  "patterns": ["cdc", "changefeed", "realtime", "stream", "wal"]},
    8:  {"name": "Timeseries",      "patterns": ["timeseries", "ts_", "_ts_", "hypertable", "downsamp", "gorilla"]},
    9:  {"name": "Analytics (OLAP)","patterns": ["olap", "analytics", "ivm", "agg", "olap"]},
    10: {"name": "Vector / Embedding", "patterns": ["vector", "embedding", "knn", "pq", "bq", "binary_quant"]},
    11: {"name": "Geospatial",      "patterns": ["geo", "spatial", "rtree", "tile", "haversine", "radius"]},
    12: {"name": "Full-Text Search","patterns": ["fulltext", "fts", "text_search", "lucene", "bm25"]},
    13: {"name": "Auth / Security", "patterns": ["auth", "jwt", "token", "oauth", "security", "rls", "encrypt"]},
    14: {"name": "Sharding",        "patterns": ["shard", "consistent_hash", "partition"]},
    15: {"name": "Replication / HA","patterns": ["repl", "raft", "ha_", "failover", "crdt"]},
    16: {"name": "Backup / Storage Tiers", "patterns": ["backup", "restore", "pitr", "tiered", "archiv"]},
    17: {"name": "LLM Integration", "patterns": ["llm", "inference", "prompt", "llama", "rag", "embeddings_llm"]},
    18: {"name": "ML / Training",   "patterns": ["ml_", "train", "lora", "finetune", "automl"]},
    19: {"name": "Exporters",       "patterns": ["export", "parquet", "arrow", "jsonl", "hugging"]},
    20: {"name": "Importers",       "patterns": ["import", "postgres_import", "mongo_import", "kafka_import"]},
    21: {"name": "API / Protocols", "patterns": ["api", "http", "grpc", "websocket", "wire_proto", "quic"]},
    22: {"name": "Plugins / WASM",  "patterns": ["plugin", "wasm", "extension"]},
    23: {"name": "Scheduler",       "patterns": ["scheduler", "task_sched", "cron_", "job_queue"]},
    24: {"name": "Observability",   "patterns": ["observ", "metric", "tracing", "profil", "alert"]},
    25: {"name": "Acceleration",    "patterns": ["accel", "dispatch", "gpu", "cuda", "simd", "avx"]},
    26: {"name": "Adaptive Query Compilation", "patterns": ["adaptive_query", "jit", "query_compil", "query_cache"]},
    27: {"name": "Chimera Suite",   "patterns": ["chimera"]},
    28: {"name": "AQL Reference",   "patterns": ["aql_func", "aql_valid", "aql_optim"]},
    29: {"name": "Schema / Metadata", "patterns": ["schema", "catalog", "metadata", "info_schema"]},
    30: {"name": "Cluster Updates", "patterns": ["update_sched", "canary", "hot_reload", "blue_green"]},
    31: {"name": "Governance",      "patterns": ["governance", "policy", "compliance", "masking", "lineage"]},
    32: {"name": "Ethics AI",       "patterns": ["ethics", "constitutional", "confidence_detect"]},
    33: {"name": "System-Level (TPC/YCSB)", "patterns": ["tpcc", "ycsb", "tpc_", "system_bench"]},
    # Module 34–36: Wave-2 reference benchmark modules (added 2026-04-15)
    34: {"name": "Temporal",        "patterns": ["temporal_queries", "bitemporal", "bi_temporal", "temporal"]},
    35: {"name": "Process Mining",  "patterns": ["process_mining", "process_retrieval"]},
    36: {"name": "ONNX-CLIP (Image Embedding)", "patterns": ["onnx_clip", "image_analysis"]},
}

GREEN = "🟢"
YELLOW = "🟡"
RED = "🔴"
GREY = "⚪"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _load_json(path: Path) -> dict[str, Any] | None:
    try:
        with open(path) as fh:
            return json.load(fh)
    except Exception:
        return None


def _bench_name_to_module(bench_name: str) -> int | None:
    """Return the module number that owns *bench_name*, or None."""
    lower = bench_name.lower()
    for mod_id, info in MODULE_MAP.items():
        for pat in info["patterns"]:
            if pat in lower:
                return mod_id
    return None


def _summarise_bench_file(data: dict[str, Any]) -> dict[str, Any]:
    """Extract high-level stats from a Google Benchmark JSON output."""
    benchmarks = data.get("benchmarks", [])
    if not benchmarks:
        return {"count": 0, "error_count": 0, "mean_cpu_ns": None}

    cpu_times = [
        b.get("cpu_time", 0)
        for b in benchmarks
        if b.get("run_type") != "aggregate" and b.get("cpu_time") is not None
    ]
    error_count = sum(1 for b in benchmarks if b.get("error_occurred", False))

    return {
        "count": len(benchmarks),
        "error_count": error_count,
        "mean_cpu_ns": (sum(cpu_times) / len(cpu_times)) if cpu_times else None,
    }


def _delta_str(current: float | None, previous: float | None) -> str:
    """Human-readable delta string, e.g. '+3.4 %' or '-1.2 %'."""
    if current is None or previous is None or previous == 0.0:
        return "n/a"
    pct = (current - previous) / previous * 100.0
    sign = "+" if pct >= 0 else ""
    return f"{sign}{pct:.1f} %"


def _traffic_light(summary: dict[str, Any]) -> str:
    """Determine traffic-light status for a module summary entry."""
    if summary.get("bench_count", 0) == 0:
        return GREY  # no benchmarks found → uncovered
    if summary.get("error_count", 0) > 0:
        return RED   # errors in benchmark output
    delta_raw = summary.get("mean_cpu_ns_delta_pct")
    if delta_raw is not None:
        if delta_raw > 10.0:
            return RED    # ≥ 10 % regression
        if delta_raw > 5.0:
            return YELLOW # 5–10 % potential regression
    return GREEN


# ---------------------------------------------------------------------------
# Core logic
# ---------------------------------------------------------------------------

def collect_module_data(
    bench_dir: Path,
    prev_dir: Path | None,
) -> dict[int, dict[str, Any]]:
    """
    Walk *bench_dir* for ``*.json`` files produced by benchmark binaries,
    aggregate per module, and optionally compute delta vs *prev_dir*.
    """
    # Collect all current JSON bench result files
    current_files: dict[str, Path] = {}
    for p in bench_dir.glob("bench_*.json"):
        current_files[p.stem] = p

    # Collect previous JSON bench result files (if available)
    prev_files: dict[str, Path] = {}
    if prev_dir and prev_dir.exists():
        for p in prev_dir.glob("bench_*.json"):
            prev_files[p.stem] = p

    # Per-module accumulator
    modules: dict[int, dict[str, Any]] = {
        mid: {
            "id": mid,
            "name": info["name"],
            "bench_count": 0,
            "error_count": 0,
            "bench_names": [],
            "mean_cpu_ns": None,
            "mean_cpu_ns_prev": None,
            "mean_cpu_ns_delta_pct": None,
            "status": GREY,
        }
        for mid, info in MODULE_MAP.items()
    }

    for stem, path in current_files.items():
        mod_id = _bench_name_to_module(stem)
        if mod_id is None:
            continue
        data = _load_json(path)
        if data is None:
            continue
        stats = _summarise_bench_file(data)
        m = modules[mod_id]
        m["bench_names"].append(stem)
        m["bench_count"] += stats["count"]
        m["error_count"] += stats["error_count"]
        # Average CPU time across files for this module (naive mean of means)
        if stats["mean_cpu_ns"] is not None:
            prev_mean = m["mean_cpu_ns"]
            m["mean_cpu_ns"] = (
                stats["mean_cpu_ns"]
                if prev_mean is None
                else (prev_mean + stats["mean_cpu_ns"]) / 2.0
            )

        # Previous run data
        if stem in prev_files:
            prev_data = _load_json(prev_files[stem])
            if prev_data:
                prev_stats = _summarise_bench_file(prev_data)
                if prev_stats["mean_cpu_ns"] is not None:
                    prev_mean = m["mean_cpu_ns_prev"]
                    m["mean_cpu_ns_prev"] = (
                        prev_stats["mean_cpu_ns"]
                        if prev_mean is None
                        else (prev_mean + prev_stats["mean_cpu_ns"]) / 2.0
                    )

    # Compute deltas and traffic lights
    for m in modules.values():
        if m["mean_cpu_ns"] is not None and m["mean_cpu_ns_prev"] is not None:
            prev = m["mean_cpu_ns_prev"]
            curr = m["mean_cpu_ns"]
            if prev > 0:
                m["mean_cpu_ns_delta_pct"] = (curr - prev) / prev * 100.0
        m["status"] = _traffic_light(m)

    return modules


def build_markdown_report(
    modules: dict[int, dict[str, Any]],
    run_ts: str,
    bench_dir: Path,
    prev_dir: Path | None,
) -> str:
    """Return the full Markdown report string."""
    lines: list[str] = [
        "### 🌙 Nightly Benchmark Sweep — Coverage Report",
        "",
        f"**Generated:** {run_ts}  ",
        f"**Bench dir:** `{bench_dir}`  ",
        f"**Previous run dir:** `{prev_dir or 'n/a'}`",
        "",
        "| # | Module | Status | Cases | Errors | Mean CPU (ns) | Δ vs prev |",
        "|---|--------|--------|-------|--------|--------------|-----------|",
    ]

    for mid in sorted(modules):
        m = modules[mid]
        cpu = f"{m['mean_cpu_ns']:.0f}" if m["mean_cpu_ns"] is not None else "—"
        delta = _delta_str(m["mean_cpu_ns"], m["mean_cpu_ns_prev"])
        lines.append(
            f"| {mid} | {m['name']} | {m['status']} "
            f"| {m['bench_count']} | {m['error_count']} | {cpu} | {delta} |"
        )

    # Legend
    lines += [
        "",
        f"**Legend:** {GREEN} OK / no regression &nbsp; {YELLOW} Minor regression (5–10 %) "
        f"&nbsp; {RED} Regression ≥ 10 % or errors &nbsp; {GREY} Not covered (no bench binary matched)",
        "",
    ]

    # Summary counts
    covered = sum(1 for m in modules.values() if m["bench_count"] > 0)
    total = len(modules)
    has_red = sum(1 for m in modules.values() if m["status"] == RED)
    has_yellow = sum(1 for m in modules.values() if m["status"] == YELLOW)
    lines += [
        f"**Coverage:** {covered}/{total} modules have at least one benchmark  ",
        f"**🔴 Regressions / errors:** {has_red}  ",
        f"**🟡 Warnings (5–10 %):** {has_yellow}",
    ]

    return "\n".join(lines) + "\n"


def build_json_summary(
    modules: dict[int, dict[str, Any]],
    run_ts: str,
) -> dict[str, Any]:
    """Return machine-readable JSON summary."""
    covered = sum(1 for m in modules.values() if m["bench_count"] > 0)
    return {
        "generated": run_ts,
        "total_modules": len(modules),
        "covered_modules": covered,
        "modules": {str(mid): m for mid, m in sorted(modules.items())},
    }


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate a nightly benchmark coverage report with traffic-light status.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--bench-dir",
        default="artifacts/nightly",
        help="Directory containing bench_*.json files from the current nightly run "
             "(default: artifacts/nightly)",
    )
    parser.add_argument(
        "--prev-dir",
        default=None,
        help="Directory containing bench_*.json files from the previous nightly run "
             "(used for delta comparison; omit to skip delta)",
    )
    parser.add_argument(
        "--output-dir",
        default="artifacts/nightly/audit",
        help="Directory where coverage_report.md and coverage_report.json are written "
             "(default: artifacts/nightly/audit)",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)

    bench_dir = Path(args.bench_dir)
    prev_dir = Path(args.prev_dir) if args.prev_dir else None
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    run_ts = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    print(f"[bench_coverage_report] Scanning {bench_dir} …")
    modules = collect_module_data(bench_dir, prev_dir)

    md = build_markdown_report(modules, run_ts, bench_dir, prev_dir)
    js = build_json_summary(modules, run_ts)

    md_path = output_dir / "coverage_report.md"
    json_path = output_dir / "coverage_report.json"

    md_path.write_text(md, encoding="utf-8")
    json_path.write_text(json.dumps(js, indent=2, ensure_ascii=False), encoding="utf-8")

    # Console output
    print(md)
    print(f"[bench_coverage_report] Wrote {md_path}")
    print(f"[bench_coverage_report] Wrote {json_path}")

    covered = js["covered_modules"]
    total = js["total_modules"]
    has_red = sum(1 for m in modules.values() if m["status"] == RED)
    print(f"[bench_coverage_report] Coverage: {covered}/{total} modules | 🔴 {has_red} regression(s)")

    return 0


if __name__ == "__main__":
    sys.exit(main())
