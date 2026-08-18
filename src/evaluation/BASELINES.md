# EPIC 2.5 Evaluation Module — Performance Baselines

This file stores the captured benchmark baselines for the evaluation module path-selection
decision and error-path benchmarks. Baselines are captured on representative hardware before
each GA promotion and used as the comparison target for regression detection.

> **Note**: This file is the Phase 6 deliverable referenced by
> `benchmarks/epic2_evaluation/README.md §7.1`. Update it after every qualifying
> baseline capture run.

---

## Baseline Hardware

| Field        | Value                                    |
|--------------|------------------------------------------|
| CPU          | (record at capture time, e.g. Intel Core i7-12700K @ 3.60 GHz) |
| RAM          | (record, e.g. 32 GB DDR5-4800)          |
| OS           | (record, e.g. Ubuntu 22.04 LTS x86_64)  |
| Kernel       | (record, e.g. 5.15.0-102-generic)       |
| Capture date | (YYYY-MM-DD)                             |
| Build preset | linux-release                            |

---

## planner_decision_bench Baselines

Capture with:

```bash
cmake --preset linux-release -DTHEMIS_BUILD_EPIC2=ON -DTHEMIS_BUILD_BENCHMARKS=ON
cmake --build --preset linux-release --target planner_decision_bench
./build/linux-release/benchmarks/epic2_evaluation/planner_decision_bench
```

| Path                        | avg (ns) | p50 (ns) | p95 (ns) | p99 (ns) | min (ns) | max (ns) |
|-----------------------------|----------|----------|----------|----------|----------|----------|
| Path 1 — ANN Only           | —        | —        | —        | —        | —        | —        |
| Path 2 — ANN + Tensor       | —        | —        | —        | —        | —        | —        |
| Path 4 — Stale Tensor       | —        | —        | —        | —        | —        | —        |
| Path 4 — force_exact        | —        | —        | —        | —        | —        | —        |
| Path 4 — ModuleGapThreshold | —        | —        | —        | —        | —        | —        |
| Path 5 — Distributed        | —        | —        | —        | —        | —        | —        |

---

## planner_error_path_bench Baselines

Capture with:

```bash
cmake --build --preset linux-release --target planner_error_path_bench
./build/linux-release/benchmarks/epic2_evaluation/planner_error_path_bench
```

| Scenario                                | avg (ns) | p50 (ns) | p95 (ns) | p99 (ns) | overhead vs nominal |
|-----------------------------------------|----------|----------|----------|----------|---------------------|
| Nominal (all systems OK)                | —        | —        | —        | —        | 0 %                 |
| Error 1 — exception_handling_disabled   | —        | —        | —        | —        | —                   |
| Error 2 — index_buffer_safety_failed    | —        | —        | —        | —        | —                   |
| Error 3 — thread_safety_failed          | —        | —        | —        | —        | —                   |
| Error 4 — compound error                | —        | —        | —        | —        | —                   |

---

## Regression Thresholds

| Metric        | Warning gate        | Block-promotion gate |
|---------------|---------------------|----------------------|
| p95 latency   | > baseline × 1.20   | > 800,000 ns (800 µs)|
| p99 latency   | > baseline × 1.30   | > 1,000,000 ns (1 ms)|
| Any path Δ    | > 50%               | requires root-cause  |

Reference: `src/evaluation/PERFORMANCE_EXPECTATIONS.md §6`, `benchmarks/epic2_evaluation/README.md §7`.
