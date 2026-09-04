/**
 * @file maintenance_schedule.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "maintenance/maintenance_task.h"
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace maintenance {

// ---------------------------------------------------------------------------
// Schedule frequency
// ---------------------------------------------------------------------------

/**
 * @brief Built-in schedule frequency presets.
 *
 * CUSTOM lets the caller supply an explicit cron_expression.
 */
enum class ScheduleFrequency {
    DAILY,      ///< Every day at the window start hour
    WEEKLY,     ///< Every Sunday at the window start hour
    MONTHLY,    ///< First day of each month at the window start hour
    QUARTERLY,  ///< First day of Jan/Apr/Jul/Oct at the window start hour
    CUSTOM,     ///< Use cron_expression verbatim
};

inline std::string frequencyToString(ScheduleFrequency f) {
    switch (f) {
        case ScheduleFrequency::DAILY:     return "daily";
        case ScheduleFrequency::WEEKLY:    return "weekly";
        case ScheduleFrequency::MONTHLY:   return "monthly";
        case ScheduleFrequency::QUARTERLY: return "quarterly";
        case ScheduleFrequency::CUSTOM:    return "custom";
        default:                           return "unknown";
    }
}

inline ScheduleFrequency frequencyFromString(const std::string& s) {
    if (s == "daily") {
      return ScheduleFrequency::DAILY;
    }
    if (s == "weekly") {
      return ScheduleFrequency::WEEKLY;
    }
    if (s == "monthly") {
      return ScheduleFrequency::MONTHLY;
    }
    if (s == "quarterly") {
      return ScheduleFrequency::QUARTERLY;
    }
    return ScheduleFrequency::CUSTOM;
}

/// Returns the canonical cron expression for a built-in frequency.
/// @param frequency     Must not be CUSTOM.
/// @param window_start_hour  Hour-of-day (0–23) for the maintenance window.
inline std::string frequencyToCron(ScheduleFrequency frequency, int window_start_hour = 2) {
    std::string h = std::to_string(window_start_hour);
    switch (frequency) {
        case ScheduleFrequency::DAILY:     return "0 " + h + " * * *";
        case ScheduleFrequency::WEEKLY:    return "0 " + h + " * * 0";
        case ScheduleFrequency::MONTHLY:   return "0 " + h + " 1 * *";
        case ScheduleFrequency::QUARTERLY: return "0 " + h + " 1 1,4,7,10 *";
        default:                           return "0 " + h + " * * *";
    }
}

// ---------------------------------------------------------------------------
// MaintenanceTaskDependency
// ---------------------------------------------------------------------------

/**
 * @brief Declares explicit execution dependencies for a single task type.
 *
 * When one or more MaintenanceTaskDependency entries are present in a
 * MaintenanceScheduleEntry::task_dependencies list the orchestrator will
 * topologically sort the task execution graph instead of relying on the
 * positional order of the `tasks` vector.
 *
 * Example: to express "run storage_compaction only after mvcc_cleanup":
 * @code
 *   MaintenanceTaskDependency dep;
 *   dep.task_type  = MaintenanceTaskType::STORAGE_COMPACTION;
 *   dep.depends_on = { MaintenanceTaskType::MVCC_CLEANUP };
 * @endcode
 */
struct MaintenanceTaskDependency {
    MaintenanceTaskType              task_type;  ///< The task that has dependencies
    std::vector<MaintenanceTaskType> depends_on; ///< Tasks that must complete before task_type

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["task_type"] = taskTypeToString(task_type);
        nlohmann::json deps = nlohmann::json::array();
        for (const auto& d : depends_on) {
          deps.push_back(taskTypeToString(d));
        }
        j["depends_on"] = deps;
        return j;
    }

    static MaintenanceTaskDependency fromJson(const nlohmann::json& j) {
        if (!j.contains("task_type") || !j["task_type"].is_string()) {
            throw std::invalid_argument(
                "MaintenanceTaskDependency: 'task_type' field is required and must be a string");
        }
        MaintenanceTaskDependency d;
        d.task_type = taskTypeFromString(j["task_type"].get<std::string>());
        if (j.contains("depends_on")) {
            if (!j["depends_on"].is_array()) {
                throw std::invalid_argument(
                    "MaintenanceTaskDependency: 'depends_on' must be an array of strings");
            }
            for (const auto& dep : j["depends_on"]) {
                if (!dep.is_string()) {
                    throw std::invalid_argument(
                        "MaintenanceTaskDependency: each 'depends_on' entry must be a string");
                }
                d.depends_on.push_back(taskTypeFromString(dep.get<std::string>()));
            }
        }
        return d;
    }
};

