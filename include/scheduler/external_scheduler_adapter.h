/**
 * @file external_scheduler_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "scheduler/task_scheduler.h"
#include "scheduler/scheduler_api_contract.h"
#include "scheduler/task_result_store.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <map>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace scheduler {

/**
 * @brief Identifies the external scheduler platform.
 */
enum class ExternalSchedulerType {
    KUBERNETES_CRONJOB, ///< Kubernetes CronJob (batch/v1)
    AIRFLOW             ///< Apache Airflow DAG (HttpOperator)
};

/**
 * @brief Configuration shared across all Kubernetes CronJob manifests generated
 *        from ThemisDB tasks.
 */
struct KubernetesCronJobConfig {
    /// Kubernetes namespace (defaults to "default")
    std::string k8s_namespace = "default";

    /// Container image used inside the CronJob pod.
    /// The image must include curl or an equivalent HTTP client.
    std::string job_image = "curlimages/curl:8.6.0";

    /// Base URL of the ThemisDB HTTP API, e.g. "https://themisdb.example.com".
    std::string themisdb_base_url;

    /// Optional Bearer token for authenticating with the ThemisDB API.
    /// When set, the generated manifest references a Kubernetes Secret named
    /// "themisdb-api-token" and reads the token from the "token" key.
    /// Leave empty to generate an unauthenticated manifest.
    std::string api_token_secret_name;

    /// How many completed Jobs to keep for inspection (successfulJobsHistoryLimit).
    int32_t successful_jobs_history_limit = 3;

    /// How many failed Jobs to keep (failedJobsHistoryLimit).
    int32_t failed_jobs_history_limit = 1;

    /// Whether to suspend new CronJob executions (spec.suspend).
    bool suspend = false;

    /// Extra labels to add to the generated CronJob resource.
    std::vector<std::pair<std::string, std::string>> extra_labels;
};

/**
 * @brief Configuration shared across all Airflow DAGs generated from ThemisDB tasks.
 */
struct AirflowDagConfig {
    /// DAG identifier in Airflow (must be unique within the Airflow instance).
    std::string dag_id = "themisdb_scheduled_tasks";

    /// DAG owner shown in the Airflow UI.
    std::string owner = "themisdb";

    /// ISO-8601 start date used for the Airflow DAG, e.g. "2026-01-01".
    std::string start_date = "2026-01-01";

    /// Base URL of the ThemisDB HTTP API, e.g. "https://themisdb.example.com".
    std::string themisdb_base_url;

    /// Airflow HTTP connection ID that holds the ThemisDB base URL and credentials.
    /// Defaults to "themisdb_default".
    std::string http_conn_id = "themisdb_default";

    /// Default cron schedule for the generated DAG when tasks have mixed schedules.
    /// Individual task schedules override this when they carry a cron expression.
    std::string default_schedule = "@daily";

    /// Whether the DAG is paused on creation.
    bool is_paused_upon_creation = false;

    /// Free-form description embedded in the generated DAG file.
    std::string description = "ThemisDB scheduled tasks exported via ExternalSchedulerAdapter";

    /// Additional tags added to the Airflow DAG.
    std::vector<std::string> tags;
};

/**
 * @brief Converts ThemisDB ScheduledTask definitions to and from external scheduler formats.
 *
 * All methods are stateless and thread-safe.
 */
class ExternalSchedulerAdapter {
public:
    ExternalSchedulerAdapter() = default;

    // ── Kubernetes CronJob ────────────────────────────────────────────────────

    /**
     * @brief Generate a Kubernetes CronJob manifest in JSON format for a single task.
     *
     * The manifest represents a `batch/v1 CronJob` resource that, on each trigger,
     * runs a pod which calls
     *   POST {themisdb_base_url}/api/v1/scheduler/tasks/{task.id}/execute
     *
     * @param task   ThemisDB scheduled task to export.
     * @param config Kubernetes generation options (namespace, image, auth, …).
     * @return JSON object representing the Kubernetes CronJob manifest.
     *
     * @throws std::invalid_argument if task.id is empty or if the task has no
     *         cron expression and no finite interval to convert to a cron schedule.
     */
    nlohmann::json toKubernetesCronJobJson(const ScheduledTask& task,
                                           const KubernetesCronJobConfig& config) const;

