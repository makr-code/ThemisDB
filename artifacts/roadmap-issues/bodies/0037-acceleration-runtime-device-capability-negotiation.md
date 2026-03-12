### Context

This issue implements the roadmap item 'Runtime Device Capability Negotiation' for the acceleration domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Runtime Device Capability Negotiation

### Goal

Deliver the scoped changes for Runtime Device Capability Negotiation in src/acceleration/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Runtime Device Capability Negotiation
**Priority:** High
**Target Version:** v1.7.0
**Status:** ✅ IMPLEMENTED

`BackendRegistry` selects backends at startup by probing device capabilities (compute capability, available VRAM, driver version) through `DeviceManager`.

**Implementation Notes:**
- `[x]` Create `device_capability_probe.cpp` / `.h`; expose `DeviceInfo` struct with `computeCapabilityMajor`, `computeCapabilityMinor`, `totalMemoryBytes`, `driverVersion`, `backendType`. — implemented as `device_manager.h` / `device_manager.cpp`; `DeviceCapabilityInfo` struct in `compute_backend.h`
- `[x]` Probe order: CUDA → HIP → Vulkan → Metal → OpenCL → CPU. — delegated to `themis::gpu::DeviceDiscovery::Enumerate()` which follows this order
- `[x]` Cache probe results for 60 s; re-probe on explicit `BackendRegistry::refresh()` call. — `DeviceManager::probeDevices()` caches for `kCacheTTL = 60 s`; `DeviceManager::refresh()` forces re-probe; `BackendRegistry::initializeRuntime()` calls `DeviceManager::refresh()`
- `[x]` Emit structured log line via `utils/logger.h` listing selected backend and device name on startup. — `DeviceManager::logDeviceInfo()` emits structured output; called from `BackendRegistry::initializeRuntime()`
- `[x]` Expose probe results via `BackendRegistry::deviceInfo()` for observability. — `BackendRegistry::deviceInfo()` returns the `DeviceCapabilityInfo` snapshot captured at `initializeRuntime()` time

**Performance Targets:**
- Probe completes in < 50 ms on a system with 4 GPUs.
- Zero false-positive backend selection failures in CI matrix covering CUDA 11.8, CUDA 12.x, ROCm 5.7, Vulkan 1.3.

### Acceptance Criteria

- [ ] Create `device_capability_probe.cpp` / `.h`; expose `DeviceInfo` struct with `computeCapabilityMajor`, `computeCapabilityMinor`, `totalMemoryBytes`, `driverVersion`, `backendType`. — implemented as `device_manager.h` / `device_manager.cpp`; `DeviceCapabilityInfo` struct in `compute_backend.h`
- [ ] Probe order: CUDA → HIP → Vulkan → Metal → OpenCL → CPU. — delegated to `themis::gpu::DeviceDiscovery::Enumerate()` which follows this order
- [ ] Cache probe results for 60 s; re-probe on explicit `BackendRegistry::refresh()` call. — `DeviceManager::probeDevices()` caches for `kCacheTTL = 60 s`; `DeviceManager::refresh()` forces re-probe; `BackendRegistry::initializeRuntime()` calls `DeviceManager::refresh()`
- [ ] Emit structured log line via `utils/logger.h` listing selected backend and device name on startup. — `DeviceManager::logDeviceInfo()` emits structured output; called from `BackendRegistry::initializeRuntime()`
- [ ] Expose probe results via `BackendRegistry::deviceInfo()` for observability. — `BackendRegistry::deviceInfo()` returns the `DeviceCapabilityInfo` snapshot captured at `initializeRuntime()` time
- [ ] Probe completes in < 50 ms on a system with 4 GPUs.
- [ ] Zero false-positive backend selection failures in CI matrix covering CUDA 11.8, CUDA 12.x, ROCm 5.7, Vulkan 1.3.

### Relationships

- Roadmap row: #37 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/acceleration/FUTURE_ENHANCEMENTS.md#runtime-device-capability-negotiation
- Source key: roadmap:37:acceleration:v1.7.0:runtime-device-capability-negotiation

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:37:acceleration:v1.7.0:runtime-device-capability-negotiation -->
<!-- roadmap-ref: row=37;module=acceleration;target=v1.7.0 -->
<!-- roadmap-detail: src/acceleration/FUTURE_ENHANCEMENTS.md#runtime-device-capability-negotiation -->
