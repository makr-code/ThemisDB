/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            external_scheduler_adapter.cpp                     ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 04:19:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     519                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • d302c88467  2026-02-23  fix(scheduler): remove dead code and use AirflowDagConfig... ║
    • 9ed4897df0  2026-02-23  feat(scheduler): integrate Kubernetes CronJob and Airflow... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file external_scheduler_adapter.cpp
 * @brief Integration adapters for Kubernetes CronJob and Apache Airflow schedulers.
 */

#include "scheduler/external_scheduler_adapter.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace scheduler {

// ─────────────────────────────────────────────────────────────────────────────
// Static helpers
// ─────────────────────────────────────────────────────────────────────────────

std::string ExternalSchedulerAdapter::intervalToCron(std::chrono::milliseconds interval) {
    using namespace std::chrono;

    // Round up intervals shorter than 1 minute to 1 minute.
    if (interval < minutes(1)) {
        return "* * * * *"; // every minute
    }

    const auto total_minutes = duration_cast<minutes>(interval).count();
    const auto total_hours   = duration_cast<hours>(interval).count();
    const auto total_days    = total_hours / 24;

    if (total_minutes < 60) {
        // e.g. "*/5 * * * *" for every 5 minutes
        return "*/" + std::to_string(total_minutes) + " * * * *";
    }
    if (total_hours < 24) {
        // e.g. "0 */6 * * *" for every 6 hours
        return "0 */" + std::to_string(total_hours) + " * * *";
    }
    if (total_days <= 1) {
        return "0 0 * * *"; // @daily
    }
    // e.g. "0 0 */3 * *" for every 3 days
    return "0 0 */" + std::to_string(total_days) + " * *";
}

std::string ExternalSchedulerAdapter::effectiveCronSchedule(const ScheduledTask& task) {
    if (task.trigger_type == ScheduledTask::TriggerType::CRON &&
        !task.cron_expression.empty()) {
        return task.cron_expression;
    }
    return intervalToCron(task.interval);
}

std::string ExternalSchedulerAdapter::toK8sName(const std::string& name) {
    std::string result;
    result.reserve(name.size());
    for (unsigned char ch : name) {
        if (std::isalnum(ch)) {
            result += static_cast<char>(std::tolower(ch));
        } else {
            result += '-';
        }
    }
    // Trim leading hyphens
    const auto first = result.find_first_not_of('-');
    if (first == std::string::npos) {
        return "task"; // fallback
    }
    result = result.substr(first);
    // Trim trailing hyphens
    const auto last = result.find_last_not_of('-');
    if (last != std::string::npos) {
        result = result.substr(0, last + 1);
    }
    // Collapse consecutive hyphens
    std::string collapsed;
    collapsed.reserve(result.size());
    bool prev_hyphen = false;
    for (char ch : result) {
        if (ch == '-') {
            if (!prev_hyphen) collapsed += ch;
            prev_hyphen = true;
        } else {
            collapsed += ch;
            prev_hyphen = false;
        }
    }
    // Truncate to 52 characters
    if (collapsed.size() > 52) {
        collapsed = collapsed.substr(0, 52);
        // Remove trailing hyphen after truncation
        while (!collapsed.empty() && collapsed.back() == '-') {
            collapsed.pop_back();
        }
    }
    return collapsed.empty() ? "task" : collapsed;
}

// ─────────────────────────────────────────────────────────────────────────────
// YAML serialiser (subset sufficient for Kubernetes manifests)
// ─────────────────────────────────────────────────────────────────────────────