    /**
     * @brief Generate a Kubernetes CronJob manifest in YAML format for a single task.
     *
     * Convenience wrapper around toKubernetesCronJobJson() that serialises the JSON
     * as YAML text.
     *
     * @param task   ThemisDB scheduled task to export.
     * @param config Kubernetes generation options.
     * @return YAML string ready to apply with `kubectl apply -f`.
     *
     * @throws std::invalid_argument (see toKubernetesCronJobJson).
     */
    std::string toKubernetesCronJobYaml(const ScheduledTask& task,
                                        const KubernetesCronJobConfig& config) const;

    /**
     * @brief Parse a Kubernetes CronJob manifest (JSON) and create a matching
     *        ThemisDB ScheduledTask.
     *
     * The resulting task uses TriggerType::CRON with the cron expression taken from
     * spec.schedule. The task id is derived from metadata.name; the name and
     * description are also mapped.
     *
     * @param manifest JSON object of the Kubernetes CronJob resource.
     * @return ThemisDB ScheduledTask mirroring the CronJob schedule.
     *
     * @throws std::invalid_argument if the manifest is missing required fields.
     */
    ScheduledTask fromKubernetesCronJobJson(const nlohmann::json& manifest) const;

    // ── Airflow DAG ───────────────────────────────────────────────────────────

    /**
     * @brief Generate an Airflow DAG Python file for one or more ThemisDB tasks.
     *
     * The generated file can be dropped directly into an Airflow `dags/` directory.
     * Each task becomes a `SimpleHttpOperator` (Airflow provider:
     * `apache-airflow-providers-http`) that POSTs to ThemisDB's task-execute endpoint.
     * Task `dependencies` are translated into Airflow operator dependencies using
     * the `>>` syntax.
     *
     * @param tasks  List of ThemisDB tasks to export. Must not be empty.
     * @param config Airflow DAG generation options.
     * @return Python source code of the Airflow DAG file.
     *
     * @throws std::invalid_argument if tasks is empty.
     */
    std::string toAirflowDagPython(const std::vector<ScheduledTask>& tasks,
                                   const AirflowDagConfig& config) const;

    // ── Shared utilities ──────────────────────────────────────────────────────

    /**
     * @brief Convert a fixed millisecond interval to a best-effort cron expression.
     *
     * Intervals shorter than 1 minute are rounded up to 1 minute.  Intervals
     * shorter than 1 hour are expressed as `*\/N * * * *`.  Intervals shorter
     * than 24 hours are expressed as `0 *\/N * * *`.  Daily or longer intervals
     * are expressed as `0 0 *\/D * *`.
     *
     * @param interval Duration to convert.
     * @return 5-field cron expression string.
     */
    static std::string intervalToCron(std::chrono::milliseconds interval);

    /**
     * @brief Return the effective cron schedule for a task.
     *
     * Returns task.cron_expression when TriggerType is CRON and the expression is
     * non-empty. Otherwise converts task.interval using intervalToCron().
     *
     * @param task ThemisDB scheduled task.
     * @return Cron expression string.
     */
    static std::string effectiveCronSchedule(const ScheduledTask& task);

    /**
     * @brief Sanitise a string for use as a Kubernetes resource name.
     *
     * Converts to lowercase, replaces non-alphanumeric characters with '-', and
     * trims leading/trailing hyphens.  Truncates to 52 characters to leave room
     * for a suffix.
     *
     * @param name Raw name string (e.g. task.id or task.name).
     * @return DNS-label-safe name.
     */
    static std::string toK8sName(const std::string& name);

    // ── External Scheduler Integration and Status Sync ───────────────────────

