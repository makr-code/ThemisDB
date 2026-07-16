#!/usr/bin/env python3
"""Scientific evaluation framework for reproducible benchmark experiments.

Implements:
- hypothesis-driven experiment validation
- deterministic seeded execution checks
- baseline freeze validation
- bootstrap confidence intervals and effect sizes (Cohen's d, Cliff's Delta)
- p-value based decision support + practical significance
- classification (regressiv / neutral / signifikant_positiv)
- governance gates, subsystem budgets, and auto regression ticket payloads
"""

from __future__ import annotations

import argparse
import json
import math
import random
import statistics
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, List, Optional, Sequence

_REQUIRED_WORKLOAD_FAMILIES = {"oltp", "olap", "graph", "vector", "rag", "hybrid"}
_PERMUTATION_SEED_OFFSET = 11


@dataclass(frozen=True)
class BaselineFreeze:
    compiler: str
    compiler_flags: str
    preset: str
    hardware_profile: str
    os_image: str

    @staticmethod
    def from_dict(data: Dict[str, Any]) -> "BaselineFreeze":
        return BaselineFreeze(
            compiler=str(data.get("compiler", "")).strip(),
            compiler_flags=str(data.get("compiler_flags", "")).strip(),
            preset=str(data.get("preset", "")).strip(),
            hardware_profile=str(data.get("hardware_profile", "")).strip(),
            os_image=str(data.get("os_image", "")).strip(),
        )

    def validate(self) -> None:
        missing = [
            name
            for name, value in (
                ("compiler", self.compiler),
                ("compiler_flags", self.compiler_flags),
                ("preset", self.preset),
                ("hardware_profile", self.hardware_profile),
                ("os_image", self.os_image),
            )
            if not value
        ]
        if missing:
            raise ValueError(f"Baseline-Freeze unvollständig: {', '.join(missing)}")


@dataclass(frozen=True)
class Hypothesis:
    h0: str
    h1: str
    expected_effect_direction: str
    risks: List[str]
    stop_criteria: List[str]

    @staticmethod
    def from_dict(data: Dict[str, Any]) -> "Hypothesis":
        return Hypothesis(
            h0=str(data.get("h0", "")).strip(),
            h1=str(data.get("h1", "")).strip(),
            expected_effect_direction=str(data.get("expected_effect_direction", "")).strip().lower(),
            risks=[str(x).strip() for x in data.get("risks", []) if str(x).strip()],
            stop_criteria=[str(x).strip() for x in data.get("stop_criteria", []) if str(x).strip()],
        )

    def validate(self) -> None:
        if not self.h0 or not self.h1:
            raise ValueError("Hypothese muss H0 und H1 enthalten")
        if self.expected_effect_direction not in {"improve", "regress", "neutral"}:
            raise ValueError("expected_effect_direction muss improve|regress|neutral sein")
        if not self.risks:
            raise ValueError("Hypothese muss Risiken enthalten")
        if not self.stop_criteria:
            raise ValueError("Hypothese muss Stop-Kriterien enthalten")


@dataclass(frozen=True)
class Scenario:
    workload_family: str
    dataset_size: str
    query_mix: str
    concurrency_profile: str
    warmup_runs: int
    cache_mode: str
    numa_mode: str
    io_profile: str
    gpu_allocation: str

    @staticmethod
    def from_dict(data: Dict[str, Any]) -> "Scenario":
        return Scenario(
            workload_family=str(data.get("workload_family", "")).strip().lower(),
            dataset_size=str(data.get("dataset_size", "")).strip(),
            query_mix=str(data.get("query_mix", "")).strip(),
            concurrency_profile=str(data.get("concurrency_profile", "")).strip(),
            warmup_runs=int(data.get("warmup_runs", 0)),
            cache_mode=str(data.get("cache_mode", "")).strip(),
            numa_mode=str(data.get("numa_mode", "")).strip(),
            io_profile=str(data.get("io_profile", "")).strip(),
            gpu_allocation=str(data.get("gpu_allocation", "")).strip(),
        )

    def validate(self) -> None:
        if self.workload_family not in _REQUIRED_WORKLOAD_FAMILIES:
            raise ValueError(f"Unbekannte workload_family: {self.workload_family}")
        for field_name in (
            "dataset_size",
            "query_mix",
            "concurrency_profile",
            "cache_mode",
            "numa_mode",
            "io_profile",
            "gpu_allocation",
        ):
            if not getattr(self, field_name):
                raise ValueError(f"Szenariofeld fehlt: {field_name}")
        if self.warmup_runs < 0:
            raise ValueError("warmup_runs darf nicht negativ sein")


