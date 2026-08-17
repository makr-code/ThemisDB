/**
 * @file access_model_logging.h
 * @brief Structured logging framework for access model coordinator state transitions.
 *
 * ThemisDB | File: access_model_logging.h | Version: 1.0.0
 * Maturity: 🟡 ALPHA (Phase 5 Implementation) | Status: Active development
 * Author: Copilot | Date: 2026-08-17
 *
 * Defines structured log entry types for:
 * - Tier transitions (promotions/demotions)
 * - Eviction events
 * - Promotion/demotion decisions
 *
 * All log entries include:
 * - Correlation ID for trace correlation
 * - Thread ID for identifying worker threads
 * - Timestamp (millisecond precision)
 * - Operation latency tracking
 *
 * @see include/access_model/access_coordinator.h
 * @see include/access_model/access_model_trace.h
 */

#pragma once

#include "access_tier_interface.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <thread>

namespace themis {
namespace access_model {

// ============================================================================
// § 1  Structured Log Entry Types
// ============================================================================

/**
 * @brief Structured log entry for tier transition events.
 *
 * Captures all context needed to understand why and how a key moved between tiers.
 */
struct TierTransitionLog {
    /// Key being transitioned
    std::string key;
    
    /// Source tier level
    TierLevel from_tier;
    
    /// Target tier level
    TierLevel to_tier;
    
    /// Reason for transition (e.g., "age_policy", "eviction_feedback", "hot_access")
    std::string reason;
    
    /// Operation latency in milliseconds
    uint64_t latency_ms;
    
    /// Correlation ID for trace correlation
    std::string correlation_id;
    
    /// Thread ID of processing thread
    std::thread::id thread_id;
    
    /// Operation timestamp
    std::chrono::system_clock::time_point timestamp;
    
    /// Operation success status
    std::string status;  // "SUCCESS", "REJECTED", "FAILED", "DEFERRED"
};

/**
 * @brief Structured log entry for cache eviction events.
 *
 * Captures context from cache module when a key is evicted.
 */
struct EvictionEventLog {
    /// Evicted key
    std::string key;
    
    /// Cache tier that evicted the key
    TierLevel from_tier;
    
    /// Reason for eviction (e.g., "lru", "lfu", "ttl", "memory_pressure")
    std::string eviction_reason;
    
    /// Size of evicted value in bytes
    size_t size_bytes;
    
    /// Number of accesses before eviction
    uint64_t access_count;
    
    /// Age since last access
    std::chrono::seconds last_access_age;
    
    /// Demotion decision result ("PROMOTE", "DEMOTE", "RETAIN", "UNKNOWN")
    std::string decision;
    
    /// Correlation ID for trace correlation
    std::string correlation_id;
    
    /// Thread ID of processing thread
    std::thread::id thread_id;
    
    /// Event timestamp
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief Structured log entry for promotion/demotion policy decisions.
 *
 * Captures the reasoning behind promotion or demotion decisions.
 */
struct PromotionDecisionLog {
    /// Key subject to decision
    std::string key;
    
    /// Current tier
    TierLevel current_tier;
    
    /// Target tier (if promotion)
    std::optional<TierLevel> target_tier;
    
    /// Decision outcome ("PROMOTE", "REJECT", "DEFER", "DEMOTE")
    std::string decision;
    
    /// Access count triggering decision
    uint64_t access_count;
    
    /// Age in seconds
    std::chrono::seconds age_secs;
    
    /// Policy threshold that was checked
    std::string threshold_name;
    
    /// Threshold value
    uint64_t threshold_value;
    
    /// Actual value compared
    uint64_t actual_value;
    
    /// Human-readable reason
    std::string reason;
    
    /// Correlation ID for trace correlation
    std::string correlation_id;
    
    /// Thread ID of processing thread
    std::thread::id thread_id;
    
    /// Decision timestamp
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief Structured log entry for coordinator lifecycle events.
 *
 * Captures startup, shutdown, and configuration changes.
 */
struct CoordinatorLifecycleLog {
    /// Event type ("START", "SHUTDOWN", "POLICY_CHANGE", "THREAD_SPAWN", "THREAD_EXIT")
    std::string event_type;
    
    /// Event details (e.g., number of worker threads)
    std::string details;
    
    /// Correlation ID if applicable
    std::string correlation_id;
    
    /// Thread ID of processing thread
    std::thread::id thread_id;
    
    /// Event timestamp
    std::chrono::system_clock::time_point timestamp;
};

// ============================================================================
// § 2  Logging Interface
// ============================================================================

/**
 * @brief Thread-safe logger interface for access model events.
 *
 * Emits structured log entries using spdlog with fmt integration.
 * All methods are exception-safe and non-blocking.
 */
class AccessModelLogger {
public:
    virtual ~AccessModelLogger() = default;
    
    /**
     * @brief Log a tier transition event.
     *
     * Emitted at INFO level when a key moves between tiers.
     * Format enables parsing by structured logging backends.
     *
     * @param log Transition log entry
     */
    virtual void logTierTransition(const TierTransitionLog& log) = 0;
    
    /**
     * @brief Log a cache eviction event.
     *
     * Emitted at DEBUG level when cache reports an eviction.
     * Includes decision on whether to demote to storage.
     *
     * @param log Eviction log entry
     */
    virtual void logEvictionEvent(const EvictionEventLog& log) = 0;
    
    /**
     * @brief Log a promotion/demotion policy decision.
     *
     * Emitted at DEBUG level for all policy decisions.
     * Enables debugging of policy application.
     *
     * @param log Promotion decision log entry
     */
    virtual void logPromotionDecision(const PromotionDecisionLog& log) = 0;
    
    /**
     * @brief Log coordinator lifecycle event.
     *
     * Emitted at INFO level for startup/shutdown and config changes.
     *
     * @param log Lifecycle log entry
     */
    virtual void logCoordinatorLifecycle(const CoordinatorLifecycleLog& log) = 0;
};

/**
 * @brief Default structured logger implementation using spdlog.
 *
 * Emits logs using the canonical logger instance.
 */
class DefaultAccessModelLogger : public AccessModelLogger {
public:
    void logTierTransition(const TierTransitionLog& log) override;
    void logEvictionEvent(const EvictionEventLog& log) override;
    void logPromotionDecision(const PromotionDecisionLog& log) override;
    void logCoordinatorLifecycle(const CoordinatorLifecycleLog& log) override;
};

/**
 * @brief Get the global access model logger instance.
 *
 * Thread-safe; returns singleton instance.
 *
 * @return Reference to global logger
 */
AccessModelLogger& accessModelLogger();

}  // namespace access_model
}  // namespace themis


