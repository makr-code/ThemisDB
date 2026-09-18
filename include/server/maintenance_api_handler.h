/**
 * @file maintenance_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "maintenance/database_maintenance_orchestrator.h"
#include <nlohmann/json.hpp>
#include <string>
#include <memory>

namespace themis {
namespace server {

class MaintenanceApiHandler {
public:
    /**
     * @brief Maintenance Api Handler.
     * @param[in,out] orchestrator Input/output parameter.
     * @return Return value.
     */
    explicit MaintenanceApiHandler(
        maintenance::DatabaseMaintenanceOrchestrator* orchestrator)
        : orchestrator_(orchestrator) {}


    /**
     * @brief Create Schedule.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    nlohmann::json createSchedule(const nlohmann::json& body);

    nlohmann::json listSchedules(const std::string& tenant_id = "");

    /**
     * @brief Get Schedule.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    nlohmann::json getSchedule(const std::string& id);

    /**
     * @brief Update Schedule.
     * @param[in] id Input parameter.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    nlohmann::json updateSchedule(const std::string& id, const nlohmann::json& body);

    /**
     * @brief Patch Schedule.
     * @param[in] id Input parameter.
     * @param[in] patch Input parameter.
     * @return Return value.
     */
    nlohmann::json patchSchedule(const std::string& id, const nlohmann::json& patch);

    /**
     * @brief Delete Schedule.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    nlohmann::json deleteSchedule(const std::string& id);

    // ---- Jobs & control ------------------------------------------------------

    nlohmann::json listJobs(bool active_only = false);

    /**
     * @brief Get Job.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    nlohmann::json getJob(const std::string& id);

    /**
     * @brief Cancel Job.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    nlohmann::json cancelJob(const std::string& id);

    nlohmann::json triggerNow(const std::string& schedule_id, bool force = false);


    /**
     * @brief Get Status.
     * @return Return value.
     */
    nlohmann::json getStatus();

    /**
     * @brief Get Health.
     * @return Return value.
     */
    nlohmann::json getHealth();

    /**
     * @brief List Task Handlers.
     * @return Return value.
     */
    nlohmann::json listTaskHandlers();

private:
    maintenance::DatabaseMaintenanceOrchestrator* orchestrator_;

    /**
     * @brief Error Response.
     * @param[in] msg Input parameter.
     * @return Return value.
     * @details Implements errorResponse without additional internal calls.
     */
    static nlohmann::json errorResponse(const std::string& msg) {
        return {{"status", "error"}, {"error", msg}};
    }
};

} // namespace server
} // namespace themis
