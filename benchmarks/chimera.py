#!/usr/bin/env python3
"""
CHIMERA Benchmark Framework — Core Python Library
=================================================
Comprehensive Hybrid Inferencing & Multi-model Evaluation Resource Assessment

Provides the foundational classes used by:
- ``external/chimera/run_ci_benchmarks.py``  (native CI benchmark path)
- ``benchmarks/performance_optimizations/validate_optimization.py``

Classes
-------
HarnessConfig
    Configuration for warm-up and measurement phases plus percentile targets.
WorkloadDefinition
    Descriptor for a single benchmark workload (callable + metadata).
WorkloadResult
    Measured results for one workload execution.
BenchmarkHarness
    Orchestrates warm-up, timed measurement, and per-percentile statistics.
StatisticalAnalyzer
    Welch's t-test and descriptive statistics for result comparison.
TTestResult
    Output of :meth:`StatisticalAnalyzer.t_test`.

Schema version: 1.0.0
Standard: IEEE Std 2807-2022, ISO/IEC 14756:2015
"""

from __future__ import annotations

import math
import statistics
import time
from dataclasses import dataclass, field
from typing import Callable, Dict, List, Optional

__all__ = [
    "HarnessConfig",
    "WorkloadDefinition",
    "WorkloadResult",
    "BenchmarkHarness",
    "StatisticalAnalyzer",
    "TTestResult",
]

# ---------------------------------------------------------------------------
# Data classes
# ---------------------------------------------------------------------------

@dataclass
class HarnessConfig:
    """Configuration passed to :class:`BenchmarkHarness`.

    Parameters
    ----------
    warmup_iterations:
        Number of iterations discarded before measurement begins.
    run_iterations:
        Number of timed measurement iterations.
    percentiles:
        Percentile points (0–100) to compute over iteration latencies.
    """
    warmup_iterations: int = 3
    run_iterations: int = 100
    percentiles: List[float] = field(default_factory=lambda: [50.0, 95.0, 99.0])


@dataclass
class WorkloadDefinition:
    """Descriptor for one benchmark workload.

    Parameters
    ----------
    workload_id:
        Unique string identifier (e.g. ``"relational_sort"``).
    operation:
        Zero-argument callable that performs one unit of work.
    description:
        Human-readable description of the workload.
    workload_family:
        Broad category (e.g. ``"relational"``, ``"vector"``, ``"graph"``).
    """
    workload_id: str
    operation: Callable[[], None]
    description: str = ""
    workload_family: str = "generic"


@dataclass
class WorkloadResult:
    """Measurement results for a single workload run.

    All latency fields are in **milliseconds**.
    """
    workload_id: str
    throughput_ops_per_sec: float
    mean_latency_ms: float
    p50_latency_ms: float
    p95_latency_ms: float
    p99_latency_ms: float
    #: Mapping from percentile → latency (ms) for every value in
    #: :attr:`HarnessConfig.percentiles`.
    percentile_latencies_ms: Dict[float, float] = field(default_factory=dict)
    error_count: int = 0
    run_iterations: int = 0
    elapsed_seconds: float = 0.0


# ---------------------------------------------------------------------------
# BenchmarkHarness
# ---------------------------------------------------------------------------

