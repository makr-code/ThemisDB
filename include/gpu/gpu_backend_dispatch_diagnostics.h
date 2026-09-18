/**
 * @file gpu_backend_dispatch_diagnostics.h
 * @brief Unified diagnostics infrastructure for GPU backend allocation and dispatch.
 * @version 1.0.0
 * @date 2026-08-05
 * 
 * Phase 2/3 hardening: structured logging and event emission for all GPU
 * backend dispatch error conditions. Ensures consistent observability across
 * allocation failures, backend selection mismatches, and dispatch errors.
 */

#pragma once
#include "themis/export.h"

#include "gpu_backend_dispatch_contract.h"
#include <functional>
#include <string>
#include <memory>

namespace themis {
namespace gpu {

enum class GPUDispatchEventType : uint8_t {
    ALLOCATION_FAILED = 10,
    BACKEND_SELECTION_FAILED = 20,
    CAPABILITY_MISMATCH = 21,
    DISPATCH_FAILED = 30,
    DEVICE_DEGRADED = 31,
    FALLBACK_TO_CPU = 40,
};

using GPUDispatchEventCallback = std::function<void(
    GPUDispatchEventType event_type,
    GPUDispatchErrorCode error_code,
    int device_id,
    const std::string& detail)>;

class THEMIS_GEO_API GPUBackendDispatchDiagnostics {
public:
    /**
     * @brief Emit Diagnostic.
     * @param[in] error_code Input parameter.
     * @param[in] device_id Identifier of the device.
     * @param[in] detail Input parameter.
     * @note Exception safety: noexcept.
     */
    static void emitDiagnostic(
        GPUDispatchErrorCode error_code,
        int device_id,
        const std::string& detail) noexcept;

    /**
     * @brief Set Event Callback.
     * @param[in] callback Input parameter.
     * @note Exception safety: noexcept.
     */
    static void setEventCallback(GPUDispatchEventCallback callback) noexcept;

    /**
     * @brief Get Event Callback.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static GPUDispatchEventCallback getEventCallback() noexcept;

    /**
     * @brief Error Code To Event Type.
     * @param[in] code Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static GPUDispatchEventType errorCodeToEventType(GPUDispatchErrorCode code) noexcept;

    /**
     * @brief Error Code To String.
     * @param[in] code Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static std::string errorCodeToString(GPUDispatchErrorCode code) noexcept;

    /**
     * @brief Event Type To String.
     * @param[in] type Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static std::string eventTypeToString(GPUDispatchEventType type) noexcept;
};

class THEMIS_GEO_API DiagnosticEmissionGuard {
public:
    /**
     * @brief Diagnostic Emission Guard.
     * @param[in] description Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    explicit DiagnosticEmissionGuard(const std::string& description) noexcept;

    ~DiagnosticEmissionGuard() noexcept;

private:
    std::string description_;
    uint64_t start_time_us_;
};

}  // namespace gpu
}  // namespace themis