    /**
     * @brief Dispatch a ThemisDB task to an external scheduler backend.
     *
     * This method translates a ThemisDB ScheduledTask into the target external
     * scheduler's format and sends it to the external scheduler for registration.
     * The task remains registered locally even if dispatch fails, allowing for
     * retry logic and diagnostic introspection.
     *
     * When the external scheduler is unavailable or unresponsive:
     * - The method fails explicitly with kCoordinationError
     * - The task remains in the local registry for retry
     * - No partial state is left on the external backend
     *
     * This is a fail-closed operation: if dispatch fails, the task is NOT lost,
     * but it will not execute on the external scheduler until a successful retry.
     *
     * @param task              ThemisDB task to dispatch.
     * @param scheduler_type    Target scheduler platform (Kubernetes, Airflow, etc).
     * @param external_config   Backend-specific configuration (URL, auth, etc).
     * @return SchedulerError::kSuccess if dispatch successful,
     *         SchedulerError::kCoordinationError if backend unavailable,
     *         other SchedulerError codes for validation/format errors.
     *
     * @throws std::exception on internal serialization or network errors.
     *
     * @see scheduler_api_contract.h for error taxonomy.
     */
    SchedulerError dispatchTaskToExternal(
        const ScheduledTask& task,
        ExternalSchedulerType scheduler_type,
        const nlohmann::json& external_config);

    /**
     * @brief Poll an external scheduler for task status and sync results.
     *
     * Queries the external scheduler backend for the current execution status
     * of a previously dispatched task, and if the status has changed (e.g., from
     * SCHEDULED to COMPLETED), syncs the result back to the local result store.
     *
     * Uses exponential backoff on transient failures (network errors, timeouts)
     * to avoid overwhelming the external backend.  Permanent errors (invalid
     * task ID, authentication failure) fail fast without retry.
     *
     * The local result store is the source of truth; external results are synced
     * only if they indicate task completion (success or failure).
     *
     * @param task_id               ThemisDB task identifier.
     * @param external_task_id      External scheduler's task identifier (e.g., K8s pod name).
     * @param scheduler_type        Target scheduler platform.
     * @param external_config       Backend-specific configuration.
     * @param result_store          Local result store for syncing outcomes.
     * @param max_retries           Maximum retry attempts on transient errors (default 3).
     * @param initial_backoff_ms    Initial backoff in milliseconds (default 100).
     * @return SchedulerError::kSuccess if poll succeeded and status was synced,
     *         SchedulerError::kCoordinationError if backend unavailable (after retries),
     *         other SchedulerError codes for other failures.
     *
     * @see scheduler_api_contract.h for error taxonomy.
     */
    SchedulerError pollExternalStatus(
        const std::string& task_id,
        const std::string& external_task_id,
        ExternalSchedulerType scheduler_type,
        const nlohmann::json& external_config,
        TaskResultStore* result_store,
        int max_retries = 3,
        std::chrono::milliseconds initial_backoff_ms = std::chrono::milliseconds{100});

    /**
     * @brief Classify and map external scheduler errors to SchedulerError codes.
     *
     * Examines error information from an external scheduler backend (HTTP status,
     * error message, exception type) and maps it to a standard SchedulerError code.
     * This enables consistent error handling across different external scheduler
     * implementations and makes diagnostics observable and traceable.
     *
     * Error classification rules:
     * - Network/timeout errors (connection refused, deadline exceeded) → kCoordinationError
     * - Configuration errors (invalid URL, missing auth) → kInternalError
     * - Permanent task errors (not found, permission denied) → kExecutionFailed
     * - Transient errors (service temporarily unavailable) → kCoordinationError
     *
     * All classified errors are logged with context (task ID, scheduler type,
     * error details) to support production incident diagnostics.
     *
     * @param error_msg        Error message or exception description.
     * @param http_status      HTTP response status code (if applicable; 0 if not HTTP-based).
     * @param task_id          Task ID for diagnostic logging.
     * @param scheduler_type   Target scheduler for contextual logging.
     * @return Mapped SchedulerError code.
     *
     * @see scheduler_api_contract.h for error taxonomy.
     */
    SchedulerError classifyAndMapExternalError(
        const std::string& error_msg,
        int http_status,
        const std::string& task_id,
        ExternalSchedulerType scheduler_type);

private:
    /// Serialise a JSON object as minimal YAML (enough for Kubernetes manifests).
    static std::string jsonToYaml(const nlohmann::json& j, int indent = 0);

    /// Escape a string for embedding in a Python string literal.
    static std::string pyStringEscape(const std::string& s);
};

} // namespace scheduler
} // namespace themis
