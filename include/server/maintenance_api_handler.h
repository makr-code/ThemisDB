/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            maintenance_api_handler.h                          ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-13 20:26:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     140                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 12bb69b756  2026-04-13  feat(maintenance): multi-tenant schedule isolation (v2.0.... ║
    • 717093f9bc  2026-03-12  feat: implement IMaintenanceTaskHandler registry for main... ║
    • a63629a5c5  2026-03-12  feat: Force-Run Endpoint Window Override (v1.1.0) ║
    • 0eb79f3e41  2026-03-11  feat: add DatabaseMaintenanceOrchestrator with full sched... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file maintenance_api_handler.h
 * @brief HTTP API handler for DatabaseMaintenanceOrchestrator endpoints.
 *
 * Exposes a RESTful API over the orchestrator:
 *
 *   Schedule CRUD:
 *     POST   /api/v1/maintenance/schedules          – Create schedule
 *     GET    /api/v1/maintenance/schedules           – List all schedules
 *     GET    /api/v1/maintenance/schedules/{id}      – Get schedule
 *     PUT    /api/v1/maintenance/schedules/{id}      – Full update (replace)
 *     PATCH  /api/v1/maintenance/schedules/{id}      – Partial update
 *     DELETE /api/v1/maintenance/schedules/{id}      – Delete schedule
 *
 *   Jobs & Control:
 *     GET    /api/v1/maintenance/jobs               – List jobs
 *     GET    /api/v1/maintenance/jobs/{id}          – Get job
 *     POST   /api/v1/maintenance/jobs/{id}/cancel   – Cancel job
 *     POST   /api/v1/maintenance/schedules/{id}/run – Trigger immediately
 *
 *   Observability:
 *     GET    /api/v1/maintenance/status             – Orchestrator status
 *     GET    /api/v1/maintenance/health             – Aggregated health
 *     GET    /api/v1/maintenance/task-handlers      – Registered task handlers
 *
 * All endpoints require authentication (enforced at the HttpServer layer).
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
