/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            external_scheduler_adapter.h                       ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 05:37:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     271                                            ║
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
 * @file external_scheduler_adapter.h
 * @brief Integration adapters for external schedulers: Kubernetes CronJob and Apache Airflow.
 *
 * ExternalSchedulerAdapter converts ThemisDB ScheduledTask definitions to and from
 * formats used by external orchestration platforms. It also generates ready-to-use
 * manifests / DAG definitions so that external schedulers can trigger task execution
 * via the ThemisDB HTTP API.
 *
 * ### Kubernetes CronJob integration
 * The adapter generates a Kubernetes CronJob manifest (JSON or YAML) that launches a
 * lightweight HTTP call to the ThemisDB task-scheduler REST endpoint
 * (`POST /api/v1/scheduler/tasks/{id}/execute`). The ThemisDB operator deploys the
 * manifest once; Kubernetes then owns the scheduling cadence.
 *
 * Reverse direction: given a raw Kubernetes CronJob manifest (JSON), the adapter
 * creates a matching ThemisDB ScheduledTask so the same schedule can be mirrored
 * inside ThemisDB.
 *
 * ### Apache Airflow integration
 * The adapter generates a self-contained Python DAG file that can be dropped into
 * an Airflow dags/ folder. Each ThemisDB task becomes an HttpOperator that POSTs to
 * the same REST endpoint. Task `dependencies` are translated into Airflow task
 * dependencies (``>>``), preserving the DAG structure.
 *
 * ⚠️ SECURITY NOTE
 * - Generated manifests / DAGs may contain ThemisDB endpoint URLs. Treat them as
 *   configuration artefacts and store them securely.
 * - Bearer-token authentication is supported for both Kubernetes CronJob and Airflow
 *   specs so that the external scheduler can authenticate against ThemisDB's API.
 * - Do **not** embed plain-text secrets in Kubernetes manifests; use Kubernetes Secrets
 *   or Airflow Connections instead.
 */

#pragma once

#include "scheduler/task_scheduler.h"
#include <string>
#include <vector>
#include <stdexcept>
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

private:
    /// Serialise a JSON object as minimal YAML (enough for Kubernetes manifests).
    static std::string jsonToYaml(const nlohmann::json& j, int indent = 0);

    /// Escape a string for embedding in a Python string literal.
    static std::string pyStringEscape(const std::string& s);
};

} // namespace scheduler
} // namespace themis
