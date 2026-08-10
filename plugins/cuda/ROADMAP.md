# CUDA Acceleration Plugin – Roadmap

## Current Status

**Status:** 🟡 Implemented plugin path, but environment validation pending (toolchain/dependency blockers in CI-like Linux environment)

| Component | Location | Status |
|-----------|----------|--------|
| CUDA backend (built-in) | `src/acceleration/cuda_backend.cpp` | 🔧 Requires build flag |
| CUDA kernels | `src/acceleration/cuda/vector_kernels.cu` | 🔧 Requires build flag |
| External loadable plugin | `plugins/cuda/` (this directory) | ✅ Implemented (build/load path present) |

Build flag: `-DTHEMIS_ENABLE_CUDA=ON`

---

## In Progress

- [~] Validate that `CMakeLists.txt.example` and `cuda_plugin.cpp.example` compile cleanly with CUDA Toolkit 12.x
- [~] Document minimum driver / toolkit version requirements

## Planned Features

- [x] Standalone loadable CUDA plugin (`.so`/`.dll`) independent of the built-in backend (Q3 2026)
- [x] FP16 / BF16 mixed-precision vector operations (Q3 2026)
- [x] Multi-GPU support: device selection, peer-to-peer transfers (Q3 2026)
- [x] Integration with RAPIDS cuVS / FAISS-GPU for ANN search (Q3 2026)
- [ ] Auto-tuning kernel parameters per GPU architecture (Target: 2027)
- [ ] Unified hardware-acceleration interface (CUDA, Vulkan, HIP, Metal, OpenCL) (Target: 2027)

---

## Short-term Goals (next 1–2 sprints)

- [ ] Validate that `CMakeLists.txt.example` and `cuda_plugin.cpp.example` compile cleanly with CUDA Toolkit 12.x.
- [ ] Document minimum driver / toolkit version requirements in this README.
- [ ] Add health-check / capability-query entry point to the plugin interface.

## Mid-term Goals (1–3 months)

- [x] Add benchmark suite: throughput (vectors/sec) vs. CPU baseline.
- [x] Implement mixed-precision (FP16 / BF16) vector operations.
- [x] Add support for multi-GPU setups (device selection, peer-to-peer transfers).

## Long-term Goals (3–12 months)

- [ ] Unified hardware-acceleration plugin interface shared by CUDA, Vulkan, HIP, Metal, OpenCL backends.
- [ ] Auto-tuning kernel parameters (block size, shared memory) per GPU architecture.
- [ ] Formal performance targets: ≥ 8× speedup over CPU baseline on RTX-class GPUs.

## Milestones

| Milestone | Target | Status |
|-----------|--------|--------|
| Standalone CUDA plugin DLL | Q3 2026 | ✅ Implemented |
| FP16 support | Q3 2026 | ✅ Implemented |
| Multi-GPU support | Q3 2026 | ✅ Implemented |
| ANN integration (cuVS) | Q3 2026 | ✅ Implemented |

## Implementation Phases

### Phase 1 – Standalone Plugin DLL
- [x] Extract built-in CUDA backend into a loadable `cuda_plugin.so`/`.dll`
- [x] Define and implement `IComputeBackend` plugin entry-point (`CreateBackendPlugin`)
- [x] CMake target for standalone plugin with correct RPATH / install rules
- [x] Smoke test: load plugin via `PluginManager`, run a vector similarity query

### Phase 2 – FP16 / Mixed-Precision Support
- [x] Implement FP16 and BF16 variants of core vector kernels (`fp16_vector_kernels.cu`)
- [x] Precision selection via plugin configuration (`FP32`, `FP16`, `BF16`)
- [ ] Benchmark: FP16 throughput vs FP32 baseline on RTX-class GPU

### Phase 3 – Multi-GPU & ANN Integration
- [x] Device enumeration and selection API (`MultiGPUVectorBackend::Config::deviceIds`)
- [x] Peer-to-peer transfers for sharded index across multiple GPUs
- [x] cuVS / FAISS-GPU ANN integration: IVF-Flat and HNSW index types
- [ ] Multi-GPU CI tests skipped gracefully when only one GPU is present

### Phase 4 – Auto-tuning & Formal Perf Targets
- [ ] Auto-tuning: select optimal block size and shared-memory config per GPU arch
- [ ] Formal performance target: ≥ 8× speedup over CPU baseline on RTX-class GPUs
- [ ] Publish benchmark results in `benchmarks/cuda/`

---

## Dependencies

