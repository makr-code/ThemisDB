"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            benchmark_harness.py                               ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-06 04:04:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     421                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • edfcc1a70a  2026-02-28  feat(chimera): implement BenchmarkDashboard for result ag... ║
    • 717272ce84  2026-02-27  fix(chimera): remove unused StatisticalAnalyzer import; c... ║
    • c39051d13c  2026-02-27  feat(chimera): implement unified benchmark harness ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
CHIMERA Suite: Unified Benchmark Harness

Provides workload definitions, warm-up, measurement, and reporting phases
for vendor-neutral database benchmarks.

Standards:
    - IEEE Std 2807-2022: Framework for Artificial Intelligence (AI) in Databases
    - ISO/IEC 14756:2015: Performance benchmarking
    - YCSB: Cooper et al., SoCC 2010 (DOI: 10.1145/1807128.1807152)
"""

import time
from dataclasses import dataclass, field
from typing import Any, Callable, Dict, List, Optional

import numpy as np


# ---------------------------------------------------------------------------
# Public data structures
# ---------------------------------------------------------------------------

@dataclass
class WorkloadDefinition:
    """Describes a single benchmark workload.

    Attributes:
        workload_id: Unique identifier used to reference the workload.
        operation: Zero-argument callable that performs one workload
            operation.  The harness measures the wall-clock time of each
            call.  Exceptions raised by the callable are counted as errors
            but do not abort the run.
        description: Human-readable summary of the workload.
        workload_family: Standard workload family (e.g. ``"YCSB"``,
            ``"TPC-C"``, ``"custom"``).
        standard_reference: IEEE/ACM citation for the workload standard.
        parameters: Arbitrary workload-specific configuration values (e.g.
            ``record_count``, ``read_ratio``).
    """

    workload_id: str
    operation: Callable[[], Any]
    description: str = ""
    workload_family: str = "custom"
    standard_reference: str = ""
    parameters: Dict[str, Any] = field(default_factory=dict)


@dataclass
class HarnessConfig:
    """Configuration for a :class:`BenchmarkHarness` run.

    Attributes:
        warmup_iterations: Number of un-timed warm-up calls to make before
            the measurement phase.
        run_iterations: Number of timed measurement calls.
        percentiles: Latency percentiles (in the range ``[0, 100]``) to
            include in the result summary.
    """

    warmup_iterations: int = 100
    run_iterations: int = 1000
    percentiles: List[float] = field(default_factory=lambda: [50.0, 95.0, 99.0])

    def __post_init__(self) -> None:
        if self.warmup_iterations < 0:
            raise ValueError(
                f"warmup_iterations must be >= 0, got {self.warmup_iterations}"
            )
        if self.run_iterations <= 0:
            raise ValueError(
                f"run_iterations must be > 0, got {self.run_iterations}"
            )
        for p in self.percentiles:
            if not (0.0 <= p <= 100.0):
                raise ValueError(
                    f"Each percentile must be in [0, 100], got {p}"
                )


@dataclass
class WorkloadResult:
    """Timing and throughput measurements from a single workload run.

    Attributes:
        workload_id: Identifier of the workload that was executed.
        system_name: Name of the system under test.
        latencies_ms: Raw per-operation latency samples in milliseconds.
        throughput_ops_per_sec: Measured operations per second.
        error_count: Number of operations that raised an exception.
        warmup_iterations: Configured warm-up iteration count.
        run_iterations: Configured measurement iteration count.
        elapsed_seconds: Total wall-clock time of the measurement phase.
        mean_latency_ms: Arithmetic mean of ``latencies_ms``.
        p50_latency_ms: 50th-percentile (median) latency in ms.
        p95_latency_ms: 95th-percentile latency in ms.
        p99_latency_ms: 99th-percentile latency in ms.
        percentile_latencies_ms: Mapping of *percentile* → *latency (ms)*
            for all percentiles requested via :attr:`HarnessConfig.percentiles`.
    """

    workload_id: str
    system_name: str
    latencies_ms: List[float]
    throughput_ops_per_sec: float
    error_count: int
    warmup_iterations: int
    run_iterations: int
    elapsed_seconds: float
    mean_latency_ms: float = 0.0
    p50_latency_ms: float = 0.0
    p95_latency_ms: float = 0.0
    p99_latency_ms: float = 0.0
    percentile_latencies_ms: Dict[float, float] = field(default_factory=dict)


# ---------------------------------------------------------------------------
# Harness
# ---------------------------------------------------------------------------

class BenchmarkHarness:
    """Unified benchmark harness: warm-up, run, and report.

    Usage::

        harness = BenchmarkHarness("SystemA", HarnessConfig(warmup_iterations=10,
                                                             run_iterations=100))
        harness.add_workload(WorkloadDefinition("ycsb_a", my_operation))
        harness.warm_up("ycsb_a")
        result = harness.run("ycsb_a")
        summary = harness.report()
    """

    def __init__(
        self,
        system_name: str,
        config: Optional[HarnessConfig] = None,
    ) -> None:
        """
        Args:
            system_name: Vendor-neutral identifier for the system under test.
            config: Harness configuration; defaults to :class:`HarnessConfig`
                defaults if *None*.
        """
        self._system_name = system_name
        self._config: HarnessConfig = config if config is not None else HarnessConfig()
        self._workloads: Dict[str, WorkloadDefinition] = {}
        self._results: Dict[str, WorkloadResult] = {}

    @property
    def system_name(self) -> str:
        """Vendor-neutral identifier for the system under test."""
        return self._system_name

    # ------------------------------------------------------------------
    # Workload registration
    # ------------------------------------------------------------------

    def add_workload(self, workload: WorkloadDefinition) -> None:
        """Register a workload with the harness.

        Args:
            workload: The workload to register.  If a workload with the same
                :attr:`~WorkloadDefinition.workload_id` is already registered
                it is replaced.
        """
        self._workloads[workload.workload_id] = workload

    def get_workload_ids(self) -> List[str]:
        """Return the IDs of all registered workloads."""
        return list(self._workloads.keys())

    # ------------------------------------------------------------------
    # Phases
    # ------------------------------------------------------------------

    def warm_up(self, workload_id: str) -> int:
        """Execute un-timed warm-up iterations for *workload_id*.

        This phase is intended to prime OS page caches, JIT compilers, and
        database buffer pools before the measurement phase begins.  Errors
        during warm-up are silently ignored.

        Args:
            workload_id: ID of the workload to warm up.

        Returns:
            Number of warm-up iterations that completed without error.

        Raises:
            KeyError: If *workload_id* is not registered.
        """
        workload = self._workloads[workload_id]
        successful = 0
        for _ in range(self._config.warmup_iterations):
            try:
                workload.operation()
                successful += 1
            except Exception:
                pass
        return successful

    def run(self, workload_id: str) -> WorkloadResult:
        """Execute the timed measurement phase for *workload_id*.

        Each of the :attr:`~HarnessConfig.run_iterations` calls is timed
        individually with :func:`time.perf_counter`.  Errors are counted but
        do not stop the measurement phase.

        Args:
            workload_id: ID of the workload to measure.

        Returns:
            A :class:`WorkloadResult` with timing statistics.

        Note:
            Latency samples are recorded for **every** iteration, including
            those that raised an exception.  This preserves the true end-to-end
            cost of each call (network timeout, error-path code, etc.) in the
            statistics.  The :attr:`WorkloadResult.error_count` field lets
            callers filter or annotate failed samples in post-processing.

        Raises:
            KeyError: If *workload_id* is not registered.
        """
        workload = self._workloads[workload_id]
        latencies: List[float] = []
        errors = 0

        phase_start = time.perf_counter()
        for _ in range(self._config.run_iterations):
            t0 = time.perf_counter()
            try:
                workload.operation()
            except Exception:
                errors += 1
            latencies.append((time.perf_counter() - t0) * 1_000.0)  # → ms

        elapsed = time.perf_counter() - phase_start

        throughput = len(latencies) / elapsed if elapsed > 0.0 else 0.0

        result = self._build_result(
            workload_id=workload_id,
            latencies_ms=latencies,
            throughput_ops_per_sec=throughput,
            error_count=errors,
            elapsed_seconds=elapsed,
        )
        self._results[workload_id] = result
        return result

    def run_all(self, warmup: bool = True) -> Dict[str, WorkloadResult]:
        """Run warm-up (optional) and measurement for every registered workload.

        Args:
            warmup: When *True* (default), the warm-up phase is executed
                before each workload's measurement phase.

        Returns:
            Mapping of *workload_id* → :class:`WorkloadResult`.
        """
        for wid in list(self._workloads):
            if warmup:
                self.warm_up(wid)
            self.run(wid)
        return dict(self._results)

    # ------------------------------------------------------------------
    # Results access
    # ------------------------------------------------------------------

    def get_result(self, workload_id: str) -> Optional[WorkloadResult]:
        """Return the :class:`WorkloadResult` for *workload_id*, or *None*."""
        return self._results.get(workload_id)

    # ------------------------------------------------------------------
    # Reporting
    # ------------------------------------------------------------------

    def report(self, output_path: Optional[str] = None) -> Dict[str, Any]:
        """Generate a summary report of all completed workload runs.

        The summary is always returned as a :class:`dict`.  When
        *output_path* is provided and ends with ``".html"``, an HTML report
        is written via :class:`~chimera.reporter.ChimeraReporter`.

        Args:
            output_path: Optional file path for an HTML report.

        Returns:
            A JSON-serialisable summary dict with the structure::

                {
                    "system_name": "<name>",
                    "workloads": {
                        "<workload_id>": {
                            "throughput_ops_per_sec": ...,
                            "mean_latency_ms": ...,
                            "p50_latency_ms": ...,
                            "p95_latency_ms": ...,
                            "p99_latency_ms": ...,
                            "error_count": ...,
                            "run_iterations": ...,
                            "elapsed_seconds": ...,
                        },
                        ...
                    }
                }
        """
        summary: Dict[str, Any] = {
            "system_name": self._system_name,
            "workloads": {},
        }
        for wid, result in self._results.items():
            summary["workloads"][wid] = {
                "throughput_ops_per_sec": result.throughput_ops_per_sec,
                "mean_latency_ms": result.mean_latency_ms,
                "p50_latency_ms": result.p50_latency_ms,
                "p95_latency_ms": result.p95_latency_ms,
                "p99_latency_ms": result.p99_latency_ms,
                "error_count": result.error_count,
                "run_iterations": result.run_iterations,
                "elapsed_seconds": result.elapsed_seconds,
            }
            # Include any additional requested percentiles
            for pct, lat in result.percentile_latencies_ms.items():
                key = f"p{pct:.4g}_latency_ms"
                summary["workloads"][wid][key] = lat

        if output_path and output_path.endswith(".html"):
            self._write_html_report(output_path)

        return summary

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    def _build_result(
        self,
        workload_id: str,
        latencies_ms: List[float],
        throughput_ops_per_sec: float,
        error_count: int,
        elapsed_seconds: float,
    ) -> WorkloadResult:
        """Compute summary statistics and return a :class:`WorkloadResult`."""
        arr = np.array(latencies_ms, dtype=float) if latencies_ms else np.array([0.0])

        mean_lat = float(np.mean(arr))
        p50 = float(np.percentile(arr, 50))
        p95 = float(np.percentile(arr, 95))
        p99 = float(np.percentile(arr, 99))

        extra_percentiles: Dict[float, float] = {
            pct: float(np.percentile(arr, pct))
            for pct in self._config.percentiles
        }

        return WorkloadResult(
            workload_id=workload_id,
            system_name=self._system_name,
            latencies_ms=latencies_ms,
            throughput_ops_per_sec=throughput_ops_per_sec,
            error_count=error_count,
            warmup_iterations=self._config.warmup_iterations,
            run_iterations=self._config.run_iterations,
            elapsed_seconds=elapsed_seconds,
            mean_latency_ms=mean_lat,
            p50_latency_ms=p50,
            p95_latency_ms=p95,
            p99_latency_ms=p99,
            percentile_latencies_ms=extra_percentiles,
        )

    def _write_html_report(self, output_path: str) -> None:
        """Write an HTML report using :class:`~chimera.reporter.ChimeraReporter`."""
        from .reporter import ChimeraReporter

        reporter = ChimeraReporter()
        for wid, result in self._results.items():
            reporter.add_system_results(
                system_name=self._system_name,
                metric_name=f"{wid} Throughput (ops/s)",
                metric_unit="ops/s",
                data=[result.throughput_ops_per_sec],
            )
            reporter.add_system_results(
                system_name=self._system_name,
                metric_name=f"{wid} Mean Latency (ms)",
                metric_unit="ms",
                data=[result.mean_latency_ms],
            )
        reporter.generate_html_report(output_path)
