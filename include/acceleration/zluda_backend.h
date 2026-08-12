/**
 * @file zluda_backend.h
 * @brief Public API for the ZLUDA backend and its injection-bridge helpers.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 *
 * Exposes the ZludaKernelFn bridge type and the setZludaKernelFn() setter
 * that allow callers to inject a PTX-level kernel implementation at runtime
 * without recompiling ThemisDB.  Also declares the two typed callback
 * overrides and the factory that creates a ZLUDAVectorBackend instance.
 *
 * Thread-safety: all static setter functions are guarded by the backend's
 * internal std::mutex; concurrent calls from multiple threads are safe.
 */


#pragma once

#include "acceleration/compute_backend.h"
#include <functional>
#include <memory>
#include <utility>
#include <vector>
#include <cstdint>

namespace themis {
namespace acceleration {

#ifdef THEMIS_ENABLE_ZLUDA

/**
 * @brief Generic ZLUDA PTX-replacement kernel bridge.
 *
 * A callable with this signature may be injected via setZludaKernelFn().
 * Input convention for computeDistances:
 *   input = [ queries (numQueries*dim floats), vectors (numVectors*dim floats) ]
 *   expected output size = numQueries * numVectors (one distance per pair).
 * Input convention for batchKnnSearch:
 *   input = [ queries (numQueries*dim floats), vectors (numVectors*dim floats) ]
 *   expected output size = numQueries * numVectors (flat distance matrix,
 *   row-major query-major order); the bridge layer then performs top-k
 *   selection internally.
 *
 * If the function returns an empty vector the backend falls through to the
 * default CPU fallback path.
 *
 * @note Exceptions thrown by the injected function are caught and logged;
 *       execution falls through to the CPU fallback (fail-closed behaviour).
 */
using ZludaKernelFn = std::function<std::vector<float>(const std::vector<float>&)>;

/**
 * @brief Inject a generic ZludaKernelFn bridge into the ZLUDA backend.
 *
 * Replaces any previously set ZludaKernelFn.  Pass a default-constructed
 * (null) std::function to clear the bridge and revert to the CPU fallback.
 *
 * @param fn  Callable satisfying ZludaKernelFn; may be a null std::function.
 * @note Thread-safe — the fn is stored under the backend's static mutex.
 */
void setZludaKernelFn(ZludaKernelFn fn);

/**
 * @brief Typed bridge for ZLUDAVectorBackend::computeDistances().
 *
 * When set, the backend calls @p fn with the original pointer arguments
 * instead of attempting PTX kernel execution.  This bridge is checked
 * before ZludaKernelFn and before the initialized-guard.
 *
 * @param fn  Callable with the exact computeDistances signature.
 * @note Thread-safe — stored under the backend's static mutex.
 */
void setZLUDAComputeDistancesFn(
    std::function<std::vector<float>(
        const float*, size_t, size_t, const float*, size_t, bool)> fn);

/**
 * @brief Typed bridge for ZLUDAVectorBackend::batchKnnSearch().
 *
 * When set, the backend calls @p fn with the original pointer arguments
 * instead of attempting PTX kernel execution.  This bridge is checked
 * before ZludaKernelFn and before the initialized-guard.
 *
 * @param fn  Callable with the exact batchKnnSearch signature.
 * @note Thread-safe — stored under the backend's static mutex.
 */
void setZLUDABatchKnnSearchFn(
    std::function<std::vector<std::vector<std::pair<uint32_t, float>>>(
        const float*, size_t, size_t, const float*, size_t, size_t, bool)> fn);

/**
 * @brief Factory function that creates a ZLUDA vector backend instance.
 *
 * @return Owning pointer to a newly created ZLUDAVectorBackend.
 * @note The backend must be initialised by the caller via initialize() before
 *       use.  The injected bridge functions work even without calling
 *       initialize(), allowing unit tests to exercise the bridge path without
 *       requiring ZLUDA-capable hardware.
 */
[[nodiscard]] std::unique_ptr<IVectorBackend> createZLUDABackend();

#endif // THEMIS_ENABLE_ZLUDA

} // namespace acceleration
} // namespace themis
