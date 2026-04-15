/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            task_scheduler_api_handler.h                       ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:13:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     253                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e963d4e9ba  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 71d99c4f28  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file task_scheduler_api_handler.h
 * @brief HTTP API handler for task scheduler operations
 * 
 * ⚠️ SECURITY CRITICAL: This API exposes task scheduling functionality.
 * ALL endpoints MUST be protected by:
 * - Strong authentication (API keys, JWT, mutual TLS)
 * - Authorization (RBAC - only admins can manage tasks)
 * - Rate limiting (prevent API abuse)
 * - Input validation (sanitize all inputs)
 * - Audit logging (log all operations)
 * - HTTPS only (no plain HTTP)
 */

#pragma once

#include "scheduler/task_scheduler.h"
#include <nlohmann/json.hpp>
#include <string>
#include <memory>

namespace themis {
namespace server {

/**
 * @brief API handler for task scheduler HTTP endpoints
 * 
 * ⚠️ SECURITY WARNING: These endpoints allow task management and execution.
 * 
 * Provides RESTful API for:
 * - POST /api/tasks - Register a new task [AUTH REQUIRED]
 * - GET /api/tasks - List all tasks [AUTH REQUIRED]
 * - GET /api/tasks/:id - Get task details [AUTH REQUIRED]
 * - PUT /api/tasks/:id - Update a task [AUTH REQUIRED]
 * - DELETE /api/tasks/:id - Unregister a task [AUTH REQUIRED]
 * - POST /api/tasks/:id/enable - Enable a task [AUTH REQUIRED]
 * - POST /api/tasks/:id/disable - Disable a task [AUTH REQUIRED]
 * - POST /api/tasks/:id/execute - Execute task immediately [AUTH REQUIRED]
 * - GET /api/tasks/:id/history - Get searchable execution history [AUTH REQUIRED]
 * - POST /api/tasks/dag/execute - Execute a DAG of tasks [AUTH REQUIRED]
 * - GET /api/tasks/stats - Get scheduler statistics [AUTH REQUIRED]
 * 
 * SECURITY REQUIREMENTS:
 * - All endpoints must verify user authentication
 * - All endpoints must check user authorization (admin role)
 * - All operations must be audit logged
 * - Input validation on all parameters
 * - Rate limiting on execute endpoint
 */
class TaskSchedulerApiHandler {
public:
    explicit TaskSchedulerApiHandler(TaskScheduler* scheduler)
        : scheduler_(scheduler) {}
    
    // Task registration and management
    nlohmann::json registerTask(const nlohmann::json& request);
    nlohmann::json listTasks();
    nlohmann::json getTask(const std::string& task_id);
    nlohmann::json updateTask(const std::string& task_id, const nlohmann::json& request);
    nlohmann::json unregisterTask(const std::string& task_id);
    
    // Task control
    nlohmann::json enableTask(const std::string& task_id);
    nlohmann::json disableTask(const std::string& task_id);
    nlohmann::json executeTask(const std::string& task_id);

    /**
     * @brief Execute a set of tasks as a DAG respecting their dependency order.
     *
     * Accepts a JSON object with a "task_ids" array and returns the per-task
     * outcome (succeeded, failed, skipped, condition_skipped).
     *
     * Request body:
     * @code
     * { "task_ids": ["task_a", "task_b", "task_c"] }
     * @endcode
     *
     * Response:
     * @code
     * {
     *   "succeeded": { "task_a": {…}, "task_b": {…} },
     *   "failed":    { "task_c": "error message" },
     *   "skipped":   [],
     *   "condition_skipped": []
     * }
     * @endcode
     *
     * ⚠️ SECURITY: This method MUST be protected by authentication and authorization.
     */
    nlohmann::json executeDAG(const nlohmann::json& request);
    
    // Statistics
    nlohmann::json getStats();

    /**
     * @brief Get the N most-recent execution results for a task.
     *
     * Returns up to `limit` stored results (newest first) for the given task.
     * Returns an error object if result storage is disabled or the scheduler
     * is not initialized.
     *
     * @param task_id  Task identifier.
     * @param limit    Maximum number of results to return (default: 10).
     */
    nlohmann::json getTaskResults(const std::string& task_id, size_t limit = 10);

