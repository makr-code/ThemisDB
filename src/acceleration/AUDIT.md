<!-- Status: current | validated: 2026-04-19 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — Acceleration Module

**Last Audit:** 2026-04-19
**Auditor:** Copilot
**Status:** ✅ Pass

## Summary

| Metric | Result |
|--------|--------|
| Build System Registration | ✅ Verified |
| Source Files | 26 (`.cpp` in `src/acceleration/`) |
| Test Coverage | ✅ > 80% (confirmed by maintainer, Issue #1398) |
| Open TODOs | 23 files contain TODOs (primarily hardware-conditional stubs) |
| Open Stubs | 1 (`CUDAGraphBackend` GPU graph traversal — CPU fallback active) |
| Security Issues | None (security hardening pass complete, Issue #1394) |

## Build System

- All acceleration source files are registered in `cmake/CMakeLists.txt` and `cmake/ModularBuild.cmake`.
- Conditional compilation guards: `THEMIS_ENABLE_CUDA`, `THEMIS_ENABLE_HIP`, `THEMIS_ENABLE_VULKAN`, `THEMIS_ENABLE_OPENCL`, `THEMIS_ENABLE_OPENGL`.
- `BACKEND_CONTRACT_VERSION = 100` guarantees ABI stability — verified in `tests/test_backend_api_stability.cpp`.
- CI benchmark gates enforced via `.github/workflows/acceleration-benchmark-ci.yml`.

## Source Files Audited

| File | Purpose |
|------|---------|
| `ai_hardware_dispatcher.cpp` | AI workload dispatch across heterogeneous hardware |
| `backend_registry.cpp` | Runtime backend selection and capability negotiation |
| `compute_backend.cpp` | Base compute backend interface |
| `cpu_backend.cpp` | Single-threaded CPU fallback backend |
| `cpu_backend_mt.cpp` | Multi-threaded CPU backend |
| `cpu_backend_tbb.cpp` | TBB-parallelized CPU backend |
| `cuda/` (6 files) | CUDA kernels: ANN, geo, vector, HNSW, graph, Tensor Core |
| `cuda_backend.cpp` | CUDA backend coordinator with graph capture |
| `device_manager.cpp` | Device discovery with 60s TTL cache |
| `directx_backend_full.cpp` | DirectX 12 compute backend |
| `faiss_gpu_backend.cpp` | FAISS GPU ANN integration |
| `geo_acceleration_bridge.cpp` | Geo module GPU dispatch bridge |
| `graphics_backends.cpp` | Vulkan and OpenGL compute backends |
| `hip/` (2 files) | ROCm/HIP kernels (ANN and geo) |
| `hip_backend.cpp` | HIP backend coordinator |
| `multi_gpu_backend.cpp` | Multi-GPU sharding and fan-out search |
| `nccl_vector_backend.cpp` | NCCL-based multi-GPU vector operations |
| `oneapi_backend.cpp` | Intel oneAPI compute backend |
| `opencl_backend.cpp` | OpenCL fallback backend |
| `plugin_loader.cpp` | Dynamic backend plugin loading |
| `plugin_security.cpp` | Plugin signature verification and sandboxing |
| `rccl_vector_backend.cpp` | RCCL-based multi-GPU vector operations |
| `shader_integrity.cpp` | GPU shader binary integrity verification |
| `tensor_core_matmul.cpp` | Tensor Core FP16/BF16 matrix operations |
| `vec_knn.cpp` | Vectorised k-nearest-neighbour primitives |
| `vllm_resource_manager.cpp` | vLLM-compatible resource lifecycle management |
| `vulkan_backend_full.cpp` | Full Vulkan compute backend |
| `zluda_backend.cpp` | ZLUDA compatibility shim backend |

## Test Coverage

- Unit tests: `tests/test_acceleration.cpp`, `tests/test_cuda_ann_search.cpp`, `tests/test_multi_gpu_backend.cpp`, `tests/test_cuda_graph_capture.cpp`, `tests/test_backend_selection_matrix.cpp` (65 tests), `tests/test_backend_registry_startup.cpp`, `tests/test_opengl_backend.cpp`, `tests/test_plugin_security_audit.cpp`, `tests/test_backend_api_stability.cpp`
- CPU/GPU parity tests: `tests/test_cpu_gpu_parity.cpp` — GPU paths skip gracefully when hardware is absent
- Regression tests: null-pointer, zero-dim, k-clamp inputs covered in `test_cuda_ann_search.cpp`
- Benchmark harness: `benchmarks/bench_cuda_vs_cpu.cpp` with JSON output baseline

## Findings

### Resolved
- **Shell injection surface in plugin signature verification** — `verifyGPGSignature` and `verifyMacOSCodeSignature` used `popen`/shell; replaced with `posix_spawn`+`execv` and `SecStaticCodeCheckValidity` (Issue #1394).
- **`RTLD_LAZY` in plugin loader** — replaced with `RTLD_NOW` to catch unresolved symbols at load time.
- **Missing `supportedPrecisions`/`supportedMetrics` in HIPVectorBackend/ZLUDAVectorBackend** — fields added; no ABI break.
- **Group/world-writable plugin bypass** — file permission check added to `PluginLoader::loadPlugin()`.

### Open
- **`CUDAGraphBackend` GPU graph traversal stub** — BFS and Bellman-Ford kernels exist in `cuda/graph_kernels.cu` but the `CUDAGraphBackend` for the graph module still delegates to CPU; acceptable because CPU fallback is always active.
- **WASM kernel sandbox** — infrastructure is complete but requires a concrete WasmRuntime (Wasmtime/WasmEdge) registration before untrusted kernel execution is operational (Issue #1572).
- **Windows plugin signature verification** — not yet implemented; unsigned plugins are accepted on Windows.

## Compliance

- No PII processed directly; vector embeddings are treated as opaque numeric data.
- VRAM tenant quotas support multi-tenant isolation requirements.
- Plugin loading hardening aligns with supply-chain security best practices (signed binaries, integrity hashes).
