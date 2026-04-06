"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            run_ci_benchmarks.py                               ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-06 04:04:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   93.0/100                                       ║
    • Total Lines:     318                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 0829d54f32  2026-02-28  feat(chimera): automated benchmark CI pipeline with regre... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
run_ci_benchmarks.py – CHIMERA CI Benchmark Runner

Runs a compact set of synthetic workloads through the CHIMERA
:class:`~chimera.benchmark_harness.BenchmarkHarness` and writes a JSON
results file suitable for:

  * Storing as a new baseline on ``main`` pushes.
  * Comparing against an existing baseline via
    ``chimera_regression_detector.py`` on pull-request builds.

Synthetic workloads are designed to be:
  - **Fast** (< 60 s total on any CI runner).
  - **Deterministic** in structure (same workload IDs every run).
  - **Representative** of the four chimera operation families:
    relational (sort), vector (dot-product), document (dict lookup),
    graph (BFS over an adjacency list).

Output format::

    {
        "version":   "<VERSION file contents>",
        "branch":    "<GITHUB_REF_NAME or local>",
        "commit":    "<GITHUB_SHA or git rev-parse>",
        "timestamp": "<ISO-8601 UTC>",
        "workloads": {
            "<workload_id>": {
                "throughput_ops_per_sec": <float>,
                "mean_latency_ms":        <float>,
                "p95_latency_ms":         <float>,
                "p99_latency_ms":         <float>
            },
            ...
        }
    }

Usage::

    python3 run_ci_benchmarks.py \\
        --output benchmark_results/chimera_results.json \\
        [--warmup 50] [--iterations 500]
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, List

# ---------------------------------------------------------------------------
# Make sure the chimera package is importable when run from the repo root
# ---------------------------------------------------------------------------
_REPO_ROOT = Path(__file__).resolve().parents[2]
_CHIMERA_DIR = _REPO_ROOT / "benchmarks" / "chimera"
if str(_CHIMERA_DIR) not in sys.path:
    sys.path.insert(0, str(_CHIMERA_DIR))

# Also add the benchmarks/ directory so that top-level benchmark modules
# (baseline_manager, performance_regression_detector) are importable if needed.
_BENCHMARKS_DIR = _REPO_ROOT / "benchmarks"
if str(_BENCHMARKS_DIR) not in sys.path:
    sys.path.insert(0, str(_BENCHMARKS_DIR))

from benchmark_harness import BenchmarkHarness, HarnessConfig, WorkloadDefinition  # noqa: E402


# ---------------------------------------------------------------------------
# Module-level data structures shared across benchmark iterations
# (built once to isolate the measured operation from setup overhead)
# ---------------------------------------------------------------------------

# Document store used by the document-lookup workload
_DOCUMENT_STORE: Dict[str, Any] = {
    f"doc_{i}": {"value": i, "tags": [i % 5]} for i in range(500)
}

# Adjacency list for the graph-BFS workload: chain graph 0→1→2→…→199
_GRAPH_ADJACENCY: Dict[int, List[int]] = {i: [i + 1] for i in range(199)}
_GRAPH_ADJACENCY[199] = []


# ---------------------------------------------------------------------------
# Synthetic workload implementations
# ---------------------------------------------------------------------------

def _relational_sort_op() -> None:
    """Simulate a relational ORDER-BY via an in-memory sort."""
    data = list(range(1000, 0, -1))
    data.sort()


def _vector_dot_product_op() -> None:
    """Simulate a vector similarity (dot-product) operation."""
    a = list(range(128))
    b = list(range(128, 256))
    _ = sum(x * y for x, y in zip(a, b))


def _document_lookup_op() -> None:
    """Simulate a document-store key lookup."""
    _ = _DOCUMENT_STORE.get("doc_250")


def _graph_bfs_op() -> None:
    """Simulate a BFS traversal over a small adjacency list."""
    visited = set()
    queue = [0]
    while queue:
        node = queue.pop(0)
        if node in visited:
            continue
        visited.add(node)
        queue.extend(_GRAPH_ADJACENCY.get(node, []))


# Workload registry: id → (operation, description, family)
_WORKLOADS = [
    WorkloadDefinition(
        workload_id="relational_sort",
        operation=_relational_sort_op,
        description="In-memory ORDER-BY simulation (relational family)",
        workload_family="custom",
    ),
    WorkloadDefinition(
        workload_id="vector_dot_product",
        operation=_vector_dot_product_op,
        description="128-dimensional dot-product similarity (vector family)",
        workload_family="custom",
    ),
    WorkloadDefinition(
        workload_id="document_lookup",
        operation=_document_lookup_op,
        description="Key-based document store lookup (document family)",
        workload_family="custom",
    ),
    WorkloadDefinition(
        workload_id="graph_bfs",
        operation=_graph_bfs_op,
        description="Breadth-first search over a chain graph (graph family)",
        workload_family="custom",
    ),
]


