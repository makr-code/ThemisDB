/**
 * @file audit_log.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * GPU Audit Log — in-process ring buffer for structured GPU event records.
 */

#include "themis/gpu/audit_log.h"

namespace themis {
namespace gpu {

// ============================================================================
// Construction
// ============================================================================

GPUAuditLog::GPUAuditLog(size_t capacity) : capacity_(capacity > 0 ? capacity : 1) {
    ring_.resize(capacity_);
}

// ============================================================================
// Core record
// ============================================================================

/**
 * @brief Record.
 * @param[in] type Input parameter.
 * @param[in] size_bytes Input parameter.
 * @param[in] tag Input parameter.
 * @param[in] tenant_id Identifier of the tenant.
 * @param[in] message Input parameter.
 * @details Calls: lock(), std::chrono::system_clock::now().
 */
void GPUAuditLog::record(EventType type, uint64_t size_bytes, const std::string &tag, const std::string &tenant_id,
                         const std::string &message) {
    std::lock_guard<std::mutex> lock(mutex_);
    Event &e     = ring_[head_];
    e.type       = type;
    e.size_bytes = size_bytes;
    e.tag        = tag;
    e.tenant_id  = tenant_id;
    e.message    = message;
    e.timestamp  = std::chrono::system_clock::now();

    head_ = (head_ + 1) % capacity_;
    if (count_ < capacity_) {
        ++count_;
    }
    ++total_;
}

// ============================================================================
// Convenience overloads
// ============================================================================

/**
 * @brief Record Alloc Success.
 * @param[in] bytes Input parameter.
 * @param[in] tag Input parameter.
 * @param[in] tenant_id Identifier of the tenant.
 * @details Calls: record().
 */
void GPUAuditLog::recordAllocSuccess(uint64_t bytes, const std::string &tag, const std::string &tenant_id) {
    record(EventType::ALLOC_SUCCESS, bytes, tag, tenant_id);
}

/**
 * @brief Record Alloc Fail Global Limit.
 * @param[in] bytes Input parameter.
 * @param[in] tag Input parameter.
 * @param[in] tenant_id Identifier of the tenant.
 * @details Calls: record().
 */
void GPUAuditLog::recordAllocFailGlobalLimit(uint64_t bytes, const std::string &tag, const std::string &tenant_id) {
    record(EventType::ALLOC_FAIL_GLOBAL_LIMIT, bytes, tag, tenant_id, "Rejected: edition VRAM limit exceeded");
}

/**
 * @brief Record Alloc Fail Tenant Quota.
 * @param[in] bytes Input parameter.
 * @param[in] tag Input parameter.
 * @param[in] tenant_id Identifier of the tenant.
 * @details Calls: record().
 */
void GPUAuditLog::recordAllocFailTenantQuota(uint64_t bytes, const std::string &tag, const std::string &tenant_id) {
    record(EventType::ALLOC_FAIL_TENANT_QUOTA, bytes, tag, tenant_id, "Rejected: per-tenant quota exceeded");
}

/**
 * @brief Record Dealloc.
 * @param[in] bytes Input parameter.
 * @param[in] tag Input parameter.
 * @param[in] tenant_id Identifier of the tenant.
 * @details Calls: record().
 */
void GPUAuditLog::recordDealloc(uint64_t bytes, const std::string &tag, const std::string &tenant_id) {
    record(EventType::DEALLOC, bytes, tag, tenant_id);
}

/**
 * @brief Record Fallback To CPU.
 * @param[in] reason Input parameter.
 * @param[in] tenant_id Identifier of the tenant.
 * @details Calls: record().
 */
void GPUAuditLog::recordFallbackToCPU(const std::string &reason, const std::string &tenant_id) {
    record(EventType::FALLBACK_TO_CPU, 0, "cpu_fallback", tenant_id, reason);
}

/**
 * @brief Record Device Unavailable.
 * @param[in] detail Input parameter.
 * @details Calls: record().
 */
void GPUAuditLog::recordDeviceUnavailable(const std::string &detail) {
    record(EventType::DEVICE_UNAVAILABLE, 0, "", "", detail);
}

/**
 * @brief Record Circuit Opened.
 * @param[in] detail Input parameter.
 * @details Calls: record().
 */
void GPUAuditLog::recordCircuitOpened(const std::string &detail) {
    record(EventType::CIRCUIT_OPENED, 0, "", "", detail);
}

/**
 * @brief Record Circuit Reset.
 * @details Calls: record().
 */
void GPUAuditLog::recordCircuitReset() {
    record(EventType::CIRCUIT_RESET, 0, "", "", "Circuit breaker reset");
}

// ============================================================================
// Queries
// ============================================================================

std::vector<GPUAuditLog::Event> GPUAuditLog::snapshot() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    if (count_ == 0) {
        return {};
    }

    std::vector<Event> result;
    result.reserve(count_);

    if (count_ < capacity_) {
        // Buffer not yet wrapped: events live at [0..count_).
        for (size_t i = 0; i < count_; ++i) {
            result.push_back(ring_[i]);
        }
    } else {
        // Buffer has wrapped: oldest event is at head_.
        for (size_t i = 0; i < capacity_; ++i) {
            result.push_back(ring_[(head_ + i) % capacity_]);
        }
    }
    return result;
}

size_t GPUAuditLog::size() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    return count_;
}

size_t GPUAuditLog::capacity() const {
    return capacity_;
}

uint64_t GPUAuditLog::totalRecorded() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    return total_;
}

/**
 * @brief Clear.
 * @details Calls: lock().
 */
void GPUAuditLog::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    count_ = 0;
    head_  = 0;
    // total_ is intentionally preserved (lifetime counter).
}

} // namespace gpu
} // namespace themis
