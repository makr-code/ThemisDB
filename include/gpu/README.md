> **Build:** `cmake --preset release && cmake --build build/release`

# GPU Module — Public Headers

**Module Path:** `include/gpu/`
**Implementation:** `../../src/gpu/`

## Purpose

Public interfaces and declarations for ThemisDB's hardware-accelerated execution and GPU resource governance, including device discovery, memory management, execution dispatch, and fallback handling.

## Canonical Module Documentation

`include/gpu/` contains public header contracts. Canonical module behavior, architecture, and operations docs live in `src/gpu/`:

- [`../../src/gpu/README.md`](../../src/gpu/README.md)
- [`../../src/gpu/ARCHITECTURE.md`](../../src/gpu/ARCHITECTURE.md)
- [`../../src/gpu/GPU_CONTRACT.md`](../../src/gpu/GPU_CONTRACT.md)
- [`../../src/gpu/ROADMAP.md`](../../src/gpu/ROADMAP.md)
- [`../../src/gpu/FUTURE_ENHANCEMENTS.md`](../../src/gpu/FUTURE_ENHANCEMENTS.md)

## Header Files

| Header | Primary Class / Interface |
|--------|--------------------------|
| `gpu_module.h` | `GPUModule` — integration facade for policy, allocation, execution |
| `gpu_memory_manager.h` | `GPUMemoryManager` — VRAM allocation and quota management |
| `memory_pool.h` | `MemoryPool` — slab/pool allocation and fragmentation control |
| `device_discovery.h` | `DeviceDiscovery` — CUDA/ROCm device enumeration and probing |
| `stream_manager.h` | `StreamManager` — stream lifecycle and synchronization |
| `launcher.h` | `Launcher` — execution dispatch and backend launch |
| `safe_fail.h` | `SafeFail` — circuit-breaker and GPU-to-CPU fallback |
| `query_accelerator.h` | `QueryAccelerator` — accelerated query operations |
| `training_loop.h` | `TrainingLoop` — accelerated training orchestration |
| `tensor_buffer.h` | `TensorBuffer` — tensor memory layout and transfer |
| `rocm_backend.h` | `ROCmBackend` — ROCm/HIP backend adapter |
| `vulkan_backend.h` | `VulkanBackend` — Vulkan backend adapter |
| `cluster_coordinator.h` | `ClusterCoordinator` — multi-GPU/multi-node coordination |
| `cluster_topology.h` | `ClusterTopology` — topology-aware scheduling metadata |
| `mig_manager.h` | `MIGManager` — MIG partition management |
| `gpu_metrics.h` | `GPUMetrics` — GPU metrics and telemetry |
| `gpu_profiler.h` | `GPUProfiler` — profiling markers and measurement |

## Usage

```cpp
#include "gpu/gpu_module.h"

auto gpu_module = themis::gpu::createGPUModule({
    .backends = {GPU_BACKEND_CUDA, GPU_BACKEND_ROCM},
    .fallback_enabled = true,
    .device_limit = 4
});

auto device = gpu_module->getDevice(0);
auto stream = device->createStream();
gpu_module->launch(kernel, args, stream);
```

For full runtime usage examples (device management, execution, fallback), see [`../../src/gpu/README.md`](../../src/gpu/README.md).

## Key Configuration Surface

Important configuration entry points are declared in:

- `gpu_module.h` (`GPUModule::Config` for backend selection and policy)
- `gpu_memory_manager.h` (`GPUMemoryManager::Config` for quota and allocation)
- `safe_fail.h` (`SafeFail::Config` for fallback behavior)
- `stream_manager.h` (stream configuration and lifecycle)
- `cluster_coordinator.h` (multi-node coordination settings)

## Build

```cmake
cmake --preset release && cmake --build build/release --target themis-gpu
```

## See Also

- [`../../src/gpu/README.md`](../../src/gpu/README.md) — implementation details
- [`../../src/gpu/GPU_CONTRACT.md`](../../src/gpu/GPU_CONTRACT.md) — GPU execution contract
- [`../../src/query/README.md`](../../src/query/README.md) — query execution integration

## Installation

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
