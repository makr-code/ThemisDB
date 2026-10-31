---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# ONNX CLIP benchmark fixtures

The benchmark executables in this directory measure synthetic workloads, not
`ONNXClipPlugin` inference. `bench_onnx_clip_cpu.cpp` uses
`MockOnnxClipModel`; `bench_onnx_clip_vit_backend.cpp` uses
`MockOnnxClipBackendModel`. They do not load model weights or execute an ONNX
graph. The backend fixture sets CUDA availability to false and simulates its
workloads on the CPU.

Consequently, benchmark timings, RSS values, CUDA throughput figures, and the
thresholds in `baselines.json` are not measurements of the live plugin and are
not production acceptance evidence. Linking the benchmark targets to the plugin
does not replace the mock fixtures with plugin calls.

## Build and run

The targets are available when `THEMIS_BUILD_BENCHMARKS=ON`:

```bash
cmake --preset linux-release -DTHEMIS_BUILD_BENCHMARKS=ON
cmake --build --preset linux-release --target \
    bench_onnx_clip_cpu \
    bench_onnx_clip_vit_backend
```

Run either Google Benchmark executable directly, for example:

```bash
./build/linux-release/benchmarks/onnx_clip/bench_onnx_clip_cpu
./build/linux-release/benchmarks/onnx_clip/bench_onnx_clip_vit_backend
```

These runs characterize only the synthetic fixtures. A production performance
gate requires benchmarks that invoke the live plugin with a verified model,
supported provider, and representative inputs on the target hardware.

## Files

- `bench_onnx_clip_cpu.cpp` — synthetic CPU latency and initialization workloads
- `bench_onnx_clip_vit_backend.cpp` — synthetic throughput and memory workloads
- `baselines.json` — legacy fixture thresholds and values; not a current release gate
- `run_baseline.sh` — fixture baseline collection helper
- `CMakeLists.txt` — benchmark target registration

For implementation behavior and production readiness, see
`src/onnx_clip/README.md`, `src/onnx_clip/ARCHITECTURE.md`, and
`src/onnx_clip/PRODUCTION_REQUIREMENTS.md`.
