# Acceleration: Capability Negotiation and Fallback Behavior

This document explains how ThemisDB selects hardware-acceleration backends at
runtime, how it falls back gracefully when a preferred backend is unavailable or
fails, and how to configure these mechanisms for production deployments.

## Table of Contents

1. [Overview](#overview)
2. [Backend Capability Matrix](#backend-capability-matrix)
3. [Capability Requirements](#capability-requirements)
4. [Runtime Backend Selection](#runtime-backend-selection)
5. [Static Fallback Chain](#static-fallback-chain)
6. [Kernel-Level Fallback Dispatcher](#kernel-level-fallback-dispatcher)
7. [Retry Policy and Transient Error Handling](#retry-policy-and-transient-error-handling)
8. [Backend Health Monitoring](#backend-health-monitoring)
9. [Configuration Guide](#configuration-guide)
10. [Operational Troubleshooting](#operational-troubleshooting)

---

## Overview

ThemisDB acceleration supports multiple compute backends (CUDA, HIP, Vulkan,
CPU, and others). At startup the system performs **capability negotiation**: it
discovers which backends are actually available on the host, evaluates each one
against a set of *capability requirements*, and selects the highest-priority
candidate that satisfies all requirements for each operation category (vector
ANN, graph traversal, geospatial).

If no GPU backend is available, or if an accelerator fails to initialize, the
system **falls back** to CPU backends automatically — no crash, no hard error.
This guarantee holds at two levels:

1. **Registry-level fallback** — `BackendRegistry` walks a priority-ordered
   fallback chain and returns the first backend that satisfies the requirements.
2. **Kernel-level fallback** — `ANNKernelFallbackDispatcher` and
   `GeoKernelFallbackDispatcher` wrap a primary (GPU) kernel dispatch table and
   a fallback (CPU) table, routing unsupported or transiently failing operations
   to the CPU path.
3. **Probe-level fallback** — `DeviceManager` caches the latest capability
   snapshot and emits a synthetic `CPU Fallback` device when discovery yields
   no accelerator entries.

---

## Backend Capability Matrix

Every backend implements `IComputeBackend::getCapabilities()`, which returns a
`BackendCapabilities` struct describing what the backend supports:

```cpp
struct BackendCapabilities {
    // Operation support
    bool supportsVectorOps    = false;  // ANN / vector similarity search
    bool supportsGraphOps     = false;  // BFS, shortest-path traversal
    bool supportsGeoOps       = false;  // Haversine distance, point-in-polygon
    bool supportsMatrixOps    = false;  // FP16/BF16 matrix multiply (Tensor Core)
    bool supportsBatchProcessing = false;
    bool supportsAsync        = false;

    // Precision bitmask (OR of PrecisionMode flags)
    PrecisionMode supportedPrecisions = PrecisionMode::NONE;

    // Distance-metric bitmask (OR of metricBit(DistanceMetric::*))
    uint32_t supportedMetrics = 0;

    // Device info
    size_t      maxMemoryBytes = 0;
    int         computeUnits   = 0;
    std::string deviceName;
    std::string vendorName;  // "NVIDIA", "AMD", "Intel", "" for CPU
};
```

### Default CPU backend capabilities

| Field | CPUVectorBackend | CPUGraphBackend | CPUGeoBackend |
|-------|-----------------|-----------------|---------------|
| `supportsVectorOps` | ✅ | ❌ | ❌ |
| `supportsGraphOps` | ❌ | ✅ | ❌ |
| `supportsGeoOps` | ❌ | ❌ | ✅ |
| `supportedPrecisions` | FP32 | FP32 | FP32 |
| `supportedMetrics` | L2 \| COSINE \| INNER_PRODUCT | — | — |
| `supportsAsync` | ❌ | ❌ | ❌ |

### CUDA backend capabilities

| Field | CUDAVectorBackend | CUDAGraphBackend | CUDAGeoBackend | CUDAMatrixBackend |
|-------|-------------------|------------------|----------------|-------------------|
| `supportsVectorOps` | ✅ | ❌ | ❌ | ❌ |
| `supportsGraphOps` | ❌ | ✅ | ❌ | ❌ |
| `supportsGeoOps` | ❌ | ❌ | ✅ | ❌ |
| `supportsMatrixOps` | ❌ | ❌ | ❌ | ✅ |
| `supportsBatchProcessing` | ✅ | ❌ | ✅ | ✅ |
| `supportsAsync` | ✅ | ❌ | ✅ | ✅ |
| `supportedPrecisions` | FP32 | — | FP32 | FP32 \| FP16 \| BF16 |
| `supportedMetrics` | L2 \| COSINE \| INNER_PRODUCT | — | — | — |
| Build flag | `THEMIS_ENABLE_CUDA` | `THEMIS_ENABLE_CUDA` | `THEMIS_ENABLE_CUDA` | `THEMIS_ENABLE_CUDA` |
| Stability | Stub (falls back to CPU) | Stub | Stub (falls back to CPU) | Production |

> **Note:** `CUDAMatrixBackend` requires SM 7.0+ for FP16 and SM 8.0+ (Ampere) for BF16 Tensor Core acceleration.

### HIP (AMD ROCm) backend capabilities

| Field | HIPVectorBackend | HIPGeoBackend |
|-------|------------------|---------------|
| `supportsVectorOps` | ✅ | ❌ |
| `supportsGraphOps` | ❌ | ❌ |
| `supportsGeoOps` | ❌ | ✅ |
| `supportsBatchProcessing` | ✅ | ✅ |
| `supportsAsync` | ✅ | ✅ |
| `supportedPrecisions` | FP32 | FP32 |
| `supportedMetrics` | L2 \| COSINE \| INNER_PRODUCT | — |
| Build flag | `THEMIS_ENABLE_HIP` | `THEMIS_ENABLE_HIP` |
| Stability | Production | Production |

### Vulkan backend capabilities

| Field | VulkanVectorBackend | VulkanGeoBackend |
|-------|---------------------|------------------|
| `supportsVectorOps` | ✅ | ❌ |
| `supportsGraphOps` | ❌ | ❌ |
| `supportsGeoOps` | ❌ | ✅ |
| `supportsBatchProcessing` | ✅ | ✅ |
| `supportsAsync` | ✅ | ✅ |
| `supportedPrecisions` | FP32 | FP32 |
| `supportedMetrics` | L2 \| COSINE \| INNER_PRODUCT | — |
| Build flag | `THEMIS_ENABLE_VULKAN` | `THEMIS_ENABLE_VULKAN` |
| Stability | Production | Production |

### Other GPU / compute backends

| Field | ZLUDAVectorBackend | DirectXVectorBackend | OpenGLVectorBackend | OpenCLVectorBackend |
|-------|--------------------|----------------------|---------------------|---------------------|
| `supportsVectorOps` | ✅ | ✅ | ✅ | ✅ (when initialized) |
| `supportsBatchProcessing` | ✅ | ✅ | ✅ | ❌ |
| `supportsAsync` | ✅ | ✅ | ❌ | ❌ |
| `supportedPrecisions` | FP32 | — | — | — |
| `supportedMetrics` | L2 \| COSINE \| INNER_PRODUCT | — | — | — |
| Build flag | `THEMIS_ENABLE_ZLUDA` | `THEMIS_ENABLE_DIRECTX` (Windows only) | `THEMIS_ENABLE_OPENGL` | `THEMIS_ENABLE_OPENCL` |
| Stability | Experimental | Stub | Stub | Minimal |

> **Fallback chain priority:** CUDA → HIP → ZLUDA → VULKAN → DIRECTX → ROCM → ONEAPI → METAL → OPENCL → OPENGL → WEBGPU → CPU

### PrecisionMode flags

`PrecisionMode` is a bitmask (`uint32_t` underlying type). Flags are combined
with bitwise OR and tested with `hasPrecision()`:

```cpp
PrecisionMode combined = PrecisionMode::FP32 | PrecisionMode::FP16;
bool hasFP32 = hasPrecision(combined, PrecisionMode::FP32); // true
bool hasBF16 = hasPrecision(combined, PrecisionMode::BF16); // false
```

| Flag | Value | Meaning |
|------|-------|---------|
| `NONE` | 0 | No precision declared (default / not applicable) |
| `FP32` | bit 0 | 32-bit IEEE 754 single precision |
| `FP16` | bit 1 | 16-bit IEEE 754 half precision |
| `BF16` | bit 2 | bfloat16 |
| `INT8` | bit 3 | 8-bit integer quantisation |

### DistanceMetric bitmask

Distance metrics are represented as bitmask positions via `metricBit()`:

```cpp
uint32_t required = metricBit(DistanceMetric::L2)
                  | metricBit(DistanceMetric::COSINE);
bool supported = (caps.supportedMetrics & required) == required;
```

---

## Capability Requirements

The caller declares what the chosen backend **must** support using
`BackendRegistry::CapabilityRequirements`:

```cpp
struct CapabilityRequirements {
    bool needsVectorOps = false;
    bool needsGraphOps  = false;
    bool needsGeoOps    = false;
    bool needsMatrixOps = false;
    bool needsBatch     = false;
    bool needsAsync     = false;

    PrecisionMode requiredPrecisions = PrecisionMode::NONE; // bitmask
    uint32_t      requiredMetrics    = 0;                   // bitmask
};
```

Zero / `NONE` / `false` fields are **don't-care** and impose no constraint.

### Satisfies check

`BackendRegistry::satisfies(caps, reqs)` returns `true` only when the backend
declares every flag and bitmask bit that the requirements demand:

```cpp
BackendRegistry::CapabilityRequirements reqs;
reqs.needsVectorOps     = true;
reqs.requiredPrecisions = PrecisionMode::FP32;
reqs.requiredMetrics    = metricBit(DistanceMetric::L2)
                        | metricBit(DistanceMetric::COSINE);

bool ok = BackendRegistry::satisfies(backend->getCapabilities(), reqs);
```

### Default requirements

Three factory helpers produce the requirements used by `initializeRuntime()`
when the caller does not supply custom ones:

| Helper | Requirements |
|--------|--------------|
| `defaultVectorRequirements()` | `needsVectorOps=true`, `FP32`, L2 \| COSINE \| INNER_PRODUCT |
| `defaultGraphRequirements()`  | `needsGraphOps=true` |
| `defaultGeoRequirements()`    | `needsGeoOps=true`, `FP32` |

---

## Runtime Backend Selection

### DeviceManager probe bridge

`DeviceManager` is the canonical bridge between low-level hardware discovery and
registry-level backend selection:

- production path: call `themis::gpu::DeviceDiscovery::Enumerate()`
- focused validation path: call `DeviceManager::setEnumerateFn()` with an
  injected `std::vector<DeviceCapabilityInfo>`
- empty-result safety: synthesize a single `CPU Fallback` device (`index == -1`)
  so capability negotiation remains fail-closed and observable

This keeps the runtime selection code identical across production and focused
tests while allowing deterministic validation of cache reuse, refresh re-probe,
best-device selection, and CPU fallback behavior on headless CI.

### initializeRuntime()

Call `BackendRegistry::instance().initializeRuntime()` **once** during
single-threaded server startup, before spawning worker threads:

```cpp
#include "acceleration/compute_backend.h"

using namespace themis::acceleration;

// Default: selects best available backend for each operation category.
BackendRegistry::instance().initializeRuntime();

// Custom: require FP16 vector backend (falls back to nullptr if no GPU present).
BackendRegistry::CapabilityRequirements fp16reqs;
fp16reqs.needsVectorOps     = true;
fp16reqs.requiredPrecisions = PrecisionMode::FP32 | PrecisionMode::FP16;
fp16reqs.requiredMetrics    = metricBit(DistanceMetric::L2)
                            | metricBit(DistanceMetric::COSINE);

BackendRegistry::instance().initializeRuntime(
    fp16reqs,
    BackendRegistry::defaultGraphRequirements(),
    BackendRegistry::defaultGeoRequirements());
```

After this call:

```cpp
auto* vb  = BackendRegistry::instance().getSelectedVectorBackend(); // may be nullptr
auto* gb  = BackendRegistry::instance().getSelectedGraphBackend();
auto* geo = BackendRegistry::instance().getSelectedGeoBackend();

if (!vb) {
    // No backend matched the requirements; handle gracefully.
}
```

`isRuntimeInitialized()` returns `true` after the first call and `false` again
after `shutdownAll()`.

### Direct capability-driven selection

For more granular control, use the `selectXBackendFor()` methods:

```cpp
auto* vb = BackendRegistry::instance().selectVectorBackendFor(reqs);
auto* gb = BackendRegistry::instance().selectGraphBackendFor(reqs);
auto* geo = BackendRegistry::instance().selectGeoBackendFor(reqs);
auto* any = BackendRegistry::instance().selectBackendFor(reqs); // any type
```

These scan the registered backends in fallback-chain order and return the first
match, or `nullptr` if none satisfy the requirements.

---

## Static Fallback Chain

`BackendRegistry::getFallbackOrder()` returns the canonical priority list used
by all `getBestXBackend()` and `selectXBackendFor()` methods:

```
CUDA → HIP → ZLUDA → VULKAN → DIRECTX → ROCM → ONEAPI → METAL → OPENCL → OPENGL → WEBGPU → CPU
```

**Key properties:**
- CUDA (NVIDIA) has the highest priority.
- CPU is always last, providing a guaranteed fallback on every platform.
- The list contains no duplicates.
- Backends not registered (e.g., because the SDK is absent) are silently skipped.

### Build-time guards

Acceleration backends are optional at build time:

| CMake flag | Effect |
|------------|--------|
| `THEMIS_ENABLE_CUDA` | Compiles CUDA sources; registers `CUDAVectorBackend` |
| `THEMIS_ENABLE_VULKAN` | Compiles Vulkan sources; registers `VulkanVectorBackend` |
| (neither set) | CPU-only build; all operations fall through to CPU backends |

When these flags are **OFF**, `BackendRegistry` still initializes successfully
and CPU backends handle all requests.

---

## Kernel-Level Fallback Dispatcher

In addition to registry-level fallback, individual kernel slots can fall back
per-operation using the dispatcher classes in
`include/acceleration/kernel_fallback_dispatcher.h`.

### ANNKernelFallbackDispatcher

Wraps a primary (GPU) and a fallback (CPU) `ANNKernelDispatch` table:

```cpp
#include "acceleration/kernel_fallback_dispatcher.h"
#include "acceleration/cpu_backend.h"

CPUVectorBackend cpu;
cpu.initialize();
ANNKernelDispatch cpuTable = cpu.populateANNDispatch();
ANNKernelDispatch gpuTable = gpuBackend.populateANNDispatch();

// Optional: customise retry behaviour for transient GPU errors.
RetryPolicy policy;
policy.maxAttempts       = 3;
policy.initialDelayMs    = 5;
policy.maxDelayMs        = 100;
policy.backoffMultiplier = 2.0f;

ANNKernelFallbackDispatcher dispatcher(gpuTable, cpuTable, policy);

// Launch with automatic fallback / retry:
int rc = dispatcher.launchL2Distance(q, v, dists, nq, nv, dim, stream);
// rc == 0 on success; non-zero AccelerationErrorCode on failure.
```

**Dispatch logic for each operation:**

1. If the primary slot is `nullptr` → delegate immediately to fallback slot.
2. If the primary slot returns 0 (success) → done.
3. If the primary slot returns a **transient error** (see below) → sleep and retry,
   up to `policy.maxAttempts` times.
4. If retries are exhausted, or a **permanent error** occurs → fall back to CPU.
5. If the fallback slot is also `nullptr` → return
   `AccelerationErrorCode::KernelNotFound`.

### GeoKernelFallbackDispatcher

Identical semantics for the two geospatial kernel slots:

```cpp
GeoKernelFallbackDispatcher geo(gpuGeoTable, cpuGeoTable, policy);
int rc = geo.launchDistance(lats1, lons1, lats2, lons2, out, count, formula, stream);
int rc2 = geo.launchContainment(pLats, pLons, n, polygon, nv, results, stream);
```

### resolvedDispatch()

Both dispatchers expose `resolvedDispatch()`, which returns a plain
`ANNKernelDispatch` / `GeoKernelDispatch` with null primary slots replaced by
fallback slots (static resolution only, no retry):

```cpp
ANNKernelDispatch resolved = dispatcher.resolvedDispatch();
// All null slots in gpuTable are now filled from cpuTable.
// Pass 'resolved' to components that call function pointers directly.
```

---

## Retry Policy and Transient Error Handling

Transient device conditions are errors that may succeed on a subsequent attempt
without any code change:

| Error code | Value | Meaning |
|------------|-------|---------|
| `SynchronizationFailed` | 303 | GPU sync timed out (e.g., Windows TDR) |
| `OperationTimeout` | 304 | Operation deadline exceeded |
| `DeviceLost` | 305 | GPU reset or disconnected |

All other error codes are **permanent** and trigger immediate fallback without
retries.

### RetryPolicy fields

```cpp
struct RetryPolicy {
    uint32_t maxAttempts       = 3;     // total attempts (1 = no retry)
    uint32_t initialDelayMs    = 1;     // ms before first retry; 0 = no sleep
    uint32_t maxDelayMs        = 100;   // back-off upper bound in ms
    float    backoffMultiplier = 2.0f;  // multiply delay after each failure
};
```

**Exponential back-off sequence** (defaults):
- Attempt 1: invoke primary.
- Attempt 2: sleep 1 ms, invoke primary.
- Attempt 3: sleep 2 ms, invoke primary.
- → fall back to CPU.

Set `initialDelayMs = 0` in tests to disable sleeping.

---

## Backend Health Monitoring

Every backend exposes a health status via `IComputeBackend::getHealthStatus()`:

```cpp
BackendHealthStatus status = backend->getHealthStatus();
// status.status:  "healthy" | "degraded" | "unhealthy"
// status.healthy: combined liveness + readiness
// status.ready:   backend initialized and pipelines loaded
// status.alive:   driver/process reachable
// status.issues:  list of actionable issue descriptions
// status.message: human-readable summary
```

**States and their meanings:**

| Status | `healthy` | `ready` | `alive` | Cause |
|--------|-----------|---------|---------|-------|
| `healthy` | ✅ | ✅ | ✅ | Fully operational |
| `degraded` | ❌ | ❌ | ✅ | Driver reachable but last operation failed |
| `unhealthy` | ❌ | ❌ | ❌ | Backend unavailable (driver missing, device lost) |

The default implementation in `IComputeBackend` derives status from
`isAvailable()` and `getLastError()`. GPU backends may override it to report
device temperature, memory pressure, or driver version.

Use health status for readiness probes in container environments:

```cpp
auto* vb = BackendRegistry::instance().getSelectedVectorBackend();
if (vb) {
    auto h = vb->getHealthStatus();
    if (!h.healthy) {
        LOG_WARN("Vector backend degraded: " + h.message);
        // Optionally re-run initializeRuntime() to re-select.
    }
}
```

---

## Configuration Guide

### Minimal startup (CPU-only deployment)

No extra configuration needed. `BackendRegistry` auto-registers CPU backends
and `initializeRuntime()` selects them by default:

```cpp
BackendRegistry::instance().initializeRuntime();
// All operations use CPUVectorBackend / CPUGraphBackend / CPUGeoBackend.
```

### GPU deployment (CUDA preferred, CPU fallback)

```cpp
// 1. Compile with -DTHEMIS_ENABLE_CUDA=ON.
// 2. At startup:
BackendRegistry::instance().initializeRuntime();
// If CUDA device is present, CUDAVectorBackend is selected for vector ops.
// If not (CI, CPU-only server), CPUVectorBackend is used automatically.
```

### Require GPU — fail if unavailable

```cpp
BackendRegistry::CapabilityRequirements gpuReqs;
gpuReqs.needsVectorOps  = true;
gpuReqs.needsAsync      = true; // CPU backends set supportsAsync=false
gpuReqs.requiredPrecisions = PrecisionMode::FP32;

BackendRegistry::instance().initializeRuntime(
    gpuReqs,
    BackendRegistry::defaultGraphRequirements(),
    BackendRegistry::defaultGeoRequirements());

auto* vb = BackendRegistry::instance().getSelectedVectorBackend();
if (!vb) {
    throw std::runtime_error("No async-capable GPU vector backend found");
}
```

### FP16 / Tensor Core path

```cpp
BackendRegistry::CapabilityRequirements fp16reqs;
fp16reqs.needsVectorOps     = true;
fp16reqs.needsMatrixOps     = true;
fp16reqs.requiredPrecisions = PrecisionMode::FP32 | PrecisionMode::FP16;

BackendRegistry::instance().initializeRuntime(
    fp16reqs,
    BackendRegistry::defaultGraphRequirements(),
    BackendRegistry::defaultGeoRequirements());
// Returns nullptr if no Tensor Core-capable GPU is registered.
```

### Kernel-level fallback with custom retry

```cpp
RetryPolicy aggressive;
aggressive.maxAttempts       = 5;
aggressive.initialDelayMs    = 10;
aggressive.maxDelayMs        = 500;
aggressive.backoffMultiplier = 1.5f;

ANNKernelFallbackDispatcher d(gpuTable, cpuTable, aggressive);
```

---

## Operational Troubleshooting

### CPU always selected despite GPU being installed

**Cause:** GPU backend not registered (build flag off or SDK absent).

**Check:**
```bash
# Verify CUDA library is present
ldconfig -p | grep libcuda
# Verify the binary was compiled with CUDA support
strings themisdb | grep THEMIS_ENABLE_CUDA
```

**Resolution:**
- Rebuild with `-DTHEMIS_ENABLE_CUDA=ON`.
- Ensure the NVIDIA driver is installed: `nvidia-smi`.

---

### `getSelectedVectorBackend()` returns `nullptr`

**Cause:** The capability requirements passed to `initializeRuntime()` cannot
be satisfied by any registered backend.

**Resolution:** Relax requirements or use the defaults:
```cpp
// Check which backend would be selected with default requirements:
auto* fallback = BackendRegistry::instance().selectVectorBackendFor(
    BackendRegistry::defaultVectorRequirements());
// If this is non-null, your custom requirements are too strict.
```

---

### Kernel dispatcher falling back to CPU unexpectedly

**Possible causes:**

| Symptom | Likely cause |
|---------|--------------|
| GPU kernel slot is null | CUDA kernels not yet compiled (stub build) |
| Transient error on every call | GPU under thermal throttle or driver TDR |
| Permanent error (e.g., 301 KernelLaunchFailed) | Invalid kernel arguments |

**Steps:**
1. Call `backend->getLastError()` after the operation to retrieve structured
   error context including a troubleshooting hint.
2. Increase `RetryPolicy::maxAttempts` and `maxDelayMs` for transient GPU
   instability.
3. Check GPU thermals: `nvidia-smi --query-gpu=temperature.gpu --format=csv`.
4. See [error_codes.md](error_codes.md) for per-code resolution steps.

---

### Vulkan backend not selected on non-NVIDIA hardware

**Cause:** Vulkan ICD not installed or `THEMIS_ENABLE_VULKAN` not set.

**Check:**
```bash
vulkaninfo --summary   # Linux / macOS
# Windows: vulkaninfo.exe
```

**Resolution:**
- Install Vulkan ICD for your vendor (Mesa RADV for AMD, Intel ANV, MoltenVK
  for macOS).
- Rebuild with `-DTHEMIS_ENABLE_VULKAN=ON`.

---

### Degraded health status in production

When `getHealthStatus().status == "degraded"`:

1. Read `status.issues` for actionable descriptions.
2. Check GPU driver logs: `journalctl -k | grep -i nvidia` (Linux).
3. Call `BackendRegistry::instance().initializeRuntime()` again to re-run
   detection — this re-evaluates all backends and may promote a healthy
   alternative.
4. If the issue is transient (temperature, PCIe bandwidth spike), increase
   `RetryPolicy::maxAttempts` for the kernel dispatcher.

---

## See Also

- [error_codes.md](error_codes.md) — full error code reference with per-code
  troubleshooting steps.
- [production_readiness.md](production_readiness.md) — production deployment
  checklist.
- `src/acceleration/README.md` — module overview, build flags, and directory
  layout.
- `include/acceleration/compute_backend.h` — `BackendCapabilities`,
  `CapabilityRequirements`, `BackendRegistry` API.
- `include/acceleration/kernel_fallback_dispatcher.h` — `RetryPolicy`,
  `ANNKernelFallbackDispatcher`, `GeoKernelFallbackDispatcher`.
