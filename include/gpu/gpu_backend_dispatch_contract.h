/**
 * @file gpu_backend_dispatch_contract.h
 * @brief Canonical bounded runtime contract for GPU backend allocation and dispatch.
 * @version 1.0.0
 * @date 2026-08-05
 * 
 * Phase 2/3 hardening: defines bounded runtime guarantees, fail-closed behavior,
 * and diagnostic expectations for GPU memory allocation, backend selection, and
 * query/kernel dispatch paths.
 * 
 * @see src/gpu/ROADMAP.md Phase 2-3 delivery
 */

#pragma once

#include <cstdint>
#include <string>

namespace themis {
namespace gpu {

/**
 * @brief GPU backend dispatch error codes (fail-closed classification).
 * 
 * All error codes are fail-closed: the system degrades gracefully to CPU-only
 * execution when any condition cannot be met.
 */
enum class GPUDispatchErrorCode : uint8_t {
    /// No error; operation succeeded or was not attempted.
    SUCCESS = 0,

    // ========== Allocation Errors (ALLOC_*) ==========
    /// Requested allocation exceeds configured maximum per-device limit.
    ALLOC_SIZE_EXCEEDS_LIMIT = 10,
    /// No device has sufficient free VRAM for requested allocation.
    ALLOC_INSUFFICIENT_VRAM = 11,
    /// Device malloc or cudaMalloc failed (OOM, invalid device, or driver issue).
    ALLOC_DEVICE_FAILURE = 12,
    /// Invalid allocation parameters (negative size, invalid device ID).
    ALLOC_INVALID_PARAMS = 13,
    /// Allocation request would exceed global quota (multi-tenant).
    ALLOC_QUOTA_EXCEEDED = 14,

    // ========== Backend Selection Errors (BACKEND_*) ==========
    /// No backend available for requested operation (all devices unhealthy).
    BACKEND_NO_DEVICE_AVAILABLE = 20,
    /// Requested backend feature not supported on available devices.
    BACKEND_CAPABILITY_MISMATCH = 21,
    /// Backend topology information is stale or uninitialized.
    BACKEND_TOPOLOGY_UNAVAILABLE = 22,
    /// Requested backend type is not compiled/enabled in this build.
    BACKEND_NOT_ENABLED = 23,
    /// Backend degradation detected; fallback to CPU recommended.
    BACKEND_DEGRADED = 24,

    // ========== Dispatch Errors (DISPATCH_*) ==========
    /// Query/kernel dispatch timeout (exceeded configured SLA).
    DISPATCH_TIMEOUT = 30,
    /// Kernel launch failed (invalid grid/block dims, resource limit).
    DISPATCH_KERNEL_LAUNCH_FAILED = 31,
    /// Dispatch stream is full; enqueue rejected.
    DISPATCH_STREAM_FULL = 32,
    /// Concurrent dispatch rejected (mutex acquisition or lock order violation).
    DISPATCH_CONCURRENT_EXECUTION_REJECTED = 33,
    /// Query type not supported by GPU acceleration path.
    DISPATCH_QUERY_TYPE_UNSUPPORTED = 34,

    // ========== Fallback/Degradation Errors (FALLBACK_*) ==========
    /// CPU fallback executed (expected; not an error).
    FALLBACK_CPU_DEGRADED = 40,
    /// Fallback mechanism unavailable (should not occur in production).
    FALLBACK_UNAVAILABLE = 41,

