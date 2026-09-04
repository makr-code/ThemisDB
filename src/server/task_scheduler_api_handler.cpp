/**
 * @file task_scheduler_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=8; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=0, M=123, L=5
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/task_scheduler_api_handler.h"
#include <stdexcept>
#include "scheduler/task_audit_manager.h"
#include "scheduler/external_scheduler_adapter.h"
#include "utils/input_validator.h"

#include <spdlog/spdlog.h>
#include <chrono>
#include <ctime>
#include <limits>
#include <optional>
#include <sstream>
#include "utils/tracing.h"

using nlohmann::json;

namespace themis {
namespace server {

// ============================================================================
// Helper utilities
// ============================================================================

namespace {

constexpr size_t kMaxTaskIdentifierLength = 128;

std::optional<std::string> validateTaskIdentifier(
    const std::string& value,
    const std::string& field_name)
{
    themis::utils::InputValidator validator;
    if (value.empty()) {
        return field_name + " must not be empty";
    }
    if (!validator.validateStringLength(value, kMaxTaskIdentifierLength)) {
        return field_name + " exceeds maximum allowed length";
    }
    if (!validator.validatePathSegment(value)) {
        return field_name + " contains invalid characters";
    }
    return std::nullopt;
}

int64_t checkedSecondsToMilliseconds(int64_t seconds, const char* field_name) {
    if (seconds <= 0) {
        throw std::invalid_argument(std::string(field_name) + " must be a positive integer");
    }

    constexpr int64_t kMillisPerSecond = 1000;
    if (seconds > (std::numeric_limits<int64_t>::max() / kMillisPerSecond)) {
        throw std::invalid_argument(std::string(field_name) + " is too large");
    }

    return seconds * kMillisPerSecond;
}

/// Convert a system_clock time_point to an ISO-8601 string (UTC).
std::string timePointToIso(std::chrono::system_clock::time_point tp) {
    if (tp == std::chrono::system_clock::time_point{} ||
        tp == std::chrono::system_clock::time_point::max()) {
        return "";
    }
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm{};
#ifdef _WIN32
    if (gmtime_s(&tm, &t) != 0) {
        return "";
    }
#else
    if (gmtime_r(&t, &tm) == nullptr) {
        return "";
    }
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buf;
}

/// Stringify ScheduledTask::TaskType.
std::string taskTypeStr(ScheduledTask::TaskType t) {
    return t == ScheduledTask::TaskType::AQL_QUERY ? "aql_query" : "function";
}

/// Stringify ScheduledTask::TriggerType.
std::string triggerTypeStr(ScheduledTask::TriggerType t) {
    switch (t) {
        case ScheduledTask::TriggerType::CRON:      return "cron";
        case ScheduledTask::TriggerType::INTERVAL:  return "interval";
        case ScheduledTask::TriggerType::CDC_EVENT: return "cdc_event";
        case ScheduledTask::TriggerType::WEBHOOK:   return "webhook";
        case ScheduledTask::TriggerType::MANUAL:    return "manual";
        default:                                    return "unknown";
    }
}

/// Stringify ScheduledTask::ErrorCategory.
std::string errorCategoryStr(ScheduledTask::ErrorCategory c) {
    switch (c) {
        case ScheduledTask::ErrorCategory::NONE:       return "none";
        case ScheduledTask::ErrorCategory::TRANSIENT:  return "transient";
        case ScheduledTask::ErrorCategory::PERMANENT:  return "permanent";
        case ScheduledTask::ErrorCategory::TIMEOUT:    return "timeout";
        case ScheduledTask::ErrorCategory::RESOURCE:   return "resource";
        case ScheduledTask::ErrorCategory::SECURITY:   return "security";
        default:                                       return "unknown";
    }
}

} // anonymous namespace

// ============================================================================
// Public API
// ============================================================================

json TaskSchedulerApiHandler::registerTask([[maybe_unused]] const json& request) {
    if (!scheduler_) {
    auto span = Tracer::startSpan("registerTask");
        return json{{"status", "error"}, {"error", "Scheduler not initialized"}};
    }
    auto& scheduler = *scheduler_;
    try {
        auto task = parseTaskFromJson(request);
        std::string id = scheduler.registerTask(task);
        spdlog::info("TaskSchedulerApiHandler: registered task '{}'", id);
        return json{{"status", "created"}, {"id", id}};
    } catch (const std::invalid_argument& e) {
        spdlog::warn("TaskSchedulerApiHandler: registerTask failed: {}", e.what());
        return json{{"status", "error"}, {"error", e.what()}};
    } catch (const std::exception& e) {
        spdlog::warn("TaskSchedulerApiHandler: registerTask failed: {}", e.what());
        return json{{"status", "error"}, {"error", "Internal server error"}};
    }
}

json TaskSchedulerApiHandler::listTasks() {
    if (!scheduler_) {
    auto span = Tracer::startSpan("listTasks");
        return json{{"status", "error"}, {"error", "Scheduler not initialized"}};
    }
    auto& scheduler = *scheduler_;
    auto tasks = scheduler.listTasks();
    json items = json::array();
    for (const auto& t : tasks) {
        items.push_back(taskToJson(t));
    }
    return json{{"items", items}, {"total", static_cast<int64_t>(items.size())}};
}

json TaskSchedulerApiHandler::getTask([[maybe_unused]] const std::string& task_id) {
    if (!scheduler_) {
    auto span = Tracer::startSpan("getTask");
        return json{{"status", "error"}, {"error", "Scheduler not initialized"}};
    }
    auto& scheduler = *scheduler_;

    if (const auto err = validateTaskIdentifier(task_id, "task_id")) {
        return json{{"status", "error"}, {"error", *err}};
    }

    auto task_ptr = scheduler.getTask(task_id);
    if (!task_ptr) {
        return json{{"status", "error"}, {"error", "Task not found"}};
    }
    return taskToJson(*task_ptr);
}

json TaskSchedulerApiHandler::updateTask(const std::string& task_id, const json& request) {
    if (!scheduler_) {
    auto span = Tracer::startSpan("updateTask");
        return json{{"status", "error"}, {"error", "Scheduler not initialized"}};
    }
    auto& scheduler = *scheduler_;

    if (const auto err = validateTaskIdentifier(task_id, "task_id")) {
        return json{{"status", "error"}, {"error", *err}};
    }

    try {
        auto task = parseTaskFromJson(request);
        task.id = task_id; // Ensure ID matches the URL parameter
        scheduler.updateTask(task);
        spdlog::info("TaskSchedulerApiHandler: updated task '{}'", task_id);
        return json{{"status", "updated"}, {"id", task_id}};
    } catch (const std::exception& e) {
        spdlog::warn("TaskSchedulerApiHandler: updateTask '{}' failed: {}", task_id, e.what());
        return json{{"status", "error"}, {"error", "Internal server error"}};
    }
}

json TaskSchedulerApiHandler::unregisterTask([[maybe_unused]] const std::string& task_id) {
    if (!scheduler_) {
    auto span = Tracer::startSpan("unregisterTask");
        return json{{"status", "error"}, {"error", "Scheduler not initialized"}};
    }
    auto& scheduler = *scheduler_;

    if (const auto err = validateTaskIdentifier(task_id, "task_id")) {
        return json{{"status", "error"}, {"error", *err}};
    }

    if (!scheduler.getTask(task_id)) {
        spdlog::warn("TaskSchedulerApiHandler: unregisterTask '{}': task not found", task_id);
        return json{{"status", "error"}, {"error", "Task not found"}};
    }
    try {
        scheduler.unregisterTask(task_id);
        spdlog::info("TaskSchedulerApiHandler: unregistered task '{}'", task_id);
        return json{{"status", "deleted"}, {"id", task_id}};
    } catch (const std::exception& e) {
        spdlog::warn("TaskSchedulerApiHandler: unregisterTask '{}' failed: {}", task_id, e.what());
        return json{{"status", "error"}, {"error", "Internal server error"}};
    }
}

json TaskSchedulerApiHandler::enableTask([[maybe_unused]] const std::string& task_id) {
    if (!scheduler_) {
    auto span = Tracer::startSpan("enableTask");
        return json{{"status", "error"}, {"error", "Scheduler not initialized"}};
    }
    auto& scheduler = *scheduler_;

    if (const auto err = validateTaskIdentifier(task_id, "task_id")) {
        return json{{"status", "error"}, {"error", *err}};
    }

    if (!scheduler.getTask(task_id)) {
        spdlog::warn("TaskSchedulerApiHandler: enableTask '{}': task not found", task_id);
        return json{{"status", "error"}, {"error", "Task not found"}};
    }
    try {
        scheduler.enableTask(task_id);
        spdlog::info("TaskSchedulerApiHandler: enabled task '{}'", task_id);
        return json{{"status", "enabled"}, {"id", task_id}};
    } catch (const std::exception& e) {
        spdlog::warn("TaskSchedulerApiHandler: enableTask '{}' failed: {}", task_id, e.what());
        return json{{"status", "error"}, {"error", "Internal server error"}};
    }
}

json TaskSchedulerApiHandler::disableTask([[maybe_unused]] const std::string& task_id) {
    if (!scheduler_) {
    auto span = Tracer::startSpan("disableTask");
        return json{{"status", "error"}, {"error", "Scheduler not initialized"}};
    }
    auto& scheduler = *scheduler_;

    if (const auto err = validateTaskIdentifier(task_id, "task_id")) {
        return json{{"status", "error"}, {"error", *err}};
    }

    if (!scheduler.getTask(task_id)) {
        spdlog::warn("TaskSchedulerApiHandler: disableTask '{}': task not found", task_id);
        return json{{"status", "error"}, {"error", "Task not found"}};
    }
    try {
        scheduler.disableTask(task_id);
        spdlog::info("TaskSchedulerApiHandler: disabled task '{}'", task_id);
        return json{{"status", "disabled"}, {"id", task_id}};
    } catch (const std::exception& e) {
        spdlog::warn("TaskSchedulerApiHandler: disableTask '{}' failed: {}", task_id, e.what());
        return json{{"status", "error"}, {"error", "Internal server error"}};
    }
}

json TaskSchedulerApiHandler::executeTask([[maybe_unused]] const std::string& task_id) {
    if (!scheduler_) {
    auto span = Tracer::startSpan("executeTask");
        return json{{"status", "error"}, {"error", "Scheduler not initialized"}};
    }
    auto& scheduler = *scheduler_;

    if (const auto err = validateTaskIdentifier(task_id, "task_id")) {
        return json{{"status", "error"}, {"error", *err}};
    }

    try {
        auto result = scheduler.executeTaskNow(task_id);
        if (result.contains("error")) {
            spdlog::warn("TaskSchedulerApiHandler: executeTask '{}' denied/failed: {}",
                         task_id, result.value("error", "unknown"));
            return json{{"status", "error"}, {"error", result.value("error", "Internal server error")}};
        }
        spdlog::info("TaskSchedulerApiHandler: executed task '{}'", task_id);
        return json{{"status", "executed"}, {"id", task_id}, {"result", result}};
    } catch (const std::exception& e) {
        spdlog::warn("TaskSchedulerApiHandler: executeTask '{}' failed: {}", task_id, e.what());
        return json{{"status", "error"}, {"error", "Internal server error"}};
    }
}

json TaskSchedulerApiHandler::getStats() {
    if (!scheduler_) {
    auto span = Tracer::startSpan("getStats");
        return json{{"status", "error"}, {"error", "Scheduler not initialized"}};
    }
    auto& scheduler = *scheduler_;
    auto stats = scheduler.getStats();
    return json{
        {"registered_tasks",  stats.registered_tasks},
        {"active_tasks",      stats.active_tasks},
        {"running_tasks",     stats.running_tasks},
        {"total_executions",  stats.total_executions},
        {"failed_executions", stats.failed_executions},
        {"last_run",          timePointToIso(stats.last_run)},
        {"next_run",          timePointToIso(stats.next_run)},
        {"scheduler_running", scheduler.isRunning()}
    };
}

json TaskSchedulerApiHandler::getTaskResults(const std::string& task_id, size_t limit) {
    if (!scheduler_) {
    auto span = Tracer::startSpan("getTaskResults");
        return json{{"status", "error"}, {"error", "Scheduler not initialized"}};
    }
    auto& scheduler = *scheduler_;

    if (const auto err = validateTaskIdentifier(task_id, "task_id")) {
        return json{{"status", "error"}, {"error", *err}};
    }

    auto results = scheduler.getTaskResults(task_id, limit);
    json items = json::array();
    for (const auto& r : results) {
        items.push_back(r.toJson());
    }
    return json{{"task_id", task_id}, {"items", items}, {"count", items.size()}};
}

json TaskSchedulerApiHandler::getLatestTaskResult([[maybe_unused]] const std::string& task_id) {
    if (!scheduler_) {
    auto span = Tracer::startSpan("getLatestTaskResult");
        return json{{"status", "error"}, {"error", "Scheduler not initialized"}};
    }
    auto& scheduler = *scheduler_;

    if (const auto err = validateTaskIdentifier(task_id, "task_id")) {
        return json{{"status", "error"}, {"error", *err}};
    }

    auto result = scheduler.getLatestTaskResult(task_id);
    if (!result.has_value()) {
        return json{{"status", "not_found"}, {"task_id", task_id}};
    }
    return result->toJson();
}

json TaskSchedulerApiHandler::getExecutionHistory(
    const std::string& task_id,
    const json& query_params)
{
    auto span = Tracer::startSpan("getExecutionHistory");
    if (!scheduler_) {
        return json{{"status", "error"}, {"error", "Scheduler not initialized"}};
    }
    auto& scheduler = *scheduler_;

    auto audit_mgr = scheduler.getAuditManager();
    if (!audit_mgr) {
        // Audit logging disabled – return empty history
        return json{{"items", json::array()}, {"total", 0}};
    }

    scheduler::AuditQueryParams params;

    // task_id filter
    if (!task_id.empty()) {
        params.task_id = task_id;
    }

    // Pagination - handle both string values (from URL query params) and integer values
    auto getSize = [&](const char* key, size_t def) -> size_t {
        if (!query_params.contains(key)) {
          return def;
        }
        const auto& v = query_params[key];
        if (v.is_number_unsigned()) {
          return v.get<size_t>();
        }
        if (v.is_number_integer()) {
            auto iv = v.get<int64_t>();
            return iv > 0 ? static_cast<size_t>(iv) : def;
        }
        if (v.is_string()) {
            try { auto iv = std::stoll(v.get<std::string>()); return iv > 0 ? static_cast<size_t>(iv) : def; }
            catch (...) { return def; }
        }
        return def;
    };
    params.limit  = getSize("limit",  100);
    params.offset = getSize("offset", 0);

    // Optional filters
    if (query_params.contains("success") && !query_params["success"].is_null()) {
        const auto& sv = query_params["success"];
        if (sv.is_boolean()) {
            params.success = sv.get<bool>();
        } else if (sv.is_string()) {
            const auto s = sv.get<std::string>();
            if (s == "true" || s == "1") {
              params.success = true;
            }
            else if (s == "false" || s == "0") params.success = false;
        }
    }
    if ([[maybe_unused]] query_params.contains("event_type") && query_params["event_type"].is_string()) {
        params.event_type = scheduler::taskEventTypeFromString(
            query_params["event_type"].get<std::string>());
    }
    if (query_params.contains("trigger_type") && query_params["trigger_type"].is_string()) {
        params.trigger_type = query_params["trigger_type"].get<std::string>();
    }
    if (query_params.contains("user_id") && query_params["user_id"].is_string()) {
        params.user_id = query_params["user_id"].get<std::string>();
    }
    if (query_params.contains("start_time_ms") && !query_params["start_time_ms"].is_null()) {
        int64_t ms = 0;
        const auto& v = query_params["start_time_ms"];
        if (v.is_number_integer()) {
          ms = v.get<int64_t>();
        }
        else if (v.is_string()) { try { ms = std::stoll(v.get<std::string>()); } catch (...) {} }
        if (ms > 0) {
          params.start_time = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
        }
    }
    if (query_params.contains("end_time_ms") && !query_params["end_time_ms"].is_null()) {
        int64_t ms = 0;
        const auto& v = query_params["end_time_ms"];
        if (v.is_number_integer()) {
          ms = v.get<int64_t>();
        }
        else if (v.is_string()) { try { ms = std::stoll(v.get<std::string>()); } catch (...) {} }
        if (ms > 0) {
          params.end_time = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
        }
    }

    params.sort_by = scheduler::AuditQueryParams::SortBy::TIMESTAMP_DESC;

    auto events = audit_mgr->queryAuditEvents([[maybe_unused]] params);

    // Compute total matching records (without pagination) for proper pagination support.
    // Use a count-only query with max_query_results to bound memory usage.
    scheduler::AuditQueryParams count_params = params;
    count_params.offset = 0;
    count_params.limit  = audit_mgr->getConfig().max_query_results;
    auto all_events = audit_mgr->queryAuditEvents([[maybe_unused]] count_params);
    const int64_t total_count = static_cast<int64_t>([[maybe_unused]] all_events.size());

    json items = json::array();
    for ([[maybe_unused]] const auto& ev : events) {
        items.push_back(ev.toJson(false));
    }

    return json{{"items", items}, {"total", total_count}};
}

std::string TaskSchedulerApiHandler::getWebUi() {
    std::string html = {};
    html.reserve(65536);

    html += "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
    html += "<meta charset=\"UTF-8\">\n";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n";
    html += "<title>ThemisDB – Task Scheduler</title>\n";
    html += "<style>\n";
    html += "*, *::before, *::after{box-sizing:border-box}\n";
    auto span = Tracer::startSpan("getWebUi");
    html += "body{font-family:'Segoe UI',system-ui,sans-serif;background:#0f172a;color:#e2e8f0;margin:0;padding:0}\n";
    html += "header{background:#1e293b;padding:14px 24px;display:flex;align-items:center;gap:16px;border-bottom:1px solid #334155}\n";
    html += "header h1{margin:0;font-size:1.25rem;color:#38bdf8}\n";
    html += "header .badge{font-size:.75rem;background:#0ea5e9;color:#fff;padding:2px 8px;border-radius:999px}\n";
    html += ".container{max-width:1200px;margin:24px auto;padding:0 16px}\n";
    html += ".toolbar{display:flex;gap:10px;margin-bottom:16px;flex-wrap:wrap;align-items:center}\n";
    html += "button{cursor:pointer;border:none;border-radius:6px;padding:8px 16px;font-size:.875rem;font-weight:500;transition:opacity .15s}\n";
    html += "button:hover{opacity:.85}\n";
    html += ".btn-primary{background:#0ea5e9;color:#fff}\n";
    html += ".btn-success{background:#22c55e;color:#fff}\n";
    html += ".btn-warning{background:#f59e0b;color:#fff}\n";
    html += ".btn-danger{background:#ef4444;color:#fff}\n";
    html += ".btn-secondary{background:#475569;color:#e2e8f0}\n";
    html += "table{width:100%;border-collapse:collapse;background:#1e293b;border-radius:8px;overflow:hidden;font-size:.875rem}\n";
    html += "thead{background:#0f172a}\n";
    html += "th{padding:10px 14px;text-align:left;color:#94a3b8;font-weight:600;white-space:nowrap}\n";
    html += "td{padding:10px 14px;border-top:1px solid #334155}\n";
    html += "tr:hover td{background:#253347}\n";
    html += ".status-enabled{color:#22c55e;font-weight:600}\n";
    html += ".status-disabled{color:#94a3b8}\n";
    html += ".status-running{color:#f59e0b;font-weight:600}\n";
    html += ".tag{display:inline-block;padding:1px 8px;border-radius:999px;font-size:.75rem;font-weight:500}\n";
    html += ".tag-aql{background:#1d4ed8;color:#bfdbfe}\n";
    html += ".tag-fn{background:#6d28d9;color:#ede9fe}\n";
    html += ".stats-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(160px,1fr));gap:12px;margin-bottom:20px}\n";
    html += ".stat-card{background:#1e293b;border-radius:8px;padding:14px;text-align:center}\n";
    html += ".stat-card .val{font-size:1.75rem;font-weight:700;color:#38bdf8}\n";
    html += ".stat-card .lbl{font-size:.75rem;color:#94a3b8;margin-top:2px}\n";
    html += "dialog{background:#1e293b;color:#e2e8f0;border:1px solid #334155;border-radius:10px;padding:24px;min-width:500px;max-width:680px}\n";
    html += "dialog::backdrop{background:rgba(0,0,0,.6)}\n";
    html += "dialog h2{margin:0 0 16px;font-size:1.1rem;color:#38bdf8}\n";
    html += "label{display:block;margin:10px 0 4px;font-size:.8125rem;color:#94a3b8}\n";
    html += "input,select,textarea{width:100%;background:#0f172a;border:1px solid #334155;border-radius:6px;padding:8px 10px;color:#e2e8f0;font-size:.875rem}\n";
    html += "input:focus,select:focus,textarea:focus{outline:none;border-color:#0ea5e9}\n";
    html += "textarea{resize:vertical;min-height:80px;font-family:monospace}\n";
    html += ".dialog-actions{display:flex;justify-content:flex-end;gap:8px;margin-top:16px}\n";
    html += ".toast{position:fixed;bottom:20px;right:20px;background:#1e293b;border:1px solid #334155;border-radius:8px;padding:12px 20px;font-size:.875rem;opacity:0;transition:opacity .3s;pointer-events:none}\n";
    html += ".toast.show{opacity:1;pointer-events:auto}\n";
    html += ".toast.ok{border-left:4px solid #22c55e}\n";
    html += ".toast.err{border-left:4px solid #ef4444}\n";
    html += "#refresh-indicator{font-size:.75rem;color:#64748b;margin-left:auto}\n";
    html += "</style>\n</head>\n<body>\n";

    html += "<header>\n";
    html += "  <h1>&#x23F2; Task Scheduler</h1>\n";
    html += "  <span class=\"badge\">ThemisDB</span>\n";
    html += "</header>\n";

    html += "<div class=\"container\">\n";

    // Stats grid
    html += "<div class=\"stats-grid\" id=\"stats-grid\">\n";
    html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-registered\">–</div><div class=\"lbl\">Registered</div></div>\n";
    html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-active\">–</div><div class=\"lbl\">Active</div></div>\n";
    html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-running\">–</div><div class=\"lbl\">Running</div></div>\n";
    html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-total\">–</div><div class=\"lbl\">Executions</div></div>\n";
    html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-failed\">–</div><div class=\"lbl\">Failures</div></div>\n";
    html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-status\">–</div><div class=\"lbl\">Scheduler</div></div>\n";
    html += "</div>\n";

    // Toolbar
    html += "<div class=\"toolbar\">\n";
    html += "  <button class=\"btn-primary\" onclick=\"openCreateDialog()\">&#43; New Task</button>\n";
    html += "  <button class=\"btn-secondary\" onclick=\"loadAll()\">&#8635; Refresh</button>\n";
    html += "  <span id=\"refresh-indicator\"></span>\n";
    html += "</div>\n";

    // Table
    html += "<table>\n<thead><tr>\n";
    html += "  <th>ID / Name</th><th>Type</th><th>Trigger</th><th>Status</th>\n";
    html += "  <th>Executions</th><th>Last Error</th><th>Next Run</th><th>Actions</th>\n";
    html += "</tr></thead>\n";
    html += "<tbody id=\"task-table-body\"><tr><td colspan=\"8\" style=\"text-align:center;color:#64748b\">Loading…</td></tr></tbody>\n";
    html += "</table>\n";

    html += "</div>\n"; // end .container

    // Toast
    html += "<div class=\"toast\" id=\"toast\"></div>\n";

    // Create / Edit dialog
    html += "<dialog id=\"task-dialog\">\n";
    html += "  <h2 id=\"dialog-title\">New Task</h2>\n";
    html += "  <input type=\"hidden\" id=\"edit-task-id\">\n";
    html += "  <label>Name</label>\n";
    html += "  <input id=\"f-name\" type=\"text\" placeholder=\"My Task\" required>\n";
    html += "  <label>Description</label>\n";
    html += "  <input id=\"f-desc\" type=\"text\" placeholder=\"Optional description\">\n";
    html += "  <label>Type</label>\n";
    html += "  <select id=\"f-type\" onchange=\"onTypeChange()\">\n";
    html += "    <option value=\"aql_query\">AQL Query</option>\n";
    html += "    <option value=\"function\">Function</option>\n";
    html += "  </select>\n";
    html += "  <div id=\"aql-section\">\n";
    html += "    <label>AQL Query</label>\n";
    html += "    <textarea id=\"f-aql\" placeholder=\"FOR d IN collection RETURN d\"></textarea>\n";
    html += "  </div>\n";
    html += "  <div id=\"fn-section\" style=\"display:none\">\n";
    html += "    <label>Function Name</label>\n";
    html += "    <input id=\"f-fn-name\" type=\"text\" placeholder=\"my_function\">\n";
    html += "  </div>\n";
    html += "  <label>Trigger</label>\n";
    html += "  <select id=\"f-trigger\" onchange=\"onTriggerChange()\">\n";
    html += "    <option value=\"interval\">Interval</option>\n";
    html += "    <option value=\"cron\">Cron</option>\n";
    html += "    <option value=\"manual\">Manual</option>\n";
    html += "  </select>\n";
    html += "  <div id=\"interval-section\">\n";
    html += "    <label>Interval (seconds)</label>\n";
    html += "    <input id=\"f-interval\" type=\"number\" min=\"1\" value=\"300\">\n";
    html += "  </div>\n";
    html += "  <div id=\"cron-section\" style=\"display:none\">\n";
    html += "    <label>Cron Expression</label>\n";
    html += "    <input id=\"f-cron\" type=\"text\" placeholder=\"*/5 * * * *\">\n";
    html += "  </div>\n";
    html += "  <label>Timeout (seconds)</label>\n";
    html += "  <input id=\"f-timeout\" type=\"number\" min=\"1\" value=\"600\">\n";
    html += "  <label>Max Retries</label>\n";
    html += "  <input id=\"f-retries\" type=\"number\" min=\"0\" value=\"3\">\n";
    html += "  <div class=\"dialog-actions\">\n";
    html += "    <button class=\"btn-secondary\" onclick=\"closeDialog()\">Cancel</button>\n";
    html += "    <button class=\"btn-primary\" onclick=\"saveTask()\">Save</button>\n";
    html += "  </div>\n";
    html += "</dialog>\n";

    // JavaScript
    html += "<script>\n";
    html += "const API = '/api/tasks';\n";
    html += "\n";
    html += "async function api(method, path, body) {\n";
    html += "  const opts = { method, headers: {'Content-Type':'application/json'} };\n";
    html += "  if (body !== undefined) opts.body = JSON.stringify(body);\n";
    html += "  const r = await fetch(path, opts);\n";
    html += "  return r.json();\n";
    html += "}\n";
    html += "\n";
    html += "function toast(msg, ok=true) {\n";
    html += "  const el = document.getElementById('toast');\n";
    html += "  el.textContent = msg;\n";
    html += "  el.className = 'toast show ' + (ok ? 'ok' : 'err');\n";
    html += "  setTimeout(() => el.className = 'toast', 3000);\n";
    html += "}\n";
    html += "\n";
    html += "async function loadStats() {\n";
    html += "  const s = await api('GET', API + '/stats');\n";
    html += "  document.getElementById('s-registered').textContent = s.registered_tasks ?? '?';\n";
    html += "  document.getElementById('s-active').textContent = s.active_tasks ?? '?';\n";
    html += "  document.getElementById('s-running').textContent = s.running_tasks ?? '?';\n";
    html += "  document.getElementById('s-total').textContent = s.total_executions ?? '?';\n";
    html += "  document.getElementById('s-failed').textContent = s.failed_executions ?? '?';\n";
    html += "  const running = s.scheduler_running;\n";
    html += "  document.getElementById('s-status').textContent = running ? 'Running' : 'Stopped';\n";
    html += "  document.getElementById('s-status').style.color = running ? '#22c55e' : '#ef4444';\n";
    html += "}\n";
    html += "\n";
    html += "function formatDate(iso) {\n";
    html += "  if (!iso) return '–';\n";
    html += "  return new Date(iso).toLocaleString();\n";
    html += "}\n";
    html += "\n";
    html += "async function loadTasks() {\n";
    html += "  const data = await api('GET', API);\n";
    html += "  const tbody = document.getElementById('task-table-body');\n";
    html += "  if (!data.items || data.items.length === 0) {\n";
    html += "    tbody.innerHTML = '<tr><td colspan=\"8\" style=\"text-align:center;color:#64748b\">No tasks registered.</td></tr>';\n";
    html += "    return;\n";
    html += "  }\n";
    html += "  tbody.innerHTML = data.items.map(t => {\n";
    html += "    const statusClass = t.running ? 'status-running' : (t.enabled ? 'status-enabled' : 'status-disabled');\n";
    html += "    const statusText = t.running ? 'Running' : (t.enabled ? 'Enabled' : 'Disabled');\n";
    html += "    const typeTag = t.type === 'aql_query'\n";
    html += "      ? '<span class=\"tag tag-aql\">AQL</span>'\n";
    html += "      : '<span class=\"tag tag-fn\">Fn</span>';\n";
    html += "    const toggleBtn = t.enabled\n";
    html += "      ? `<button class=\"btn-warning\" onclick=\"disableTask('${t.id}')\">Pause</button>`\n";
    html += "      : `<button class=\"btn-success\" onclick=\"enableTask('${t.id}')\">Resume</button>`;\n";
    html += "    return `<tr>\n";
    html += "      <td><b>${escHtml(t.id)}</b><br><small style=\"color:#94a3b8\">${escHtml(t.name)}</small></td>\n";
    html += "      <td>${typeTag}</td>\n";
    html += "      <td>${escHtml(t.trigger_type || '–')}</td>\n";
    html += "      <td class=\"${statusClass}\">${statusText}</td>\n";
    html += "      <td>${t.successful_executions ?? 0} / ${t.total_executions ?? 0}</td>\n";
    html += "      <td style=\"color:#f87171;font-size:.8rem\">${escHtml(t.last_error || '')}</td>\n";
    html += "      <td style=\"font-size:.8rem\">${formatDate(t.next_run)}</td>\n";
    html += "      <td style=\"white-space:nowrap\">\n";
    html += "        <button class=\"btn-primary\" onclick=\"runTask('${t.id}')\">&#9654; Run</button>\n";
    html += "        ${toggleBtn}\n";
    html += "        <button class=\"btn-secondary\" onclick=\"editTask('${t.id}')\">&#9998; Edit</button>\n";
    html += "        <button class=\"btn-danger\" onclick=\"deleteTask('${t.id}')\">&#x1F5D1;</button>\n";
    html += "      </td>\n";
    html += "    </tr>`;\n";
    html += "  }).join('');\n";
    html += "}\n";
    html += "\n";
    html += "function escHtml(s) {\n";
    html += "  return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/\"/g,'&quot;');\n";
    html += "}\n";
    html += "\n";
    html += "async function loadAll() {\n";
    html += "  document.getElementById('refresh-indicator').textContent = 'Refreshing…';\n";
    html += "  await Promise.all([loadStats(), loadTasks()]);\n";
    html += "  document.getElementById('refresh-indicator').textContent = 'Updated ' + new Date().toLocaleTimeString();\n";
    html += "}\n";
    html += "\n";
    html += "async function runTask(id) {\n";
    html += "  const r = await api('POST', API + '/' + id + '/execute');\n";
    html += "  if (r.status === 'executed') { toast('Task executed'); loadAll(); }\n";
    html += "  else toast(r.error || 'Execute failed', false);\n";
    html += "}\n";
    html += "\n";
    html += "async function enableTask(id) {\n";
    html += "  const r = await api('POST', API + '/' + id + '/enable');\n";
    html += "  if (r.status === 'enabled') { toast('Task enabled'); loadAll(); }\n";
    html += "  else toast(r.error || 'Enable failed', false);\n";
    html += "}\n";
    html += "\n";
    html += "async function disableTask(id) {\n";
    html += "  const r = await api('POST', API + '/' + id + '/disable');\n";
    html += "  if (r.status === 'disabled') { toast('Task paused'); loadAll(); }\n";
    html += "  else toast(r.error || 'Disable failed', false);\n";
    html += "}\n";
    html += "\n";
    html += "async function deleteTask(id) {\n";
    html += "  if (!confirm('Delete task ' + id + '?')) return;\n";
    html += "  const r = await api('DELETE', API + '/' + id);\n";
    html += "  if (r.status === 'deleted') { toast('Task deleted'); loadAll(); }\n";
    html += "  else toast(r.error || 'Delete failed', false);\n";
    html += "}\n";
    html += "\n";
    html += "function openCreateDialog() {\n";
    html += "  document.getElementById('dialog-title').textContent = 'New Task';\n";
    html += "  document.getElementById('edit-task-id').value = '';\n";
    html += "  document.getElementById('f-name').value = '';\n";
    html += "  document.getElementById('f-desc').value = '';\n";
    html += "  document.getElementById('f-type').value = 'aql_query';\n";
    html += "  document.getElementById('f-aql').value = '';\n";
    html += "  document.getElementById('f-fn-name').value = '';\n";
    html += "  document.getElementById('f-trigger').value = 'interval';\n";
    html += "  document.getElementById('f-interval').value = '300';\n";
    html += "  document.getElementById('f-cron').value = '';\n";
    html += "  document.getElementById('f-timeout').value = '600';\n";
    html += "  document.getElementById('f-retries').value = '3';\n";
    html += "  onTypeChange(); onTriggerChange();\n";
    html += "  document.getElementById('task-dialog').showModal();\n";
    html += "}\n";
    html += "\n";
    html += "async function editTask(id) {\n";
    html += "  const t = await api('GET', API + '/' + id);\n";
    html += "  if (t.status === 'error') { toast(t.error, false); return; }\n";
    html += "  document.getElementById('dialog-title').textContent = 'Edit Task';\n";
    html += "  document.getElementById('edit-task-id').value = t.id;\n";
    html += "  document.getElementById('f-name').value = t.name || '';\n";
    html += "  document.getElementById('f-desc').value = t.description || '';\n";
    html += "  document.getElementById('f-type').value = t.type || 'aql_query';\n";
    html += "  document.getElementById('f-aql').value = t.aql_query || '';\n";
    html += "  document.getElementById('f-fn-name').value = t.function_name || '';\n";
    html += "  document.getElementById('f-trigger').value = t.trigger_type || 'interval';\n";
    html += "  document.getElementById('f-interval').value = Math.round((t.interval_ms || 300000) / 1000);\n";
    html += "  document.getElementById('f-cron').value = t.cron_expression || '';\n";
    html += "  document.getElementById('f-timeout').value = Math.round((t.timeout_ms || 600000) / 1000);\n";
    html += "  document.getElementById('f-retries').value = t.max_retries ?? 3;\n";
    html += "  onTypeChange(); onTriggerChange();\n";
    html += "  document.getElementById('task-dialog').showModal();\n";
    html += "}\n";
    html += "\n";
    html += "function closeDialog() {\n";
    html += "  document.getElementById('task-dialog').close();\n";
    html += "}\n";
    html += "\n";
    html += "function onTypeChange() {\n";
    html += "  const v = document.getElementById('f-type').value;\n";
    html += "  document.getElementById('aql-section').style.display = v === 'aql_query' ? '' : 'none';\n";
    html += "  document.getElementById('fn-section').style.display = v === 'function' ? '' : 'none';\n";
    html += "}\n";
    html += "\n";
    html += "function onTriggerChange() {\n";
    html += "  const v = document.getElementById('f-trigger').value;\n";
    html += "  document.getElementById('interval-section').style.display = v === 'interval' ? '' : 'none';\n";
    html += "  document.getElementById('cron-section').style.display = v === 'cron' ? '' : 'none';\n";
    html += "}\n";
    html += "\n";
    html += "async function saveTask() {\n";
    html += "  const id = document.getElementById('edit-task-id').value;\n";
    html += "  const type = document.getElementById('f-type').value;\n";
    html += "  const trigger = document.getElementById('f-trigger').value;\n";
    html += "  const body = {\n";
    html += "    name: document.getElementById('f-name').value,\n";
    html += "    description: document.getElementById('f-desc').value,\n";
    html += "    type: type,\n";
    html += "    aql_query: document.getElementById('f-aql').value,\n";
    html += "    function_name: document.getElementById('f-fn-name').value,\n";
    html += "    trigger_type: trigger,\n";
    html += "    interval_ms: parseInt(document.getElementById('f-interval').value || '300', 10) * 1000,\n";
    html += "    cron_expression: document.getElementById('f-cron').value,\n";
    html += "    timeout_ms: parseInt(document.getElementById('f-timeout').value || '600', 10) * 1000,\n";
    html += "    max_retries: parseInt(document.getElementById('f-retries').value || '3', 10),\n";
    html += "    enabled: true\n";
    html += "  };\n";
    html += "  const r = id\n";
    html += "    ? await api('PUT', API + '/' + id, body)\n";
    html += "    : await api('POST', API, body);\n";
    html += "  if (r.status === 'created' || r.status === 'updated') {\n";
    html += "    toast(id ? 'Task updated' : 'Task created');\n";
    html += "    closeDialog();\n";
    html += "    loadAll();\n";
    html += "  } else {\n";
    html += "    toast(r.error || 'Save failed', false);\n";
    html += "  }\n";
    html += "}\n";
    html += "\n";
    html += "// Auto-refresh every 30 seconds\n";
    html += "loadAll();\n";
    html += "setInterval(loadAll, 30000);\n";
    html += "</script>\n";
    html += "</body>\n</html>\n";

    return html;
}

