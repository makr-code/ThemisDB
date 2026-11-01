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
- Relevant benchmark files:
  - benchmarks/bench_gpu_training_cycle.cpp
  - benchmarks/bench_lora_training.cpp
  - benchmarks/training/bench_lora_data_paths.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| TRNP-1 | training cycle CPU/GPU capability paths remain bounded | BM_GPUTrainingCycle_GPUDisabled, BM_TrainingCycle_CPU_Baseline, BM_TrainingCycle_CUDA, BM_TrainingCycle_HIP, BM_TrainingCycle_Vulkan, BM_CompleteTrainingStep_CUDA |
| TRNP-2 | LoRA layer construction and forward/backward paths remain bounded | BM_LoRALayer_Construction, BM_LoRALayer_Forward, BM_LoRALayer_Backward, BM_LoRALayer_RankImpact |
| TRNP-3 | attention and sequential LoRA composition paths remain bounded | BM_AttentionLoRA_Construction, BM_AttentionLoRA_Forward, BM_AttentionLoRA_Backward, BM_Sequential_Construction, BM_Sequential_Forward, BM_Sequential_Backward, BM_CompositePattern_Overhead |
| TRNP-4 | parameter and memory efficiency paths remain bounded | BM_LoRALayer_ParameterCount, BM_LoRALayer_ParameterAccess, BM_Sequential_ParameterCollection, BM_LoRALayer_MemoryUsage, BM_Compare_LoRAvsFullFinetuning |
| TRNP-5 | DB-backed sample acquisition is bounded and deterministic | BM_LoRADataPath_TrainingDataIteratorAql/128, BM_LoRADataPath_TrainingDataIteratorAql/512, BM_LoRADataPath_IncrementalTrainerCollection/32, BM_LoRADataPath_IncrementalTrainerCollection/128 |
| TRNP-6 | DB-backed labeling, selection, and training report stage costs independently; only selected IDs are trained | BM_LoRADataPath_TrainingPipelineEndToEnd |
| TRNP-7 | non-DB inputs remain separately measurable and are not labeled DB-backed | BM_LoRADataPath_AQLBuiltin, BM_LoRADataPath_CustomSamples, BM_LoRADataPath_JsonObject, BM_LoRADataPath_JsonFile, BM_LoRADataPath_PreparedBatchMerge |
| TRNP-8 | model-backed LoRA training identifies the source path and requires a real GGUF artifact; missing artifacts are not measurements | BM_LoRATrainFromAQLBuiltin, BM_LoRATrainFromCustomSamples, BM_LoRATrainFromJsonObject, BM_LoRATrainFromJsonFile, BM_LoRATrainFromPreparedBatch, BM_LoRADataPath_TrainBatchWithModel |

## Data-Path Metrics

The new data-path benchmark reports, per source: records fetched, valid/skipped samples, bytes read, DB query time, conversion time, trainer update time, end-to-end time, samples/second, and p50/p95 latency. DB paths use a temporary RocksDB-backed `QueryEngine`; failed reads must fail the benchmark rather than silently exercise synthetic fallback. Baseline comparisons use the same host and build profile and flag throughput regressions above 10 percent.

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
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