class BenchmarkHarness:
    """Executes benchmark workloads with warm-up and statistical collection.

    Usage::

        config = HarnessConfig(warmup_iterations=3, run_iterations=100)
        harness = BenchmarkHarness(system_name="my_system", config=config)
        harness.add_workload(WorkloadDefinition(
            workload_id="my_workload",
            operation=lambda: do_work(),
        ))
        harness.warm_up("my_workload")
        result = harness.run("my_workload")
        print(result.throughput_ops_per_sec)
    """

    def __init__(
        self,
        system_name: str = "chimera",
        config: Optional[HarnessConfig] = None,
    ) -> None:
        self.system_name = system_name
        self.config = config or HarnessConfig()
        self._workloads: Dict[str, WorkloadDefinition] = {}

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def add_workload(self, workload: WorkloadDefinition) -> None:
        """Register a workload for later measurement."""
        self._workloads[workload.workload_id] = workload

    def warm_up(self, workload_id: str) -> None:
        """Execute warm-up iterations for *workload_id* (results discarded)."""
        wl = self._get_workload(workload_id)
        for _ in range(self.config.warmup_iterations):
            wl.operation()

    def run(self, workload_id: str) -> WorkloadResult:
        """Time *run_iterations* executions and return a :class:`WorkloadResult`.

        Parameters
        ----------
        workload_id:
            Must have been added via :meth:`add_workload`.

        Returns
        -------
        WorkloadResult
            Throughput, mean/percentile latencies, error count, and wall time.
        """
        wl = self._get_workload(workload_id)
        n = self.config.run_iterations
        error_count = 0
        latencies_ms: List[float] = []

        t_start_total = time.perf_counter()
        for _ in range(n):
            t0 = time.perf_counter()
            try:
                wl.operation()
            except Exception:
                error_count += 1
            t1 = time.perf_counter()
            latencies_ms.append((t1 - t0) * 1_000.0)
        elapsed = time.perf_counter() - t_start_total

        return self._compute_result(
            workload_id=workload_id,
            latencies_ms=latencies_ms,
            elapsed_seconds=elapsed,
            error_count=error_count,
        )

    def run_all(self) -> Dict:
        """Run all registered workloads and return a harness report dict.

        Returns a dict with ``system_name`` and a ``workloads`` sub-dict
        mapping workload_id → result dict.
        """
        workloads_out: Dict[str, Dict] = {}
        for wl_id in self._workloads:
            self.warm_up(wl_id)
            result = self.run(wl_id)
            workloads_out[wl_id] = {
                "throughput_ops_per_sec": result.throughput_ops_per_sec,
                "mean_latency_ms": result.mean_latency_ms,
                "p95_latency_ms": result.p95_latency_ms,
                "p99_latency_ms": result.p99_latency_ms,
                "error_count": result.error_count,
                "run_iterations": result.run_iterations,
                "elapsed_seconds": result.elapsed_seconds,
            }
        return {"system_name": self.system_name, "workloads": workloads_out}

    # ------------------------------------------------------------------
    # Internals
    # ------------------------------------------------------------------

    def _get_workload(self, workload_id: str) -> WorkloadDefinition:
        if workload_id not in self._workloads:
            raise KeyError(f"Workload '{workload_id}' not registered in this harness.")
        return self._workloads[workload_id]

    def _compute_result(
        self,
        workload_id: str,
        latencies_ms: List[float],
        elapsed_seconds: float,
        error_count: int,
    ) -> WorkloadResult:
        n = len(latencies_ms)
        if n == 0:
            return WorkloadResult(
                workload_id=workload_id,
                throughput_ops_per_sec=0.0,
                mean_latency_ms=0.0,
                p50_latency_ms=0.0,
                p95_latency_ms=0.0,
                p99_latency_ms=0.0,
                error_count=error_count,
                run_iterations=0,
                elapsed_seconds=elapsed_seconds,
            )

        sorted_latencies = sorted(latencies_ms)
        mean_ms = statistics.mean(latencies_ms)
        throughput = n / elapsed_seconds if elapsed_seconds > 0.0 else 0.0

        percentile_map: Dict[float, float] = {}
        for pct in self.config.percentiles:
            percentile_map[pct] = self._percentile(sorted_latencies, pct)

        p50 = percentile_map.get(50.0, self._percentile(sorted_latencies, 50.0))
        p95 = percentile_map.get(95.0, self._percentile(sorted_latencies, 95.0))
        p99 = percentile_map.get(99.0, self._percentile(sorted_latencies, 99.0))

        return WorkloadResult(
            workload_id=workload_id,
            throughput_ops_per_sec=throughput,
            mean_latency_ms=mean_ms,
            p50_latency_ms=p50,
            p95_latency_ms=p95,
            p99_latency_ms=p99,
            percentile_latencies_ms=percentile_map,
            error_count=error_count,
            run_iterations=n,
            elapsed_seconds=elapsed_seconds,
        )

    @staticmethod
    def _percentile(sorted_data: List[float], pct: float) -> float:
        """Compute *pct*-th percentile using nearest-rank method."""
        n = len(sorted_data)
        if n == 0:
            return 0.0
        # Clamp to [0, 100]
        pct = max(0.0, min(100.0, pct))
        # Nearest-rank index (1-based → 0-based)
        rank = math.ceil(pct / 100.0 * n)
        rank = max(1, min(rank, n))
        return sorted_data[rank - 1]


# ---------------------------------------------------------------------------
# StatisticalAnalyzer
# ---------------------------------------------------------------------------

@dataclass
class TTestResult:
    """Output of :meth:`StatisticalAnalyzer.t_test`."""
    p_value: float
    t_statistic: float
    is_significant: bool
    degrees_of_freedom: float


