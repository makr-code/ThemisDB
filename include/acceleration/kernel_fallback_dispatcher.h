/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            kernel_fallback_dispatcher.h                       ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 07:05:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     313                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

// =============================================================================
// ThemisDB - Kernel Fallback / Retry Dispatcher
//
// File:    include/acceleration/kernel_fallback_dispatcher.h
// Version: 1.0.0
//
// Provides fallback and retry semantics on top of the frozen kernel dispatch
// tables defined in kernel_invocation.h.
//
// Behaviour
// ---------
//  1. **Unsupported kernel (null slot):** If a slot in the primary dispatch
//     table is nullptr the corresponding fallback slot is invoked directly.
//
//  2. **Transient device error (retry):** If the primary kernel returns a
//     transient error code (DeviceLost, OperationTimeout, SynchronizationFailed)
//     the dispatcher sleeps for an exponentially increasing delay and retries
//     up to RetryPolicy::maxAttempts times.
//
//  3. **Permanent error or exhausted retries:** After a non-transient error
//     or once all retry attempts are consumed the dispatcher falls back to
//     the corresponding fallback slot.
//
// All dispatch functions return 0 on success.  Errors are returned as
// static_cast<int>(AccelerationErrorCode::XYZ) following the same convention
// used by the CPU-backend kernel adapters.
// =============================================================================

#include "acceleration/kernel_invocation.h"
#include "acceleration/error_codes.h"

#include <cstdint>
#include <thread>
#include <chrono>

namespace themis {
namespace acceleration {

// ---------------------------------------------------------------------------
// RetryPolicy
// ---------------------------------------------------------------------------

/// Controls retry behaviour when a kernel returns a transient error.
struct RetryPolicy {
    /// Total invocation attempts (primary only; 1 = no retry).
    uint32_t maxAttempts       = 3;

    /// Delay before the first retry, in milliseconds.  0 disables sleeping
    /// (useful in tests and when the caller manages its own back-off).
    uint32_t initialDelayMs    = 1;

    /// Upper bound on back-off delay, in milliseconds.
    uint32_t maxDelayMs        = 100;

    /// Multiplicative factor applied after each failed attempt.
    float    backoffMultiplier = 2.0f;
};

// ---------------------------------------------------------------------------
// Transient-error predicate
// ---------------------------------------------------------------------------

/// Returns true when @p rc represents a transient device condition that may
/// succeed on a subsequent attempt.
///
/// The following AccelerationErrorCode values (cast to int) are considered
/// transient:
///   - SynchronizationFailed (303)
///   - OperationTimeout      (304)
///   - DeviceLost            (305)
inline bool isTransientDispatchError(int rc) noexcept {
    return rc == static_cast<int>(AccelerationErrorCode::SynchronizationFailed)
        || rc == static_cast<int>(AccelerationErrorCode::OperationTimeout)
        || rc == static_cast<int>(AccelerationErrorCode::DeviceLost);
}

// ---------------------------------------------------------------------------
// Internal helper
// ---------------------------------------------------------------------------

namespace detail {

/// Invoke @p primary (when non-null) with retry/fallback semantics.
///
/// Falls back to @p fallback when:
///   - @p primary is nullptr (unsupported kernel), or
///   - @p primary exhausted all retry attempts on transient errors, or
///   - @p primary returned a permanent (non-transient) error.
///
/// All argument types must be trivially copyable (they are all pointers,
/// integers, enums, or floats so this requirement is always met in practice).
template<typename Fn, typename... Args>
int invokeWithFallback(Fn primary, Fn fallback,
                       const RetryPolicy& policy,
                       Args... args) {
    // ---- Case 1: primary slot is null → delegate immediately ----
    if (!primary) {
        if (!fallback) {
            return static_cast<int>(AccelerationErrorCode::KernelNotFound);
        }
        return fallback(args...);
    }

    // ---- Case 2: try primary with retry on transient errors ----
    uint32_t delayMs = policy.initialDelayMs;
    int rc = 0;

    const uint32_t attempts = (policy.maxAttempts >= 1) ? policy.maxAttempts : 1u;
    for (uint32_t attempt = 0; attempt < attempts; ++attempt) {
        rc = primary(args...);
        if (rc == 0) {
            return 0;  // Success.
        }

        if (!isTransientDispatchError(rc)) {
            // Permanent error: do not retry, fall through to fallback.
            break;
        }

        // Transient error: wait before retrying (if more attempts remain).
        if (attempt + 1 < attempts) {
            if (delayMs > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
            }
            const uint32_t next =
                static_cast<uint32_t>(static_cast<float>(delayMs) * policy.backoffMultiplier);
            delayMs = (next < policy.maxDelayMs) ? next : policy.maxDelayMs;
        }
    }

    // ---- Case 3: fall back ----
    if (fallback) {
        return fallback(args...);
    }
    return rc;
}

} // namespace detail

// ---------------------------------------------------------------------------
// ANNKernelFallbackDispatcher
// ---------------------------------------------------------------------------

/// Fallback/retry dispatcher for ANN (vector similarity) kernels.
///
/// Usage
/// -----
/// @code
///   CPUVectorBackend cpu;
///   cpu.initialize();
///   ANNKernelDispatch cpuTable  = cpu.populateANNDispatch();
///   ANNKernelDispatch gpuTable  = gpuBackend.populateANNDispatch();
///
///   ANNKernelFallbackDispatcher dispatcher(gpuTable, cpuTable);
///
///   // Invoke with automatic fallback / retry:
///   int rc = dispatcher.launchL2Distance(q, v, d, nq, nv, dim, stream);
/// @endcode
class ANNKernelFallbackDispatcher {
public:
    /// @param primary  Dispatch table from the preferred (GPU) backend.
    ///                 Null slots are treated as unsupported and are routed
    ///                 directly to the corresponding fallback slot.
    /// @param fallback Dispatch table from the fallback (typically CPU) backend.
    /// @param policy   Retry policy applied to transient errors from primary.
    ANNKernelFallbackDispatcher(ANNKernelDispatch primary,
                                ANNKernelDispatch fallback,
                                RetryPolicy       policy = {}) noexcept
        : primary_(primary), fallback_(fallback), policy_(policy) {}

