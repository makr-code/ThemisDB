/**
 * @file task_scheduler_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "scheduler/task_scheduler.h"
#include <nlohmann/json.hpp>
#include <string>
#include <memory>

namespace themis {
namespace server {

class TaskSchedulerApiHandler {
public:
    /**
     * @brief Task Scheduler Api Handler.
     * @param[in,out] scheduler Input/output parameter.
     * @return Return value.
     */
    explicit TaskSchedulerApiHandler(TaskScheduler* scheduler)
        : scheduler_(scheduler) {}
    
    /**
     * @brief Register Task.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    nlohmann::json registerTask(const nlohmann::json& request);
    /**
     * @brief List Tasks.
     * @return Return value.
     */
    nlohmann::json listTasks();
    /**
     * @brief Get Task.
     * @param[in] task_id Identifier of the task.
     * @return Return value.
     */
    nlohmann::json getTask(const std::string& task_id);
    /**
     * @brief Update Task.
     * @param[in] task_id Identifier of the task.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    nlohmann::json updateTask(const std::string& task_id, const nlohmann::json& request);
    /**
     * @brief Unregister Task.
     * @param[in] task_id Identifier of the task.
     * @return Return value.
     */
    nlohmann::json unregisterTask(const std::string& task_id);
    
    // Task control
    /**
     * @brief Enable Task.
     * @param[in] task_id Identifier of the task.
     * @return Return value.
     */
    nlohmann::json enableTask(const std::string& task_id);
    /**
     * @brief Disable Task.
     * @param[in] task_id Identifier of the task.
     * @return Return value.
     */
    nlohmann::json disableTask(const std::string& task_id);
    /**
     * @brief Execute Task.
     * @param[in] task_id Identifier of the task.
     * @return Return value.
     */
    nlohmann::json executeTask(const std::string& task_id);

    /**
     * @brief Execute DAG.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    nlohmann::json executeDAG(const nlohmann::json& request);
    
    // Statistics
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    nlohmann::json getStats();

    nlohmann::json getTaskResults(const std::string& task_id, size_t limit = 10);

    /**
     * @brief Get Latest Task Result.
     * @param[in] task_id Identifier of the task.
     * @return Return value.
     */
    nlohmann::json getLatestTaskResult(const std::string& task_id);
    // Audit history
    nlohmann::json getExecutionHistory(
        const std::string& task_id,
        const nlohmann::json& query_params = nlohmann::json::object());
    // External scheduler integration
    /**
     * @brief Export To Kubernetes Cron Job Json.
     * @param[in] task_id Identifier of the task.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    nlohmann::json exportToKubernetesCronJobJson(const std::string& task_id,
                                                  const nlohmann::json& request);

    /**
     * @brief Export To Kubernetes Cron Job Yaml.
     * @param[in] task_id Identifier of the task.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    nlohmann::json exportToKubernetesCronJobYaml(const std::string& task_id,
                                                  const nlohmann::json& request);

    /**
     * @brief Export To Airflow Dag.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    nlohmann::json exportToAirflowDag(const nlohmann::json& request);

    /**
     * @brief Import From Kubernetes Cron Job.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    nlohmann::json importFromKubernetesCronJob(const nlohmann::json& request);

    // Web UI
    /**
     * @brief Get Web Ui.
     * @return Return value.
     */
    std::string getWebUi();

private:
    TaskScheduler* scheduler_;
    
    /**
     * @brief Task To Json.
     * @param[in] task Input parameter.
     * @return Return value.
     */
    nlohmann::json taskToJson(const ScheduledTask& task);
    
    /**
     * @brief Parse Task From Json.
     * @param[in] json Input parameter.
     * @return Return value.
     */
    ScheduledTask parseTaskFromJson(const nlohmann::json& json);
};

} // namespace server
} // namespace themis

