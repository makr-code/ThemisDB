---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-11-16"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# PERFORMANCE_EXPECTATIONS - src/acceleration

## Scope
- Module: src/acceleration
- This file defines candidate performance targets and the evidence required before release-gate approval.

## Benchmark Reference
- Relevant benchmark files:
  - benchmarks/acceleration/bench_acceleration_dispatch.cpp (production CPU ANN/geo dispatch tables)
  - benchmarks/acceleration/bench_acceleration_cuda_gates.cpp (hardware-gated CPU/CUDA distance timing; no numerical parity assertion)
  - benchmarks/gpu/bench_cuda_vs_cpu.cpp
  - benchmarks/gpu/bench_multi_gpu_scaling.cpp
  - benchmarks/gpu/bench_gpu_backends.cpp
  - benchmarks/gpu/bench_gpu_module.cpp
  - benchmarks/gpu/bench_gpu_hardware_capability.cpp
  - benchmarks/acceleration/bench_acceleration_dedicated_gates.cpp (stubbed router/queue/CPU executor; not production runtime evidence)

## Specific Expectations
| Target ID | Expectation | Benchmark case |
|---|---|---|
| ACC-1 | dispatch-path overhead remains within release baseline budget | BENCHMARK_REGISTER_F(AnnDispatchBenchFixture, L2Distance), BENCHMARK_REGISTER_F(AnnDispatchBenchFixture, CosineDistance), BENCHMARK_REGISTER_F(AnnDispatchBenchFixture, TopK) |
| ACC-2 | geo-dispatch overhead remains bounded | BENCHMARK_REGISTER_F(GeoDispatchBenchFixture, HaversineBatch) |
| ACC-3 | single-vs-multi-GPU scaling remains bounded | `benchmarks/gpu/bench_multi_gpu_scaling.cpp`; hardware-gated |
| ACC-4 | gradient-sync and comm-compute overhead remains bounded | No acceleration-module-owned production benchmark mapping verified |
| ACC-5 | backend distance-computation performance remains within release baseline budget | `benchmarks/gpu/bench_gpu_backends.cpp`; hardware-gated |
| ACC-6 | backend init and throughput behavior remains bounded | BM_BackendInitializationOverhead, BM_ThroughputComparison |
| ACC-7 | gpu-module memory and policy path overhead remains bounded | `benchmarks/gpu/bench_gpu_module.cpp`; GPU-module scope, not backend dispatch |
| ACC-8 | capability-probe and load-balancer overhead remains bounded | BM_GPUHardwareCapabilityProbe, BM_GpuNVLinkTopologyDetect, BM_GpuLoadBalancer_LeastLoaded |
| ACC-9 | CUDA-vs-CPU acceleration ratio remains within release baseline budget | `benchmarks/gpu/bench_cuda_vs_cpu.cpp`; `bench_acceleration_cuda_gates.cpp` for the separate ≥8× CUDA distance target |
| ACC-10 | fallback and capability-gate behavior remains bounded in disabled/guarded paths | Stub benchmark is proxy-only; production fallback latency has no verified mapping |

## Candidate Module Gates (not release-approved)

| Gate ID | Expectation | Measurement |
|---|---|---|
| AG-1 | Regression <= 10 percent vs an approved profile baseline | (current - baseline) / baseline; no approved artifact currently established |
| AG-2 | Dispatch and backend p99 within an approved profile envelope | Production-path output with workload, build, and host metadata; numeric threshold remains pending |
| AG-3 | Multi-device p95/p99 and scaling within an approved envelope | Real multi-device run on a named topology; hardware evidence pending |
| AG-4 | No required benchmark missing or skipped | Versioned manifest must record each benchmark and completion/skip reason |

## Validation
- A gate passes only when raw production-path benchmark output, profile metadata, and the versioned manifest are reviewed together.
- Current simulated/proxy targets are smoke checks, not runtime performance evidence; no release performance sign-off is implied by their passing.
- The CUDA gate benchmark measures CPU/CUDA distance timing and enforces its target when CUDA is available; pair it with the relevant numerical parity tests because this benchmark does not compare result values.
- `benchmarks/baselines/acceleration/baseline.json` is sample CPU data explicitly marked representative; it must not be used as a release baseline without a new reviewed run artifact.

## Sourcecode Verification (Module: acceleration/performance)

- Verified benchmark sources:
  - benchmarks/acceleration/bench_acceleration_dispatch.cpp
  - benchmarks/acceleration/bench_acceleration_cuda_gates.cpp
  - benchmarks/gpu/bench_cuda_vs_cpu.cpp
  - benchmarks/gpu/bench_multi_gpu_scaling.cpp
  - benchmarks/gpu/bench_gpu_backends.cpp
  - benchmarks/gpu/bench_gpu_module.cpp
  - benchmarks/gpu/bench_gpu_hardware_capability.cpp
- Verified mapping surfaces:
  - dispatch and geo-dispatch benchmarks
  - backend and gpu-module performance benchmarks
  - multi-GPU scaling and overhead benchmarks
  - capability and fallback/feature-gate benchmarks
- Result:
  - CPU dispatch and GPU benchmark sources exist at the paths above; dedicated acceleration gates use stubs.
  - No checked-in run manifest or approved hardware-profile baseline was found, so release performance gates remain pending.
