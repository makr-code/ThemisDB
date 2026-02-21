# Acceleration Module

Hardware acceleration layer for compute-heavy ThemisDB workloads with CUDA, Vulkan and portable backend options.

## Module Purpose

Provides GPU and hardware acceleration for compute-intensive database operations including vector similarity search, geospatial queries, graph algorithms, and matrix operations using CUDA and Vulkan backends.

## Subsystem Scope

**In scope:** CUDA kernel implementations for HNSW/ANN search, Vulkan compute shaders for cross-platform GPU, GPU device detection and capability negotiation, CPU fallback paths.

**Out of scope:** CPU-only algorithm implementations (handled by index/geo/graph modules), GPU driver management.

## Relevant Interfaces

- `cuda/ann_kernels.cu` — CUDA ANN search kernels (stub)
- `vulkan/compute_pipeline.cpp` — Vulkan compute shaders (stub)
- `device_manager` — runtime device detection

## Current Delivery State

**Maturity:** 🔴 Alpha — Infrastructure scaffolding complete; CUDA and Vulkan kernel implementations are stubs pending production implementation.

## Status

- Module is in pre-production hardening.
- Several roadmap items are already tracked on GitHub (`[I]`) and synchronized from `ROADMAP.md`.
- Backends are present in source, but production coverage still depends on full kernel completion, runtime capability negotiation and cross-backend parity testing.

## Backend Surface

- `cuda/` + `cuda_backend.cpp` + `vector_kernels.cu`: NVIDIA CUDA path (primary for ANN/geospatial kernels).
- `vulkan/` + `vulkan_backend_full.cpp`: cross-vendor compute shader path.
- Additional backend entry points (OpenCL, HIP, oneAPI, Metal, DirectX, NCCL/RCCL, ZLUDA) are staged in module sources for phased enablement.

## Consolidated Roadmap Inputs

- Execution roadmap and statuses: `ROADMAP.md`
- Detailed implementation constraints: `future_enhancements.md`
- GitHub issue generation uses both files to build non-stub implementation tickets with phases, acceptance criteria and risk constraints.

## Implementation Contract

Every roadmap implementation item is expected to carry:

- concrete subsystem/file scope
- runtime behavior and fallback expectations
- validation/error handling requirements
- unit/integration/performance test requirements
- measurable targets (latency/throughput/parity)

## Documentation

- [CUDA Backend](../../docs/performance/CUDA_BACKEND.md)
- [Vulkan Backend](../../docs/performance/VULKAN_BACKEND.md)
- [Hardware Acceleration Plan](../../docs/performance/HARDWARE_ACCELERATION.md)
- [GPU Acceleration Plan](../../docs/performance/GPU_ACCELERATION_PLAN.md)
