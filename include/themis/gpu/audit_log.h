/**
 * @file audit_log.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
        DISPATCH_RESULT,         ///< Work result with execution status and mode
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
     * @brief Create a bounded in-memory audit log.
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
    /**
     * @brief Append a structured GPU operation event.
     * @param type Event category.
     * @param size_bytes Requested or released bytes, or zero when not applicable.
     * @param tag Caller-supplied reason or owner label.
     * @param tenant_id Tenant identifier, or empty for a global event.
     * @param message Optional human-readable detail.
     */
    void record(EventType   type,
                uint64_t    size_bytes,
                const std::string& tag,
                const std::string& tenant_id = "",
                const std::string& message   = "");

    /** @brief Record a successful allocation.
     * @param bytes Allocated byte count.
     * @param tag Caller-supplied reason or owner label.
     * @param tenant_id Tenant identifier, or empty for a global allocation.
     */
    void recordAllocSuccess(uint64_t bytes, const std::string& tag,
                            const std::string& tenant_id = "");
    /** @brief Record an allocation rejected by the global VRAM limit.
     * @param bytes Requested byte count.
     * @param tag Caller-supplied reason or owner label.
     * @param tenant_id Tenant identifier, or empty for a global allocation.
     */
    void recordAllocFailGlobalLimit(uint64_t bytes, const std::string& tag,
                                    const std::string& tenant_id = "");
    /** @brief Record an allocation rejected by a tenant quota.
     * @param bytes Requested byte count.
     * @param tag Caller-supplied reason or owner label.
     * @param tenant_id Tenant whose quota rejected the allocation.
     */
    void recordAllocFailTenantQuota(uint64_t bytes, const std::string& tag,
                                    const std::string& tenant_id);
    /** @brief Record a deallocation.
     * @param bytes Released byte count.
     * @param tag Caller-supplied reason or owner label.
     * @param tenant_id Tenant identifier, or empty for a global allocation.
     */
    void recordDealloc(uint64_t bytes, const std::string& tag,
                       const std::string& tenant_id = "");
    /** @brief Record an operation that completed through CPU fallback.
     * @param reason Explanation for using CPU execution.
     * @param tenant_id Tenant identifier, or empty for a global operation.
     */
    void recordFallbackToCPU(const std::string& reason,
                              const std::string& tenant_id = "");
    /** @brief Record that no usable GPU device was available.
     * @param detail Device availability diagnostic.
     */
    void recordDeviceUnavailable(const std::string& detail);
    /** @brief Record that the GPU circuit breaker opened.
     * @param detail Circuit-breaker diagnostic.
     */
    void recordCircuitOpened(const std::string& detail);
    /** @brief Record that the GPU circuit breaker was reset. */
    void recordCircuitReset();

    // -----------------------------------------------------------------------
    // Queries
    // -----------------------------------------------------------------------
    /**
     * @brief Return a snapshot of the events currently in the ring buffer.
     * @return Events in oldest-first order; the log is unchanged.
     */
    std::vector<Event> snapshot() const;

    /**
     * @brief Return the number of events currently stored.
     * @return Number of stored events, no greater than the configured capacity.
     */
    size_t size() const;

    /** @brief Return the maximum number of events retained by the ring buffer. */
    size_t capacity() const;

    /**
     * @brief Return the number of events recorded since construction.
     * @return Lifetime event count, which may exceed the current buffer size.
     */
    uint64_t totalRecorded() const;

    /** @brief Remove all stored events without resetting the lifetime count. */
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