class StatisticalAnalyzer:
    """Provides statistical tests for benchmark result comparison.

    Parameters
    ----------
    significance_level:
        α threshold for hypothesis testing (default 0.05).
    """

    def __init__(self, significance_level: float = 0.05) -> None:
        self.significance_level = significance_level

    def t_test(self, a: List[float], b: List[float]) -> TTestResult:
        """Perform Welch's two-sample t-test (unequal variances).

        Tests H₀: mean(a) == mean(b) vs H₁: mean(a) ≠ mean(b).

        Parameters
        ----------
        a, b:
            Sample observation lists (each ≥ 2 elements).

        Returns
        -------
        TTestResult
            With ``p_value``, ``t_statistic``, ``degrees_of_freedom``,
            and ``is_significant`` set to ``True`` when p < α.

        Raises
        ------
        ValueError
            If either sample has fewer than 2 observations.
        """
        if len(a) < 2 or len(b) < 2:
            raise ValueError("Each sample must have at least 2 observations for t_test.")

        mean_a = statistics.mean(a)
        mean_b = statistics.mean(b)
        var_a = statistics.variance(a)
        var_b = statistics.variance(b)
        n_a = len(a)
        n_b = len(b)

        s_a2 = var_a / n_a
        s_b2 = var_b / n_b
        se = math.sqrt(s_a2 + s_b2)

        if se == 0.0:
            # Identical samples: t = 0, p = 1.0
            return TTestResult(
                p_value=1.0,
                t_statistic=0.0,
                is_significant=False,
                degrees_of_freedom=float(n_a + n_b - 2),
            )

        t_stat = (mean_a - mean_b) / se

        # Welch–Satterthwaite degrees of freedom
        numerator = (s_a2 + s_b2) ** 2
        denominator = (s_a2 ** 2) / (n_a - 1) + (s_b2 ** 2) / (n_b - 1)
        dof = numerator / denominator if denominator > 0.0 else float(n_a + n_b - 2)

        p_value = self._t_cdf_two_tailed(abs(t_stat), dof)
        return TTestResult(
            p_value=p_value,
            t_statistic=t_stat,
            is_significant=p_value < self.significance_level,
            degrees_of_freedom=dof,
        )

    # ------------------------------------------------------------------
    # Descriptive statistics helpers
    # ------------------------------------------------------------------

    def describe(self, data: List[float]) -> Dict[str, float]:
        """Return basic descriptive statistics for *data*."""
        if not data:
            return {}
        sorted_d = sorted(data)
        n = len(data)
        return {
            "n": float(n),
            "mean": statistics.mean(data),
            "median": statistics.median(data),
            "stdev": statistics.stdev(data) if n > 1 else 0.0,
            "min": sorted_d[0],
            "max": sorted_d[-1],
            "p95": BenchmarkHarness._percentile(sorted_d, 95.0),
            "p99": BenchmarkHarness._percentile(sorted_d, 99.0),
        }

    # ------------------------------------------------------------------
    # Internals: approximate t-distribution CDF (two-tailed p-value)
    # ------------------------------------------------------------------

    @staticmethod
    def _t_cdf_two_tailed(t: float, dof: float) -> float:
        """Approximate two-tailed p-value via incomplete Beta function (regularised).

        Uses the relationship p = I(dof / (dof + t²), dof/2, 1/2).
        For large dof this closely approximates the normal distribution.
        """
        if dof <= 0.0:
            return 1.0
        x = dof / (dof + t * t)
        p_one_tail = 0.5 * StatisticalAnalyzer._regularised_incomplete_beta(x, dof / 2.0, 0.5)
        return min(1.0, 2.0 * p_one_tail)

    @staticmethod
    def _regularised_incomplete_beta(x: float, a: float, b: float, max_iter: int = 200) -> float:
        """Regularised incomplete beta function I_x(a, b) via continued fractions (Lentz)."""
        if x <= 0.0:
            return 0.0
        if x >= 1.0:
            return 1.0

        # Use the symmetry relation when x > (a+1)/(a+b+2)
        if x > (a + 1.0) / (a + b + 2.0):
            return 1.0 - StatisticalAnalyzer._regularised_incomplete_beta(
                1.0 - x, b, a, max_iter
            )

        ln_beta = math.lgamma(a) + math.lgamma(b) - math.lgamma(a + b)
        front = math.exp(math.log(x) * a + math.log(1.0 - x) * b - ln_beta) / a

        # Lentz's continued-fraction method
        cf = StatisticalAnalyzer._beta_cf(x, a, b, max_iter)
        return front * cf

    @staticmethod
    def _beta_cf(x: float, a: float, b: float, max_iter: int) -> float:
        """Evaluate the continued fraction for the regularised incomplete beta.

        Uses the modified Lentz algorithm (Numerical Recipes, 3rd ed. §6.4).
        """
        eps = 3e-12
        tiny = 1e-30

        qab = a + b
        qap = a + 1.0
        qam = a - 1.0

        c = 1.0
        d = 1.0 - qab * x / qap
        if abs(d) < tiny:
            d = tiny
        d = 1.0 / d
        h = d

        for m in range(1, max_iter + 1):
            m2 = 2 * m
            # Even step
            aa = m * (b - m) * x / ((qam + m2) * (a + m2))
            d = 1.0 + aa * d
            if abs(d) < tiny:
                d = tiny
            c = 1.0 + aa / c
            if abs(c) < tiny:
                c = tiny
            d = 1.0 / d
            h *= d * c

            # Odd step
            aa = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2))
            d = 1.0 + aa * d
            if abs(d) < tiny:
                d = tiny
            c = 1.0 + aa / c
            if abs(c) < tiny:
                c = tiny
            d = 1.0 / d
            delta = d * c
            h *= delta

            if abs(delta - 1.0) < eps:
                break

        return h
