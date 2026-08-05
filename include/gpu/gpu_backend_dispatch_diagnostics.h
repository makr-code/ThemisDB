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

#include "gpu_backend_dispatch_contract.h"
#include <functional>
#include <string>
#include <memory>

namespace themis {
namespace gpu {

/**
 * @brief Event types emitted by GPU backend dispatch diagnostics.
 */
enum class GPUDispatchEventType : uint8_t {
    /// Allocation request rejected (size/quota/device failure).
    ALLOCATION_FAILED = 10,
    /// Backend selection failed (no device available).
    BACKEND_SELECTION_FAILED = 20,
    /// Device capability mismatch (feature not supported).
    CAPABILITY_MISMATCH = 21,
    /// Dispatch operation rejected due to concurrency or timeout.
    DISPATCH_FAILED = 30,
    /// Device degradation detected (health check failed).
    DEVICE_DEGRADED = 31,
    /// Fallback to CPU executed (expected recovery).
    FALLBACK_TO_CPU = 40,
};

/**
 * @brief Event callback signature for GPU backend dispatch diagnostics.
 * 
 * Callers can register an event callback to receive structured events for
 * observability/monitoring/alerting purposes.
 * 
 * @param event_type Type of event that occurred.
 * @param error_code Associated error code.
 * @param device_id GPU device ID (-1 if N/A).
 * @param detail Detailed context/reason for the event.
 */
using GPUDispatchEventCallback = std::function<void(
    GPUDispatchEventType event_type,
    GPUDispatchErrorCode error_code,
    int device_id,
    const std::string& detail)>;

/**
 * @brief Unified diagnostics emitter for GPU backend dispatch.
 * 
 * Provides synchronous structured logging and event callbacks for all
 * GPU backend dispatch error conditions.
 */
class GPUBackendDispatchDiagnostics {
public:
    /**
     * @brief Emit a diagnostic event for a GPU backend dispatch error.
     * 
     * Performs:
     * 1. Structured error log via spdlog::error
     * 2. Event callback invocation (if registered)
     * 
     * @param error_code The error code being reported.
     * @param device_id GPU device ID (-1 if N/A or not applicable).
     * @param detail Descriptive context for the error.
     */
    static void emitDiagnostic(
        GPUDispatchErrorCode error_code,
        int device_id,
        const std::string& detail) noexcept;

    /**
     * @brief Register an event callback for GPU backend dispatch diagnostics.
     * 
     * The callback will be invoked synchronously for every diagnostic event.
     * Only one callback is active at a time; registering a new callback
     * replaces the previous one.
     * 
     * @param callback The event callback function (nullptr to disable callbacks).
     */
    static void setEventCallback(GPUDispatchEventCallback callback) noexcept;

    /**
     * @brief Get the currently registered event callback.
     * 
     * @return The active callback, or nullptr if no callback is registered.
     */
    static GPUDispatchEventCallback getEventCallback() noexcept;

    /// Map error code to corresponding event type.
    static GPUDispatchEventType errorCodeToEventType(GPUDispatchErrorCode code) noexcept;

    /// Get human-readable string for error code.
    static std::string errorCodeToString(GPUDispatchErrorCode code) noexcept;

    /// Get human-readable string for event type.
    static std::string eventTypeToString(GPUDispatchEventType type) noexcept;
};

/**
 * @brief RAII helper for bounded diagnostic emission timing.
 * 
 * Ensures diagnostic operations complete within MAX_EMIT_DIAGNOSTIC_LATENCY_US
 * and logs a warning if timeout is exceeded.
 */
class DiagnosticEmissionGuard {
public:
    /**
     * @brief Construct and start timing.
     * 
     * @param description Brief description of the operation being timed (for logging).
     */
    explicit DiagnosticEmissionGuard(const std::string& description) noexcept;

    /**
     * @brief Destructor: verify emission completed within SLA and log if exceeded.
     */
    ~DiagnosticEmissionGuard() noexcept;

private:
    std::string description_;
    uint64_t start_time_us_;
};

}  // namespace gpu
}  // namespace themis
