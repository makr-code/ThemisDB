# GPU Public Headers

This directory contains GPU dispatch-contract and lower-level CUDA/resource
helpers. The module facade and most public GPU interfaces are declared under
`include/themis/gpu/`; implementation code is under `src/gpu/`.

## Public interface locations

| Concern | Canonical declaration |
|---|---|
| Module facade, initialization, work submission, accounting | [`../themis/gpu/gpu_module.h`](../themis/gpu/gpu_module.h) |
| Configuration and feature flags | [`../themis/gpu/config.h`](../themis/gpu/config.h), [`../themis/gpu/feature_flags.h`](../themis/gpu/feature_flags.h) |
| Launcher and work item | [`../themis/gpu/launcher.h`](../themis/gpu/launcher.h) |
| Edition-aware VRAM accounting and `IVRAMPolicy` | [`../themis/gpu/memory_manager.h`](../themis/gpu/memory_manager.h), [`../themis/gpu/ivram_policy.h`](../themis/gpu/ivram_policy.h) |
| Bounded dispatch error taxonomy | [`gpu_backend_dispatch_contract.h`](gpu_backend_dispatch_contract.h) |
| CUDA RAII utilities | [`cuda_raii.h`](cuda_raii.h) |

## Facade example

The facade accepts a `GPUConfig` and an optional launcher backend callback.
Feature flags and build-time backend availability are separate conditions.
`submitWork` returns status; a non-GPU success from its current fallback
callback does **not** mean that the work item was computed on the CPU.

```cpp
#include "themis/gpu/gpu_module.h"

themis::gpu::GPUModule module;
themis::gpu::GPUConfig config;
auto initialized = module.initialize(config);
if (initialized.ok) {
    themis::gpu::GPULauncher::WorkItem item{/* populate for the workload */};
    auto result = module.submitWork("trusted-caller", "tenant-id", item);
    // Verify workload output at the caller boundary; do not infer CPU
    // computation from result.submitted or result.via_gpu alone.
}
```

See [`../../src/gpu/API.md`](../../src/gpu/API.md) for facade behavior and
[`../../src/gpu/README.md`](../../src/gpu/README.md) for module boundaries.

## Build

GPU sources are included through the repository's CMake configuration when
`THEMIS_ENABLE_GPU` is enabled. Build the configured project target; there is no
standalone `themis-gpu` target documented by the current build files.

## Related documentation

- [`../../src/gpu/ARCHITECTURE.md`](../../src/gpu/ARCHITECTURE.md)
- [`GPU_CONTRACT.md`](GPU_CONTRACT.md)
- [`../../src/gpu/ROADMAP.md`](../../src/gpu/ROADMAP.md)
