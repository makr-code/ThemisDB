> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

# Acceleration Module — Public Headers

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `include/acceleration/`

---

## Purpose

This directory contains the public C++ header files (`.h`) that define the stable API for ThemisDB's hardware-acceleration layer. Consumers of the acceleration module (vector index, geo engine, graph analytics, LLM pipeline) include only these headers — they never depend on backend-specific CUDA/Vulkan/HIP headers directly.

## Key Headers

| Header | Role |
|---|---|
| `compute_backend.h` | Abstract `ComputeBackend` base class and `DeviceCapabilityInfo` struct |
| `compute_future.h` | `ComputeFuture<T>` — async result handle for GPU kernel submissions |
| `compute_graph.h` | `ComputeGraph` — DAG-based kernel dependency and execution graph |
| `device_manager.h` | Device enumeration, capability probing, 60 s TTL cache |
| `kernel_fallback_dispatcher.h` | `ANNKernelFallbackDispatcher` and `GeoKernelFallbackDispatcher` with retry logic |
| `kernel_invocation.h` | Frozen ANN and geospatial kernel invocation interfaces (`ANNKernelDispatch`, `GeoKernelDispatch`); `INTERFACE_VERSION = 100` |
| `batch_validator.h` | Input validation utilities shared across all backends |
| `cpu_backend.h` | CPU fallback backend (no GPU SDK required) |
| `cuda_backend.h` | CUDA backend API including `CUDAGraphCache` (guarded by `THEMIS_ENABLE_CUDA`) |
| `hip_backend.h` | HIP/ROCm backend API (guarded by `THEMIS_ENABLE_HIP`) |
| `vulkan_backend.h` | Vulkan compute backend API (guarded by `THEMIS_ENABLE_VULKAN`) |
| `graphics_backends.h` | `DirectXVectorBackend`, `VulkanVectorBackend`, `VulkanGeoBackend`, `OpenGLVectorBackend` |
| `opencl_backend.h` | OpenCL backend API |
| `faiss_gpu_backend.h` | FAISS GPU wrapper for billion-scale ANN search |
| `multi_gpu_backend.h` | Multi-GPU load balancing and work distribution |
| `nccl_vector_backend.h` / `rccl_vector_backend.h` | Multi-GPU collective operations (NVIDIA/AMD) |
| `tensor_core_matmul.h` | Tensor Core FP16/BF16 matrix multiplication |
| `quantized_backend.h` | INT8/INT4 quantized inference backend |
| `vec_knn.h` | `VecKNN` — vectorised k-nearest-neighbour search interface |
| `ai_hardware_dispatcher.h` | `AIHardwareDispatcher` — routes AI workloads to best available accelerator |
| `geo_acceleration_bridge.h` | Geospatial kernel dispatch bridge |
| `plugin_loader.h` | Dynamic external backend plugin loading |
| `plugin_security.h` | Signature verification for loaded plugins |
| `shader_integrity.h` | SPIR-V shader integrity verification |
| `vllm_resource_manager.h` | vLLM GPU VRAM resource lease management |
| `error_codes.h` / `error_context.h` | Structured error taxonomy for device and kernel failures |

## Subdirectories

### `raii/`

Header-only RAII wrappers for GPU resources ensuring automatic cleanup and exception safety.

| Header | Wraps |
|---|---|
| `raii/cuda_raii.h` | `CudaStream`, `CudaDeviceMemory`, `ScopedCudaDevice` |
| `raii/hip_raii.h` | HIP stream and device memory RAII wrappers |
| `raii/opencl_raii.h` | `OpenCLContext`, `OpenCLQueue`, `OpenCLProgram`, `OpenCLKernel`, `OpenCLBuffer` |
| `raii/vulkan_raii.h` | Vulkan object RAII wrappers |

All wrappers are move-only and header-only with zero runtime overhead. See [`raii/README.md`](raii/README.md) for usage examples and design principles.

### `metrics/`

Backend performance metrics collection.

| Header | Role |
|---|---|
| `metrics/backend_metrics.h` | `BackendMetrics` — per-backend latency and throughput counters |
| `metrics/metrics_collector.h` | `MetricsCollector` — central registry for `Counter`, `Gauge`, `Histogram`, and `Timer` metrics |

## Build Flags

All GPU-specific headers are guarded by compile-time feature flags:

- `THEMIS_ENABLE_CUDA` — exposes CUDA backend API
- `THEMIS_ENABLE_VULKAN` — exposes Vulkan backend API
- `THEMIS_ENABLE_HIP` — exposes HIP/ROCm backend API

Builds without any GPU SDK must still compile successfully; only CPU fallback symbols are required.

## Primary Entry Points

