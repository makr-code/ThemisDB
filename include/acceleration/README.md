---
Author: "ThemisDB Team"
Created: "2026-10-29"
Last Updated: "2026-10-29"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Team"
---

> Build: `cmake --preset linux-release && cmake --build --preset linux-release`

# Acceleration Module — Public Headers

**Version:** 1.1
**Last Updated:** 2026-10-29
**Module path:** `include/acceleration/`

## Purpose

This directory contains the public C++ headers that define the stable API for ThemisDB backend selection, capability probing, execution dispatch, and CPU/GPU fallback behavior. Runtime backends are implemented in `src/acceleration/`; callers are expected to depend only on the public headers in `include/acceleration/`.

## Key headers

| Header | Role |
|---|---|
| `compute_backend.h` | Core backend contract: `BackendType`, `PrecisionMode`, device capability metadata, capability enums, and `BackendRegistry` entry points |
| `device_manager.h` | Device enumeration, capability probing, cache TTL, and deterministic injection hook for tests |
| `ai_hardware_dispatcher.h` | AI inference routing across CPU/GPU/NPU-style execution backends |
| `kernel_invocation.h` | Frozen kernel-dispatch contracts for ANN/geospatial kernel calls |
| `kernel_fallback_dispatcher.h` | Retry + fallback tuple for heterogeneous kernels |
| `compute_future.h` | Async result handle for compute operations |
| `compute_graph.h` | DAG-based execution graph for backend scheduling |
| `batch_validator.h` | Input validation, bounded execution, and fail-closed safety rules |
| `cpu_backend.h` | CPU fallback backend implementations |
| `cuda_backend.h` | CUDA backend API surface |
| `hip_backend.h` | HIP/ROCm backend API surface |
| `vulkan_backend.h` | Vulkan compute backend API surface |
| `opencl_backend.h` | OpenCL backend API surface |
| `graphics_backends.h` | DirectX/Vulkan/OpenGL compute wrapper surface |
| `faiss_gpu_backend.h` | FAISS GPU vector-search wrapper surface |
| `multi_gpu_backend.h` | Multi-GPU coordinator and fan-out surface |
| `nccl_vector_backend.h` / `rccl_vector_backend.h` | NCCL/RCCL collective backend APIs |
| `tensor_core_matmul.h` | Tensor-core matrix multiply surface |
| `quantized_backend.h` | Quantized backend contract |
| `vec_knn.h` | Vector KNN entry-point surface |
| `geo_acceleration_bridge.h` | Geospatial acceleration bridge |
| `plugin_loader.h` | Dynamic backend plugin loading |
| `plugin_security.h` | Plugin integrity and trust validation |
| `shader_integrity.h` | Shader correctness/integrity validation |
| `vllm_resource_manager.h` | vLLM GPU resource leasing and utilization control |
| `error_codes.h` / `error_context.h` | Structured backend error taxonomy and contextual metadata |

## Subdirectories

### `raii/`

RAII wrappers are present under `include/acceleration/raii/` for GPU resource lifetime safety. Current module docs should treat these as helper-level wrappers, not as the primary public architecture surface.

| Header | Wraps |
|---|---|
| `raii/cuda_raii.h` | CUDA stream / device memory / scoped device wrappers |
| `raii/hip_raii.h` | HIP resource wrappers |
| `raii/opencl_raii.h` | OpenCL context / program / queue / buffer wrappers |
| `raii/vulkan_raii.h` | Vulkan resource wrappers |

### `metrics/`

The metrics subdirectory contains backend-health and collector interfaces used for runtime observability.

| Header | Role |
|---|---|
| `metrics/backend_metrics.h` | per-backend health and throughput counters |
| `metrics/metrics_collector.h` | registry for counters/gauges/histograms and timing collection |

## Build flags and feature gates

The public API documents the following compile-time gates:

