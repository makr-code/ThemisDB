/**
 * @file promotion_demotion.h
 * @brief Data structures for promotion/demotion operations.
 *
 * ThemisDB | File: promotion_demotion.h | Version: 1.0.0
 * Maturity: 🟡 ALPHA (Phase 1 API Definition) | Status: Frozen for v1.x
 * Author: Copilot | Date: 2026-08-03
 *
 * @see include/access_model/access_coordinator.h
 */

#pragma once

#include "access_tier_interface.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace access_model {

// ============================================================================
// § 1  Demotion Plan & Execution
// ============================================================================

/**
 * @brief Plan for demoting data from a tier.
 *
 * Created by AccessCoordinator::planDemotion() but not executed until
 * AccessCoordinator::executeDemotion() is called.
 */
struct DemotionPlan {
    /// Unique plan ID (for tracking)
    std::string plan_id;

    /// Data key being demoted
    std::string key;

    /// Source tier (where data currently resides)
    TierLevel from_tier;

    /// Destination tier (where data will be moved)
    TierLevel to_tier;

    /// Reason for demotion (e.g., "age", "cache_eviction", "capacity_pressure")
    std::string reason;

    /// Grace period before demotion execution (allows cancellation)
    std::chrono::seconds grace_period_secs{600};  // Default: 10 minutes

    /// Size of data to be demoted (bytes)
    std::size_t data_size_bytes = 0;

    /// Access count at time of planning
    uint64_t access_count_at_plan = 0;

    /// Whether demotion is scheduled (vs. already executing or completed)
    bool is_scheduled = true;

    /// Timestamp when plan was created
    std::chrono::system_clock::time_point created_at = std::chrono::system_clock::now();

    /// Timestamp when demotion should begin (created_at + grace_period)
    std::chrono::system_clock::time_point scheduled_execution_time = std::chrono::system_clock::now();
};

/**
 * @brief Result of a demotion operation.
 */
struct DemotionResult {
    /// Operation succeeded
    bool success = false;

    /// Error message (if success=false)
    std::string error_message;

    /// Data size demoted (bytes)
    std::size_t size_bytes = 0;

    /// Source tier (where data was demoted from)
    TierLevel from_tier;

    /// Destination tier (where data was moved to)
    TierLevel to_tier;

    /// Total latency for demotion (end-to-end, in ms)
    std::chrono::milliseconds total_latency_ms;

    /// Correlation ID for tracing
    std::string correlation_id;

    /// Timestamp when demotion completed
    std::chrono::system_clock::time_point completed_at = std::chrono::system_clock::now();
};

}  // namespace access_model
}  // namespace themis