    // -------------------------------------------------------------------------
    // resolvedDispatch()
    //
    // Returns a plain ANNKernelDispatch where each null slot in the primary
    // is replaced by the corresponding fallback slot.  The returned table
    // does NOT perform retries — it is a static resolution only.  Callers that
    // need retry behaviour should use the invoke* methods below.
    // -------------------------------------------------------------------------

    /// Returns a plain ANNKernelDispatch with static null-slot resolution.
    [[nodiscard]] ANNKernelDispatch resolvedDispatch() const noexcept {
        ANNKernelDispatch d;
        d.launchL2Distance   = primary_.launchL2Distance
                             ? primary_.launchL2Distance
                             : fallback_.launchL2Distance;
        d.launchCosine       = primary_.launchCosine
                             ? primary_.launchCosine
                             : fallback_.launchCosine;
        d.launchInnerProduct = primary_.launchInnerProduct
                             ? primary_.launchInnerProduct
                             : fallback_.launchInnerProduct;
        d.launchTopK         = primary_.launchTopK
                             ? primary_.launchTopK
                             : fallback_.launchTopK;
        return d;
    }

    // -------------------------------------------------------------------------
    // Per-kernel invocations with full retry/fallback semantics
    // -------------------------------------------------------------------------

    int launchL2Distance(const float* q, const float* v, float* d,
                         int nq, int nv, int dim, void* stream) {
        return detail::invokeWithFallback(
            primary_.launchL2Distance, fallback_.launchL2Distance, policy_,
            q, v, d, nq, nv, dim, stream);
    }

    int launchCosine(const float* q, const float* v, float* d,
                     int nq, int nv, int dim, void* stream) {
        return detail::invokeWithFallback(
            primary_.launchCosine, fallback_.launchCosine, policy_,
            q, v, d, nq, nv, dim, stream);
    }

    int launchInnerProduct(const float* q, const float* v, float* d,
                           int nq, int nv, int dim, void* stream) {
        return detail::invokeWithFallback(
            primary_.launchInnerProduct, fallback_.launchInnerProduct, policy_,
            q, v, d, nq, nv, dim, stream);
    }

    int launchTopK(const float* d_dists, uint32_t* idx, float* out_dists,
                   int nq, int nv, int topK, void* stream) {
        return detail::invokeWithFallback(
            primary_.launchTopK, fallback_.launchTopK, policy_,
            d_dists, idx, out_dists, nq, nv, topK, stream);
    }

private:
    ANNKernelDispatch primary_;
    ANNKernelDispatch fallback_;
    RetryPolicy       policy_;
};

// ---------------------------------------------------------------------------
// GeoKernelFallbackDispatcher
// ---------------------------------------------------------------------------

/// Fallback/retry dispatcher for geospatial kernels.
///
/// Provides the same semantics as ANNKernelFallbackDispatcher for the two
/// geospatial kernel slots (launchDistance, launchContainment).
class GeoKernelFallbackDispatcher {
public:
    GeoKernelFallbackDispatcher(GeoKernelDispatch primary,
                                GeoKernelDispatch fallback,
                                RetryPolicy       policy = {}) noexcept
        : primary_(primary), fallback_(fallback), policy_(policy) {}

    /// Returns a plain GeoKernelDispatch with static null-slot resolution.
    [[nodiscard]] GeoKernelDispatch resolvedDispatch() const noexcept {
        GeoKernelDispatch d;
        d.launchDistance    = primary_.launchDistance
                            ? primary_.launchDistance
                            : fallback_.launchDistance;
        d.launchContainment = primary_.launchContainment
                            ? primary_.launchContainment
                            : fallback_.launchContainment;
        return d;
    }

    int launchDistance(const double* lats1, const double* lons1,
                       const double* lats2, const double* lons2,
                       float* out, int count,
                       GeoDistanceFormula formula, void* stream) {
        return detail::invokeWithFallback(
            primary_.launchDistance, fallback_.launchDistance, policy_,
            lats1, lons1, lats2, lons2, out, count, formula, stream);
    }

    int launchContainment(const double* pLats, const double* pLons,
                          int numPoints,
                          const double* polygon, int numVertices,
                          uint8_t* results, void* stream) {
        return detail::invokeWithFallback(
            primary_.launchContainment, fallback_.launchContainment, policy_,
            pLats, pLons, numPoints, polygon, numVertices, results, stream);
    }

private:
    GeoKernelDispatch primary_;
    GeoKernelDispatch fallback_;
    RetryPolicy       policy_;
};

} // namespace acceleration
} // namespace themis
