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

/**
 * @brief Translates HTTP requests into DatabaseMaintenanceOrchestrator calls.
 *
 * All methods return a JSON object.  On success the object contains the
 * requested data.  On error it contains:
 *   { "status": "error", "error": "<message>" }
 */
class MaintenanceApiHandler {
public:
    /**
     * @brief TBD: Describe MaintenanceApiHandler.
     * @param[in,out] orchestrator Input/output parameter.
     * @return Return value.
     */
    explicit MaintenanceApiHandler(
        maintenance::DatabaseMaintenanceOrchestrator* orchestrator)
        : orchestrator_(orchestrator) {}

    // ---- Schedule CRUD -------------------------------------------------------

     * @brief TBD: Describe createSchedule.
     * @param[in] body Input parameter.
     * @return Return value.
    /** POST /api/v1/maintenance/schedules */
    nlohmann::json createSchedule(const nlohmann::json& body);

    /** GET /api/v1/maintenance/schedules
     *
     *  Optional query parameter: tenant_id
     *  When provided, only schedules belonging to the specified tenant are returned.
     */
    nlohmann::json listSchedules(const std::string& tenant_id = "");

     * @brief TBD: Describe getSchedule.
     * @param[in] id Input parameter.
     * @return Return value.
    /** GET /api/v1/maintenance/schedules/{id} */
    nlohmann::json getSchedule(const std::string& id);

     * @brief TBD: Describe updateSchedule.
     * @param[in] id Input parameter.
     * @param[in] body Input parameter.
     * @return Return value.
    /** PUT /api/v1/maintenance/schedules/{id} */
    nlohmann::json updateSchedule(const std::string& id, const nlohmann::json& body);

     * @brief TBD: Describe patchSchedule.
     * @param[in] id Input parameter.
     * @param[in] patch Input parameter.
     * @return Return value.
    /** PATCH /api/v1/maintenance/schedules/{id} */
    nlohmann::json patchSchedule(const std::string& id, const nlohmann::json& patch);

     * @brief TBD: Describe deleteSchedule.
     * @param[in] id Input parameter.
     * @return Return value.
    /** DELETE /api/v1/maintenance/schedules/{id} */
    nlohmann::json deleteSchedule(const std::string& id);

    // ---- Jobs & control ------------------------------------------------------

    /** GET /api/v1/maintenance/jobs */
    nlohmann::json listJobs(bool active_only = false);

     * @brief TBD: Describe getJob.
     * @param[in] id Input parameter.
     * @return Return value.
    /** GET /api/v1/maintenance/jobs/{id} */
    nlohmann::json getJob(const std::string& id);

     * @brief TBD: Describe cancelJob.
     * @param[in] id Input parameter.
     * @return Return value.
    /** POST /api/v1/maintenance/jobs/{id}/cancel */
    nlohmann::json cancelJob(const std::string& id);

    /** POST /api/v1/maintenance/schedules/{id}/run
     *
     *  Optional body: { "force": true }
     *  When force=true the maintenance window check is bypassed.
     *  Requires maintenance:admin scope; regular (non-forced) trigger
     *  requires only maintenance:write (enforced at the HTTP layer).
     */
    nlohmann::json triggerNow(const std::string& schedule_id, bool force = false);

    // ---- Observability -------------------------------------------------------

     * @brief TBD: Describe getStatus.
     * @return Return value.
    /** GET /api/v1/maintenance/status */
    nlohmann::json getStatus();

     * @brief TBD: Describe getHealth.
     * @return Return value.
    /** GET /api/v1/maintenance/health */
    nlohmann::json getHealth();

     * @brief TBD: Describe listTaskHandlers.
     * @return Return value.
    /** GET /api/v1/maintenance/task-handlers */
    nlohmann::json listTaskHandlers();

private:
    maintenance::DatabaseMaintenanceOrchestrator* orchestrator_;

    /**
     * @brief TBD: Describe errorResponse.
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
