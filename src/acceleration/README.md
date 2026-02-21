# Acceleration Module

Hardware acceleration layer for compute-heavy ThemisDB workloads with CUDA, Vulkan and portable backend options.

## Current Delivery State

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
