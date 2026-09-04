// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ThemisDB Contributors
//
// @file
// @brief Promotion/demotion data-structure helpers.
// @version 1.0.0 (Phase 1-2 frozen API contract + Phase 2 struct utilities)

#include "access_model/promotion_demotion.h"

#include <chrono>
#include <sstream>
#include <string>

namespace themis {
namespace access_model {

// ============================================================================
// DemotionPlan utilities
// ============================================================================

/**
 * @brief Returns true when the grace period has elapsed and the plan is ready
 *        for execution.
 *
 * Used by AccessCoordinator::executeDemotion() to guard against early execution.
 *
 * @param plan Demotion plan created by AccessCoordinator::planDemotion()
 * @return true if now >= plan.scheduled_execution_time
 */
static bool isPlanExecutable(const DemotionPlan& plan) {
    return std::chrono::system_clock::now() >= plan.scheduled_execution_time;
}

// ============================================================================
// DemotionResult helpers
// ============================================================================

/**
 * @brief Returns a human-readable description of a DemotionResult.
 *
 * Intended for logging and diagnostic output.
 *
 * @param result Completed demotion result
 * @return Formatted string representation
 */
static std::string describeResult(const DemotionResult& result) {
    std::ostringstream oss = {};
    oss << "DemotionResult{"
        << "success=" << (result.success ? "true" : "false")
        << ", size_bytes=" << result.size_bytes
        << ", latency_ms=" << result.total_latency_ms.count()
        << ", correlation_id=" << result.correlation_id
        << "}";
    return oss.str();
}

}  // namespace access_model
}  // namespace themis
