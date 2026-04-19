> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# CHIMERA Benchmark Framework

**CHIMERA** = Comprehensive Hybrid Inferencing & Multi-model Evaluation Resource Assessment  
Version: 1.0.0 · Standard: IEEE Std 2807-2022, ISO/IEC 14756:2015  
Native CI path: `external/chimera/run_ci_benchmarks.py`

---

## Overview

CHIMERA defines a **unified native benchmark path** for ThemisDB module comparison.
It provides four canonical workloads that exercise the core paradigms of ThemisDB
(relational, vector, document, graph) in a single, reproducible Python harness.

The results are exported in a schema compatible with the existing
traffic-light / coverage reporting logic in `tools/bench_coverage_report.py`
and `benchmarks/performance_regression_detector.py`.

---

## Native CI Benchmark Path

```
external/chimera/run_ci_benchmarks.py
    │
    ├── _WORKLOADS  (four WorkloadDefinition objects)
    │     ├── relational_sort      → proxies query/storage ordering (Module 4, 2)
    │     ├── vector_dot_product   → proxies vector index throughput (Module 10, 3)
    │     ├── document_lookup      → proxies document-store / cache read (Module 8, cache)
    │     └── graph_bfs            → proxies graph engine traversal  (Module 5)
    │
    ├── run_benchmarks(warmup, iterations) → harness report dict
    ├── build_output(report, repo_root)   → versioned JSON (same schema as baselines/)
    └── main(argv)                         → CLI entry point
```

### Workload → Target Module Mapping

| Workload ID         | Workload Family | Target Module(s)        | Ziel-ID       |
|---------------------|-----------------|-------------------------|---------------|
| `relational_sort`   | relational      | Query (M4), Storage (M2)| CHI-1         |
| `vector_dot_product`| vector          | Vector/Embedding (M10)  | CHI-2         |
| `document_lookup`   | document        | Cache (C-*), Index (M3) | CHI-3         |
| `graph_bfs`         | graph           | Graph (M5)              | CHI-4         |

All four workloads are registered in `benchmarks/benchmark_target_mapping.json`
under the `"chimera"` module block (entries CHI-1 … CHI-4).

---

## Result Schema

Output follows the same schema as `benchmarks/baselines/chimera/baseline.json`:

```json
{
  "version":   "1.5.0-dev",
  "branch":    "main",
  "commit":    "abc1234",
  "timestamp": "2026-03-01T00:00:00Z",
  "workloads": {
    "relational_sort": {
      "throughput_ops_per_sec": 42503.0,
      "mean_latency_ms":        0.024,
      "p95_latency_ms":         0.023,
      "p99_latency_ms":         0.034
    },
    "vector_dot_product": { ... },
    "document_lookup":    { ... },
    "graph_bfs":          { ... }
  }
}
```

This schema is compatible with:

- `tools/bench_coverage_report.py` — traffic-light module coverage
- `benchmarks/performance_regression_detector.py` — regression detection
- `benchmarks/cross_module_regression_detector.py` — cross-module aggregation

---

## Benchmark Methodology

### Sampling Strategy

| Parameter          | Default | Rationale                                    |
|--------------------|---------|----------------------------------------------|
| Warm-up iterations | 3       | Stabilise JIT, page cache, branch predictor  |
| Run iterations     | 100     | ≥ 30 samples required for CLT applicability  |
| Percentiles        | 50, 95, 99 | Standard latency SLO markers              |

### Variance Treatment

- Outlier removal: none (raw measurements preserved; IQR-based removal is
  performed post-hoc by `StatisticalAnalyzer` when comparing baseline vs current).
- Reported statistics: arithmetic mean, P95, P99 (nearest-rank method).
- Regression threshold: ≥ 10 % increase in mean CPU time → 🔴 (critical);
  5–10 % → 🟡 (warning). See `bench_coverage_report.py:_traffic_light`.

### Statistical Significance

Welch's two-sample t-test (`StatisticalAnalyzer.t_test`) is used when comparing
two runs (e.g. baseline vs PR). Default α = 0.05.

The degrees of freedom are computed via the Welch–Satterthwaite equation.
The p-value is approximated via the regularised incomplete beta function
(Lentz continued-fraction method, ε = 1 × 10⁻¹²).

### Platform Requirements

| Requirement           | Minimum              |
|-----------------------|----------------------|
| Python                | 3.10+                |
| CPU                   | Any x86-64 / arm64   |
| RAM                   | 64 MB                |
| Dependencies          | stdlib only          |
| Reproducibility gate  | All workloads deterministic (no GPU, no network) |

---

## Running the Benchmarks

```bash
# Quick run (CI defaults)
python external/chimera/run_ci_benchmarks.py

# Custom output path
python external/chimera/run_ci_benchmarks.py --output results/my_run.json

# Faster (reduced iterations)
python external/chimera/run_ci_benchmarks.py --warmup 1 --iterations 20
```

Output is written to `benchmark_results/chimera_results.json` by default.

---

## Comparing Against Baseline

```python
from benchmarks.performance_regression_detector import RegressionDetector

detector = RegressionDetector(
    baseline_path="benchmarks/baselines/chimera/baseline.json",
    current_path="benchmark_results/chimera_results.json",
)
report = detector.compare()
```

---

## File Map

```
benchmarks/chimera/
├── CHIMERA_README.md              ← This file (methodology documentation)
├── benchmark_config_schema.yaml   ← Canonical workload configuration schema
└── demo_reports/
    └── benchmark_comparison.csv   ← Anonymised vendor-neutral demo results

external/chimera/
└── run_ci_benchmarks.py           ← Native CI benchmark runner (entry point)

benchmarks/
└── chimera.py                     ← BenchmarkHarness / StatisticalAnalyzer library

benchmarks/baselines/chimera/
└── baseline.json                  ← v1.5.0-dev reference baseline (2026-03-01)
```
