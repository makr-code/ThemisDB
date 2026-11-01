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

# Toolbox benchmark targets

`benchmarks/toolbox/CMakeLists.txt` registers three targets when `THEMIS_BUILD_BENCHMARKS` is enabled:

| Target | Source | What it measures |
|---|---|---|
| `bench_toolbox_native_workloads` | `bench_toolbox_native_workloads.cpp` | P1-P5 extraction/helper/fingerprint measurements, bridge proxy, empty extraction, and metrics text generation. |
| `bench_toolbox_release_gates` | `bench_toolbox_release_gates.cpp` | Fingerprint default initialization/memset and `ToolboxError` cast/switch costs only. It does not validate TBXG criteria. |
| `bench_toolbox_dedicated_gates` | `bench_toolbox_dedicated_gates.cpp` | Four in-process stub/simulation benchmarks, TB-BM-01..04; not production-path measurements. |

## Native workload symbols

- `BM_ExtractEntities_Throughput` (P1)
- `BM_ExtractEntitySet_Latency` (P2)
- `BM_TextNormalization_Latency` (P3)
- `BM_LanguageDetection_Latency` (P4)
- `BM_ContentFingerprinting_Throughput` (P5)
- `BM_BridgeProxy_EntitySetLatency` — calls `IngestionToolbox::extractEntitySet`; not a real `ContentToolboxBridge`, and explicitly excluded from gate enforcement in source
- `BM_EmptyExtraction_Throughput`
- `BM_MetricsGeneration`

There is no `BM_BridgeEnrichment_Placeholder` symbol. P1-P5 labels and comments do not implement threshold enforcement or establish a certified gate. The dedicated gate source explicitly measures stubs/simulations, and the release-gates source does not compute baselines, percentiles, or TBXG pass/fail results.

## Baseline and certification caveat

Previously documented numerical baselines, reference-hardware results, and certification claims are retained only as historical/unverified material in older module records. This repository view contains no run artifacts supporting those figures. They are not current measurements, acceptance criteria, or evidence that a release gate passed.

## Run a benchmark target

```bash
cmake --preset linux-release
cmake --build --preset linux-release --target bench_toolbox_native_workloads
./build/linux-release/benchmarks/toolbox/bench_toolbox_native_workloads
```

Use raw output and record revision, build settings, machine, invocation, and repetitions before making a performance claim. The commands above are examples; this update did not build or run benchmarks.

## Source references

- [`CMakeLists.txt`](./CMakeLists.txt)
- [`src/toolbox/PERFORMANCE_EXPECTATIONS.md`](../../src/toolbox/PERFORMANCE_EXPECTATIONS.md)
- [`tests/toolbox/README.md`](../../tests/toolbox/README.md)

> Exact model backend not verifiable from final document text alone.
