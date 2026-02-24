# ThemisDB Acceleration API — Production-Readiness Sign-Off

**Document Version:** 1.0  
**Date:** 2026-02-24  
**Milestone:** Q4 2026  
**Status:** ✅ Signed Off — Production Ready  
**API Version Signed Off:** `BACKEND_CONTRACT_VERSION = 100` (v1.0)  
**Sign-Off Constant:** `BACKEND_SIGNOFF_VERSION = 100`

---

## Executive Summary

This document records the formal production-readiness review and API stability
sign-off for the ThemisDB acceleration module at contract version 1.0
(`BACKEND_CONTRACT_VERSION = 100`).

All interfaces listed in the **Stable API Surface** section below are declared
frozen. Additive changes (new fields, new enum values) are allowed; breaking
changes (removed or renamed symbols, changed virtual signatures) require a new
formal review and a `BACKEND_CONTRACT_VERSION` / `BACKEND_SIGNOFF_VERSION` bump.

---

## Review Scope

- Module: `src/acceleration/` and `include/acceleration/`
- Interfaces reviewed: all public headers listed in **Stable API Surface**
- Test suite: `tests/test_backend_api_stability.cpp` (compile-time + runtime)
- Related reviews: production-readiness assessment, security audit, RAII review

---

## Stable API Surface

The following types and constants are declared stable at v1.0:

### Version Constants (`compute_backend.h`)

| Constant | Value | Description |
|---|---|---|
| `BACKEND_CONTRACT_VERSION` | 100 | API contract version (major×100 + minor) |
| `KERNEL_INVOCATION_INTERFACE_VERSION` | 100 | Kernel launcher typedef version |
| `BACKEND_SIGNOFF_VERSION` | 100 | Version at which formal sign-off was granted |

### Enumerations

| Enum | Underlying Type | Frozen Values |
|---|---|---|
| `DistanceMetric` | `uint32_t` | `L2=0`, `COSINE=1`, `INNER_PRODUCT=2` |
| `GeoDistanceFormula` | `uint32_t` | `HAVERSINE=0`, `VINCENTY=1` |
| `MatrixPrecision` | `uint32_t` | `FP32=0`, `FP16=1`, `BF16=2` |
| `PrecisionMode` | `uint32_t` (bitmask) | `NONE=0`, `FP32=1`, `FP16=2`, `BF16=4`, `INT8=8` |
| `AccelerationErrorCode` | `uint32_t` | 33 codes; `Success=0`, `NotImplemented=902`, `InputRangeViolation=605`, `KernelNotFound=502` (see `error_codes.h`) |

### Kernel Parameter Structs (`kernel_invocation.h`)

| Struct | Required Fields |
|---|---|
| `ANNKernelParams` | `queries`, `numQueries`, `dim`, `vectors`, `numVectors`, `topK`, `metric` |
| `ANNKernelResult` | `indices`, `distances` |
| `GeoKernelParams` | `latitudes1`, `longitudes1`, `latitudes2`, `longitudes2`, `count`, `formula` |
| `GeoContainmentParams` | `pointLats`, `pointLons`, `numPoints`, `polygonCoords`, `numPolygonVertices` |
| `MatrixKernelParams` | `A`, `B`, `C`, `M`, `K`, `N`, `alpha`, `beta`, `precision` |

### Dispatch Tables (`kernel_invocation.h`)

| Struct | Frozen Slots |
|---|---|
| `ANNKernelDispatch` | `launchL2Distance`, `launchCosine`, `launchInnerProduct`, `launchTopK`, `distanceLauncherFor()` |
| `GeoKernelDispatch` | `launchDistance`, `launchContainment` |
| `MatrixKernelDispatch` | `launchMatmul` |

### Core Structs (`compute_backend.h`)

| Struct | Frozen Fields |
|---|---|
| `BackendCapabilities` | `supportsVectorOps`, `supportsGraphOps`, `supportsGeoOps`, `supportsMatrixOps`, `supportsBatchProcessing`, `supportsAsync`, `supportedPrecisions`, `supportedMetrics`, `maxMemoryBytes`, `computeUnits`, `deviceName`, `vendorName` |
| `BackendHealthStatus` | `status`, `healthy`, `ready`, `alive`, `message`, `issues`, `deviceName`, `driverInfo`, `memoryUsedBytes`, `memoryAvailableBytes`; builders `makeHealthy()`, `makeDegraded()`, `makeUnhealthy()` |
| `KnnQueryResult` | `neighbors`, `status`, `errorMessage` |
| `PartialBatchResult` | `queryResults`, `successCount`, `failureCount` |