// ---------------------------------------------------------------------------
// MaintenanceScheduleEntry
// ---------------------------------------------------------------------------

/**
 * @brief A durable, operator-managed maintenance schedule.
 *
 * This is the primary entity managed by the CRUD API.  Each entry
 * binds a frequency/cron expression to a set of MaintenanceTaskType
 * operations and optional maintenance-window constraints.
 */
struct MaintenanceScheduleEntry {
    // ---- Identity --------------------------------------------------------
    std::string id;          ///< UUID, assigned by orchestrator on create
    std::string name;        ///< Human-readable label (required, must be non-empty)
    std::string description; ///< Optional longer description
    std::string tenant_id;   ///< Optional tenant identifier; empty = global/system schedule

    // ---- Scheduling ------------------------------------------------------
    ScheduleFrequency frequency = ScheduleFrequency::DAILY;
    std::string cron_expression; ///< Used when frequency == CUSTOM, or auto-derived otherwise

    // ---- Operations ------------------------------------------------------
    std::vector<MaintenanceTaskType>        tasks;            ///< Ordered list of operations to run
    std::vector<MaintenanceTaskDependency>  task_dependencies; ///< Optional per-task dependency declarations.
                                                               ///< When non-empty the orchestrator resolves
                                                               ///< execution order via topological sort instead
                                                               ///< of relying on positional order in `tasks`.

    // ---- Window constraints ----------------------------------------------
    bool enabled              = true; ///< When false the schedule is registered but never fired
    bool enforce_window       = true; ///< Abort if started outside the window
    int  window_start_hour    = 2;   ///< Inclusive start (0–23, UTC)
    int  window_end_hour      = 6;   ///< Exclusive end (0–23, UTC)

    // ---- DAG / dependency enforcement ------------------------------------
    bool halt_on_task_failure = false; ///< Stop subsequent tasks if any task fails

    // ---- Distributed lock ------------------------------------------------
    /**
     * Time-to-live for the distributed lock acquired before each scheduled run,
     * in milliseconds.  0 (default) means the orchestrator auto-computes the
     * TTL from the maintenance window duration plus a 30-second safety margin.
     * Operators should set this to at least the estimated task duration + 30 s
     * to prevent premature lock expiry on slow nodes.
     */
    int64_t lock_ttl_ms = 0;

    // ---- Churn / rate-limit policy ---------------------------------------

    /**
     * @brief Maximum permitted mutations (create/update/patch/delete) per
     *        rolling interval (DatabaseMaintenanceOrchestrator::kChurnIntervalMs,
     *        default 60 s).  0 = disabled (no rate limit applied).
     *
     * ### Semantics
     * When > 0 the orchestrator counts each CRUD write for this schedule
     * within the current interval window.  The (N+1)th mutation within the
     * same window is rejected with MaintenanceError::kChurnLimitExceeded and
     * a structured error message containing the schedule_id and the current
     * count.
     *
     * ### Max schedule age concept
     * Schedules may carry an implicit age derived from `created_at_ms`.
     * Future tooling will surface schedules older than a configurable TTL
     * as candidates for review or auto-expiry (TTL-policy placeholder — see
     * FUTURE_ENHANCEMENTS.md).  The `max_schedule_changes_per_interval` field
     * participates in that governance by slowing accidental churn on aged
     * schedules.
     *
     * ### Auto-expire behaviour note
     * Auto-expiry (removing schedules that have not run within a TTL window)
     * is a planned feature.  The presence of this field does not imply
     * auto-expiry is currently active.  When auto-expiry is introduced it
     * will be gated on a separate `auto_expire_ttl_ms` field (TTL-policy
     * placeholder, currently unset / 0 = disabled).
     */
    uint32_t max_schedule_changes_per_interval = 0;

