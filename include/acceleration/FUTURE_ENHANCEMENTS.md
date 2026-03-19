# Acceleration Module - Future Header Enhancements

## Scope

- `IComputeBackend` interface extensions for CUDA and Vulkan backends
- `THEMIS_ENABLE_GPU` compile-time guards controlling GPU API visibility in public headers
- Device capability query API (`IDeviceCapabilityQuery`) for runtime feature negotiation
- Multi-GPU selection interface (`IMultiGPUSelector`) for workload distribution across devices
- Async compute dispatch API for non-blocking kernel submission and result collection
- Opaque GPU handle types exposed via forward declarations to insulate callers from backend ABI

## Design Constraints

- `[x]` All GPU APIs are compile-time optional; `THEMIS_ENABLE_GPU` must be defined to expose GPU-specific symbols — pure-interface types in `compute_backend.h` include no CUDA/Vulkan SDK headers; `IVulkanComputeBackend` body guarded by `THEMIS_ENABLE_VULKAN` in `vulkan_backend.h`
- `[x]` `IComputeBackend` interface must never throw; all errors returned via `Result<T>` or error-code out-parameters — `submitSimilarityKernel()` default implementation returns `make_ready()`; documented in Doxygen
- `[x]` All GPU handle types (`DeviceHandle`, `BufferHandle`, `StreamHandle`) are opaque to callers; no raw CUDA/Vulkan types in public headers — `VulkanPipelineHandle` is a plain `uint64_t` wrapper; no `VkPipeline`/`CUdeviceptr` in any public header
- `[x]` Device selection API must be thread-safe and reentrant; no global mutable state in the interface layer — `IMultiGPUSelector::selectDevices()` is documented as safe for concurrent calls from N threads; no mutable state in the interface
- `[x]` Async dispatch interface must support cancellation tokens compatible with the engine's cancellation framework — `CancellationToken` (shared `atomic<bool>`) in `compute_future.h`; passed through `IAsyncComputeDispatch::submit()`
- `[ ]` All deprecated GPU interfaces must be removed via a `THEMIS_DEPRECATED` macro before the next major version — no deprecated interfaces exist yet; to be enforced at the v2.0 API freeze

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

- `[x]` Add `IComputeBackend::submitSimilarityKernel(BatchDescriptor, KernelConfig)` to public header — default virtual in `include/acceleration/compute_backend.h`; returns `ComputeFuture<SimilarityKernelResult>::make_ready()` for CPU fallback
- `[x]` Expose `KernelConfig` as a plain-data struct (no CUDA headers required at include site) — `struct KernelConfig` in `compute_backend.h`; all fields are primitive C++ types
- `[x]` Define `SimilarityKernelResult` value type returned via `ComputeFuture<SimilarityKernelResult>` — `struct SimilarityKernelResult` with `indices`, `distances`, `topk`, `dim` fields in `compute_backend.h`
- `[x]` Document FP tolerance guarantee (≤ 1e-6 vs. CPU baseline) in header Doxygen — documented above `submitSimilarityKernel()` and in `SimilarityKernelResult` Doxygen in `compute_backend.h`

### Vulkan Compute Backend Interface

- `[x]` Define `IVulkanComputeBackend : IComputeBackend` with Vulkan-specific pipeline config struct — `class IVulkanComputeBackend : public IVectorBackend` in `include/acceleration/vulkan_backend.h`; guarded by `THEMIS_ENABLE_VULKAN`
- `[x]` Expose pipeline descriptor as an opaque handle; no raw `VkPipeline` in public headers — `struct VulkanPipelineHandle { uint64_t id; }` in `vulkan_backend.h`
- `[x]` Add `THEMIS_ENABLE_VULKAN` compile guard nested within `THEMIS_ENABLE_GPU` — `#ifdef THEMIS_ENABLE_VULKAN` around `IVulkanComputeBackend` class body in `vulkan_backend.h`
- `[x]` Provide `VulkanDeviceInfo` POD struct queryable without a live Vulkan instance — `struct VulkanDeviceInfo` with `device_name[256]`, `vram_bytes`, `supports_fp16/int8`, `is_discrete` in `vulkan_backend.h`

### Multi-GPU Load Balancer API

- `[x]` Define `IMultiGPUSelector` with `selectDevices(WorkloadDescriptor) -> DeviceSet` — `class IMultiGPUSelector` with pure-virtual `selectDevices(const WorkloadDescriptor&) const` in `compute_backend.h`
- `[x]` Add `DeviceSet` as a small-vector value type (max 8 devices) in public header — `struct DeviceSet` with `std::array<int, 8>` + `count` field in `compute_backend.h`
- `[x]` Expose `WorkloadDescriptor` with byte-size, FLOP estimate, and latency-class fields — `struct WorkloadDescriptor { size_t data_bytes; uint64_t flop_estimate; LatencyClass latency_class; }` in `compute_backend.h`
- `[x]` Document thread-safety guarantee in header: callable concurrently from N threads — documented in `IMultiGPUSelector` Doxygen

### Device Capability Negotiation API

- `[x]` Define `IDeviceCapabilityQuery::queryCapabilities(DeviceHandle) -> DeviceCapabilityFlags` — `virtual DeviceCapabilityFlags queryCapabilities(int device_index) const noexcept = 0` in `compute_backend.h`
- `[x]` Expose `DeviceCapabilityFlags` as a strongly-typed bitmask enum class — `enum class DeviceCapabilityFlags : uint32_t` with `operator|`, `operator&`, `hasCapability()` helpers in `compute_backend.h`
- `[x]` Add `queryAll() -> std::vector<DeviceCapabilityFlags>` for enumeration at startup — `virtual std::vector<DeviceCapabilityFlags> queryAll() const = 0` in `IDeviceCapabilityQuery`
- `[x]` Ensure all query methods are callable before CUDA/Vulkan context creation — `IDeviceCapabilityQuery` holds no GPU handles; pre-context callability documented in Doxygen

### Async Compute Dispatch Interface

- `[x]` Define `IAsyncComputeDispatch::submit(KernelDescriptor, CancellationToken) -> ComputeFuture<T>` — `virtual ComputeFuture<T> submit(const KernelDescriptor&, CancellationToken) = 0` in `compute_backend.h`
- `[x]` Expose `ComputeFuture<T>` with `then()`, `get()`, and `cancel()` in a separate `compute_future.h` — `include/acceleration/compute_future.h` provides all three methods
- `[x]` Guarantee that `cancel()` is safe to call after kernel completion (no-op) — documented in `CancellationToken::cancel()` Doxygen: *"Idempotent and safe to call concurrently from any thread"*
- `[x]` Add `DispatchStats` struct to `ComputeFuture<T>` for latency introspection — `struct DispatchStats` with `submit_time_ns`, `start_time_ns`, `finish_time_ns`, `queue_depth`, `from_cache` in `compute_future.h`

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
