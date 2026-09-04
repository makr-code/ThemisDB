/**
 * @file gpu_backend_dispatch_diagnostics.cpp
 * @brief Implementation of unified GPU backend dispatch diagnostics.
 * @version 1.0.0
 * @date 2026-08-05
 */

#include "themis/gpu/gpu_backend_dispatch_diagnostics.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <mutex>

namespace themis {
namespace gpu {

// Global event callback (thread-safe via mutex)
static std::mutex g_callback_mutex;
static GPUDispatchEventCallback g_event_callback;

// ============================================================================
// emitDiagnostic
// ============================================================================

void GPUBackendDispatchDiagnostics::emitDiagnostic(
    GPUDispatchErrorCode error_code,
    int device_id,
    const std::string& detail) noexcept {
    
    // SUCCESS is not an error condition; nothing to log or emit.
    if (error_code == GPUDispatchErrorCode::SUCCESS) {
        return;
    }

    try {
        // Structured error log
        auto logger = spdlog::get("gpu");
        if (!logger) {
            logger = spdlog::get("default");
        }
        
        std::string error_str = errorCodeToString(error_code);
        std::string device_str = (device_id >= 0) ? ("device_id=" + std::to_string(device_id)) : "device_id=N/A";
        
        if (logger) {
            logger->error("GPU backend dispatch error: {} [{}] detail={}", 
                         error_str, device_str, detail);
        }

        // Event callback emission
        {
            std::lock_guard<std::mutex> lock(g_callback_mutex);
            if (g_event_callback) {
                GPUDispatchEventType event_type = errorCodeToEventType(error_code);
                g_event_callback(event_type, error_code, device_id, detail);
            }
        }
    } catch (...) {
        // Diagnostic emission must not throw
    }
}

// ============================================================================
// Event callback management
// ============================================================================

void GPUBackendDispatchDiagnostics::setEventCallback(GPUDispatchEventCallback callback) noexcept {
    std::lock_guard<std::mutex> lock(g_callback_mutex);
    g_event_callback = callback;
}

GPUDispatchEventCallback GPUBackendDispatchDiagnostics::getEventCallback() noexcept {
    std::lock_guard<std::mutex> lock(g_callback_mutex);
    return g_event_callback;
}

// ============================================================================
// Error code to event type mapping
// ============================================================================

GPUDispatchEventType GPUBackendDispatchDiagnostics::errorCodeToEventType(
    GPUDispatchErrorCode code) noexcept {
    
    switch (code) {
        // Allocation errors
        case GPUDispatchErrorCode::ALLOC_SIZE_EXCEEDS_LIMIT:
        [[fallthrough]];
        case GPUDispatchErrorCode::ALLOC_INSUFFICIENT_VRAM:
        [[fallthrough]];
        case GPUDispatchErrorCode::ALLOC_DEVICE_FAILURE:
        [[fallthrough]];
        case GPUDispatchErrorCode::ALLOC_INVALID_PARAMS:
        [[fallthrough]];
        case GPUDispatchErrorCode::ALLOC_QUOTA_EXCEEDED:
            return GPUDispatchEventType::ALLOCATION_FAILED;

        // Backend selection errors
        case GPUDispatchErrorCode::BACKEND_NO_DEVICE_AVAILABLE:
            return GPUDispatchEventType::BACKEND_SELECTION_FAILED;
        
        case GPUDispatchErrorCode::BACKEND_CAPABILITY_MISMATCH:
            return GPUDispatchEventType::CAPABILITY_MISMATCH;
        
        case GPUDispatchErrorCode::BACKEND_TOPOLOGY_UNAVAILABLE:
        [[fallthrough]];
        case GPUDispatchErrorCode::BACKEND_NOT_ENABLED:
        [[fallthrough]];
        case GPUDispatchErrorCode::BACKEND_DEGRADED:
            return GPUDispatchEventType::DEVICE_DEGRADED;

        // Dispatch errors
        case GPUDispatchErrorCode::DISPATCH_TIMEOUT:
        [[fallthrough]];
        case GPUDispatchErrorCode::DISPATCH_KERNEL_LAUNCH_FAILED:
        [[fallthrough]];
        case GPUDispatchErrorCode::DISPATCH_STREAM_FULL:
        [[fallthrough]];
        case GPUDispatchErrorCode::DISPATCH_CONCURRENT_EXECUTION_REJECTED:
        [[fallthrough]];
        case GPUDispatchErrorCode::DISPATCH_QUERY_TYPE_UNSUPPORTED:
            return GPUDispatchEventType::DISPATCH_FAILED;

        // Fallback/degradation
        case GPUDispatchErrorCode::FALLBACK_CPU_DEGRADED:
        [[fallthrough]];
        case GPUDispatchErrorCode::FALLBACK_UNAVAILABLE:
            return GPUDispatchEventType::FALLBACK_TO_CPU;

        // Internal/unknown
        case GPUDispatchErrorCode::INTERNAL_ERROR:
        [[fallthrough]];
        case GPUDispatchErrorCode::SUCCESS:
        [[fallthrough]];
        default:
            return GPUDispatchEventType::DISPATCH_FAILED;
    }
}

// ============================================================================
// Error code to string conversion
// ============================================================================

std::string GPUBackendDispatchDiagnostics::errorCodeToString(GPUDispatchErrorCode code) noexcept {
    switch (code) {
        case GPUDispatchErrorCode::SUCCESS:
            return "SUCCESS";
        
        // Allocation errors
        case GPUDispatchErrorCode::ALLOC_SIZE_EXCEEDS_LIMIT:
            return "ALLOC_SIZE_EXCEEDS_LIMIT";
        case GPUDispatchErrorCode::ALLOC_INSUFFICIENT_VRAM:
            return "ALLOC_INSUFFICIENT_VRAM";
        case GPUDispatchErrorCode::ALLOC_DEVICE_FAILURE:
            return "ALLOC_DEVICE_FAILURE";
        case GPUDispatchErrorCode::ALLOC_INVALID_PARAMS:
            return "ALLOC_INVALID_PARAMS";
        case GPUDispatchErrorCode::ALLOC_QUOTA_EXCEEDED:
            return "ALLOC_QUOTA_EXCEEDED";
        
        // Backend selection errors
        case GPUDispatchErrorCode::BACKEND_NO_DEVICE_AVAILABLE:
            return "BACKEND_NO_DEVICE_AVAILABLE";
        case GPUDispatchErrorCode::BACKEND_CAPABILITY_MISMATCH:
            return "BACKEND_CAPABILITY_MISMATCH";
        case GPUDispatchErrorCode::BACKEND_TOPOLOGY_UNAVAILABLE:
            return "BACKEND_TOPOLOGY_UNAVAILABLE";
        case GPUDispatchErrorCode::BACKEND_NOT_ENABLED:
            return "BACKEND_NOT_ENABLED";
        case GPUDispatchErrorCode::BACKEND_DEGRADED:
            return "BACKEND_DEGRADED";
        
        // Dispatch errors
        case GPUDispatchErrorCode::DISPATCH_TIMEOUT:
            return "DISPATCH_TIMEOUT";
        case GPUDispatchErrorCode::DISPATCH_KERNEL_LAUNCH_FAILED:
            return "DISPATCH_KERNEL_LAUNCH_FAILED";
        case GPUDispatchErrorCode::DISPATCH_STREAM_FULL:
            return "DISPATCH_STREAM_FULL";
        case GPUDispatchErrorCode::DISPATCH_CONCURRENT_EXECUTION_REJECTED:
            return "DISPATCH_CONCURRENT_EXECUTION_REJECTED";
        case GPUDispatchErrorCode::DISPATCH_QUERY_TYPE_UNSUPPORTED:
            return "DISPATCH_QUERY_TYPE_UNSUPPORTED";
        
        // Fallback/degradation
        case GPUDispatchErrorCode::FALLBACK_CPU_DEGRADED:
            return "FALLBACK_CPU_DEGRADED";
        case GPUDispatchErrorCode::FALLBACK_UNAVAILABLE:
            return "FALLBACK_UNAVAILABLE";
        
        // Internal/unknown
        case GPUDispatchErrorCode::INTERNAL_ERROR:
            return "INTERNAL_ERROR";
        
        default:
            return "UNKNOWN_ERROR";
    }
}

// ============================================================================
// Event type to string conversion
// ============================================================================

std::string GPUBackendDispatchDiagnostics::eventTypeToString(GPUDispatchEventType type) noexcept {
    switch (type) {
        case GPUDispatchEventType::ALLOCATION_FAILED:
            return "ALLOCATION_FAILED";
        case GPUDispatchEventType::BACKEND_SELECTION_FAILED:
            return "BACKEND_SELECTION_FAILED";
        case GPUDispatchEventType::CAPABILITY_MISMATCH:
            return "CAPABILITY_MISMATCH";
        case GPUDispatchEventType::DISPATCH_FAILED:
            return "DISPATCH_FAILED";
        case GPUDispatchEventType::DEVICE_DEGRADED:
            return "DEVICE_DEGRADED";
        case GPUDispatchEventType::FALLBACK_TO_CPU:
            return "FALLBACK_TO_CPU";
        default:
            return "UNKNOWN_EVENT";
    }
}

// ============================================================================
// DiagnosticEmissionGuard
// ============================================================================

DiagnosticEmissionGuard::DiagnosticEmissionGuard(const std::string& description) noexcept
    : description_(description) {
    start_time_us_ = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

DiagnosticEmissionGuard::~DiagnosticEmissionGuard() noexcept {
    try {
        uint64_t end_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        uint64_t elapsed_us = end_time_us - start_time_us_;
        
        if (elapsed_us > GPUBackendDispatchContract::MAX_EMIT_DIAGNOSTIC_LATENCY_US) {
            auto logger = spdlog::get("gpu");
            if (!logger) {
                logger = spdlog::get("default");
            }
            if (logger) {
                logger->warn(
                    "Diagnostic emission exceeded SLA: {} elapsed={}µs threshold={}µs",
                    description_,
                    elapsed_us,
                    GPUBackendDispatchContract::MAX_EMIT_DIAGNOSTIC_LATENCY_US);
            }
        }
    } catch (...) {
        // Guard destruction must not throw
    }
}

}  // namespace gpu
}  // namespace themis