std::string ExternalSchedulerAdapter::jsonToYaml(const nlohmann::json& j, int indent) {
    const std::string pad(static_cast<size_t>(indent) * 2, ' ');
    const std::string child_pad(static_cast<size_t>(indent + 1) * 2, ' ');
    std::ostringstream out;

    if (j.is_object()) {
        for (auto it = j.begin(); it != j.end(); ++it) {
            out << pad << it.key() << ":";
            const auto& val = it.value();
            if (val.is_object()) {
                out << "\n" << jsonToYaml(val, indent + 1);
            } else if (val.is_array()) {
                out << "\n";
                for (const auto& item : val) {
                    if (item.is_object() || item.is_array()) {
                        // Indent the first key of the object with "- "
                        std::string block = jsonToYaml(item, indent + 2);
                        // Replace the first occurrence of leading spaces with "- "
                        const std::string marker(static_cast<size_t>(indent + 2) * 2, ' ');
                        const std::string dash_marker(static_cast<size_t>(indent + 1) * 2, ' ');
                        // Find first non-empty line and prefix it with "- "
                        auto pos = block.find_first_not_of('\n');
                        if (pos != std::string::npos) {
                            out << dash_marker << "- " << block.substr(marker.size());
                        }
                    } else if (item.is_string()) {
                        out << child_pad << "- " << item.get<std::string>() << "\n";
                    } else {
                        out << child_pad << "- " << item.dump() << "\n";
                    }
                }
            } else if (val.is_string()) {
                const std::string s = val.get<std::string>();
                // Quote strings that contain special YAML characters or look like numbers
                bool needs_quotes = false;
                for (char c : s) {
                    if (c == ':' || c == '{' || c == '}' || c == '[' || c == ']' ||
                        c == '#' || c == '&' || c == '*' || c == '!' || c == '|' ||
                        c == '>' || c == '\'' || c == '"' || c == '%' || c == '@' ||
                        c == '`' || c == '\n') {
                        needs_quotes = true;
                        break;
                    }
                }
                if (!s.empty() && std::isdigit(static_cast<unsigned char>(s[0]))) {
                    needs_quotes = true;
                }
                if (needs_quotes) {
                    out << " \"" << s << "\"\n";
                } else {
                    out << " " << s << "\n";
                }
            } else if (val.is_boolean()) {
                out << " " << (val.get<bool>() ? "true" : "false") << "\n";
            } else if (val.is_null()) {
                out << " null\n";
            } else {
                out << " " << val.dump() << "\n";
            }
        }
    } else if (j.is_string()) {
        out << j.get<std::string>() << "\n";
    } else {
        out << j.dump() << "\n";
    }
    return out.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// Python string escaping
// ─────────────────────────────────────────────────────────────────────────────

std::string ExternalSchedulerAdapter::pyStringEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '\'': out += "\\'"; break;
            case '"':  out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:   out += static_cast<char>(c); break;
        }
    }
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// Kubernetes CronJob – export
// ─────────────────────────────────────────────────────────────────────────────

nlohmann::json ExternalSchedulerAdapter::toKubernetesCronJobJson(
        const ScheduledTask& task,
        const KubernetesCronJobConfig& config) const {

    if (task.id.empty()) {
        throw std::invalid_argument(
            "ExternalSchedulerAdapter: task.id must not be empty");
    }
    if (config.themisdb_base_url.empty()) {
        throw std::invalid_argument(
            "ExternalSchedulerAdapter: KubernetesCronJobConfig::themisdb_base_url "
            "must not be empty");
    }

    const std::string resource_name = toK8sName(task.id);
    const std::string cron_schedule = effectiveCronSchedule(task);
    const std::string execute_url   =
        config.themisdb_base_url + "/api/v1/scheduler/tasks/" + task.id + "/execute";

    // Build the curl command that runs inside the CronJob pod
    std::string curl_cmd =
        "curl -sS -X POST " + execute_url +
        " -H 'Content-Type: application/json' -d '{}'";

    if (!config.api_token_secret_name.empty()) {
        // Reference the token via an environment variable projected from the K8s Secret
        curl_cmd = "curl -sS -X POST " + execute_url +
                   " -H 'Content-Type: application/json'"
                   " -H \"Authorization: Bearer ${THEMISDB_API_TOKEN}\" -d '{}'";
    }

    // Labels
    nlohmann::json labels = {
        {"app", "themisdb-scheduler"},
        {"themisdb/task-id", resource_name}
    };
    for (const auto& kv : config.extra_labels) {
        labels[kv.first] = kv.second;
    }

    // Container spec
    nlohmann::json container = {
        {"name", "themisdb-task-trigger"},
        {"image", config.job_image},
        {"command", nlohmann::json::array({"/bin/sh", "-c", curl_cmd})}
    };
    if (!config.api_token_secret_name.empty()) {
        container["env"] = nlohmann::json::array({
            {
                {"name", "THEMISDB_API_TOKEN"},
                {"valueFrom", {
                    {"secretKeyRef", {
                        {"name", config.api_token_secret_name},
                        {"key", "token"}
                    }}
                }}
            }
        });
    }

    // Full manifest
    nlohmann::json manifest = {
        {"apiVersion", "batch/v1"},
        {"kind", "CronJob"},
        {"metadata", {
            {"name", resource_name},
            {"namespace", config.k8s_namespace},
            {"labels", labels},
            {"annotations", {
                {"themisdb/task-id",          task.id},
                {"themisdb/task-name",        task.name},
                {"themisdb/task-description", task.description}
            }}
        }},
        {"spec", {
            {"schedule", cron_schedule},
            {"suspend",  config.suspend},
            {"successfulJobsHistoryLimit", config.successful_jobs_history_limit},
            {"failedJobsHistoryLimit",     config.failed_jobs_history_limit},
            {"jobTemplate", {
                {"spec", {
                    {"template", {
                        {"metadata", {
                            {"labels", {
                                {"app", "themisdb-scheduler"},
                                {"themisdb/task-id", resource_name}
                            }}
                        }},
                        {"spec", {
                            {"restartPolicy", "OnFailure"},
                            {"containers",   nlohmann::json::array({container})}
                        }}
                    }}
                }}
            }}
        }}
    };

    return manifest;
}