    /**
     * @brief Get the most-recent execution result for a task.
     *
     * Returns the latest stored result or a not-found object if none exists.
     *
     * @param task_id  Task identifier.
     */
    nlohmann::json getLatestTaskResult(const std::string& task_id);
    // Audit history
    /**
     * @brief Get searchable execution history for a task (or all tasks)
     *
     * Exposes the underlying TaskAuditManager query interface over HTTP.
     * Supports filtering by success, event_type, trigger_type, user_id and
     * time range via @p query_params (JSON object whose keys mirror the
     * AuditQueryParams fields).
     *
     * @param task_id      Task ID to filter on (empty string = all tasks)
     * @param query_params Optional JSON object with filter/pagination keys:
     *                       limit (int, default 100), offset (int, default 0),
     *                       success (bool), event_type (string),
     *                       trigger_type (string), user_id (string),
     *                       start_time_ms (int64), end_time_ms (int64)
     * @return JSON object { "items": [...], "total": <int> } where "total" is the
     *         total count of all matching records (regardless of limit/offset),
     *         bounded by the audit manager's max_query_results setting.
     *         Use limit/offset parameters to paginate through results.
     */
    nlohmann::json getExecutionHistory(
        const std::string& task_id,
        const nlohmann::json& query_params = nlohmann::json::object());
    // External scheduler integration
    /**
     * @brief Export a task as a Kubernetes CronJob manifest (JSON).
     *
     * Request body fields:
     *   - themisdb_base_url  (string, required) – ThemisDB HTTP API base URL
     *   - k8s_namespace      (string, optional) – Kubernetes namespace (default "default")
     *   - job_image          (string, optional) – Container image (default "curlimages/curl:8.6.0")
     *   - api_token_secret_name (string, optional) – K8s Secret name holding the bearer token
     *   - suspend            (bool,   optional) – Suspend the CronJob on creation (default false)
     *   - extra_labels       (object, optional) – Additional key/value labels for the resource
     *
     * @param task_id  ID of the ThemisDB task to export.
     * @param request  JSON configuration (see above).
     * @return JSON with { "manifest": <CronJob JSON> } on success,
     *         or { "status": "error", "error": "..." } on failure.
     */
    nlohmann::json exportToKubernetesCronJobJson(const std::string& task_id,
                                                  const nlohmann::json& request);

    /**
     * @brief Export a task as a Kubernetes CronJob manifest (YAML).
     *
     * Same request fields as exportToKubernetesCronJobJson().
     *
     * @param task_id  ID of the ThemisDB task to export.
     * @param request  JSON configuration.
     * @return JSON with { "yaml": "<YAML string>" } on success,
     *         or { "status": "error", "error": "..." } on failure.
     */
    nlohmann::json exportToKubernetesCronJobYaml(const std::string& task_id,
                                                  const nlohmann::json& request);

    /**
     * @brief Export one or more tasks as an Apache Airflow DAG Python file.
     *
     * Request body fields:
     *   - task_ids           (array<string>, required) – Task IDs to export
     *   - dag_id             (string, optional) – Airflow DAG id
     *   - owner              (string, optional) – DAG owner
     *   - start_date         (string, optional) – ISO-8601 start date (e.g. "2026-01-01")
     *   - themisdb_base_url  (string, optional) – ThemisDB HTTP API base URL
     *   - http_conn_id       (string, optional) – Airflow HTTP connection id
     *   - default_schedule   (string, optional) – Fallback cron/special expression
     *   - is_paused_upon_creation (bool, optional) – Start the DAG paused
     *   - description        (string, optional) – DAG description
     *   - tags               (array<string>, optional) – Airflow tags
     *
     * @param request  JSON configuration (see above).
     * @return JSON with { "dag_python": "<Python source>" } on success,
     *         or { "status": "error", "error": "..." } on failure.
     */
    nlohmann::json exportToAirflowDag(const nlohmann::json& request);

    /**
     * @brief Import a task from a Kubernetes CronJob manifest (JSON).
     *
     * Parses the manifest and registers the resulting ThemisDB ScheduledTask
     * in the scheduler.  The manifest must contain at least metadata.name and
     * spec.schedule.  ThemisDB-specific annotations (themisdb/task-name,
     * themisdb/task-description, themisdb/task-id) are honoured when present.
     *
     * @param request  JSON object representing a Kubernetes CronJob resource.
     * @return JSON with { "status": "created", "id": "<task_id>" } on success,
     *         or { "status": "error", "error": "..." } on failure.
     */
    nlohmann::json importFromKubernetesCronJob(const nlohmann::json& request);

    // Web UI
    /**
     * @brief Serve the task management web UI
     * @return HTML string for the task scheduler web UI
     *
     * Serves a self-contained single-page application at GET /ui/tasks that
     * allows operators to create, monitor, pause and delete scheduled tasks
     * without using the raw REST API directly.
     */
    std::string getWebUi();

private:
    TaskScheduler* scheduler_;
    
    // Helper to convert ScheduledTask to JSON
    nlohmann::json taskToJson(const ScheduledTask& task);
    
    // Helper to parse task from JSON request
    ScheduledTask parseTaskFromJson(const nlohmann::json& json);
};

} // namespace server
} // namespace themis
