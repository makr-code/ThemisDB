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

# Training benchmarks

This directory contains two Google Benchmark executables. CMake discovers
`bench_*.cpp` sources here only when `THEMIS_BUILD_BENCHMARKS` is enabled. These
benchmarks are separate from LoRA/GPU benchmarks in `benchmarks/llm/` and
`benchmarks/gpu/`.

## Targets and coverage

| Target / source | Measured operations | Current threshold behavior |
|---|---|---|
| `bench_training_lifecycle_gates` / `bench_training_lifecycle_gates.cpp` | Actual `LoRAAdapter` construction, updates, linear/TIES merge, weight export/import, and 1,000 repeated updates | TLG-01..08 compare measurements with the in-source limits and print `[PERF_GATE]` diagnostics to stderr when exceeded |
| `bench_training_dedicated_gates` / `bench_training_dedicated_gates.cpp` | Queue enqueue/dequeue, stub checkpoint save/restore, atomic stub gradient sync, and 8-worker queue operations | TR-BM-01..04 set labels describing target values; they use in-process stubs and do not enforce threshold failures |

Lifecycle limits are: adapter construction ≤50 µs; single update ≤10 µs;
three-layer batch update ≤30 µs; linear and TIES merges ≤5 ms; weight export
and import ≤200 µs; and 1,000 adapter updates ≤100 ms. Inputs/dimensions are
defined in the benchmark source.

The dedicated benchmark's labeled targets are enqueue ≤1 µs, checkpoint stub
round-trip ≤100 µs, gradient-sync throughput ≥1,000 operations/s, and
eight-worker dispatch throughput ≥500 operations/s total. Because the queue,
checkpoint, and sync implementations in this file are stubs, these results do
not measure production database, checkpoint, GPU, or distributed-training
behavior.

## Build and run

Configure a supported CMake preset with `THEMIS_BUILD_BENCHMARKS=ON`, then
build and run the targets:

```sh
cmake --preset <supported-configure-preset> -DTHEMIS_BUILD_BENCHMARKS=ON
cmake --build <build-dir> --target \
  bench_training_lifecycle_gates bench_training_dedicated_gates
<build-dir>/bin/bench_training_lifecycle_gates
<build-dir>/bin/bench_training_dedicated_gates
```

Google Benchmark options such as `--benchmark_filter=TLG` and
`--benchmark_repetitions=5` can narrow or repeat a run. The two benchmark
targets are not registered as CTest tests. A printed gate diagnostic or target
label is measurement output, not proof that a release workflow enforces that
threshold; check the relevant workflow and baseline configuration separately.

## Source of truth

- [`CMakeLists.txt`](./CMakeLists.txt) — optional target registration.
- [`bench_training_lifecycle_gates.cpp`](./bench_training_lifecycle_gates.cpp)
  — lifecycle operations and limits.
- [`bench_training_dedicated_gates.cpp`](./bench_training_dedicated_gates.cpp)
  — stub operations and descriptive target labels.
- [`../../src/training/PERFORMANCE_EXPECTATIONS.md`](../../src/training/PERFORMANCE_EXPECTATIONS.md)
  — mapping and limitations.
