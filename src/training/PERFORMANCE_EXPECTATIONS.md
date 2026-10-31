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

Exact model backend not verifiable from final document text alone.

# Training performance expectations

This page maps the benchmarks registered from `benchmarks/training/`. The
measurements below describe the current benchmark code; they are not service
SLOs or evidence that a release workflow enforces a threshold.

## Registered benchmark targets

| Target | Source | Coverage |
|---|---|---|
| `bench_training_lifecycle_gates` | `benchmarks/training/bench_training_lifecycle_gates.cpp` | LoRA adapter construction, single/batch update, linear/TIES merge, weight export/import, and a 1,000-update lifecycle |
| `bench_training_dedicated_gates` | `benchmarks/training/bench_training_dedicated_gates.cpp` | In-process queue, checkpoint, and gradient-sync stubs, including an 8-worker queue case |

### Lifecycle thresholds

The lifecycle benchmark compares its measurements to the following source
constants and writes `[PERF_GATE]` diagnostics to stderr when a comparison
exceeds its target:

| Gate | Operation | Target |
|---|---|---:|
| TLG-01 | Adapter construction | ≤50 µs |
| TLG-02 | Single adapter update | ≤10 µs |
| TLG-03 | Three-layer batch update | ≤30 µs |
| TLG-04 | Linear merge | ≤5 ms |
| TLG-05 | TIES merge | ≤5 ms |
| TLG-06 | Export weights | ≤200 µs |
| TLG-07 | Import weights | ≤200 µs |
| TLG-08 | 1,000 adapter updates | ≤100 ms |

The dedicated benchmark labels four target values (enqueue ≤1 µs,
checkpoint-stub round-trip ≤100 µs, sync ≥1,000 operations/s, and 8-worker
dispatch ≥500 operations/s total). Its checkpoint and sync implementations
are stubs, and the benchmark does not compare the measured values with those
labels or fail on a threshold breach. Do not treat these cases as measurements
of production persistence, GPU, or distributed training.

## Build and interpretation

The benchmark subdirectory returns without adding targets unless
`THEMIS_BUILD_BENCHMARKS` is enabled. Configure with a supported repository
preset and that option, then build `bench_training_lifecycle_gates` or
`bench_training_dedicated_gates`; executables are placed under
`<build-dir>/bin/`. Example:

```sh
cmake --preset <supported-preset> -DTHEMIS_BUILD_BENCHMARKS=ON
cmake --build <build-dir> --target bench_training_lifecycle_gates
<build-dir>/bin/bench_training_lifecycle_gates
```

Google Benchmark options can filter or repeat cases. The benchmarks are not
registered as CTest tests. Record the build configuration and machine when
comparing runs; no p95/p99 release baseline or DB-backed training benchmark is
defined by these two targets.

## Source references

- [`benchmarks/training/CMakeLists.txt`](../../benchmarks/training/CMakeLists.txt)
- [`benchmarks/training/README.md`](../../benchmarks/training/README.md)
- [`ROADMAP.md`](./ROADMAP.md) — open data-reader and benchmark work.
