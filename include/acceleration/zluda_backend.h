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

using ZludaKernelFn = std::function<std::vector<float>(const std::vector<float>&)>;

/**
 * @brief Set Zluda Kernel Fn.
 * @param[in] fn Input parameter.
 */
void setZludaKernelFn(ZludaKernelFn fn);

void setZLUDAComputeDistancesFn(
    std::function<std::vector<float>(
        const float*, size_t, size_t, const float*, size_t, bool)> fn);

void setZLUDABatchKnnSearchFn(
    std::function<std::vector<std::vector<std::pair<uint32_t, float>>>(
        const float*, size_t, size_t, const float*, size_t, size_t, bool)> fn);

[[nodiscard]] std::unique_ptr<IVectorBackend> createZLUDABackend();

#endif // THEMIS_ENABLE_ZLUDA

} // namespace acceleration
} // namespace themis