| Entry point | Purpose |
|---|---|
| `BackendRegistry::initializeRuntime(...)` | Runtime auto-detection and capability-driven backend selection. |
| `BackendRegistry::getSelectedVectorBackend()` / `getSelectedGraphBackend()` / `getSelectedGeoBackend()` | Access selected runtime backends after initialization. |
| `BackendRegistry::deviceInfo()` | Read immutable device snapshot captured at runtime initialization. |
| `DeviceManager::probeDevices()` / `refresh()` / `getBestDevice()` | Device discovery and cache-aware capability probing. |
| `DeviceManager::setEnumerateFn()` | Inject deterministic capability snapshots for focused tests and CPU-only validation. |
| `ANNKernelFallbackDispatcher` / `GeoKernelFallbackDispatcher` | Retry + fallback dispatch over frozen kernel invocation tables. |
| `PluginLoader::loadPlugin()` / `loadPluginsFromDirectory()` | Dynamic backend-plugin loading surface. |
| `error_codes.h` + `error_context.h` | Structured error codes and contextualized failure metadata. |

## Runtime Configuration Options

- `BackendRegistry::CapabilityRequirements`: configure required backend capabilities (precision/metrics/features) per operation category.
- `RetryPolicy` (`kernel_fallback_dispatcher.h`): configure retry attempt count and exponential backoff behavior for transient device errors.
- `VLLMResourceManager::Config`: configure GPU lease and utilization thresholds when sharing GPU resources with inference workloads.
- `DeviceManager::setEnumerateFn()`: override runtime device discovery with an injected capability snapshot; empty snapshots still synthesize a CPU fallback device.

## Runtime Behavior, Error Cases, and Limits

- `initializeRuntime()` should be called once during startup; selected backend pointers remain queryable afterwards.
- If no compatible backend is found for requested requirements, selected-backend accessors can return `nullptr`.
- `DeviceManager` always returns at least one device entry; if discovery yields no healthy accelerator snapshot, a synthetic `CPU Fallback` device is emitted with `index == -1`.
- Fallback dispatchers treat `DeviceLost`, `OperationTimeout`, and `SynchronizationFailed` as transient and apply retry before CPU fallback.
- Validation failures surface through `AccelerationErrorCode` values (`InvalidInputShape`, `InvalidInputDtype`, `InputRangeViolation`, etc.).
- Builds without CUDA/HIP/Vulkan SDKs remain supported; CPU backends are the required baseline.

## Usage Snippets

```cpp
using namespace themis::acceleration;

auto& registry = BackendRegistry::instance();
registry.initializeRuntime();

IVectorBackend* vectorBackend = registry.getSelectedVectorBackend();
if (!vectorBackend) {
    // No compatible runtime backend selected; caller decides fallback/error path.
}
```

```cpp
using namespace themis::acceleration;

CPUVectorBackend cpu;
cpu.initialize();

ANNKernelFallbackDispatcher dispatcher(
    /*primary*/ someGpuDispatch,
    /*fallback*/ cpu.populateANNDispatch(),
    RetryPolicy{.maxAttempts = 3, .initialDelayMs = 1, .maxDelayMs = 100, .backoffMultiplier = 2.0f});
```

## Troubleshooting

- Verify build flags and SDK availability first (`THEMIS_ENABLE_CUDA`, `THEMIS_ENABLE_VULKAN`, `THEMIS_ENABLE_HIP`).
- Inspect backend health (`IComputeBackend::getHealthStatus()`) and `ErrorContext` for failure details.
- For operational runbooks and fallback diagnostics, use:
  - [`../../docs/acceleration/troubleshooting.md`](../../docs/acceleration/troubleshooting.md)
  - [`../../docs/acceleration/capability_negotiation.md`](../../docs/acceleration/capability_negotiation.md)
  - [`../../docs/acceleration/error_codes.md`](../../docs/acceleration/error_codes.md)

## See Also

- [`src/acceleration/`](../../src/acceleration/README.md) — Implementation code and development guide
- [`src/acceleration/ARCHITECTURE.md`](../../src/acceleration/ARCHITECTURE.md) — Architecture guide with component diagram and data flow
- [`src/acceleration/ROADMAP.md`](../../src/acceleration/ROADMAP.md) — Development roadmap and production readiness checklist
- [`src/acceleration/FUTURE_ENHANCEMENTS.md`](../../src/acceleration/FUTURE_ENHANCEMENTS.md) — Planned features with performance targets
- [`docs/acceleration/capability_negotiation.md`](../../docs/acceleration/capability_negotiation.md) — Backend capability negotiation deep dive
- [`docs/acceleration/troubleshooting.md`](../../docs/acceleration/troubleshooting.md) — Operational troubleshooting guide
- [`docs/de/acceleration/README.md`](../../docs/de/acceleration/README.md) — German secondary documentation

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