# ---------------------------------------------------------------------------
# Environment helpers
# ---------------------------------------------------------------------------

def _read_version(repo_root: Path) -> str:
    version_file = repo_root / "VERSION"
    if version_file.exists():
        return version_file.read_text(encoding="utf-8").strip()
    return "unknown"


def _git_sha(repo_root: Path) -> str:
    env_sha = os.environ.get("GITHUB_SHA", "")
    if env_sha:
        return env_sha
    try:
        result = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            cwd=repo_root,
            capture_output=True,
            text=True,
            timeout=10,
        )
        return result.stdout.strip() or "unknown"
    except Exception:
        return "unknown"


def _git_branch(repo_root: Path) -> str:
    env_ref = os.environ.get("GITHUB_REF_NAME", "")
    if env_ref:
        return env_ref
    try:
        result = subprocess.run(
            ["git", "rev-parse", "--abbrev-ref", "HEAD"],
            cwd=repo_root,
            capture_output=True,
            text=True,
            timeout=10,
        )
        return result.stdout.strip() or "unknown"
    except Exception:
        return "unknown"


# ---------------------------------------------------------------------------
# Main logic
# ---------------------------------------------------------------------------

def run_benchmarks(
    warmup_iterations: int = 50,
    run_iterations: int = 500,
) -> Dict[str, Any]:
    """Run all synthetic workloads and return the results dict."""
    config = HarnessConfig(
        warmup_iterations=warmup_iterations,
        run_iterations=run_iterations,
        percentiles=[50.0, 95.0, 99.0],
    )
    harness = BenchmarkHarness("chimera_ci", config)

    for wl in _WORKLOADS:
        harness.add_workload(wl)

    for wl in _WORKLOADS:
        print(f"  Warming up  : {wl.workload_id} ({warmup_iterations} iterations)")
        harness.warm_up(wl.workload_id)
        print(f"  Measuring   : {wl.workload_id} ({run_iterations} iterations)")
        harness.run(wl.workload_id)

    return harness.report()


def build_output(harness_report: Dict[str, Any], repo_root: Path) -> Dict[str, Any]:
    """Wrap the harness report with versioning metadata."""
    workloads: Dict[str, Any] = {}
    for wid, metrics in harness_report.get("workloads", {}).items():
        workloads[wid] = {
            "throughput_ops_per_sec": metrics.get("throughput_ops_per_sec"),
            "mean_latency_ms":        metrics.get("mean_latency_ms"),
            "p95_latency_ms":         metrics.get("p95_latency_ms"),
            "p99_latency_ms":         metrics.get("p99_latency_ms"),
        }

    return {
        "version":   _read_version(repo_root),
        "branch":    _git_branch(repo_root),
        "commit":    _git_sha(repo_root),
        "timestamp": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
        "workloads": workloads,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Run CHIMERA CI benchmarks and write a JSON results file."
    )
    parser.add_argument(
        "--output",
        default="benchmark_results/chimera_results.json",
        help="Destination JSON file (default: benchmark_results/chimera_results.json).",
    )
    parser.add_argument(
        "--warmup",
        type=int,
        default=50,
        help="Warm-up iterations per workload (default: 50).",
    )
    parser.add_argument(
        "--iterations",
        type=int,
        default=500,
        help="Measurement iterations per workload (default: 500).",
    )
    args = parser.parse_args(argv)

    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    print("Running CHIMERA CI benchmarks …")
    print(f"  Warmup iterations : {args.warmup}")
    print(f"  Run iterations    : {args.iterations}")
    print(f"  Output            : {output_path}")
    print()

    harness_report = run_benchmarks(
        warmup_iterations=args.warmup,
        run_iterations=args.iterations,
    )
    output = build_output(harness_report, _REPO_ROOT)

    output_path.write_text(json.dumps(output, indent=2), encoding="utf-8")
    print(f"\nResults written to : {output_path}")

    # Echo a brief summary
    for wid, metrics in output["workloads"].items():
        tput = metrics.get("throughput_ops_per_sec") or 0.0
        p99  = metrics.get("p99_latency_ms") or 0.0
        print(f"  {wid:30s}  throughput={tput:,.1f} ops/s  p99={p99:.4f} ms")

    return 0


if __name__ == "__main__":
    sys.exit(main())
