/*
 * ThemisDB | File: maintenance_health_report.h | Version: 0.0.13
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file maintenance_health_report.h
 * @brief Health aggregation schema for the DatabaseMaintenanceOrchestrator.
 *
 * The HealthReport combines per-module health signals into a single view
 * that operators can query via GET /api/v1/maintenance/health.
 */

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
        for (auto& [k, v] : details) d[k] = v;
        j["details"] = d;
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

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["overall_status"]   = moduleHealthStatusToString(overall_status);
        j["generated_at_ms"]  = generated_at_ms;
        j["active_jobs"]      = active_jobs;
        j["total_schedules"]  = total_schedules;
        j["enabled_schedules"]= enabled_schedules;
        j["failed_jobs_24h"]  = failed_jobs_24h;
        j["success_jobs_24h"] = success_jobs_24h;
        nlohmann::json signals = nlohmann::json::array();
        for (auto& s : module_signals) signals.push_back(s.toJson());
        j["module_signals"] = signals;
        return j;
    }
};

} // namespace maintenance
} // namespace themis