@dataclass(frozen=True)
class MetricConfig:
    name: str
    higher_is_better: bool
    practical_significance_percent: float
    critical: bool

    @staticmethod
    def from_dict(data: Dict[str, Any]) -> "MetricConfig":
        return MetricConfig(
            name=str(data.get("name", "")).strip(),
            higher_is_better=bool(data.get("higher_is_better", False)),
            practical_significance_percent=float(data.get("practical_significance_percent", 0.0)),
            critical=bool(data.get("critical", False)),
        )

    def validate(self) -> None:
        if not self.name:
            raise ValueError("metric.name fehlt")
        if self.practical_significance_percent < 0:
            raise ValueError("practical_significance_percent muss >= 0 sein")


def _validate_samples(samples: Sequence[float], label: str) -> List[float]:
    values = [float(x) for x in samples]
    if len(values) < 30:
        raise ValueError(f"{label}: mindestens 30 Samples erforderlich, erhalten={len(values)}")
    if any(math.isnan(x) or math.isinf(x) for x in values):
        raise ValueError(f"{label}: Samples enthalten NaN/Inf")
    return values


def _percentile(sorted_values: Sequence[float], p: float) -> float:
    if not sorted_values:
        return 0.0
    if len(sorted_values) == 1:
        return float(sorted_values[0])
    idx = (p / 100.0) * (len(sorted_values) - 1)
    lo = int(math.floor(idx))
    hi = int(math.ceil(idx))
    if lo == hi:
        return float(sorted_values[lo])
    frac = idx - lo
    return float(sorted_values[lo] + (sorted_values[hi] - sorted_values[lo]) * frac)


def _sample_percentiles(values: Sequence[float]) -> Dict[str, float]:
    sorted_values = sorted(values)
    return {
        "p50": _percentile(sorted_values, 50.0),
        "p95": _percentile(sorted_values, 95.0),
        "p99": _percentile(sorted_values, 99.0),
    }


def _bootstrap_ci_mean_diff(
    baseline: Sequence[float],
    treatment: Sequence[float],
    *,
    confidence: float,
    iterations: int,
    seed: int,
) -> Dict[str, float]:
    rng = random.Random(seed)
    n_b = len(baseline)
    n_t = len(treatment)
    diffs: List[float] = []
    for _ in range(iterations):
        bs_b = [baseline[rng.randrange(n_b)] for _ in range(n_b)]
        bs_t = [treatment[rng.randrange(n_t)] for _ in range(n_t)]
        diffs.append(statistics.mean(bs_t) - statistics.mean(bs_b))
    diffs.sort()
    alpha = (1.0 - confidence) / 2.0
    return {
        "lower": _percentile(diffs, alpha * 100.0),
        "upper": _percentile(diffs, (1.0 - alpha) * 100.0),
    }


def _cohens_d(baseline: Sequence[float], treatment: Sequence[float]) -> float:
    if len(baseline) < 2 or len(treatment) < 2:
        return 0.0
    mean_b = statistics.mean(baseline)
    mean_t = statistics.mean(treatment)
    var_b = statistics.variance(baseline)
    var_t = statistics.variance(treatment)
    pooled = math.sqrt((var_b + var_t) / 2.0)
    if pooled == 0:
        return 0.0
    return (mean_t - mean_b) / pooled


def _cliffs_delta(baseline: Sequence[float], treatment: Sequence[float]) -> float:
    wins = 0
    losses = 0
    for t in treatment:
        for b in baseline:
            if t > b:
                wins += 1
            elif t < b:
                losses += 1
    total = len(baseline) * len(treatment)
    if total == 0:
        return 0.0
    return (wins - losses) / total


def _permutation_p_value(
    baseline: Sequence[float],
    treatment: Sequence[float],
    *,
    iterations: int,
    seed: int,
) -> float:
    observed = statistics.mean(treatment) - statistics.mean(baseline)
    base_pooled = list(baseline) + list(treatment)
    n_b = len(baseline)
    rng = random.Random(seed)
    extreme = 0
    for _ in range(iterations):
        pooled = base_pooled[:]
        rng.shuffle(pooled)
        b = pooled[:n_b]
        t = pooled[n_b:]
        diff = statistics.mean(t) - statistics.mean(b)
        if abs(diff) >= abs(observed):
            extreme += 1
    return (extreme + 1) / (iterations + 1)


def _effect_interpretation(cohens_d: float) -> str:
    value = abs(cohens_d)
    if value < 0.2:
        return "negligible"
    if value < 0.5:
        return "small"
    if value < 0.8:
        return "medium"
    return "large"


