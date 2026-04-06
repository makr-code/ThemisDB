"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            scoring.py                                         ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-06 04:04:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     479                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 701b8592a3  2026-02-28  Add scoring framework docs and update header quality metrics ║
    • b081c5508d  2026-02-28  feat(chimera): integrate StatisticalAnalyzer with Benchma... ║
    • fc2f092b63  2026-02-26  audit: remove unused imports, dead code, raise coverage t... ║
    • f2104380d2  2026-02-26  feat(chimera): add benchmark result normalization and sco... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Benchmark Result Normalization and Scoring Framework

Implements vendor-neutral normalization of raw benchmark metrics into
comparable scores and composite rankings, following IEEE/ACM standards
for fair and reproducible database benchmarking.

References:
- YCSB: Cooper, B.F., et al. (2010). Benchmarking Cloud Serving Systems with YCSB.
- TPC-C: Transaction Processing Performance Council. (2010). TPC Benchmark C.
- IEEE Std 7-4.3.2-2016: Criteria for Digital Computers in Safety Systems.
"""

from enum import Enum
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass, field

import numpy as np


class MetricDirection(Enum):
    """Indicates the optimal direction for a benchmark metric.

    HIGHER_IS_BETTER: e.g., throughput (ops/s), recall, accuracy.
    LOWER_IS_BETTER: e.g., latency (ms), error rate, resource usage.
    """
    HIGHER_IS_BETTER = "higher_is_better"
    LOWER_IS_BETTER = "lower_is_better"


class NormalizationMethod(Enum):
    """Normalization strategy for converting raw values to [0, 100] scores.

    MIN_MAX: Linear rescaling to [0, 100] based on observed min/max.
    BASELINE: Score relative to a designated reference system (100 = baseline).
    Z_SCORE: Standardize via (x - mean) / std, then map to [0, 100].
    """
    MIN_MAX = "min_max"
    BASELINE = "baseline"
    Z_SCORE = "z_score"


@dataclass
class MetricConfig:
    """Configuration for a single benchmark metric.

    Attributes:
        name: Metric identifier (e.g., ``"p99_latency_ms"``).
        direction: Whether higher or lower values are better.
        weight: Relative weight in composite score (default 1.0).
        unit: Human-readable unit string (e.g., ``"ms"``, ``"ops/s"``).
    """
    name: str
    direction: MetricDirection
    weight: float = 1.0
    unit: str = ""

    def __post_init__(self) -> None:
        if self.weight <= 0:
            raise ValueError(f"MetricConfig weight must be positive, got {self.weight}")


@dataclass
class NormalizedScore:
    """Normalized score for one system on one metric.

    Attributes:
        system_name: Name of the evaluated system.
        metric_name: Name of the metric.
        raw_mean: Arithmetic mean of the raw measurement samples.
        raw_std: Sample standard deviation of the raw measurements.
        normalized_score: Score in [0, 100]; higher is always better.
        rank: Rank among all evaluated systems (1 = best).
        confidence_interval_95: 95 % confidence interval on the raw mean as
            ``(lower, upper)``, or *None* when the sample size is < 2.
    """
    system_name: str
    metric_name: str
    raw_mean: float
    raw_std: float
    normalized_score: float
    rank: int = 0
    confidence_interval_95: Optional[Tuple[float, float]] = None


@dataclass
class CompositeScore:
    """Weighted composite score across all metrics for one system.

    Attributes:
        system_name: Name of the evaluated system.
        metric_scores: Per-metric NormalizedScore objects keyed by metric name.
        composite_score: Weighted mean of all per-metric scores in [0, 100].
        rank: Composite rank among all evaluated systems (1 = best).
    """
    system_name: str
    metric_scores: Dict[str, NormalizedScore] = field(default_factory=dict)
    composite_score: float = 0.0
    rank: int = 0


class BenchmarkScorer:
    """Normalize raw benchmark measurements and compute composite scores.

    Supports multiple normalization strategies and weighted aggregation of
    heterogeneous metrics (latency, throughput, recall, …) into a single,
    comparable composite score per database system.

    Example::

        scorer = BenchmarkScorer(
            metrics=[
                MetricConfig("throughput_ops_s", MetricDirection.HIGHER_IS_BETTER, weight=2.0, unit="ops/s"),
                MetricConfig("p99_latency_ms",   MetricDirection.LOWER_IS_BETTER,  weight=1.0, unit="ms"),
            ]
        )
        scorer.add_results("SystemA", "throughput_ops_s", [10000, 10200, 9800])
        scorer.add_results("SystemA", "p99_latency_ms",   [12.3, 13.1, 11.9])
        scorer.add_results("SystemB", "throughput_ops_s", [8000, 8100, 7900])
        scorer.add_results("SystemB", "p99_latency_ms",   [9.5, 10.1, 9.8])

        scores = scorer.rank_systems()
        for cs in scores:
            print(f"{cs.rank}. {cs.system_name}: {cs.composite_score:.1f}")
    """

    def __init__(
        self,
        metrics: List[MetricConfig],
        normalization_method: NormalizationMethod = NormalizationMethod.MIN_MAX,
        baseline_system: Optional[str] = None,
    ) -> None:
        """Initialize the scorer.

        Args:
            metrics: List of MetricConfig objects defining each metric.
            normalization_method: Strategy for converting raw values to scores.
            baseline_system: Required when normalization_method is BASELINE;
                             specifies the reference system (score = 100).
        """
        if not metrics:
            raise ValueError("At least one MetricConfig must be provided")
        if normalization_method == NormalizationMethod.BASELINE and not baseline_system:
            raise ValueError("baseline_system must be set when using BASELINE normalization")

        self._metrics: Dict[str, MetricConfig] = {m.name: m for m in metrics}
        self._method = normalization_method
        self._baseline_system = baseline_system
        # raw_data[metric_name][system_name] = List[float]
        self._raw_data: Dict[str, Dict[str, List[float]]] = {
            m.name: {} for m in metrics
        }
        from .statistics import StatisticalAnalyzer
        self._stat_analyzer = StatisticalAnalyzer()

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def add_results(
        self,
        system_name: str,
        metric_name: str,
        data: List[float],
    ) -> None:
        """Record raw benchmark measurements for a system/metric pair.

        Args:
            system_name: Identifier for the evaluated system.
            metric_name: Must match a MetricConfig.name passed to __init__.
            data: Raw measurement samples (must be non-empty).

        Raises:
            KeyError: If metric_name is not registered.
            ValueError: If data is empty.
        """
        if metric_name not in self._metrics:
            raise KeyError(
                f"Unknown metric '{metric_name}'. "
                f"Registered metrics: {list(self._metrics)}"
            )
        if not data:
            raise ValueError("data must not be empty")

        self._raw_data[metric_name][system_name] = list(data)

    def normalize(self) -> Dict[str, Dict[str, NormalizedScore]]:
        """Compute per-metric normalized scores for all systems.

        Returns:
            Nested dict ``scores[metric_name][system_name] -> NormalizedScore``.
            Scores are in [0, 100]; higher is always better regardless of
            MetricDirection.

        Raises:
            ValueError: If no results have been added, or if BASELINE
                        normalization is requested but the baseline system
                        has no data for a metric.
        """
        scores: Dict[str, Dict[str, NormalizedScore]] = {}

        for metric_name, metric_cfg in self._metrics.items():
            system_data = self._raw_data[metric_name]
            if not system_data:
                continue

            raw_means: Dict[str, float] = {}
            raw_stds: Dict[str, float] = {}
            confidence_intervals: Dict[str, Optional[Tuple[float, float]]] = {}
            for system, samples in system_data.items():
                arr = np.array(samples, dtype=float)
                raw_means[system] = float(np.mean(arr))
                raw_stds[system] = float(np.std(arr, ddof=1)) if len(arr) > 1 else 0.0
                confidence_intervals[system] = (
                    self._stat_analyzer.confidence_interval(samples) if len(samples) >= 2 else None
                )

            normalized = self._apply_normalization(
                metric_name, metric_cfg, raw_means
            )

            # Assign ranks (1 = highest score)
            sorted_systems = sorted(
                normalized.keys(), key=lambda s: normalized[s], reverse=True
            )
            ranks = {sys: idx + 1 for idx, sys in enumerate(sorted_systems)}

            scores[metric_name] = {
                system: NormalizedScore(
                    system_name=system,
                    metric_name=metric_name,
                    raw_mean=raw_means[system],
                    raw_std=raw_stds[system],
                    normalized_score=normalized[system],
                    rank=ranks[system],
                    confidence_interval_95=confidence_intervals[system],
                )
                for system in normalized
            }

        return scores

    def compute_composite_scores(self) -> List[CompositeScore]:
        """Compute weighted composite scores for every system.

        The composite score is the weight-normalized arithmetic mean of all
        per-metric scores for which a system has data::

            composite = sum(w_i * s_i) / sum(w_i)

        Returns:
            List of CompositeScore objects, unsorted.
        """
        per_metric_scores = self.normalize()

        # Collect all systems that appear in any metric
        all_systems: set = set()
        for system_map in per_metric_scores.values():
            all_systems.update(system_map.keys())

        composite_scores: List[CompositeScore] = []
        for system in all_systems:
            cs = CompositeScore(system_name=system)
            total_weight = 0.0
            weighted_sum = 0.0

            for metric_name, metric_cfg in self._metrics.items():
                if metric_name not in per_metric_scores:
                    continue
                if system not in per_metric_scores[metric_name]:
                    continue

                ns = per_metric_scores[metric_name][system]
                cs.metric_scores[metric_name] = ns
                weighted_sum += metric_cfg.weight * ns.normalized_score
                total_weight += metric_cfg.weight

            cs.composite_score = weighted_sum / total_weight if total_weight > 0 else 0.0
            composite_scores.append(cs)

        return composite_scores

    def rank_systems(self) -> List[CompositeScore]:
        """Return systems sorted by composite score (best first).

        Returns:
            Sorted list of CompositeScore objects with rank field populated.
        """
        composite_scores = self.compute_composite_scores()
        composite_scores.sort(key=lambda cs: cs.composite_score, reverse=True)
        for idx, cs in enumerate(composite_scores):
            cs.rank = idx + 1
        return composite_scores

    def generate_summary(self) -> Dict:
        """Return a human-readable summary of all scores and rankings.

        Returns:
            Dictionary with keys ``"rankings"``, ``"metrics"``, and
            ``"normalization_method"``.  Each per-metric entry includes a
            ``"confidence_interval_95"`` key with the 95 % CI as
            ``[lower, upper]``, or *null* when fewer than two samples were
            provided.
        """
        ranked = self.rank_systems()
        return {
            "normalization_method": self._method.value,
            "metrics": [
                {
                    "name": cfg.name,
                    "direction": cfg.direction.value,
                    "weight": cfg.weight,
                    "unit": cfg.unit,
                }
                for cfg in self._metrics.values()
            ],
            "rankings": [
                {
                    "rank": cs.rank,
                    "system": cs.system_name,
                    "composite_score": round(cs.composite_score, 2),
                    "per_metric": {
                        metric: {
                            "raw_mean": round(ns.raw_mean, 4),
                            "raw_std": round(ns.raw_std, 4),
                            "normalized_score": round(ns.normalized_score, 2),
                            "metric_rank": ns.rank,
                            "confidence_interval_95": (
                                [round(ns.confidence_interval_95[0], 4),
                                 round(ns.confidence_interval_95[1], 4)]
                                if ns.confidence_interval_95 is not None
                                else None
                            ),
                        }
                        for metric, ns in cs.metric_scores.items()
                    },
                }
                for cs in ranked
            ],
        }

    # ------------------------------------------------------------------
    # Private helpers
    # ------------------------------------------------------------------

    def _apply_normalization(
        self,
        metric_name: str,
        metric_cfg: MetricConfig,
        raw_means: Dict[str, float],
    ) -> Dict[str, float]:
        """Dispatch to the configured normalization strategy.

        Returns:
            Dict mapping system_name -> score in [0, 100].
        """
        if self._method == NormalizationMethod.MIN_MAX:
            return self._normalize_min_max(metric_cfg, raw_means)
        if self._method == NormalizationMethod.BASELINE:
            return self._normalize_baseline(metric_cfg, raw_means)
        if self._method == NormalizationMethod.Z_SCORE:
            return self._normalize_z_score(metric_cfg, raw_means)
        raise ValueError(f"Unknown normalization method: {self._method}")  # pragma: no cover

    @staticmethod
    def _normalize_min_max(
        metric_cfg: MetricConfig,
        raw_means: Dict[str, float],
    ) -> Dict[str, float]:
        """Linear min-max normalization to [0, 100].

        For HIGHER_IS_BETTER: score = (v - min) / (max - min) * 100.
        For LOWER_IS_BETTER:  score = (max - v) / (max - min) * 100.
        When all values are identical the score is 100 for every system.
        """
        values = list(raw_means.values())
        v_min = min(values)
        v_max = max(values)
        spread = v_max - v_min

        scores: Dict[str, float] = {}
        for system, mean in raw_means.items():
            if spread == 0.0:
                scores[system] = 100.0
            elif metric_cfg.direction == MetricDirection.HIGHER_IS_BETTER:
                scores[system] = (mean - v_min) / spread * 100.0
            else:
                scores[system] = (v_max - mean) / spread * 100.0
        return scores

    def _normalize_baseline(
        self,
        metric_cfg: MetricConfig,
        raw_means: Dict[str, float],
    ) -> Dict[str, float]:
        """Normalize relative to a reference system (baseline = 100).

        For HIGHER_IS_BETTER: score = (v / baseline) * 100.
        For LOWER_IS_BETTER:  score = (baseline / v) * 100.
        """
        if self._baseline_system not in raw_means:
            raise ValueError(
                f"Baseline system '{self._baseline_system}' has no data "
                f"for metric '{metric_cfg.name}'"
            )
        baseline_val = raw_means[self._baseline_system]
        if baseline_val == 0.0:
            raise ValueError(
                f"Baseline system '{self._baseline_system}' has a mean of 0 "
                f"for metric '{metric_cfg.name}', cannot normalize"
            )

        scores: Dict[str, float] = {}
        for system, mean in raw_means.items():
            if metric_cfg.direction == MetricDirection.HIGHER_IS_BETTER:
                scores[system] = (mean / baseline_val) * 100.0
            else:
                scores[system] = (baseline_val / mean) * 100.0 if mean != 0 else 0.0
        return scores

    @staticmethod
    def _normalize_z_score(
        metric_cfg: MetricConfig,
        raw_means: Dict[str, float],
    ) -> Dict[str, float]:
        """Z-score normalization mapped to [0, 100].

        Z-scores are computed, then linearly scaled so that the full observed
        range maps to [0, 100].  Handles degenerate cases (std = 0, single
        system) by returning 100 for all systems.
        """
        values = np.array(list(raw_means.values()), dtype=float)
        systems = list(raw_means.keys())
        mu = float(np.mean(values))
        sigma = float(np.std(values, ddof=0))

        if sigma == 0.0:
            return {s: 100.0 for s in systems}

        z_scores = (values - mu) / sigma

        # For LOWER_IS_BETTER invert so that smaller raw values → higher z → higher score
        if metric_cfg.direction == MetricDirection.LOWER_IS_BETTER:
            z_scores = -z_scores

        # Map z range to [0, 100]
        z_min, z_max = float(np.min(z_scores)), float(np.max(z_scores))
        z_spread = z_max - z_min

        result: Dict[str, float] = {}
        for system, z in zip(systems, z_scores.tolist()):
            result[system] = (z - z_min) / z_spread * 100.0
        return result
