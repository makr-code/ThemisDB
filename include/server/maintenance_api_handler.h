/**
 * @file maintenance_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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
    explicit MaintenanceApiHandler(
        maintenance::DatabaseMaintenanceOrchestrator* orchestrator)
        : orchestrator_(orchestrator) {}

    // ---- Schedule CRUD -------------------------------------------------------

    /** POST /api/v1/maintenance/schedules */
    nlohmann::json createSchedule(const nlohmann::json& body);

    /** GET /api/v1/maintenance/schedules
     *
     *  Optional query parameter: tenant_id
     *  When provided, only schedules belonging to the specified tenant are returned.
     */
    nlohmann::json listSchedules(const std::string& tenant_id = "");

    /** GET /api/v1/maintenance/schedules/{id} */
    nlohmann::json getSchedule(const std::string& id);

    /** PUT /api/v1/maintenance/schedules/{id} */
    nlohmann::json updateSchedule(const std::string& id, const nlohmann::json& body);

    /** PATCH /api/v1/maintenance/schedules/{id} */
    nlohmann::json patchSchedule(const std::string& id, const nlohmann::json& patch);

    /** DELETE /api/v1/maintenance/schedules/{id} */
    nlohmann::json deleteSchedule(const std::string& id);

    // ---- Jobs & control ------------------------------------------------------

    /** GET /api/v1/maintenance/jobs */
    nlohmann::json listJobs(bool active_only = false);

    /** GET /api/v1/maintenance/jobs/{id} */
    nlohmann::json getJob(const std::string& id);

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

    /** GET /api/v1/maintenance/status */
    nlohmann::json getStatus();

    /** GET /api/v1/maintenance/health */
    nlohmann::json getHealth();

    /** GET /api/v1/maintenance/task-handlers */
    nlohmann::json listTaskHandlers();

private:
    maintenance::DatabaseMaintenanceOrchestrator* orchestrator_;

    static nlohmann::json errorResponse(const std::string& msg) {
        return {{"status", "error"}, {"error", msg}};
    }
};

} // namespace server
} // namespace themis
