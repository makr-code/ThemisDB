#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace maintenance {

// ---------------------------------------------------------------------------
// Per-module health signal
// ---------------------------------------------------------------------------

enum class ModuleHealthStatus {
    OK,       ///< Module is healthy
    DEGRADED, ///< Module is functional but shows warning indicators
    CRITICAL, ///< Module has failures that require immediate attention
    UNKNOWN,  ///< Module did not respond or is not registered
};

inline std::string moduleHealthStatusToString(ModuleHealthStatus s) {
    switch (s) {
        case ModuleHealthStatus::OK:       return "ok";
        case ModuleHealthStatus::DEGRADED: return "degraded";
        case ModuleHealthStatus::CRITICAL: return "critical";
        default:                           return "unknown";
    }
}

/**
 * @brief Health contribution from a single module.
 */
struct ModuleHealthSignal {
    std::string       module_name;
    ModuleHealthStatus status       = ModuleHealthStatus::UNKNOWN;
    std::string       message;      ///< Human-readable status message
    int64_t           checked_at_ms = 0; ///< Unix ms when this signal was collected

    /// Additional key/value details (e.g., fragmentation %, queue depth, …)
    std::map<std::string, std::string> details;

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["module_name"]   = module_name;
        j["status"]        = moduleHealthStatusToString(status);
        j["message"]       = message;
        j["checked_at_ms"] = checked_at_ms;
        nlohmann::json d = nlohmann::json::object();
        for (auto& [k, v] : details) {
          d[k] = v;
        }
        j["details"] = d;
        return j;
    }
};

// ---------------------------------------------------------------------------
// DispatchOutcome — per-dispatch diagnostic record (Phase 4)
// ---------------------------------------------------------------------------

/**
 * @brief Categorises the outcome of a single schedule/task dispatch attempt.
 */
enum class DispatchOutcomeType {
    SUCCESS,             ///< Task dispatched and completed successfully
    SKIPPED_NO_HANDLER,  ///< No IMaintenanceTaskHandler registered for this task type
    SKIPPED_CONCURRENT,  ///< Schedule was already in-flight; this invocation was skipped
    FAILED_PERSISTENCE,  ///< Persistence layer error prevented execution
    FAILED_DISPATCH,     ///< Task handler returned a failure
};

inline std::string dispatchOutcomeTypeToString(DispatchOutcomeType t) {
    switch (t) {
        case DispatchOutcomeType::SUCCESS:            return "success";
        case DispatchOutcomeType::SKIPPED_NO_HANDLER: return "skipped_no_handler";
        case DispatchOutcomeType::SKIPPED_CONCURRENT: return "skipped_concurrent";
        case DispatchOutcomeType::FAILED_PERSISTENCE: return "failed_persistence";
        case DispatchOutcomeType::FAILED_DISPATCH:    return "failed_dispatch";
        default:                                      return "unknown";
    }
}

/**
 * @brief Lightweight record emitted for every dispatch attempt.
 *
 * Stored in a ring buffer inside DatabaseMaintenanceOrchestrator and
 * surfaced via MaintenanceHealthReport::recent_dispatch_outcomes.
 */
struct DispatchOutcome {
    /// @brief Schedule identifier that triggered this dispatch.
    std::string schedule_id;
    /// @brief String representation of the task type.
    std::string task_type;
    /// @brief Outcome classification.
    DispatchOutcomeType outcome = DispatchOutcomeType::SUCCESS;
    /// @brief Wall-clock latency in microseconds (≥ 0).
    int64_t latency_us = 0;
    /// @brief Human-readable error or diagnostic message (may be empty on SUCCESS).
    std::string error_message;

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["schedule_id"]   = schedule_id;
        j["task_type"]     = task_type;
        j["outcome"]       = dispatchOutcomeTypeToString(outcome);
        j["latency_us"]    = latency_us;
        j["error_message"] = error_message;
        return j;
    }
};

// ---------------------------------------------------------------------------
// Aggregate health report
// ---------------------------------------------------------------------------

/**
 * @brief Aggregated maintenance health report.
 *
 * The overall status is the worst status across all registered modules.
 */
struct MaintenanceHealthReport {
    ModuleHealthStatus          overall_status   = ModuleHealthStatus::UNKNOWN;
    std::vector<ModuleHealthSignal> module_signals;
    int64_t                     generated_at_ms  = 0;

    // Orchestrator-level stats
    int  active_jobs       = 0;
    int  total_schedules   = 0;
    int  enabled_schedules = 0;
    int  failed_jobs_24h   = 0; ///< Failed jobs in the last 24 hours
    int  success_jobs_24h  = 0; ///< Successful jobs in the last 24 hours

    /// @brief Ring-buffer snapshot of recent dispatch outcomes (Phase 4).
    std::vector<DispatchOutcome> recent_dispatch_outcomes;

    /// @brief Configured ring buffer capacity (default 256).
    int dispatch_outcome_ring_buffer_capacity = 256;

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["overall_status"]   = moduleHealthStatusToString(overall_status);
        j["generated_at_ms"]  = generated_at_ms;
        j["active_jobs"]      = active_jobs;
        j["total_schedules"]  = total_schedules;
        j["enabled_schedules"]= enabled_schedules;
        j["failed_jobs_24h"]  = failed_jobs_24h;
        j["success_jobs_24h"] = success_jobs_24h;
        j["dispatch_outcome_ring_buffer_capacity"] = dispatch_outcome_ring_buffer_capacity;
        nlohmann::json signals = nlohmann::json::array();
        for (auto& s : module_signals) {
          signals.push_back(s.toJson());
        }
        j["module_signals"] = signals;
        nlohmann::json outcomes = nlohmann::json::array();
        for (auto& o : recent_dispatch_outcomes) {
          outcomes.push_back(o.toJson());
        }
        j["recent_dispatch_outcomes"] = outcomes;
        return j;
    }
};

} // namespace maintenance
} // namespace themis