// ============================================================================
// Private helpers
// ============================================================================

json TaskSchedulerApiHandler::taskToJson([[maybe_unused]] const ScheduledTask& task) {
    auto span = Tracer::startSpan("taskToJson");
    json j{
        {"id",                    task.id},
        {"name",                  task.name},
        {"description",           task.description},
        {"type",                  taskTypeStr(task.type)},
        {"aql_query",             task.aql_query},
        {"function_name",         task.function_name},
        {"parameters",            task.parameters},
        {"trigger_type",          triggerTypeStr(task.trigger_type)},
        {"cron_expression",       task.cron_expression},
        {"interval_ms",           task.interval.count()},
        {"timeout_ms",            task.timeout.count()},
        {"max_retries",           task.max_retries},
        {"enabled",               task.enabled},
        {"running",               task.running},
        {"total_executions",      task.total_executions},
        {"successful_executions", task.successful_executions},
        {"failed_executions",     task.failed_executions},
        {"avg_execution_time_ms", task.avg_execution_time_ms},
        {"last_error",            task.last_error},
        {"last_error_category",   errorCategoryStr(task.last_error_category)},
        {"last_success_time",     timePointToIso(task.last_success_time)},
        {"last_failure_time",     timePointToIso(task.last_failure_time)},
        {"next_run",              timePointToIso(task.next_run)},
        {"dependencies",          task.dependencies},
    };
    return j;
}

