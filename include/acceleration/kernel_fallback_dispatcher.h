/**
 * @file kernel_fallback_dispatcher.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

/// @brief Controls retry behaviour when a kernel returns a transient error.
///
/// Configures exponential backoff and retry limits for transient device errors.
/// When a kernel returns a transient error code (DeviceLost, OperationTimeout,
/// SynchronizationFailed), the dispatcher will sleep and retry up to maxAttempts times.
///
/// @details The backoff algorithm uses exponential delay: each failed attempt
/// increases the delay by backoffMultiplier (capped at maxDelayMs). This allows
/// brief GPU glitches to recover without overwhelming the device.
struct RetryPolicy {
    /// @brief Total invocation attempts (primary only; 1 = no retry).
    /// When maxAttempts=1, no retry occurs; transient errors immediately trigger fallback.
    uint32_t maxAttempts       = 3;

    /// @brief Delay before the first retry, in milliseconds.
    /// 0 disables sleeping (useful in tests or when caller manages back-off).
    uint32_t initialDelayMs    = 1;

    /// @brief Upper bound on back-off delay, in milliseconds.
    /// After maxDelayMs is reached, no further delay increase occurs.
    uint32_t maxDelayMs        = 100;

    /// @brief Multiplicative factor applied after each failed attempt.
    /// Delay grows as: d[i+1] = min(d[i] * backoffMultiplier, maxDelayMs).
    /// Typical value: 2.0 (exponential backoff).
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

/// @brief Fallback/retry dispatcher for ANN (vector similarity) kernels.
///
/// Implements automatic fallback and retry semantics on top of the frozen kernel
/// dispatch tables defined in kernel_invocation.h. When a primary kernel fails or
/// is unsupported, seamlessly delegates to the fallback kernel with exponential
/// back-off retry for transient errors.
///
/// ## Behavior
/// 1. **Unsupported kernel (null slot):** If a slot in the primary dispatch table
///    is nullptr, the corresponding fallback slot is invoked directly.
/// 2. **Transient device error (retry):** If the primary kernel returns a transient
///    error code (DeviceLost, OperationTimeout, SynchronizationFailed), the
///    dispatcher sleeps for an exponentially increasing delay and retries up to
///    RetryPolicy::maxAttempts times.
/// 3. **Permanent error or exhausted retries:** After a non-transient error or once
///    all retry attempts are consumed, the dispatcher falls back to the
///    corresponding fallback slot.
///
/// ## Return Values
/// All dispatch functions return 0 on success. Errors are returned as
/// static_cast<int>(AccelerationErrorCode::XYZ) following the same convention
/// used by the CPU-backend kernel adapters.
///
/// ## Example Usage
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
    /// @brief Construct a new dispatcher with primary and fallback dispatch tables.
    ///
    /// @param primary  Dispatch table from the preferred (GPU) backend.
    ///                 Null slots are treated as unsupported and are routed
    ///                 directly to the corresponding fallback slot.
    /// @param fallback Dispatch table from the fallback (typically CPU) backend.
    /// @param policy   Retry policy applied to transient errors from primary.
    ///                 Defaults to { maxAttempts=3, initialDelayMs=1, maxDelayMs=100 }.
    ANNKernelFallbackDispatcher(ANNKernelDispatch primary,
                                ANNKernelDispatch fallback,
                                RetryPolicy       policy = {}) noexcept
        : primary_(primary), fallback_(fallback), policy_(policy) {}

    /// @brief Returns a plain ANNKernelDispatch with static null-slot resolution.
    ///
    /// Replaces null entries in the primary table with corresponding fallback
    /// entries, creating a resolved dispatch table. This table does NOT perform
    /// retries — it is a static resolution only.
    ///
    /// Callers that need retry behaviour should use the invoke* methods below
    /// instead of this static table.
    ///
    /// @return Plain ANNKernelDispatch where each null slot in primary is
    ///         replaced by the corresponding fallback slot. Null entries
    ///         (when both primary and fallback are null) remain null.
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

    /// @brief Launch L2 distance kernel with automatic fallback and retry.
    ///
    /// @param q        Query matrix [numQueries × dim] (row-major)
    /// @param v        Vector/corpus matrix [numVectors × dim] (row-major)
    /// @param d        Output distance matrix [numQueries × numVectors] (caller-allocated)
    /// @param nq       Number of queries
    /// @param nv       Number of vectors
    /// @param dim      Vector dimensionality
    /// @param stream   Backend-specific stream handle (cudaStream_t, VkCommandBuffer, etc.)
    /// @return 0 on success, non-zero error code on failure
    int launchL2Distance(const float* q, const float* v, float* d,
                         int nq, int nv, int dim, void* stream) {
        return detail::invokeWithFallback(
            primary_.launchL2Distance, fallback_.launchL2Distance, policy_,
            q, v, d, nq, nv, dim, stream);
    }

    /// @brief Launch cosine distance kernel with automatic fallback and retry.
    /// @copydetails launchL2Distance
    int launchCosine(const float* q, const float* v, float* d,
                     int nq, int nv, int dim, void* stream) {
        return detail::invokeWithFallback(
            primary_.launchCosine, fallback_.launchCosine, policy_,
            q, v, d, nq, nv, dim, stream);
    }

    /// @brief Launch inner-product distance kernel with automatic fallback and retry.
    /// @copydetails launchL2Distance
    int launchInnerProduct(const float* q, const float* v, float* d,
                           int nq, int nv, int dim, void* stream) {
        return detail::invokeWithFallback(
            primary_.launchInnerProduct, fallback_.launchInnerProduct, policy_,
            q, v, d, nq, nv, dim, stream);
    }

    /// @brief Launch top-k selection kernel with automatic fallback and retry.
    ///
    /// @param d_dists      Input distance matrix [numQueries × numVectors]
    /// @param idx          Output indices [numQueries × topK] (caller-allocated)
    /// @param out_dists    Output distances [numQueries × topK] (caller-allocated)
    /// @param nq           Number of queries
    /// @param nv           Number of vectors
    /// @param topK         Number of top matches to select
    /// @param stream       Backend-specific stream handle
    /// @return 0 on success, non-zero error code on failure
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

/// @brief Fallback/retry dispatcher for geospatial kernels.
///
/// Provides the same fallback and retry semantics as ANNKernelFallbackDispatcher
/// for the two geospatial kernel slots (launchDistance, launchContainment).
/// Handles null slots and transient errors with exponential back-off retry.
///
/// ## Behavior
/// Identical to ANNKernelFallbackDispatcher: unsupported kernels trigger fallback,
/// transient errors trigger retry with exponential delay, and permanent errors
/// either use fallback or return the error code.
///
/// ## Example Usage
/// @code
///   CPUGeoBackend cpu;
///   cpu.initialize();
///   GeoKernelDispatch cpuTable  = cpu.populateGeoDispatch();
///   GeoKernelDispatch gpuTable  = gpuBackend.populateGeoDispatch();
///
///   GeoKernelFallbackDispatcher dispatcher(gpuTable, cpuTable);
///
///   // Invoke with automatic fallback / retry:
///   int rc = dispatcher.launchDistance(lats1, lons1, lats2, lons2,
///                                       distances, count, HAVERSINE, stream);
/// @endcode
class GeoKernelFallbackDispatcher {
public:
    /// @brief Construct a new dispatcher with primary and fallback dispatch tables.
    ///
    /// @param primary  Dispatch table from the preferred (GPU) backend.
    ///                 Null slots are treated as unsupported and are routed
    ///                 directly to the corresponding fallback slot.
    /// @param fallback Dispatch table from the fallback (typically CPU) backend.
    /// @param policy   Retry policy applied to transient errors from primary.
    ///                 Defaults to { maxAttempts=3, initialDelayMs=1, maxDelayMs=100 }.
    GeoKernelFallbackDispatcher(GeoKernelDispatch primary,
                                GeoKernelDispatch fallback,
                                RetryPolicy       policy = {}) noexcept
        : primary_(primary), fallback_(fallback), policy_(policy) {}

    /// @brief Returns a plain GeoKernelDispatch with static null-slot resolution.
    ///
    /// Replaces null entries in the primary table with corresponding fallback
    /// entries, creating a resolved dispatch table. This table does NOT perform
    /// retries — it is a static resolution only.
    ///
    /// Callers that need retry behaviour should use the invoke* methods below
    /// instead of this static table.
    ///
    /// @return Plain GeoKernelDispatch where each null slot in primary is
    ///         replaced by the corresponding fallback slot.
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

    /// @brief Launch distance calculation kernel with automatic fallback and retry.
    ///
    /// Computes geodesic distances between latitude/longitude pairs using Haversine
    /// or Vincenty formula. Supports automatic GPU fallback and transient error retry.
    ///
    /// @param lats1        First set of latitudes (degrees, WGS84) [count]
    /// @param lons1        First set of longitudes (degrees, WGS84) [count]
    /// @param lats2        Second set of latitudes (degrees, WGS84) [count]
    /// @param lons2        Second set of longitudes (degrees, WGS84) [count]
    /// @param out          Output distance array [count] in kilometers (caller-allocated)
    /// @param count        Number of point pairs
    /// @param formula      Distance formula: HAVERSINE (default, fast) or VINCENTY (precise)
    /// @param stream       Backend-specific stream handle (cudaStream_t, VkCommandBuffer, etc.)
    /// @return 0 on success, non-zero error code on failure
    int launchDistance(const double* lats1, const double* lons1,
                       const double* lats2, const double* lons2,
                       float* out, int count,
                       GeoDistanceFormula formula, void* stream) {
        return detail::invokeWithFallback(
            primary_.launchDistance, fallback_.launchDistance, policy_,
            lats1, lons1, lats2, lons2, out, count, formula, stream);
    }

    /// @brief Launch point-in-polygon test kernel with automatic fallback and retry.
    ///
    /// Tests whether each point is inside the given polygon using ray-casting.
    /// Supports automatic GPU fallback and transient error retry.
    ///
    /// @param pLats        Test point latitudes (degrees, WGS84) [numPoints]
    /// @param pLons        Test point longitudes (degrees, WGS84) [numPoints]
    /// @param numPoints    Number of test points
    /// @param polygon      Interleaved polygon vertices [lat0, lon0, lat1, lon1, ...] [numVertices × 2]
    /// @param numVertices  Number of polygon vertices (must be >= 3)
    /// @param results      Output containment flags [numPoints] (caller-allocated, non-zero=inside)
    /// @param stream       Backend-specific stream handle
    /// @return 0 on success, non-zero error code on failure
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