def _relative_improvement_percent(
    baseline_mean: float,
    treatment_mean: float,
    *,
    higher_is_better: bool,
) -> float:
    if baseline_mean == 0:
        return 0.0
    if higher_is_better:
        return ((treatment_mean - baseline_mean) / baseline_mean) * 100.0
    return ((baseline_mean - treatment_mean) / baseline_mean) * 100.0


def _analyze_directional_drift_percent(
    window_samples: Sequence[float],
    *,
    higher_is_better: bool,
) -> Optional[Dict[str, Any]]:
    if not window_samples:
        return None

    values = [float(x) for x in window_samples]
    if any(math.isnan(x) or math.isinf(x) for x in values):
        raise ValueError("time_window_samples enthält NaN/Inf")
    if len(values) < 2:
        raise ValueError("time_window_samples benötigt mindestens 2 Werte")

    first = values[0]
    last = values[-1]
    if first == 0:
        directional_drift = 0.0
    elif higher_is_better:
        directional_drift = ((last - first) / first) * 100.0
    else:
        directional_drift = ((first - last) / first) * 100.0

    regression_drift = max(0.0, -directional_drift)
    return {
        "window_count": len(values),
        "first_window_value": first,
        "last_window_value": last,
        "directional_drift_percent": directional_drift,
        "regression_drift_percent": regression_drift,
        "degrading_over_time": directional_drift < 0.0,
    }


def _analyze_saturation_points(samples: Sequence[Dict[str, Any]]) -> Optional[Dict[str, Any]]:
    if not samples:
        return None
    if len(samples) < 2:
        raise ValueError("saturation_samples benötigt mindestens 2 Lastpunkte")

    points: List[Dict[str, float]] = []
    for item in samples:
        load = float(item.get("load", 0.0))
        throughput = float(item.get("throughput", 0.0))
        if math.isnan(load) or math.isinf(load) or math.isnan(throughput) or math.isinf(throughput):
            raise ValueError("saturation_samples enthält NaN/Inf")
        points.append({"load": load, "throughput": throughput})

    points.sort(key=lambda x: x["load"])
    peak = max(points, key=lambda x: x["throughput"])
    knee_load = peak["load"]

    for idx in range(1, len(points)):
        prev_tput = points[idx - 1]["throughput"]
        curr_tput = points[idx]["throughput"]
        if prev_tput > 0 and curr_tput < prev_tput:
            knee_load = points[idx]["load"]
            break

    return {
        "peak_throughput": peak["throughput"],
        "load_at_peak_throughput": peak["load"],
        "estimated_saturation_load": knee_load,
        "sample_count": len(points),
    }


def _recovery_percentiles(samples: Sequence[float]) -> Optional[Dict[str, float]]:
    if not samples:
        return None
    values = [float(x) for x in samples]
    if any(math.isnan(x) or math.isinf(x) for x in values):
        raise ValueError("recovery_time_seconds_samples enthält NaN/Inf")
    return _sample_percentiles(values)


def _evaluate_wave6_guardrails(
    guardrails: Dict[str, Any],
    *,
    treatment_mean: float,
    treatment_percentiles: Dict[str, float],
    drift_analysis: Optional[Dict[str, Any]],
    recovery_analysis: Optional[Dict[str, float]],
) -> List[str]:
    failures: List[str] = []

    max_p99_latency = guardrails.get("max_p99_latency_ms")
    if max_p99_latency is not None and treatment_percentiles["p99"] > float(max_p99_latency):
        failures.append("max_p99_latency_ms_exceeded")

    min_mean_threshold = guardrails.get("min_treatment_mean")
    if min_mean_threshold is not None and treatment_mean < float(min_mean_threshold):
        failures.append("min_treatment_mean_not_met")

    max_regression_drift = guardrails.get("max_regression_drift_percent")
    if (
        max_regression_drift is not None
        and drift_analysis is not None
        and drift_analysis["regression_drift_percent"] > float(max_regression_drift)
    ):
        failures.append("max_regression_drift_percent_exceeded")

    max_recovery_time_p95 = guardrails.get("max_recovery_time_seconds_p95")
    if (
        max_recovery_time_p95 is not None
        and recovery_analysis is not None
        and recovery_analysis["p95"] > float(max_recovery_time_p95)
    ):
        failures.append("max_recovery_time_seconds_p95_exceeded")

    return failures