ScheduledTask TaskSchedulerApiHandler::parseTaskFromJson([[maybe_unused]] const json& j) {
    ScheduledTask task;

    // Required fields
    task.name = j.at("name").get<std::string>();
    if (const auto err = validateTaskIdentifier(task.name, "name")) {
        throw std::invalid_argument(*err);
    }

    // Optional fields with defaults
    task.description   = j.value("description", "");
    task.aql_query     = j.value("aql_query", "");
    task.function_name = j.value("function_name", "");
    task.enabled       = j.value("enabled", true);
    task.max_retries   = j.value("max_retries", static_cast<size_t>(3));

    if (j.contains("parameters")) {
        task.parameters = j["parameters"];
    }

    // Task type
    std::string type_str = j.value("type", "aql_query");
    if (type_str == "function") {
        task.type = ScheduledTask::TaskType::FUNCTION;
        if (const auto err = validateTaskIdentifier(task.function_name, "function_name")) {
            throw std::invalid_argument(*err);
        }
    } else {
        task.type = ScheduledTask::TaskType::AQL_QUERY;
        if (task.aql_query.empty()) {
            throw std::invalid_argument("aql_query must not be empty for aql_query tasks");
        }
        themis::utils::InputValidator validator;
        if (!validator.validateStringLength(task.aql_query, 100'000)) {
            throw std::invalid_argument("aql_query exceeds maximum allowed length");
        }
        if (!validator.validateAQLQuery(task.aql_query)) {
            throw std::invalid_argument("aql_query contains potentially unsafe patterns");
        }
    }

    // Trigger type
    std::string trigger_str = j.value("trigger_type", "interval");
    if (trigger_str == "cron") {
        task.trigger_type = ScheduledTask::TriggerType::CRON;
    } else if ([[maybe_unused]] trigger_str == "cdc_event") {
        task.trigger_type = ScheduledTask::TriggerType::CDC_EVENT;
    } else if (trigger_str == "webhook") {
        task.trigger_type = ScheduledTask::TriggerType::WEBHOOK;
    } else if (trigger_str == "manual") {
        task.trigger_type = ScheduledTask::TriggerType::MANUAL;
    } else {
        task.trigger_type = ScheduledTask::TriggerType::INTERVAL;
    }

    // Cron expression
    if (j.contains("cron_expression")) {
        task.cron_expression = j["cron_expression"].get<std::string>();
    }

    // Interval
    if (j.contains("interval_ms")) {
        auto ms = j["interval_ms"].get<int64_t>();
        if (ms <= 0) {
            throw std::invalid_argument("interval_ms must be a positive integer");
        }
        task.interval = std::chrono::milliseconds(ms);
    } else if (j.contains("interval_seconds")) {
        auto secs = j["interval_seconds"].get<int64_t>();
        task.interval = std::chrono::milliseconds(checkedSecondsToMilliseconds(secs, "interval_seconds"));
    }

    // Timeout
    if (j.contains("timeout_ms")) {
        auto ms = j["timeout_ms"].get<int64_t>();
        if (ms <= 0) {
            throw std::invalid_argument("timeout_ms must be a positive integer");
        }
        task.timeout = std::chrono::milliseconds(ms);
    } else if (j.contains("timeout_seconds")) {
        auto secs = j["timeout_seconds"].get<int64_t>();
        task.timeout = std::chrono::milliseconds(checkedSecondsToMilliseconds(secs, "timeout_seconds"));
    }

    // ID (optional override)
    if (j.contains("id")) {
        task.id = j["id"].get<std::string>();
        if (const auto err = validateTaskIdentifier(task.id, "id")) {
            throw std::invalid_argument(*err);
        }
    }

    // Dependencies (for DAG execution)
    if (j.contains("dependencies")) {
        task.dependencies = j["dependencies"].get<std::vector<std::string>>();
        for (const auto& dep : task.dependencies) {
            if (const auto err = validateTaskIdentifier(dep, "dependencies")) {
                throw std::invalid_argument(*err);
            }
        }
    }

    return task;
}

json TaskSchedulerApiHandler::executeDAG([[maybe_unused]] const json& request) {
    if (!scheduler_) {
    auto span = Tracer::startSpan("executeDAG");
        return json{{"status", "error"}, {"error", "Scheduler not initialized"}};
    }
    auto& scheduler = *scheduler_;
    try {
        if (!request.contains("task_ids") || !request["task_ids"].is_array()) {
            return json{{"status", "error"}, {"error", "Missing or invalid 'task_ids' array"}};
        }

        auto task_ids = request["task_ids"].get<std::vector<std::string>>();
        for (const auto& task_id : task_ids) {
            if (const auto err = validateTaskIdentifier(task_id, "task_ids")) {
                return json{{"status", "error"}, {"error", *err}};
            }
        }
        auto dag_result = scheduler.executeDAG(task_ids);

        json succeeded = json::object();
        for (const auto& [id, res] : dag_result.succeeded) {
            succeeded[id] = res;
        }

        json failed = json::object();
        for (const auto& [id, err] : dag_result.failed) {
            failed[id] = err;
        }

        spdlog::info("TaskSchedulerApiHandler: executeDAG completed: {} succeeded, {} failed, {} skipped, {} condition_skipped",
                     dag_result.succeeded.size(), dag_result.failed.size(),
                     dag_result.skipped.size(), dag_result.condition_skipped.size());

        return json{
            {"status",            "executed"},
            {"succeeded",         succeeded},
            {"failed",            failed},
            {"skipped",           dag_result.skipped},
            {"condition_skipped", dag_result.condition_skipped},
        };
    } catch (const std::invalid_argument& e) {
        spdlog::warn([[maybe_unused]] "TaskSchedulerApiHandler: executeDAG failed (invalid argument): {}", e.what());
        return json{{"status", "error"}, {"error", "Internal server error"}};
    } catch (const std::runtime_error& e) {
        spdlog::warn([[maybe_unused]] "TaskSchedulerApiHandler: executeDAG failed (runtime error): {}", e.what());
        return json{{"status", "error"}, {"error", "Internal server error"}};
    } catch (const std::exception& e) {
        spdlog::warn("TaskSchedulerApiHandler: executeDAG failed: {}", e.what());
        return json{{"status", "error"}, {"error", "Internal server error"}};
    }
}

// ============================================================================
// External scheduler integration
// ============================================================================

namespace {

/// Build a KubernetesCronJobConfig from the JSON request object.
scheduler::KubernetesCronJobConfig k8sConfigFromJson(const json& req) {
    scheduler::KubernetesCronJobConfig cfg;
    if (req.contains("themisdb_base_url"))
        cfg.themisdb_base_url = req["themisdb_base_url"].get<std::string>();
    if (req.contains("k8s_namespace"))
        cfg.k8s_namespace = req["k8s_namespace"].get<std::string>();
    if (req.contains("job_image"))
        cfg.job_image = req["job_image"].get<std::string>();
    if (req.contains("api_token_secret_name"))
        cfg.api_token_secret_name = req["api_token_secret_name"].get<std::string>();
    if (req.contains("suspend"))
        cfg.suspend = req["suspend"].get<bool>();
    if (req.contains("successful_jobs_history_limit"))
        cfg.successful_jobs_history_limit = req["successful_jobs_history_limit"].get<int32_t>();
    if (req.contains("failed_jobs_history_limit"))
        cfg.failed_jobs_history_limit = req["failed_jobs_history_limit"].get<int32_t>();
    if (req.contains("extra_labels") && req["extra_labels"].is_object()) {
        cfg.extra_labels.reserve(req["extra_labels"].size());
        for (auto it = req["extra_labels"].begin(); it != req["extra_labels"].end(); ++it) {
            cfg.extra_labels.emplace_back(it.key(), it.value().get<std::string>());
        }
    }
    return cfg;
}

/// Build an AirflowDagConfig from the JSON request object.
scheduler::AirflowDagConfig airflowConfigFromJson(const json& req) {
    scheduler::AirflowDagConfig cfg;
    if (req.contains("dag_id"))
        cfg.dag_id = req["dag_id"].get<std::string>();
    if (req.contains("owner"))
        cfg.owner = req["owner"].get<std::string>();
    if (req.contains("start_date"))
        cfg.start_date = req["start_date"].get<std::string>();
    if (req.contains("themisdb_base_url"))
        cfg.themisdb_base_url = req["themisdb_base_url"].get<std::string>();
    if (req.contains("http_conn_id"))
        cfg.http_conn_id = req["http_conn_id"].get<std::string>();
    if (req.contains("default_schedule"))
        cfg.default_schedule = req["default_schedule"].get<std::string>();
    if (req.contains("is_paused_upon_creation"))
        cfg.is_paused_upon_creation = req["is_paused_upon_creation"].get<bool>();
    if (req.contains("description"))
        cfg.description = req["description"].get<std::string>();
    if (req.contains("tags") && req["tags"].is_array()) {
        cfg.tags.reserve(req["tags"].size());
        for (const auto& tag : req["tags"]) {
            cfg.tags.push_back(tag.get<std::string>());
        }
    }
    return cfg;
}

} // anonymous namespace

// A single stateless adapter instance shared across all methods (thread-safe per its contract).
static const scheduler::ExternalSchedulerAdapter s_adapter;

json TaskSchedulerApiHandler::exportToKubernetesCronJobJson(const std::string& task_id,
                                                              const json& request) {
    auto span = Tracer::startSpan("exportToKubernetesCronJobJson");
    if (!scheduler_) {
        return json{{"status", "error"}, {"error", "Scheduler not initialized"}};
    }
    auto& scheduler = *scheduler_;
    try {
        auto task_ptr = scheduler.getTask(task_id);
        if (!task_ptr) {
            return json{{"status", "error"}, {"error", "Task not found: " + task_id}};
        }
        const auto cfg      = k8sConfigFromJson(request);
        const json manifest = s_adapter.toKubernetesCronJobJson(*task_ptr, cfg);
        return json{{"manifest", manifest}};
    } catch (const std::exception& e) {
        spdlog::warn("exportToKubernetesCronJobJson failed: {}", e.what());
        return json{{"status", "error"}, {"error", "Internal server error"}};
    }
}

json TaskSchedulerApiHandler::exportToKubernetesCronJobYaml(const std::string& task_id,
                                                              const json& request) {
    auto span = Tracer::startSpan("exportToKubernetesCronJobYaml");
    if (!scheduler_) {
        return json{{"status", "error"}, {"error", "Scheduler not initialized"}};
    }
    auto& scheduler = *scheduler_;
    try {
        auto task_ptr = scheduler.getTask(task_id);
        if (!task_ptr) {
            return json{{"status", "error"}, {"error", "Task not found: " + task_id}};
        }
        const auto cfg         = k8sConfigFromJson(request);
        const std::string yaml = s_adapter.toKubernetesCronJobYaml(*task_ptr, cfg);
        return json{{"yaml", yaml}};
    } catch (const std::exception& e) {
        spdlog::warn("exportToKubernetesCronJobYaml failed: {}", e.what());
        return json{{"status", "error"}, {"error", "Internal server error"}};
    }
}

json TaskSchedulerApiHandler::exportToAirflowDag([[maybe_unused]] const json& request) {
    if (!scheduler_) {
    auto span = Tracer::startSpan("exportToAirflowDag");
        return json{{"status", "error"}, {"error", "Scheduler not initialized"}};
    }
    auto& scheduler = *scheduler_;
    try {
        if (!request.contains("task_ids") || !request["task_ids"].is_array()) {
            return json{{"status", "error"},
                        {"error", "Request must contain a 'task_ids' array"}};
        }
        std::vector<ScheduledTask> tasks = {};

        tasks.reserve(request["task_ids"].size());
        for (const auto& id_json : request["task_ids"]) {
            const std::string id = id_json.get<std::string>();
            auto task_ptr = scheduler.getTask(id);
            if (!task_ptr) {
                return json{{"status", "error"}, {"error", "Task not found: " + id}};
            }
            tasks.push_back(*task_ptr);
        }
        const auto        cfg = airflowConfigFromJson(request);
        const std::string py  = s_adapter.toAirflowDagPython(tasks, cfg);
        return json{{"dag_python", py}};
    } catch (const std::exception& e) {
        spdlog::warn("exportToAirflowDag failed: {}", e.what());
        return json{{"status", "error"}, {"error", "Internal server error"}};
    }
}

json TaskSchedulerApiHandler::importFromKubernetesCronJob([[maybe_unused]] const json& request) {
    if (!scheduler_) {
    auto span = Tracer::startSpan("importFromKubernetesCronJob");
        return json{{"status", "error"}, {"error", "Scheduler not initialized"}};
    }
    auto& scheduler = *scheduler_;
    try {
        ScheduledTask task    = s_adapter.fromKubernetesCronJobJson(request);
        const std::string id  = scheduler.registerTask(task);
        spdlog::info("importFromKubernetesCronJob: registered task '{}'", id);
        return json{{"status", "created"}, {"id", id}};
    } catch (const std::exception& e) {
        spdlog::warn("importFromKubernetesCronJob failed: {}", e.what());
        return json{{"status", "error"}, {"error", "Internal server error"}};
    }
}

} // namespace server
} // namespace themis


