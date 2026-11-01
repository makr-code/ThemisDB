# EPIC 2.5 Evaluation Module — Baseline Capture Template

> **Update provenance (2026-10-31):** AI_Used: true; AI_Models: unknown;
> AI_Contributor: OpenAI ChatGPT; Human_Review: false; Human_Author: ThemisDB
> Contributors. Exact model backend not verifiable from final document text alone.

> **No measurements are recorded in this file.** The blank tables and numeric
> thresholds below are planning/template material, not captured baselines or
> validated regression gates. Example configure/build paths must be checked
> against the active preset and target registration before use. No benchmark was
> run for issue #7344.

Use this page to record measured benchmark results only after retaining the
complete output and hardware/build context.

> **Note**: This file is the Phase 6 deliverable referenced by
> the historical `benchmarks/epic2_evaluation/README.md §7.1` plan. Update it after every qualifying
> baseline capture run.

The EPIC 2 CMake file declares planner decision and error-path executables when
`epic2_evaluation_lib` exists, and the matrix executable when Google Benchmark
and `themis_evaluation` exist. It passes the storage-strategy source through the
standard benchmark helper and registers its CTest wrapper if the target exists.
The artifact-staleness source exercises `ArtifactLifecycleManager` through
Google Benchmark but is not registered there. Planner executables report
measured per-call percentiles when run; matrix simulations are synthetic, and
the storage harness calls `DefaultShardPlacementStrategy` with synthetic node
inputs. This template records no results. See
`benchmarks/epic2_evaluation/CMakeLists.txt` and the benchmark sources before
interpreting any output.

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

Reference: `src/evaluation/PERFORMANCE_EXPECTATIONS.md §6`. The values above
remain proposed until measured and reviewed; they are not current performance
guarantees.