def evaluate_experiment(
    experiment: Dict[str, Any],
    *,
    global_seed: int,
    confidence_level: float,
    alpha: float,
    bootstrap_iterations: int,
    permutation_iterations: int,
) -> Dict[str, Any]:
    hypothesis = Hypothesis.from_dict(experiment.get("hypothesis", {}))
    hypothesis.validate()

    scenario = Scenario.from_dict(experiment.get("scenario", {}))
    scenario.validate()

    metric = MetricConfig.from_dict(experiment.get("metric", {}))
    metric.validate()

    baseline = _validate_samples(experiment.get("baseline_samples", []), "baseline_samples")
    treatment = _validate_samples(experiment.get("treatment_samples", []), "treatment_samples")

    baseline_mean = statistics.mean(baseline)
    treatment_mean = statistics.mean(treatment)
    delta = treatment_mean - baseline_mean
    baseline_percentiles = _sample_percentiles(baseline)
    treatment_percentiles = _sample_percentiles(treatment)

    ci = _bootstrap_ci_mean_diff(
        baseline,
        treatment,
        confidence=confidence_level,
        iterations=bootstrap_iterations,
        seed=global_seed,
    )
    cohens_d = _cohens_d(baseline, treatment)
    cliffs_delta = _cliffs_delta(baseline, treatment)
    p_value = _permutation_p_value(
        baseline,
        treatment,
        iterations=permutation_iterations,
        seed=global_seed + _PERMUTATION_SEED_OFFSET,
    )

    improvement_pct = _relative_improvement_percent(
        baseline_mean,
        treatment_mean,
        higher_is_better=metric.higher_is_better,
    )
    practical = abs(improvement_pct) >= metric.practical_significance_percent

    significant = p_value < alpha
    if significant and practical and improvement_pct > 0:
        classification = "signifikant_positiv"
    elif significant and practical and improvement_pct < 0:
        classification = "regressiv"
    else:
        classification = "neutral"

    drift_analysis = _analyze_directional_drift_percent(
        experiment.get("time_window_samples", []),
        higher_is_better=metric.higher_is_better,
    )
    saturation_analysis = _analyze_saturation_points(experiment.get("saturation_samples", []))
    recovery_analysis = _recovery_percentiles(experiment.get("recovery_time_seconds_samples", []))

    wave6_guardrails = experiment.get("wave6_guardrails", {})
    if not isinstance(wave6_guardrails, dict):
        raise ValueError("wave6_guardrails muss ein Objekt sein")
    wave6_gate_failures = _evaluate_wave6_guardrails(
        wave6_guardrails,
        treatment_mean=treatment_mean,
        treatment_percentiles=treatment_percentiles,
        drift_analysis=drift_analysis,
        recovery_analysis=recovery_analysis,
    )

    budget_pct = float(experiment.get("performance_budget_percent", metric.practical_significance_percent))
    gate_violation = (metric.critical and (-improvement_pct) > budget_pct) or bool(wave6_gate_failures)

    return {
        "experiment_id": str(experiment.get("id", "")).strip() or "unnamed-experiment",
        "subsystem": str(experiment.get("subsystem", "unknown")).strip(),
        "wave6_track": str(experiment.get("wave6_track", "")).strip() or "unspecified",
        "metric": metric.name,
        "classification": classification,
        "gate_violation": gate_violation,
        "statistics": {
            "sample_count_baseline": len(baseline),
            "sample_count_treatment": len(treatment),
            "baseline_mean": baseline_mean,
            "treatment_mean": treatment_mean,
            "baseline_percentiles": baseline_percentiles,
            "treatment_percentiles": treatment_percentiles,
            "delta": delta,
            "bootstrap_ci": ci,
            "p_value": p_value,
            "cohens_d": cohens_d,
            "cohens_d_effect": _effect_interpretation(cohens_d),
            "cliffs_delta": cliffs_delta,
            "improvement_percent": improvement_pct,
            "practical_significance_reached": practical,
            "alpha": alpha,
            "confidence_level": confidence_level,
        },
        "governance": {
            "performance_budget_percent": budget_pct,
            "critical_metric": metric.critical,
            "wave6_gate_failures": wave6_gate_failures,
        },
        "wave6_analysis": {
            "drift": drift_analysis,
            "saturation": saturation_analysis,
            "recovery_time_seconds": recovery_analysis,
        },
        "scenario": {
            "workload_family": scenario.workload_family,
            "dataset_size": scenario.dataset_size,
            "query_mix": scenario.query_mix,
            "concurrency_profile": scenario.concurrency_profile,
            "warmup_runs": scenario.warmup_runs,
            "interference_factors": {
                "cache_mode": scenario.cache_mode,
                "numa_mode": scenario.numa_mode,
                "io_profile": scenario.io_profile,
                "gpu_allocation": scenario.gpu_allocation,
            },
        },
        "hypothesis": {
            "h0": hypothesis.h0,
            "h1": hypothesis.h1,
            "expected_effect_direction": hypothesis.expected_effect_direction,
            "risks": hypothesis.risks,
            "stop_criteria": hypothesis.stop_criteria,
        },
    }


