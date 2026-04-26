#!/usr/bin/env python3
"""
CHIMERA Native CI Benchmark Runner
===================================
``external/chimera/run_ci_benchmarks.py``

Defines and executes the **four canonical Chimera workloads** that form the
native benchmark path for cross-module comparison:

===================  ==========================================================
Workload ID          Description
===================  ==========================================================
relational_sort      In-memory sort of a mixed-order integer sequence; proxies
                     query-engine and storage-layer ordering performance.
vector_dot_product   Dot-product similarity over a small dense float corpus;
                     proxies vector-index and embedding-layer throughput.
document_lookup      Key-value hash-map read; proxies document-store and cache
                     read path latency.
graph_bfs            Breadth-first traversal of a synthetic adjacency list;
                     proxies graph-engine and AQL traversal throughput.
===================  ==========================================================

Output schema (JSON)
--------------------
The output file follows the same schema as ``baselines/chimera/baseline.json``:

.. code-block:: json

    {
      "version":   "<semver from VERSION file>",
      "branch":    "<git branch>",
      "commit":    "<git SHA>",
      "timestamp": "<ISO-8601 UTC>",
      "workloads": {
        "<workload_id>": {
          "throughput_ops_per_sec": 12345.6,
          "mean_latency_ms":        0.08,
          "p95_latency_ms":         0.12,
          "p99_latency_ms":         0.15
        }
      }
    }

CLI
---
::

    python run_ci_benchmarks.py [--output PATH] [--warmup N] [--iterations N]

Default output path: ``benchmark_results/chimera_results.json``

Methodology
-----------
See ``benchmarks/chimera/CHIMERA_README.md`` for full methodology documentation
(sampling strategy, variance treatment, platform requirements).
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, List, Optional

# ---------------------------------------------------------------------------
# Resolve repository root and make the benchmarks/ package importable
# ---------------------------------------------------------------------------
_REPO_ROOT: Path = Path(__file__).resolve().parents[2]
_BENCHMARKS_DIR: Path = _REPO_ROOT / "benchmarks"

if str(_BENCHMARKS_DIR) not in sys.path:
    sys.path.insert(0, str(_BENCHMARKS_DIR))

from chimera import (  # noqa: E402
    BenchmarkHarness,
    HarnessConfig,
    WorkloadDefinition,
)

# ---------------------------------------------------------------------------
# Canonical workload operations
# ---------------------------------------------------------------------------

def _op_relational_sort() -> None:
    """Sort a pre-shuffled integer sequence — proxies query/storage ordering."""
    data = list(range(500, 0, -1)) + list(range(500, 1000))
    data.sort()


def _op_vector_dot_product() -> None:
    """Dot-product similarity search over a dense float corpus — proxies vector index."""
    query = list(range(64))
    corpus = [list(range(i, i + 64)) for i in range(0, 256, 4)]
    _ = max(range(len(corpus)),
            key=lambda i: sum(a * b for a, b in zip(query, corpus[i])))


def _op_document_lookup() -> None:
    """Hash-map key lookup — proxies document-store / cache read path."""
    _STORE: Dict[str, int] = {f"doc_{j}": j * 3 for j in range(200)}
    _ = _STORE.get("doc_100")


# Pre-build the BFS graph once at module load time so per-call overhead is
# only the traversal, not graph construction.
_BFS_GRAPH: Dict[int, List[int]] = {
    i: [(i * 2 + 1) % 64, (i * 2 + 2) % 64]
    for i in range(64)
}


def _op_graph_bfs() -> None:
    """BFS traversal of a synthetic 64-node adjacency list — proxies graph engine."""
    visited: List[bool] = [False] * 64
    queue: List[int] = [0]
    visited[0] = True
    while queue:
        node = queue.pop(0)
        for neighbour in _BFS_GRAPH.get(node, []):
            if not visited[neighbour]:
                visited[neighbour] = True
                queue.append(neighbour)


# ---------------------------------------------------------------------------
# Workload registry
# ---------------------------------------------------------------------------

_WORKLOADS: List[WorkloadDefinition] = [
    WorkloadDefinition(
        workload_id="relational_sort",
        operation=_op_relational_sort,
        description="Sort a mixed-order integer sequence; proxy for query-engine ordering",
        workload_family="relational",
    ),
    WorkloadDefinition(
        workload_id="vector_dot_product",
        operation=_op_vector_dot_product,
        description="Dot-product similarity over dense float corpus; proxy for vector index",
        workload_family="vector",
    ),
    WorkloadDefinition(
        workload_id="document_lookup",
        operation=_op_document_lookup,
        description="Hash-map key-value lookup; proxy for document-store / cache read path",
        workload_family="document",
    ),
    WorkloadDefinition(
        workload_id="graph_bfs",
        operation=_op_graph_bfs,
        description="BFS traversal of synthetic adjacency list; proxy for graph engine",
        workload_family="graph",
    ),
]


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def run_benchmarks(
    warmup_iterations: int = 3,
    run_iterations: int = 100,
) -> Dict[str, Any]:
    """Execute all four canonical Chimera workloads and return a harness report.

    Parameters
    ----------
    warmup_iterations:
        Number of warm-up iterations to discard before measurement.
    run_iterations:
        Number of timed measurement iterations per workload.

    Returns
    -------
    dict
        ``{"system_name": "chimera_ci", "workloads": {<id>: {metrics...}}}``
        Compatible with the ``bench_coverage_report.py`` traffic-light schema.
    """
    config = HarnessConfig(
        warmup_iterations=warmup_iterations,
        run_iterations=run_iterations,
        percentiles=[50.0, 95.0, 99.0],
    )
    harness = BenchmarkHarness(system_name="chimera_ci", config=config)
    for wl in _WORKLOADS:
        harness.add_workload(wl)

    workloads_out: Dict[str, Any] = {}
    for wl in _WORKLOADS:
        harness.warm_up(wl.workload_id)
        result = harness.run(wl.workload_id)
        workloads_out[wl.workload_id] = {
            "throughput_ops_per_sec": result.throughput_ops_per_sec,
            "mean_latency_ms": result.mean_latency_ms,
            "p95_latency_ms": result.p95_latency_ms,
            "p99_latency_ms": result.p99_latency_ms,
            "error_count": result.error_count,
            "run_iterations": result.run_iterations,
            "elapsed_seconds": result.elapsed_seconds,
        }

    return {"system_name": "chimera_ci", "workloads": workloads_out}


def build_output(report: Dict[str, Any], repo_root: Path) -> Dict[str, Any]:
    """Wrap a harness report with versioning metadata.

    Reads ``VERSION`` from *repo_root*, queries git for branch/commit.
    Fields are safe to serialise as JSON.

    Parameters
    ----------
    report:
        Dict returned by :func:`run_benchmarks`.
    repo_root:
        Root of the ThemisDB repository (used to locate ``VERSION`` and git).

    Returns
    -------
    dict
        ``{"version": …, "branch": …, "commit": …, "timestamp": …,
           "workloads": {…}}``
    """
    version = _read_version(repo_root)
    branch = _git_info(repo_root, ["rev-parse", "--abbrev-ref", "HEAD"])
    commit = _git_info(repo_root, ["rev-parse", "--short", "HEAD"])
    timestamp = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    # Flatten workload metrics into the baseline-compatible schema
    workloads: Dict[str, Any] = {}
    for wl_id, data in report.get("workloads", {}).items():
        workloads[wl_id] = {
            "throughput_ops_per_sec": data.get("throughput_ops_per_sec", 0.0),
            "mean_latency_ms": data.get("mean_latency_ms", 0.0),
            "p95_latency_ms": data.get("p95_latency_ms", 0.0),
            "p99_latency_ms": data.get("p99_latency_ms", 0.0),
        }
        # Carry through extra fields if present (e.g. error_count for coverage logic)
        for extra_key in ("error_count", "run_iterations", "elapsed_seconds"):
            if extra_key in data:
                workloads[wl_id][extra_key] = data[extra_key]

    return {
        "version": version,
        "branch": branch,
        "commit": commit,
        "timestamp": timestamp,
        "workloads": workloads,
    }


def main(argv: Optional[List[str]] = None) -> int:
    """CLI entry point.

    Usage::

        python run_ci_benchmarks.py [--output PATH] [--warmup N] [--iterations N]

    Returns
    -------
    int
        Exit code (0 = success).
    """
    parser = argparse.ArgumentParser(
        description="Run CHIMERA native CI benchmarks and export JSON results.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--output",
        default=None,
        help=(
            "Path to output JSON file "
            "(default: benchmark_results/chimera_results.json)"
        ),
    )
    parser.add_argument(
        "--warmup",
        type=int,
        default=3,
        help="Number of warm-up iterations to discard (default: 3)",
    )
    parser.add_argument(
        "--iterations",
        type=int,
        default=100,
        help="Number of timed measurement iterations per workload (default: 100)",
    )
    args = parser.parse_args(argv)

    output_path = (
        Path(args.output)
        if args.output is not None
        else Path("benchmark_results") / "chimera_results.json"
    )
    output_path.parent.mkdir(parents=True, exist_ok=True)

    print(f"[chimera] Running {len(_WORKLOADS)} workloads "
          f"(warmup={args.warmup}, iterations={args.iterations}) …")

    report = run_benchmarks(
        warmup_iterations=args.warmup,
        run_iterations=args.iterations,
    )
    output = build_output(report, _REPO_ROOT)

    output_path.write_text(json.dumps(output, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"[chimera] Results written to {output_path}")

    # Print summary table
    print(f"\n{'Workload':<22}  {'Throughput (ops/s)':>20}  {'Mean (ms)':>12}  {'P99 (ms)':>10}")
    print("-" * 72)
    for wl_id, data in output["workloads"].items():
        tput = data["throughput_ops_per_sec"]
        mean = data["mean_latency_ms"]
        p99 = data["p99_latency_ms"]
        print(f"  {wl_id:<20}  {tput:>20,.1f}  {mean:>12.4f}  {p99:>10.4f}")

    return 0


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _read_version(repo_root: Path) -> str:
    """Read version string from ``VERSION`` file; fall back to ``"unknown"``."""
    version_file = repo_root / "VERSION"
    if version_file.exists():
        return version_file.read_text(encoding="utf-8").strip()
    return "unknown"


def _git_info(repo_root: Path, args: List[str]) -> str:
    """Run ``git <args>`` in *repo_root* and return stripped stdout.

    Returns ``"unknown"`` on failure (no git, not a repo, shallow clone, …).
    """
    try:
        result = subprocess.run(
            ["git", *args],
            cwd=str(repo_root),
            capture_output=True,
            text=True,
            timeout=10,
        )
        if result.returncode == 0:
            return result.stdout.strip()
    except Exception:
        pass
    return "unknown"


if __name__ == "__main__":
    sys.exit(main())
