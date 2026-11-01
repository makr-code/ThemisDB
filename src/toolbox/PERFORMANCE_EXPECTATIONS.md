---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Toolbox benchmark inventory and evidence limits

**Scope:** This page records what the current benchmark sources measure. It does not declare release thresholds or certify performance.

## Registered targets

[`../../benchmarks/toolbox/CMakeLists.txt`](../../benchmarks/toolbox/CMakeLists.txt) registers three targets when `THEMIS_BUILD_BENCHMARKS` is enabled:

| Target | Source | Current scope |
|---|---|---|
| `bench_toolbox_native_workloads` | `bench_toolbox_native_workloads.cpp` | P1-P5 operation measurements, one toolbox-only bridge proxy, empty extraction, and metrics text generation. |
| `bench_toolbox_release_gates` | `bench_toolbox_release_gates.cpp` | Fingerprint default initialization/memset and `ToolboxError` cast/switch microbenchmarks. It does not implement TBXG baseline, percentile, or manifest validation. |
| `bench_toolbox_dedicated_gates` | `bench_toolbox_dedicated_gates.cpp` | Four explicitly simulated stub cases (TB-BM-01..04); not production routing, bridge, or extraction measurements. |

## Native workloads

The native source measures:

- `BM_ExtractEntities_Throughput` (P1) and `BM_ExtractEntitySet_Latency` (P2)
- `BM_TextNormalization_Latency` (P3), `BM_LanguageDetection_Latency` (P4), and `BM_ContentFingerprinting_Throughput` (P5)
- `BM_BridgeProxy_EntitySetLatency`, which calls `extractEntitySet()` only; it is not a `ContentToolboxBridge` benchmark and the source excludes it from gate enforcement
- `BM_EmptyExtraction_Throughput` and `BM_MetricsGeneration`

The source contains no `BM_BridgeEnrichment_Placeholder` symbol. P1-P5 labels/comments identify measured workloads; the source does not enforce the historical threshold table or certify a release gate. The release-gates target likewise does not implement the historical TBXG validation logic.

## Baselines and result interpretation

Numeric baseline, percentile, certification, and pass/fail claims in older toolbox documentation have no corresponding run artifacts available in this documentation update. Treat those figures as unverified historical claims, not current measurements or acceptance criteria. A benchmark source registration or successful build would not establish a performance baseline.

For a new performance claim, record the exact revision, build/compiler settings, machine, invocation, raw output, repetitions, and analysis method. Do not compare stub/simulation results with production paths.

## Running a target

```bash
cmake --preset linux-release
cmake --build --preset linux-release --target bench_toolbox_native_workloads
./build/linux-release/benchmarks/toolbox/bench_toolbox_native_workloads
```

Use `bench_toolbox_release_gates` or `bench_toolbox_dedicated_gates` only with the limitations above understood. These commands are usage guidance; no build or benchmark run is asserted by this document.

> Exact model backend not verifiable from final document text alone.