def build_regression_ticket(result: Dict[str, Any], defaults: Dict[str, Any]) -> Dict[str, Any]:
    stats = result["statistics"]
    return {
        "title": f"[Perf Regression] {result['subsystem']}::{result['experiment_id']}::{result['metric']}",
        "project": defaults.get("project", "PERF"),
        "owner": defaults.get("owner", "performance-team"),
        "severity": "high" if result.get("gate_violation") else "medium",
        "classification": result.get("classification"),
        "improvement_percent": stats.get("improvement_percent"),
        "p_value": stats.get("p_value"),
        "cohens_d": stats.get("cohens_d"),
        "cliffs_delta": stats.get("cliffs_delta"),
        "recommended_action": "Investigate regression and rerun seeded experiment with frozen baseline.",
    }


def run_pipeline(payload: Dict[str, Any]) -> Dict[str, Any]:
    baseline_freeze = BaselineFreeze.from_dict(payload.get("baseline_freeze", {}))
    baseline_freeze.validate()

    confidence_level = float(payload.get("confidence_level", 0.95))
    alpha = float(payload.get("alpha", 0.05))
    bootstrap_iterations = int(payload.get("bootstrap_iterations", 2000))
    permutation_iterations = int(payload.get("permutation_iterations", 2000))
    seed = int(payload.get("seed", 42))

    if not (0 < confidence_level < 1):
        raise ValueError("confidence_level muss zwischen 0 und 1 liegen")
    if not (0 < alpha < 1):
        raise ValueError("alpha muss zwischen 0 und 1 liegen")

    experiments = payload.get("experiments", [])
    if not experiments:
        raise ValueError("Keine Experimente angegeben")

    results: List[Dict[str, Any]] = []
    tickets: List[Dict[str, Any]] = []
    ticket_defaults = payload.get("ticket_defaults", {})

    for index, experiment in enumerate(experiments):
        result = evaluate_experiment(
            experiment,
            global_seed=seed + index,
            confidence_level=confidence_level,
            alpha=alpha,
            bootstrap_iterations=bootstrap_iterations,
            permutation_iterations=permutation_iterations,
        )
        results.append(result)
        if result["gate_violation"]:
            tickets.append(build_regression_ticket(result, ticket_defaults))

    summary = {
        "total_experiments": len(results),
        "regressiv": sum(1 for r in results if r["classification"] == "regressiv"),
        "neutral": sum(1 for r in results if r["classification"] == "neutral"),
        "signifikant_positiv": sum(1 for r in results if r["classification"] == "signifikant_positiv"),
        "gate_violations": len(tickets),
        "wave6_gate_violations": sum(1 for r in results if r.get("governance", {}).get("wave6_gate_failures")),
        "wave6_tracks": sorted({r.get("wave6_track", "unspecified") for r in results}),
        "required_workload_families": sorted(_REQUIRED_WORKLOAD_FAMILIES),
        "baseline_freeze": {
            "compiler": baseline_freeze.compiler,
            "compiler_flags": baseline_freeze.compiler_flags,
            "preset": baseline_freeze.preset,
            "hardware_profile": baseline_freeze.hardware_profile,
            "os_image": baseline_freeze.os_image,
        },
    }

    return {
        "generated_at": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "seed": seed,
        "summary": summary,
        "results": results,
        "regression_tickets": tickets,
    }


def _parse_args(argv: Optional[Sequence[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run scientific evaluation pipeline for benchmark experiments.")
    parser.add_argument("--input", required=True, help="Input JSON with baseline freeze + experiment definitions")
    parser.add_argument("--output", required=True, help="Output JSON report path")
    parser.add_argument("--tickets-output", help="Optional output path for generated regression tickets JSON")
    return parser.parse_args(argv)


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = _parse_args(argv)
    input_path = Path(args.input)
    output_path = Path(args.output)

    payload = json.loads(input_path.read_text(encoding="utf-8"))
    report = run_pipeline(payload)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")

    if args.tickets_output:
        tickets_path = Path(args.tickets_output)
        tickets_path.parent.mkdir(parents=True, exist_ok=True)
        tickets_path.write_text(
            json.dumps(report.get("regression_tickets", []), indent=2, ensure_ascii=False),
            encoding="utf-8",
        )

    print(
        f"Scientific evaluation complete: experiments={report['summary']['total_experiments']} "
        f"gate_violations={report['summary']['gate_violations']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
