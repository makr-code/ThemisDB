/**
 * @file access_model_logging.cpp
 * @brief Implementation of structured logging for access model.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2024 ThemisDB Contributors
 */

#include "access_model/access_model_logging.h"

#include "utils/logger.h"

#include <iomanip>
#include <sstream>

namespace themis {
namespace access_model {

// ============================================================================
// § 1  DefaultAccessModelLogger Implementation
// ============================================================================

void DefaultAccessModelLogger::logTierTransition(const TierTransitionLog& log) {
    try {
        utils::logger()->info(
            "access_model::tier_transition | key={} | from={} | to={} | "
            "reason={} | latency_ms={} | status={} | correlation_id={} | "
            "thread_id={}",
            log.key,
            tierLevelName(log.from_tier),
            tierLevelName(log.to_tier),
            log.reason,
            log.latency_ms,
            log.status,
            log.correlation_id,
            log.thread_id);
    } catch (...) {
        // Logging exceptions are silently swallowed
    }
}

void DefaultAccessModelLogger::logEvictionEvent(const EvictionEventLog& log) {
    try {
        utils::logger()->debug(
            "access_model::eviction | key={} | from={} | reason={} | "
            "size_bytes={} | access_count={} | last_access_age_secs={} | "
            "decision={} | correlation_id={} | thread_id={}",
            log.key,
            tierLevelName(log.from_tier),
            log.eviction_reason,
            log.size_bytes,
            log.access_count,
            log.last_access_age.count(),
            log.decision,
            log.correlation_id,
            log.thread_id);
    } catch (...) {
        // Logging exceptions are silently swallowed
    }
}

void DefaultAccessModelLogger::logPromotionDecision(
    const PromotionDecisionLog& log) {
    try {
        utils::logger()->debug(
            "access_model::promotion_decision | key={} | current_tier={} | "
            "target_tier={} | decision={} | access_count={} | age_secs={} | "
            "threshold_name={} | threshold_value={} | actual_value={} | "
            "reason={} | correlation_id={} | thread_id={}",
            log.key,
            tierLevelName(log.current_tier),
            log.target_tier.has_value() ? tierLevelName(log.target_tier.value())
                                         : "NONE",
            log.decision,
            log.access_count,
            log.age_secs.count(),
            log.threshold_name,
            log.threshold_value,
            log.actual_value,
            log.reason,
            log.correlation_id,
            log.thread_id);
    } catch (...) {
        // Logging exceptions are silently swallowed
    }
}

void DefaultAccessModelLogger::logCoordinatorLifecycle(
    const CoordinatorLifecycleLog& log) {
    try {
        utils::logger()->info(
            "access_model::coordinator_lifecycle | event_type={} | "
            "details={} | correlation_id={} | thread_id={}",
            log.event_type,
            log.details,
            log.correlation_id,
            log.thread_id);
    } catch (...) {
        // Logging exceptions are silently swallowed
    }
}

// ============================================================================
// § 2  Global Logger Instance
// ============================================================================

static DefaultAccessModelLogger g_default_logger;

AccessModelLogger& accessModelLogger() {
    return g_default_logger;
}

}  // namespace access_model
}  // namespace themis
