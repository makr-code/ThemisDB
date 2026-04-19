<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Acceleration Module Public Headers

All notable changes to public headers in `include/acceleration/`.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.7.0] — 2026-03-12
### Added
- `plugin_security.h`: `PluginSecurity` interface for plugin allowlist and signature verification
- `shader_integrity.h`: `ShaderIntegrity` for SPIR-V and PTX binary hash verification
- `vllm_resource_manager.h`: `VLLMResourceManager` for GPU budget management during vLLM inference
- `rccl_vector_backend.h`: RCCL (AMD) collective operations backend header
- `geo_acceleration_bridge.h`: Bridge interface connecting geospatial module to GPU acceleration
- `kernel_fallback_dispatcher.h`: Priority-ordered kernel fallback across backends

### Changed
- `error_codes.h`: Extended `AccelErrorCode` with `PLUGIN_SIGNATURE_INVALID`, `SHADER_HASH_MISMATCH`
- `compute_backend.h`: Added `capabilities()` method returning `BackendCapabilities` struct

## [1.6.0] — 2026-02-24
### Added
- `nccl_vector_backend.h`: NCCL-backed collective vector operations
- `tensor_core_matmul.h`: Tensor core GEMM interface for embedding similarity
- `multi_gpu_backend.h`: Multi-GPU data-parallel dispatch interface
- `faiss_gpu_backend.h`: FAISS GPU approximate nearest-neighbour search backend

### Changed
- `compute_future.h`: Added `cancel()` method; `get()` now throws `ComputeCancelledException`

## [1.5.0] — 2026-01-15
### Added
- Initial public header set: `compute_backend.h`, `device_manager.h`, `kernel_invocation.h`,
  `cuda_backend.h`, `hip_backend.h`, `vulkan_backend.h`, `opencl_backend.h`, `cpu_backend.h`
- `batch_validator.h`: Tensor shape and dtype validation before kernel submission
- `error_codes.h`, `error_context.h`: Unified error taxonomy