- CUDA Toolkit ≥ 11.8 (runtime + nvcc)
- ThemisDB `IComputeBackend` interface (`include/acceleration/compute_backend.h`)
- `BackendRegistry` (`src/acceleration/backend_registry.cpp`)

## Open Questions

- [ ] Should external CUDA plugin DLLs replace or supplement the built-in backend?
- [ ] Which CUDA architectures (sm\_xx) should be compiled by default?

---

## Production Readiness Checklist

| Item | Status |
|------|--------|
| `CMakeLists.txt.example` validated with CUDA 12.x | ❌ Pending |
| Standalone loadable plugin DLL | ✅ Implemented (`plugins/cuda/CMakeLists.txt`, `cuda_plugin.cpp`) |
| FP16 / BF16 support | ✅ Implemented (`include/acceleration/fp16_vector_kernels.h`, `src/acceleration/cuda/fp16_vector_kernels.cu`) |
| Multi-GPU support | ✅ Implemented (wired via `MultiGPUVectorBackend`) |
| ANN (cuVS / FAISS-GPU) integration | ✅ Implemented (wired via `FaissGPUVectorBackend`) |
| Benchmark results published | ❌ Pending |

## Known Issues & Limitations

- Build retry in this environment is blocked before plugin targets by missing critical dependencies (`fmt`) and missing CUDA compiler/toolkit detection.
- Template (`plugins/cuda/`) not yet validated against CUDA Toolkit 12.x in automated CI.
- Multi-GPU paths are tested only on single-GPU CI runners; correctness unverified
- Standalone plugin target exists, but successful end-to-end build validation depends on CUDA toolchain availability.
- No formal performance targets documented yet

---

## Open Backlog Snapshot (33 real open items)

### A) ROADMAP Open Items (15)

1. Validate `CMakeLists.txt.example` and `cuda_plugin.cpp.example` with CUDA Toolkit 12.x.
2. Document minimum driver/toolkit version requirements.
3. Auto-tune kernel parameters per GPU architecture (Target: 2027).
4. Deliver unified hardware-acceleration interface (CUDA/Vulkan/HIP/Metal/OpenCL) (Target: 2027).
5. Add health-check / capability-query entry point to plugin interface.
6. Benchmark FP16 throughput vs FP32 baseline on RTX-class GPU.
7. Ensure multi-GPU CI tests skip gracefully on single-GPU runners.
8. Publish benchmark results in `benchmarks/cuda/`.
9. Meet formal target: ≥ 8× speedup over CPU baseline on RTX-class GPUs.
10. Define whether external CUDA plugin DLLs replace or supplement built-in backend.
11. Define default compiled CUDA architectures (`sm_xx` set).
12. Validate `CMakeLists.txt.example` and `cuda_plugin.cpp.example` as part of short-term sprint execution.
13. Complete minimum driver/toolkit documentation task in plugin docs.
14. Finish Phase 4 auto-tuning task (block size/shared memory per architecture).
15. Finalize open governance decision on external vs built-in CUDA plugin deployment model.

### B) FUTURE_ENHANCEMENTS Open Items (18)

16. Kernel registration ≤ 10 ms including CUDA context initialization.
17. Enforce SHA-256 manifest entry for each external kernel.
18. Enforce minimum compute capability declaration and rejection on incompatible devices.
19. Enforce `max_device_bytes` manifest limit for persistent device memory.
20. Keep CPU-side dispatch overhead ≤ 0.5 ms (excluding kernel runtime).
21. Compile template plugin warning-free with `-Wall -Wextra -Werror` on GCC 13 and Clang 17.
22. Add Tensor Core utilization for GEMM-heavy workloads.
23. Add persistent-kernel mode for streaming workloads.
24. Add CUDA Graph capture/replay path.
25. Add stream-based pipeline for transfer/compute overlap.
26. Add GPU-native IVF-Flat/HNSW ANN index paths.
27. Add geospatial WGS84 distance/containment kernels.
28. Add graph traversal kernels (BFS/DFS) for graph collections.
29. Add NVIDIA Triton Inference Server integration.
30. Provide container image with pre-built CUDA plugin.
31. Add MIG (Multi-Instance GPU) support.
32. Add GPU/CPU parity tests with deterministic FP tolerance ≤ 1e-6.
33. Add property-based randomized kernel correctness tests.

---

*See also: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md) · [`PLANNED_ACCELERATION_PLUGINS.md`](../PLANNED_ACCELERATION_PLUGINS.md)*