std::string ExternalSchedulerAdapter::toKubernetesCronJobYaml(
        const ScheduledTask& task,
        const KubernetesCronJobConfig& config) const {

    const nlohmann::json manifest = toKubernetesCronJobJson(task, config);
    return jsonToYaml(manifest, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Kubernetes CronJob – import
// ─────────────────────────────────────────────────────────────────────────────

ScheduledTask ExternalSchedulerAdapter::fromKubernetesCronJobJson(
        const nlohmann::json& manifest) const {

    auto require = [&](const nlohmann::json& obj, const std::string& key) -> const nlohmann::json& {
        if (!obj.contains(key)) {
            throw std::invalid_argument(
                "ExternalSchedulerAdapter: missing field '" + key + "' in CronJob manifest");
        }
        return obj.at(key);
    };

    require(manifest, "metadata");
    require(manifest, "spec");

    const auto& metadata = manifest["metadata"];
    const auto& spec     = manifest["spec"];

    require(metadata, "name");
    require(spec,     "schedule");

    ScheduledTask task;
    task.id           = metadata["name"].get<std::string>();
    task.trigger_type = ScheduledTask::TriggerType::CRON;
    task.cron_expression = spec["schedule"].get<std::string>();

    if (metadata.contains("annotations")) {
        const auto& ann = metadata["annotations"];
        if (ann.contains("themisdb/task-name")) {
            task.name = ann["themisdb/task-name"].get<std::string>();
        } else {
            task.name = task.id;
        }
        if (ann.contains("themisdb/task-description")) {
            task.description = ann["themisdb/task-description"].get<std::string>();
        }
        if (ann.contains("themisdb/task-id")) {
            // Use the original ThemisDB task id if present
            task.id = ann["themisdb/task-id"].get<std::string>();
        }
    } else {
        task.name = task.id;
    }

    // Default to a MANUAL trigger type (the task is driven by Kubernetes)
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = ""; // Caller must bind an actual function

    return task;
}

// ─────────────────────────────────────────────────────────────────────────────
// Airflow DAG – export
// ─────────────────────────────────────────────────────────────────────────────

std::string ExternalSchedulerAdapter::toAirflowDagPython(
        const std::vector<ScheduledTask>& tasks,
        const AirflowDagConfig& config) const {

    if (tasks.empty()) {
        throw std::invalid_argument(
            "ExternalSchedulerAdapter: tasks list must not be empty for Airflow DAG export");
    }

    // Determine the DAG schedule: use config.default_schedule unless all tasks
    // share the same cron expression.
    std::string dag_schedule = config.default_schedule;
    {
        const std::string first_sched = effectiveCronSchedule(tasks[0]);
        bool all_same = true;
        for (const auto& t : tasks) {
            if (effectiveCronSchedule(t) != first_sched) {
                all_same = false;
                break;
            }
        }
        if (all_same) {
            dag_schedule = first_sched;
        }
    }

    std::ostringstream py;

    // File header
    py << "# Auto-generated by ThemisDB ExternalSchedulerAdapter\n"
       << "# DO NOT EDIT – regenerate using ThemisDB's scheduler export API.\n"
       << "#\n"
       << "# Description: " << pyStringEscape(config.description) << "\n";

    if (!config.themisdb_base_url.empty()) {
        py << "#\n"
           << "# Connection setup:\n"
           << "#   In the Airflow UI go to Admin > Connections and create an HTTP connection:\n"
           << "#     Conn Id  : " << pyStringEscape(config.http_conn_id) << "\n"
           << "#     Conn Type: HTTP\n"
           << "#     Host     : " << pyStringEscape(config.themisdb_base_url) << "\n"
           << "#   Or use the Airflow CLI:\n"
           << "#     airflow connections add " << pyStringEscape(config.http_conn_id)
           << " --conn-type http --conn-host " << pyStringEscape(config.themisdb_base_url) << "\n";
    }

    py << "\n"
       << "from __future__ import annotations\n"
       << "\n"
       << "from datetime import datetime\n"
       << "from airflow import DAG\n"
       << "from airflow.providers.http.operators.http import SimpleHttpOperator\n"
       << "\n";

    // Tags list
    py << "with DAG(\n"
       << "    dag_id='" << pyStringEscape(config.dag_id) << "',\n"
       << "    description='" << pyStringEscape(config.description) << "',\n"
       << "    schedule_interval='" << pyStringEscape(dag_schedule) << "',\n"
       << "    start_date=datetime.fromisoformat('" << pyStringEscape(config.start_date) << "'),\n"
       << "    catchup=False,\n"
       << "    is_paused_upon_creation=" << (config.is_paused_upon_creation ? "True" : "False") << ",\n";

    if (!config.tags.empty()) {
        py << "    tags=[";
        for (size_t i = 0; i < config.tags.size(); ++i) {
            py << "'" << pyStringEscape(config.tags[i]) << "'";
            if (i + 1 < config.tags.size()) py << ", ";
        }
        py << "],\n";
    }

    py << ") as dag:\n\n";

    // One Airflow operator per ThemisDB task
    for (const auto& task : tasks) {
        // Use the task id as the Airflow task_id (sanitised to a Python identifier)
        std::string airflow_task_id = task.id;
        // Replace non-alphanumeric/underscore characters with _
        std::replace_if(airflow_task_id.begin(), airflow_task_id.end(),
            [](char c){ return !std::isalnum(static_cast<unsigned char>(c)) && c != '_'; }, '_');

        const std::string endpoint =
            "/api/v1/scheduler/tasks/" + task.id + "/execute";

        py << "    " << airflow_task_id << " = SimpleHttpOperator(\n"
           << "        task_id='" << pyStringEscape(airflow_task_id) << "',\n"
           << "        method='POST',\n"
           << "        http_conn_id='" << pyStringEscape(config.http_conn_id) << "',\n"
           << "        endpoint='" << pyStringEscape(endpoint) << "',\n"
           << "        headers={'Content-Type': 'application/json'},\n"
           << "        data='{}',\n"
           << "        response_check=lambda response: response.status_code == 200,\n"
           << "        dag=dag,\n";

        if (!task.name.empty()) {
            py << "        # ThemisDB task name: " << pyStringEscape(task.name) << "\n";
        }

        py << "    )\n\n";
    }

    // Task dependency wiring
    // Build a map from task id to Airflow operator variable name
    auto toAirflowId = [](const std::string& id) {
        std::string aid = id;
        std::replace_if(aid.begin(), aid.end(),
            [](char c){ return !std::isalnum(static_cast<unsigned char>(c)) && c != '_'; }, '_');
        return aid;
    };

    bool has_deps = false;
    for (const auto& task : tasks) {
        if (!task.dependencies.empty()) {
            has_deps = true;
            break;
        }
    }
    if (has_deps) {
        py << "    # Task dependency wiring\n";
        for (const auto& task : tasks) {
            for (const auto& dep_id : task.dependencies) {
                py << "    " << toAirflowId(dep_id) << " >> " << toAirflowId(task.id) << "\n";
            }
        }
        py << "\n";
    }

    return py.str();
}

} // namespace scheduler
} // namespace themis
