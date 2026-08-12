/**
 * @file audit_log.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace gpu {

/**
 * @brief Structured audit event log for GPU operations.
 *
 * Records alloc/free/failure/fallback events in an in-process ring buffer.
 * Designed for lightweight compliance tracing: no I/O dependency, no heap
 * allocation per event (the ring buffer is pre-allocated).
 *
 * Thread safety: all methods are protected by an internal mutex.
 */
class GPUAuditLog {
public:
    // -----------------------------------------------------------------------
    // Event types
    // -----------------------------------------------------------------------
    enum class EventType {
        ALLOC_SUCCESS,           ///< TryAllocateGPU returned true
        ALLOC_FAIL_GLOBAL_LIMIT, ///< Rejected by edition VRAM cap
        ALLOC_FAIL_TENANT_QUOTA, ///< Rejected by per-tenant quota
        DEALLOC,                 ///< DeallocateGPU called
        FALLBACK_TO_CPU,         ///< Operation fell back to CPU path
        DEVICE_UNAVAILABLE,      ///< Device discovery found no healthy GPU
        CIRCUIT_OPENED,          ///< Circuit breaker tripped
        CIRCUIT_RESET,           ///< Circuit breaker reset to DEGRADED/HEALTHY
    };

    // -----------------------------------------------------------------------
    // Event record
    // -----------------------------------------------------------------------
    struct Event {
        EventType   type       = EventType::ALLOC_SUCCESS;
        uint64_t    size_bytes = 0;         ///< Bytes requested/freed (0 if N/A)
        std::string tag;                    ///< Caller-supplied reason/owner
        std::string tenant_id;              ///< Empty = global / no tenant
        std::string message;                ///< Human-readable detail
        std::chrono::system_clock::time_point timestamp;
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------
    /**
     * @param capacity Maximum events kept.  When the buffer is full the
     *                 oldest event is overwritten (ring buffer semantics).
     */
    explicit GPUAuditLog(size_t capacity = 1024);

    // Disable copy; the log is intended to be a singleton or unique owner.
    GPUAuditLog(const GPUAuditLog&) = delete;
    GPUAuditLog& operator=(const GPUAuditLog&) = delete;

    // -----------------------------------------------------------------------
    // Logging
    // -----------------------------------------------------------------------
    void record(EventType   type,
                uint64_t    size_bytes,
                const std::string& tag,
                const std::string& tenant_id = "",
                const std::string& message   = "");

    // Convenience overloads for common cases.
    void recordAllocSuccess(uint64_t bytes, const std::string& tag,
                            const std::string& tenant_id = "");
    void recordAllocFailGlobalLimit(uint64_t bytes, const std::string& tag,
                                    const std::string& tenant_id = "");
    void recordAllocFailTenantQuota(uint64_t bytes, const std::string& tag,
                                    const std::string& tenant_id);
    void recordDealloc(uint64_t bytes, const std::string& tag,
                       const std::string& tenant_id = "");
    void recordFallbackToCPU(const std::string& reason,
                              const std::string& tenant_id = "");
    void recordDeviceUnavailable(const std::string& detail);
    void recordCircuitOpened(const std::string& detail);
    void recordCircuitReset();

    // -----------------------------------------------------------------------
    // Queries
    // -----------------------------------------------------------------------
    /// Snapshot of all events currently in the ring buffer (oldest-first).
    std::vector<Event> snapshot() const;

    /// Number of events currently stored (≤ capacity).
    size_t size() const;

    /// Capacity of the ring buffer.
    size_t capacity() const;

    /// Total events recorded since construction (may exceed capacity).
    uint64_t totalRecorded() const;

    /// Clear all events.
    void clear();

private:
    size_t                       capacity_;
    mutable std::mutex           mutex_;
    std::vector<Event>           ring_;
    size_t                       head_  = 0;   ///< Next write position
    size_t                       count_ = 0;   ///< Events currently in buffer
    uint64_t                     total_ = 0;   ///< Lifetime event count
};

} // namespace gpu
} // namespace themis