### Virtual Interfaces (`compute_backend.h`)

| Interface | Frozen Virtual Methods |
|---|---|
| `IComputeBackend` | `name()`, `type()`, `isAvailable()`, `getCapabilities()`, `initialize()`, `shutdown()`, `getLastError()`, `getHealthStatus()` |
| `IVectorBackend` | Inherits `IComputeBackend` + `computeDistances()`, `batchKnnSearch()`, `batchKnnSearchSafe()`, `populateANNDispatch()` |
| `IGeoBackend` | Inherits `IComputeBackend` + `batchDistances()`, `batchPointInPolygon()`, `populateGeoDispatch()` |
| `IGraphBackend` | Inherits `IComputeBackend` + `batchBFS()`, `batchShortestPath()` |
| `IMatrixBackend` | Inherits `IComputeBackend` + `matmul()`, `populateMatrixDispatch()` |

### Registry API (`compute_backend.h`)

| Symbol | Stability |
|---|---|
| `BackendRegistry::CapabilityRequirements` (all fields) | Frozen |
| `BackendRegistry::satisfies()` | Frozen |
| `BackendRegistry::getFallbackOrder()` | Frozen |
| `BackendRegistry::selectBackendFor()` and variants | Frozen |
| `BackendRegistry::initializeRuntime()` and default requirements | Frozen |

### Fallback Dispatcher (`kernel_fallback_dispatcher.h`)

| Symbol | Frozen Values |
|---|---|
| `RetryPolicy::maxAttempts` default | 3 |
| `RetryPolicy::initialDelayMs` default | 1 |
| `RetryPolicy::maxDelayMs` default | 100 |
| `RetryPolicy::backoffMultiplier` default | 2.0f |
| `isTransientDispatchError()` — transient codes | `SynchronizationFailed`, `OperationTimeout`, `DeviceLost` |

---

## Production-Readiness Checklist

| Criterion | Status | Notes |
|---|---|---|
| L2 distance consistency across all backends | ✅ Pass | All backends use squared distance (no sqrt) |
| Structured error logging on GPU init failure | ✅ Pass | All GPU backends log device info on failure |
| RAII resource management | ✅ Pass | CUDA, HIP, OpenCL backends use RAII wrappers |
| Error code coverage (33 codes) | ✅ Pass | Full enumeration in `error_codes.h`; all tested |
| API stability tests (compile-time + runtime) | ✅ Pass | `test_backend_api_stability.cpp` — 30+ assertions |
| Capability contract tests | ✅ Pass | `test_backend_capability_contract.cpp` |
| Backend consistency tests | ✅ Pass | `test_backend_consistency.cpp` |
| Regression tests (invalid input, fallback) | ✅ Pass | `test_acceleration_regression.cpp` |
| Shader integrity verification | ✅ Pass | `ShaderIntegrityVerifier` — SPIR-V hash enforcement |
| Plugin security hardening | ✅ Pass | RTLD_NOW, permission checks, macOS code signing |
| CPU fallback — no GPU required | ✅ Pass | All tests pass on CPU-only CI runners |
| `BACKEND_SIGNOFF_VERSION` matches contract | ✅ Pass | Both equal 100 |

---

## Known Limitations at Sign-Off

The following items are noted as known limitations at v1.0. They do not block
production use of the CPU-path and CUDA/HIP primary paths but should be
addressed in follow-up milestones:

1. **GPU kernel APIs not yet stable (GPU-path v1.0)**: CUDA ANN and Geo
   kernels are functional but GPU-path performance targets (≥8× vs CPU baseline)
   depend on hardware availability for benchmarking.
2. **Vulkan/Metal/OpenCL — optional backends**: These backends compile and
   fall back gracefully but are classified as "experimental" pending full
   hardware CI coverage.
3. **Error injection framework**: Deferred to a future phase; all normal and
   invalid-input error paths are covered by existing tests.
4. **Grafana dashboards**: Prometheus metrics are exported; dashboard
   configuration is environment-specific and not bundled.

---

## Sign-Off Decision

**Decision: GO ✅**

The acceleration module at `BACKEND_CONTRACT_VERSION = 100` (v1.0) is approved
for production deployment. CPU and primary GPU (CUDA/HIP) paths meet all
stability, correctness, and error-handling requirements. The API surface is
frozen; any breaking change must undergo a new formal review.

---

## Roadmap Reference

- Source roadmap: `roadmap.md`
- Production readiness assessment: `docs/acceleration/production_readiness.md`
- Phase completion: Phases 1–4 complete; Phase 6.3 (go/no-go) ✅ complete