    // ========== Internal/Unknown Errors ==========
    /// Internal error (unexpected condition, logging recommended).
    INTERNAL_ERROR = 99,
};

/**
 * @brief Return true if the error code indicates a fail-closed class.
 * 
 * Fail-closed errors are expected to trigger automatic degradation to CPU
 * execution without user intervention.
 */
inline bool isFailClosedClass(GPUDispatchErrorCode code) noexcept {
    // All defined error codes except SUCCESS are fail-closed.
    return code != GPUDispatchErrorCode::SUCCESS;
}

/**
 * @brief GPU backend dispatch bounded runtime contract.
 * 
 * Guarantees:
 * - All allocation operations complete with bounded latency (see timeout constants).
 * - Backend selection happens under a single lock acquisition (selectDevice()).
 * - Diagnostic events are emitted atomically for each error condition.
 * - Concurrent dispatch attempts are rejected fail-closed rather than silently serialized.
 */
struct GPUBackendDispatchContract {
    // ========== Allocation Latency Bounds ==========
    /// Maximum time for GPUMemoryAllocator::allocate() under normal conditions (no contention).
    /// Includes device malloc, allocation tracking, but NOT VRAM initialization.
    static constexpr uint64_t MAX_ALLOCATE_LATENCY_US = 1000;  // 1 ms
    
    /// Maximum time for GPULoadBalancer::selectDevice() (lock acquisition + device scan).
    static constexpr uint64_t MAX_SELECT_DEVICE_LATENCY_US = 100;  // 100 µs
    
    /// Maximum time for quota policy checks (lookup + accounting update).
    static constexpr uint64_t MAX_QUOTA_CHECK_LATENCY_US = 10;  // 10 µs

    // ========== Dispatch Operation Bounds ==========
    /// Default kernel/query execution SLA (hard timeout for watchdog).
    static constexpr uint64_t DEFAULT_KERNEL_SLA_US = 5'000'000;  // 5 seconds
    
    /// Minimum time between backend health checks (avoid excessive probing).
    static constexpr uint64_t MIN_HEALTH_CHECK_INTERVAL_US = 100'000;  // 100 ms
    
    /// Maximum time for emitting diagnostics (logging + callback + event).
    static constexpr uint64_t MAX_EMIT_DIAGNOSTIC_LATENCY_US = 100;  // 100 µs

    // ========== Concurrency Contracts ==========
    /// Lock order (must be respected to avoid deadlocks):
    /// 1. allocation_mutex (outer)
    /// 2. device_state_mutex (per-device)
    /// 3. dispatch_mutex (innermost)
    /// Never acquire a lock in reverse order.
    static constexpr const char* CANONICAL_LOCK_ORDER = 
        "allocation_mutex → device_state_mutex → dispatch_mutex";

    /// Maximum time for stop/cleanup operations.
    /// Includes draining dispatch queues and releasing resources.
    static constexpr uint64_t MAX_STOP_LATENCY_US = 1'000'000;  // 1 second

    // ========== Fail-Closed Behavior ==========
    /// On any allocation failure, immediately return error code (no retry, no blocking).
    static constexpr bool ALLOCATION_FAIL_CLOSED = true;
    
    /// On any backend selection failure (no device available), immediately return nullptr
    /// and emit BACKEND_NO_DEVICE_AVAILABLE diagnostic.
    static constexpr bool BACKEND_SELECTION_FAIL_CLOSED = true;
    
    /// On dispatch concurrency conflict (another dispatch in progress), immediately
    /// reject with DISPATCH_CONCURRENT_EXECUTION_REJECTED.
    static constexpr bool DISPATCH_CONCURRENT_FAIL_CLOSED = true;

    // ========== Diagnostic Behavior ==========
    /// Every error condition MUST emit an emitDiagnostic() call before returning.
    /// Emission includes: structured log + event callback.
    static constexpr bool DIAGNOSTIC_EMISSION_MANDATORY = true;
    
    /// Diagnostic events must be synchronous (not queued) to ensure observability.
    static constexpr bool DIAGNOSTIC_EMISSION_SYNCHRONOUS = true;
};

/**
 * @brief Canonical seed for deterministic GPU backend dispatch tests.
 * Used for reproducible randomization in allocation policies and device selection.
 */
static constexpr uint32_t GPU_BACKEND_DISPATCH_SEED = 42;

}  // namespace gpu
}  // namespace themis
