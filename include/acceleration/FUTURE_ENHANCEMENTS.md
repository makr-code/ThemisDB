# Acceleration Module - Future Header Enhancements

## Scope

- `IComputeBackend` interface extensions for CUDA and Vulkan backends
- `THEMIS_ENABLE_GPU` compile-time guards controlling GPU API visibility in public headers
- Device capability query API (`IDeviceCapabilityQuery`) for runtime feature negotiation
- Multi-GPU selection interface (`IMultiGPUSelector`) for workload distribution across devices
- Async compute dispatch API for non-blocking kernel submission and result collection
- Opaque GPU handle types exposed via forward declarations to insulate callers from backend ABI

## Design Constraints

- `[ ]` All GPU APIs are compile-time optional; `THEMIS_ENABLE_GPU` must be defined to expose GPU-specific symbols
- `[ ]` `IComputeBackend` interface must never throw; all errors returned via `Result<T>` or error-code out-parameters
- `[ ]` All GPU handle types (`DeviceHandle`, `BufferHandle`, `StreamHandle`) are opaque to callers; no raw CUDA/Vulkan types in public headers
- `[ ]` Device selection API must be thread-safe and reentrant; no global mutable state in the interface layer
- `[ ]` Async dispatch interface must support cancellation tokens compatible with the engine's cancellation framework
- `[ ]` All deprecated GPU interfaces must be removed via a `THEMIS_DEPRECATED` macro before the next major version

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `IComputeBackend` | `VectorIndex`, `SearchEngine`, `EmbeddingPipeline` | Base interface for CUDA and Vulkan backends; pure-virtual, never throws |
| `IDeviceCapabilityQuery` | `QueryPlanner`, `AccelerationManager` | Returns device feature flags; callable before device initialization |
| `IMultiGPUSelector` | `AccelerationManager`, `ShardingLayer` | Selects target device(s) per workload; must be thread-safe |
| `IAsyncComputeDispatch` | `SearchEngine`, `EmbeddingPipeline` | Non-blocking kernel submission; returns `ComputeFuture<T>` |
| `IKernelRegistry` | `AccelerationManager` | Registers and resolves named compute kernels at startup |

## Planned Features

### CUDA Kernel Interface for Vector Similarity

- `[ ]` Add `IComputeBackend::submitSimilarityKernel(BatchDescriptor, KernelConfig)` to public header
- `[ ]` Expose `KernelConfig` as a plain-data struct (no CUDA headers required at include site)
- `[ ]` Define `SimilarityKernelResult` value type returned via `ComputeFuture<SimilarityKernelResult>`
- `[ ]` Document FP tolerance guarantee (≤ 1e-6 vs. CPU baseline) in header Doxygen

### Vulkan Compute Backend Interface

- `[ ]` Define `IVulkanComputeBackend : IComputeBackend` with Vulkan-specific pipeline config struct
- `[ ]` Expose pipeline descriptor as an opaque handle; no raw `VkPipeline` in public headers
- `[ ]` Add `THEMIS_ENABLE_VULKAN` compile guard nested within `THEMIS_ENABLE_GPU`
- `[ ]` Provide `VulkanDeviceInfo` POD struct queryable without a live Vulkan instance

### Multi-GPU Load Balancer API

- `[ ]` Define `IMultiGPUSelector` with `selectDevices(WorkloadDescriptor) -> DeviceSet`
- `[ ]` Add `DeviceSet` as a small-vector value type (max 8 devices) in public header
- `[ ]` Expose `WorkloadDescriptor` with byte-size, FLOP estimate, and latency-class fields
- `[ ]` Document thread-safety guarantee in header: callable concurrently from N threads

### Device Capability Negotiation API

- `[ ]` Define `IDeviceCapabilityQuery::queryCapabilities(DeviceHandle) -> DeviceCapabilityFlags`
- `[ ]` Expose `DeviceCapabilityFlags` as a strongly-typed bitmask enum class
- `[ ]` Add `queryAll() -> std::vector<DeviceCapabilityFlags>` for enumeration at startup
- `[ ]` Ensure all query methods are callable before CUDA/Vulkan context creation

### Async Compute Dispatch Interface

- `[ ]` Define `IAsyncComputeDispatch::submit(KernelDescriptor, CancellationToken) -> ComputeFuture<T>`
- `[ ]` Expose `ComputeFuture<T>` with `then()`, `get()`, and `cancel()` in a separate `compute_future.h`
- `[ ]` Guarantee that `cancel()` is safe to call after kernel completion (no-op)
- `[ ]` Add `DispatchStats` struct to `ComputeFuture<T>` for latency introspection

## Test Strategy

- Compile public headers with `THEMIS_ENABLE_GPU=0` and verify zero GPU symbols leak into translation units
- Mock `IComputeBackend` in unit tests; assert no exceptions escape any interface method
- Fuzz `DeviceCapabilityFlags` bitmask combinations to verify no undefined enum values are accepted
- Integration tests instantiate both CUDA and Vulkan backends against the same `IComputeBackend` contract
- Thread-safety tests spawn 32 concurrent threads calling `IMultiGPUSelector::selectDevices` with random descriptors
- Header-only compile tests (no link) verify every public struct is trivially copyable or explicitly documented otherwise

## Performance Targets

- Kernel dispatch overhead via `IAsyncComputeDispatch::submit` ≤ 2 µs on the calling thread (measured on x86-64, GCC -O2)
- `IDeviceCapabilityQuery::queryCapabilities` cold-call ≤ 100 µs; warm-call (cached) ≤ 1 µs
- `IMultiGPUSelector::selectDevices` ≤ 500 µs for a cluster of up to 8 GPUs
- `ComputeFuture<T>::get()` spin-wait overhead ≤ 50 ns when result is already ready
- Header parse time (unity build, no precompiled headers) ≤ 200 ms for the full `acceleration/` include tree
- `DeviceSet` construction for 8-device selection ≤ 100 ns (stack-allocated small-vector)

## Security / Reliability

- GPU buffers allocated through `IComputeBackend` must be zeroized on deallocation; interface contract documented in header
- No raw pointer types (`void*`, `CUdeviceptr`, `VkBuffer`) exposed in public headers; all handles are opaque typed wrappers
- Compute backend operations restricted to an allow-list of named kernel identifiers; unknown kernels rejected at dispatch time
- `THEMIS_ENABLE_GPU` guard prevents accidental linkage of GPU code in security-sensitive build configurations
- `CancellationToken` in async dispatch prevents runaway kernel execution beyond configured deadlines
- `DeviceCapabilityFlags` bitmask validates against a known-valid mask; unknown bits trigger `Result::error()` rather than silent acceptance