- `THEMIS_ENABLE_CUDA` — exposes CUDA backend API and kernel paths
- `THEMIS_ENABLE_HIP` — exposes HIP/ROCm backend API
- `THEMIS_ENABLE_VULKAN` — exposes Vulkan compute API
- `THEMIS_ENABLE_OPENCL` — exposes OpenCL backend API
- `THEMIS_ENABLE_OPENGL` — exposes OpenGL compute path when available
- `THEMIS_ENABLE_NPU_*` and `THEMIS_DISABLE_*` — AI hardware capability toggles in `ai_hardware_dispatcher.h`

Builds without GPU SDKs remain supported; CPU fallback is the required baseline.

## Primary entry points

| Entry point | Purpose |
|---|---|
| `BackendRegistry::initializeRuntime(...)` | runtime capability discovery and backend selection |
| `BackendRegistry::getSelectedVectorBackend()` / `getSelectedGraphBackend()` / `getSelectedGeoBackend()` | access selected runtime backends after initialization |
| `DeviceManager::probeDevices()` / `refresh()` / `getBestDevice()` | capability enumeration and cache-aware selection |
| `DeviceManager::setEnumerateFn()` | deterministic test injection for capability snapshots |
| `AiHardwareDispatcher::run()` / `runOn()` | AI inference dispatch to the best available backend |
| `PluginLoader::loadPlugin()` / `loadPluginsFromDirectory()` | dynamic plugin loading |
| `BatchValidator::*` | bounded workload validation and fail-closed guardrails |

## Runtime configuration and behavior

- `DeviceManager` maintains a 60-second cache and synthesizes a `CPU Fallback` device when discovery yields no usable hardware snapshot.
- `BackendRegistry::initializeRuntime()` is the main startup selector; selected backends may remain `nullptr` when no backend satisfies the requirements.
- CPU fallback is intentionally preserved as the canonical last resort rather than a silent reduction to an unvalidated path.
- Validation and safety failures surface through the structured error model (`error_codes.h` / `error_context.h`) instead of silent runtime bypass.

## Example usage

```cpp
using namespace themis::acceleration;

auto& registry = BackendRegistry::instance();
registry.initializeRuntime();

IVectorBackend* vector_backend = registry.getSelectedVectorBackend();
if (!vector_backend) {
    // Caller decides whether to keep the CPU fallback or fail closed.
}
```

```cpp
using namespace themis::acceleration;

DeviceManager::setEnumerateFn([]() {
    DeviceCapabilityInfo cpu;
    cpu.index = -1;
    cpu.name = "CPU Fallback";
    cpu.backend_type = BackendType::CPU;
    cpu.is_healthy = true;
    return std::vector<DeviceCapabilityInfo>{cpu};
});
```

## Troubleshooting and validation

- Check feature gate availability first (`THEMIS_ENABLE_CUDA`, `THEMIS_ENABLE_VULKAN`, `THEMIS_ENABLE_HIP`, `THEMIS_ENABLE_OPENCL`).
- Validate device health and capability selection via `DeviceManager` and `BackendRegistry` before calling backend-specific kernels.
- For module-level design and operational guidance, use:
  - [`../../src/acceleration/ARCHITECTURE.md`](../../src/acceleration/ARCHITECTURE.md)
  - [`../../src/acceleration/ROADMAP.md`](../../src/acceleration/ROADMAP.md)
  - [`../../src/acceleration/FUTURE_ENHANCEMENTS.md`](../../src/acceleration/FUTURE_ENHANCEMENTS.md)
  - [`../../docs/acceleration/troubleshooting.md`](../../docs/acceleration/troubleshooting.md)

## See also

- [`../../src/acceleration/README.md`](../../src/acceleration/README.md)
- [`../../src/acceleration/ARCHITECTURE.md`](../../src/acceleration/ARCHITECTURE.md)
- [`../../src/acceleration/ROADMAP.md`](../../src/acceleration/ROADMAP.md)
- [`../../src/acceleration/FUTURE_ENHANCEMENTS.md`](../../src/acceleration/FUTURE_ENHANCEMENTS.md)
- [`../../docs/acceleration/capability_negotiation.md`](../../docs/acceleration/capability_negotiation.md)

## Installation

This module is included as part of ThemisDB. The headers are consumed via the project include path, and the implementation is compiled as part of the core library build.

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