    // ---- Audit -----------------------------------------------------------
    int64_t created_at_ms  = 0; ///< Unix ms, set on create
    int64_t updated_at_ms  = 0; ///< Unix ms, updated on every CRUD write
    std::string created_by;     ///< Principal who created this entry
    std::string updated_by;     ///< Principal who last modified this entry

    // ---- Runtime state (read-only, set by orchestrator) -----------------
    int64_t     last_run_ms    = 0;       ///< Unix ms of last execution start
    int64_t     next_run_ms    = 0;       ///< Unix ms of next scheduled execution
    std::string last_run_state;           ///< "success" | "failed" | "running" | ""
    std::string last_job_id;              ///< Job ID of the most-recent execution

    // ---- Serialisation ---------------------------------------------------

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["id"]                = id;
        j["name"]              = name;
        j["description"]       = description;
        j["tenant_id"]         = tenant_id;
        j["frequency"]         = frequencyToString(frequency);
        j["cron_expression"]   = cron_expression;
        nlohmann::json task_arr = nlohmann::json::array();
        for (auto& t : tasks) {
          task_arr.push_back(taskTypeToString(t));
        }
        j["tasks"]             = task_arr;
        nlohmann::json deps_arr = nlohmann::json::array();
        for (auto& d : task_dependencies) {
          deps_arr.push_back(d.toJson());
        }
        j["task_dependencies"] = deps_arr;
        j["enabled"]           = enabled;
        j["enforce_window"]    = enforce_window;
        j["window_start_hour"] = window_start_hour;
        j["window_end_hour"]   = window_end_hour;
        j["halt_on_task_failure"] = halt_on_task_failure;
        j["lock_ttl_ms"]          = lock_ttl_ms;
        j["max_schedule_changes_per_interval"] = max_schedule_changes_per_interval;
        j["created_at_ms"]     = created_at_ms;
        j["updated_at_ms"]     = updated_at_ms;
        j["created_by"]        = created_by;
        j["updated_by"]        = updated_by;
        j["last_run_ms"]       = last_run_ms;
        j["next_run_ms"]       = next_run_ms;
        j["last_run_state"]    = last_run_state;
        j["last_job_id"]       = last_job_id;
        return j;
    }

    static MaintenanceScheduleEntry fromJson(const nlohmann::json& j) {
        MaintenanceScheduleEntry e = {};
        if (j.contains("id")) {
          e.id             = j["id"].get<std::string>();
        }
        if (j.contains("name")) {
          e.name           = j["name"].get<std::string>();
        }
        if (j.contains("description")) {
          e.description    = j["description"].get<std::string>();
        }
        if (j.contains("tenant_id")) {
          e.tenant_id      = j["tenant_id"].get<std::string>();
        }
        if (j.contains("frequency")) {
          e.frequency      = frequencyFromString(j["frequency"].get<std::string>());
        }
        if (j.contains("cron_expression")) {
          e.cron_expression= j["cron_expression"].get<std::string>();
        }
        if (j.contains("tasks")) {
            for (auto& t : j["tasks"]) {
                e.tasks.push_back(taskTypeFromString(t.get<std::string>()));
            }
        }
        if (j.contains("task_dependencies")) {
            for (auto& d : j["task_dependencies"]) {
                e.task_dependencies.push_back(MaintenanceTaskDependency::fromJson(d));
            }
        }
        if (j.contains("enabled")) {
          e.enabled             = j["enabled"].get<bool>();
        }
        if (j.contains("enforce_window")) {
          e.enforce_window      = j["enforce_window"].get<bool>();
        }
        if (j.contains("window_start_hour")) {
          e.window_start_hour   = j["window_start_hour"].get<int>();
        }
        if (j.contains("window_end_hour")) {
          e.window_end_hour     = j["window_end_hour"].get<int>();
        }
        if (j.contains("halt_on_task_failure")) {
          e.halt_on_task_failure= j["halt_on_task_failure"].get<bool>();
        }
        if (j.contains("lock_ttl_ms")) {
          e.lock_ttl_ms         = j["lock_ttl_ms"].get<int64_t>();
        }
        if (j.contains("max_schedule_changes_per_interval"))
            e.max_schedule_changes_per_interval =
                j["max_schedule_changes_per_interval"].get<uint32_t>();
        if (j.contains("created_by")) {
          e.created_by          = j["created_by"].get<std::string>();
        }
        if (j.contains("updated_by")) {
          e.updated_by          = j["updated_by"].get<std::string>();
        }
        // Audit and runtime state: restore from persisted payload so that
        // schedule timestamps and last-run tracking survive persistence reload.
        if (j.contains("created_at_ms")) {
          e.created_at_ms  = j["created_at_ms"].get<int64_t>();
        }
        if (j.contains("updated_at_ms")) {
          e.updated_at_ms  = j["updated_at_ms"].get<int64_t>();
        }
        if (j.contains("last_run_ms")) {
          e.last_run_ms     = j["last_run_ms"].get<int64_t>();
        }
        if (j.contains("next_run_ms")) {
          e.next_run_ms     = j["next_run_ms"].get<int64_t>();
        }
        if (j.contains("last_run_state")) {
          e.last_run_state  = j["last_run_state"].get<std::string>();
        }
        if (j.contains("last_job_id")) {
          e.last_job_id     = j["last_job_id"].get<std::string>();
        }
        return e;
    }

    /// Apply a partial JSON patch (PATCH semantics: only provided fields are updated).
    void applyPatch(const nlohmann::json& patch) {
        if (patch.contains("name")) {
          name              = patch["name"].get<std::string>();
        }
        if (patch.contains("description")) {
          description       = patch["description"].get<std::string>();
        }
        if (patch.contains("tenant_id")) {
          tenant_id         = patch["tenant_id"].get<std::string>();
        }
        if (patch.contains("frequency")) {
          frequency         = frequencyFromString(patch["frequency"].get<std::string>());
        }
        if (patch.contains("cron_expression")) {
          cron_expression   = patch["cron_expression"].get<std::string>();
        }
        if (patch.contains("tasks")) {
            tasks.clear();
            for (auto& t : patch["tasks"]) {
                tasks.push_back(taskTypeFromString(t.get<std::string>()));
            }
        }
        if (patch.contains("task_dependencies")) {
            task_dependencies.clear();
            for (auto& d : patch["task_dependencies"]) {
                task_dependencies.push_back(MaintenanceTaskDependency::fromJson(d));
            }
        }
        if (patch.contains("enabled")) {
          enabled              = patch["enabled"].get<bool>();
        }
        if (patch.contains("enforce_window")) {
          enforce_window       = patch["enforce_window"].get<bool>();
        }
        if (patch.contains("window_start_hour")) {
          window_start_hour    = patch["window_start_hour"].get<int>();
        }
        if (patch.contains("window_end_hour")) {
          window_end_hour      = patch["window_end_hour"].get<int>();
        }
        if (patch.contains("halt_on_task_failure")) {
          halt_on_task_failure = patch["halt_on_task_failure"].get<bool>();
        }
        if (patch.contains("lock_ttl_ms")) {
          lock_ttl_ms          = patch["lock_ttl_ms"].get<int64_t>();
        }
        if (patch.contains("max_schedule_changes_per_interval"))
            max_schedule_changes_per_interval =
                patch["max_schedule_changes_per_interval"].get<uint32_t>();
    }
};

} // namespace maintenance
} // namespace themis

