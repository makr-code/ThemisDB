---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "ThemisDB documentation orchestration assistant"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources:
  - "benchmarks/onnx_clip/CMakeLists.txt"
  - "benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp"
  - "benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp"
  - "benchmarks/onnx_clip/baselines.json"
  - "src/onnx_clip/onnx_clip_plugin.cpp"
Related_Issue: "#7365"
---

Exact model backend not verifiable from final document text alone.

# ONNX CLIP benchmark sources

## What these benchmarks measure

The two source files define 36 Google Benchmark cases in total: 13 in
`bench_onnx_clip_cpu.cpp` and 23 in `bench_onnx_clip_vit_backend.cpp`. They are
mock/synthetic workloads, not measurements of `ONNXClipPlugin` or ONNX model
inference:

- The CPU file uses `MockOnnxClipModel`, which allocates and fills synthetic
  vectors. Its initialize, image, text, and health-check methods do not call the
  plugin.
- The backend file uses `MockOnnxClipBackendModel`. Its “CUDA” labels do not
  select CUDA execution; `encodeBatch()` loops over synthetic single-image work
  and does not use a provider; the mock's `cuda_available_` is hardcoded to
  `false`. The file's RSS cases measure synthetic process allocations, not model
  weights or actual inference activations.
- The per-case targets and gate identifiers in C++ comments are not evidence
  that any threshold was measured or met. The local benchmark CMake file and
  baseline script do not establish release-gate enforcement.

## Registered benchmark names

These are the names in the `BENCHMARK_F` registrations, grouped by source.
Names that contain `CUDA` or `ViTB32` are still mock-case labels, not evidence
of CUDA execution or a loaded ViT-B/32 model.

The benchmark CMake descriptions and source comments use the `v0.3.0` label.
That is a benchmark-source label; `ONNXClipPlugin::getInfo()` separately
reports plugin metadata version `1.0.0`. Neither version label establishes
model-backed benchmark results.

**`bench_onnx_clip_cpu.cpp` (13):**

```text
BM_SingleImageLatency_ViTB32_CPU
BM_BatchLatency_ViTB32_CPU_Batch8
BM_BatchLatency_ViTB32_CPU_Batch16
BM_TextEmbedding_Latency_CPU
BM_Initialization_Latency_CPU
BM_HealthCheck_Latency_CPU
BM_Latency_Regression_SingleImage
BM_Latency_Regression_Batch8
BM_Latency_Regression_Batch16
BM_InitTime_ModelLoad
BM_InitTime_SessionCreate
BM_InitTime_Warmup
BM_InitTime_Total
```

**`bench_onnx_clip_vit_backend.cpp` (23):**

```text
BM_Throughput_ViTB32_CUDA_Batch64
BM_Throughput_ViTB32_CPU_Batch16
BM_MemoryFootprint_ModelLoad
BM_MemoryFootprint_Runtime_State
BM_BatchSplitting_Performance
BM_ThroughputScaling_CPU_Batch1
BM_ThroughputScaling_CPU_Batch8
BM_ThroughputScaling_CPU_Batch16
BM_ThroughputScaling_CPU_Batch32
BM_ThroughputScaling_CPU_Batch64
BM_ThroughputScaling_CUDA_Batch1
BM_ThroughputScaling_CUDA_Batch8
BM_ThroughputScaling_CUDA_Batch16
BM_ThroughputScaling_CUDA_Batch32
BM_ThroughputScaling_CUDA_Batch64
BM_MemoryScaling_Batch1
BM_MemoryScaling_Batch2
BM_MemoryScaling_Batch4
BM_MemoryScaling_Batch8
BM_MemoryScaling_Batch16
BM_MemoryScaling_Batch32
BM_MemoryScaling_Batch48
BM_MemoryScaling_Batch64
```

Consequently this directory provides no validated CPU/CUDA latency, throughput,
memory, or model-initialization results. Do not use the checked-in
`baselines.json` values as observed performance. It is a static, dated
historical artifact (`generated: 2026-08-09`) containing baseline/gate figures;
its own notes identify mock models. Those values are not results verified
against the current plugin and do not establish that the thresholds were met.

## Build wiring

`CMakeLists.txt` adds `bench_onnx_clip_cpu` and
`bench_onnx_clip_vit_backend` through the repository's standard benchmark helper
only when `THEMIS_BUILD_BENCHMARKS` is enabled. It links the plugin target if
that target exists, but the benchmark source code still executes its own mock
classes rather than calling the plugin.

`run_baseline.sh` invokes those two executables and writes timestamped output
files. Its existence does not establish that the generated values are
model-backed or release-gating. No gate-checking runner is implemented in the
benchmark sources or this script; the script's next steps call for manual
review and comparison.

## Source references

- `benchmarks/onnx_clip/CMakeLists.txt` — conditional targets and optional link
- `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp` — synthetic CPU-labeled cases
- `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp` — synthetic backend,
  throughput, and RSS cases
- `benchmarks/onnx_clip/baselines.json` — unverified mock-baseline artifact
- `src/onnx_clip/onnx_clip_plugin.cpp` — current reference implementation
