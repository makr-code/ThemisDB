# Acceleration Module — Public Headers

**Version:** 1.0  
**Last Updated:** 2026-03-09  
**Module Path:** `include/acceleration/`

---

## Purpose

This directory contains the public C++ header files (`.h`) that define the stable API for ThemisDB's hardware-acceleration layer. Consumers of the acceleration module (vector index, geo engine, graph analytics, LLM pipeline) include only these headers — they never depend on backend-specific CUDA/Vulkan/HIP headers directly.

## Key Headers

| Header | Role |
|---|---|
| `compute_backend.h` | Abstract `ComputeBackend` base class and `DeviceCapabilityInfo` struct |
| `device_manager.h` | Device enumeration, capability probing, 60 s TTL cache |
| `kernel_fallback_dispatcher.h` | `ANNKernelFallbackDispatcher` and `GeoKernelFallbackDispatcher` with retry logic |
| `batch_validator.h` | Input validation utilities shared across all backends |
| `cuda_backend.h` | CUDA backend API including `CUDAGraphCache` (guarded by `THEMIS_ENABLE_CUDA`) |
| `hip_backend.h` | HIP/ROCm backend API (guarded by `THEMIS_ENABLE_HIP`) |
| `vulkan_backend.h` | Vulkan compute backend API (guarded by `THEMIS_ENABLE_VULKAN`) |
| `opencl_backend.h` | OpenCL backend API |
| `faiss_gpu_backend.h` | FAISS GPU wrapper for billion-scale ANN search |
| `multi_gpu_backend.h` | Multi-GPU load balancing and work distribution |
| `nccl_vector_backend.h` / `rccl_vector_backend.h` | Multi-GPU collective operations (NVIDIA/AMD) |
| `tensor_core_matmul.h` | Tensor Core FP16/BF16 matrix multiplication |
| `geo_acceleration_bridge.h` | Geospatial kernel dispatch bridge |
| `plugin_loader.h` | Dynamic external backend plugin loading |
| `plugin_security.h` | Signature verification for loaded plugins |
| `shader_integrity.h` | SPIR-V shader integrity verification |
| `vllm_resource_manager.h` | vLLM GPU VRAM resource lease management |
| `error_codes.h` / `error_context.h` | Structured error taxonomy for device and kernel failures |

## Build Flags

All GPU-specific headers are guarded by compile-time feature flags:

- `THEMIS_ENABLE_CUDA` — exposes CUDA backend API
- `THEMIS_ENABLE_VULKAN` — exposes Vulkan backend API
- `THEMIS_ENABLE_HIP` — exposes HIP/ROCm backend API

Builds without any GPU SDK must still compile successfully; only CPU fallback symbols are required.

## See Also

- [`src/acceleration/`](../../src/acceleration/README.md) — Implementation code and development guide
- [`src/acceleration/ARCHITECTURE.md`](../../src/acceleration/ARCHITECTURE.md) — Architecture guide with component diagram and data flow
- [`src/acceleration/ROADMAP.md`](../../src/acceleration/ROADMAP.md) — Development roadmap and production readiness checklist
- [`src/acceleration/FUTURE_ENHANCEMENTS.md`](../../src/acceleration/FUTURE_ENHANCEMENTS.md) — Planned features with performance targets
- [`docs/acceleration/capability_negotiation.md`](../../docs/acceleration/capability_negotiation.md) — Backend capability negotiation deep dive
- [`docs/acceleration/troubleshooting.md`](../../docs/acceleration/troubleshooting.md) — Operational troubleshooting guide
- [`docs/de/acceleration/README.md`](../../docs/de/acceleration/README.md) — German secondary documentation
