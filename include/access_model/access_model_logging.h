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

struct TierTransitionLog {
    std::string key;
    
    TierLevel from_tier;
    
    TierLevel to_tier;
    
    std::string reason;
    
    uint64_t latency_ms;
    
    std::string correlation_id;
    
    std::thread::id thread_id;
    
    std::chrono::system_clock::time_point timestamp;
    
    std::string status;  // "SUCCESS", "REJECTED", "FAILED", "DEFERRED"
};

struct EvictionEventLog {
    std::string key = {};
    
    TierLevel from_tier;
    
    std::string eviction_reason;
    
    size_t size_bytes;
    
    uint64_t access_count;
    
    std::chrono::seconds last_access_age;
    
    std::string decision;
    
    std::string correlation_id;
    
    std::thread::id thread_id;
    
    std::chrono::system_clock::time_point timestamp;
};

struct PromotionDecisionLog {
    std::string key;
    
    TierLevel current_tier;
    
    std::optional<TierLevel> target_tier;
    
    std::string decision = {};
    
    uint64_t access_count;
    
    std::chrono::seconds age_secs;
    
    std::string threshold_name;
    
    uint64_t threshold_value;
    
    uint64_t actual_value;
    
    std::string reason;
    
    std::string correlation_id;
    
    std::thread::id thread_id;
    
    std::chrono::system_clock::time_point timestamp;
};

struct CoordinatorLifecycleLog {
    std::string event_type;
    
    std::string details;
    
    std::string correlation_id;
    
    std::thread::id thread_id;
    
    std::chrono::system_clock::time_point timestamp;
};

// ============================================================================
// § 2  Logging Interface
// ============================================================================

class AccessModelLogger {
public:
    /**
     * @brief Access Model Logger.
     * @return Return value.
     */
    virtual ~AccessModelLogger() = default;
    
    /**
     * @brief Log Tier Transition.
     * @param[in] log Input parameter.
     */
    virtual void logTierTransition(const TierTransitionLog& log) = 0;
    
    /**
     * @brief Log Eviction Event.
     * @param[in] log Input parameter.
     */
    virtual void logEvictionEvent(const EvictionEventLog& log) = 0;
    
    /**
     * @brief Log Promotion Decision.
     * @param[in] log Input parameter.
     */
    virtual void logPromotionDecision(const PromotionDecisionLog& log) = 0;
    
    /**
     * @brief Log Coordinator Lifecycle.
     * @param[in] log Input parameter.
     */
    virtual void logCoordinatorLifecycle(const CoordinatorLifecycleLog& log) = 0;
};

class DefaultAccessModelLogger : public AccessModelLogger {
public:
    void logTierTransition(const TierTransitionLog& log) override;
    void logEvictionEvent(const EvictionEventLog& log) override;
    void logPromotionDecision(const PromotionDecisionLog& log) override;
    void logCoordinatorLifecycle(const CoordinatorLifecycleLog& log) override;
};

/**
 * @brief Access Model Logger.
 * @return Return value.
 */
AccessModelLogger& accessModelLogger();

}  // namespace access_model
}  // namespace themis


