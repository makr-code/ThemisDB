/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            maintenance_api_handler.cpp                        ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-14 11:37:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     218                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 12bb69b756  2026-04-13  feat(maintenance): multi-tenant schedule isolation (v2.0.... ║
    • 717093f9bc  2026-03-12  feat: implement IMaintenanceTaskHandler registry for main... ║
    • a63629a5c5  2026-03-12  feat: Force-Run Endpoint Window Override (v1.1.0) ║
    • 1b86d845d2  2026-03-11  feat(tracing): add OpenTelemetry spans to all major API h... ║
    • 0eb79f3e41  2026-03-11  feat: add DatabaseMaintenanceOrchestrator with full sched... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file maintenance_api_handler.cpp
 * @brief Implementation of MaintenanceApiHandler.
 */

#include "server/maintenance_api_handler.h"
#include "utils/tracing.h"

#include <spdlog/spdlog.h>

using nlohmann::json;

namespace themis {
namespace server {

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------

namespace {

json scheduleToResponse(const maintenance::MaintenanceScheduleEntry& e) {
    return e.toJson();
}

json jobToResponse(const maintenance::OrchestratorJob& j) {
    return j.toJson();
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Schedule CRUD
// ---------------------------------------------------------------------------

json MaintenanceApiHandler::createSchedule(const json& body) {
    auto span = Tracer::startSpan("POST /maintenance/schedules");
    if (!orchestrator_) {
        span.setStatus(false, "Orchestrator not initialized");
        return errorResponse("Orchestrator not initialized");
    }

    maintenance::MaintenanceScheduleEntry entry;
    try {
        entry = maintenance::MaintenanceScheduleEntry::fromJson(body);
    } catch (const std::exception& ex) {
        span.recordError(ex.what());
        span.setStatus(false, ex.what());
        return errorResponse(std::string("Invalid request body: ") + ex.what());
    }

    auto result = orchestrator_->createSchedule(std::move(entry));
    if (!result) {
        span.setStatus(false, result.error().message());
        return errorResponse(result.error().message());
    }
    span.setStatus(true);
    json resp = scheduleToResponse(*result);
    resp["status"] = "created";
    return resp;
}

json MaintenanceApiHandler::listSchedules(const std::string& tenant_id) {
    auto span = Tracer::startSpan("GET /maintenance/schedules");
    if (!orchestrator_) {
        span.setStatus(false, "Orchestrator not initialized");
        return errorResponse("Orchestrator not initialized");
    }

    auto schedules = orchestrator_->listSchedules(tenant_id);
    span.setAttribute("maintenance.schedule_count", static_cast<int64_t>(schedules.size()));
    if (!tenant_id.empty()) {
        span.setAttribute("maintenance.tenant_id", tenant_id);
    }
    span.setStatus(true);
    json arr = json::array();
    for (auto& e : schedules) arr.push_back(scheduleToResponse(e));
    return {{"schedules", arr}, {"count", static_cast<int>(schedules.size())}};
}

json MaintenanceApiHandler::getSchedule(const std::string& id) {
    if (!orchestrator_) return errorResponse("Orchestrator not initialized");
    if (id.empty())     return errorResponse("Schedule id must not be empty");

    auto result = orchestrator_->getSchedule(id);
    if (!result) return errorResponse(result.error().message());
    return scheduleToResponse(*result);
}

json MaintenanceApiHandler::updateSchedule(const std::string& id, const json& body) {
    if (!orchestrator_) return errorResponse("Orchestrator not initialized");
    if (id.empty())     return errorResponse("Schedule id must not be empty");

    maintenance::MaintenanceScheduleEntry entry;
    try {
        entry = maintenance::MaintenanceScheduleEntry::fromJson(body);
    } catch (const std::exception& ex) {
        return errorResponse(std::string("Invalid request body: ") + ex.what());
    }

    auto result = orchestrator_->updateSchedule(id, std::move(entry));
    if (!result) return errorResponse(result.error().message());
    return scheduleToResponse(*result);
}

json MaintenanceApiHandler::patchSchedule(const std::string& id, const json& patch) {
    if (!orchestrator_) return errorResponse("Orchestrator not initialized");
    if (id.empty())     return errorResponse("Schedule id must not be empty");

    auto result = orchestrator_->patchSchedule(id, patch);
    if (!result) return errorResponse(result.error().message());
    return scheduleToResponse(*result);
}

json MaintenanceApiHandler::deleteSchedule(const std::string& id) {
    if (!orchestrator_) return errorResponse("Orchestrator not initialized");
    if (id.empty())     return errorResponse("Schedule id must not be empty");

    auto result = orchestrator_->deleteSchedule(id);
    if (!result) return errorResponse(result.error().message());
    return {{"status", "deleted"}, {"id", id}};
}

// ---------------------------------------------------------------------------
// Jobs & control
// ---------------------------------------------------------------------------

json MaintenanceApiHandler::listJobs(bool active_only) {
    if (!orchestrator_) return errorResponse("Orchestrator not initialized");

    auto jobs = orchestrator_->listJobs(active_only);
    json arr = json::array();
    for (auto& j : jobs) arr.push_back(jobToResponse(j));
    return {{"jobs", arr}, {"count", static_cast<int>(jobs.size())}};
}

json MaintenanceApiHandler::getJob(const std::string& id) {
    if (!orchestrator_) return errorResponse("Orchestrator not initialized");
    if (id.empty())     return errorResponse("Job id must not be empty");

    auto result = orchestrator_->getJob(id);
    if (!result) return errorResponse(result.error().message());
    return jobToResponse(*result);
}

json MaintenanceApiHandler::cancelJob(const std::string& id) {
    if (!orchestrator_) return errorResponse("Orchestrator not initialized");
    if (id.empty())     return errorResponse("Job id must not be empty");

    auto result = orchestrator_->cancelJob(id);
    if (!result) return errorResponse(result.error().message());
    return {{"status", "cancelled"}, {"id", id}};
}

json MaintenanceApiHandler::triggerNow(const std::string& schedule_id, bool force) {
    if (!orchestrator_) return errorResponse("Orchestrator not initialized");
    if (schedule_id.empty()) return errorResponse("Schedule id must not be empty");

    auto result = orchestrator_->triggerNow(schedule_id, force);
    if (!result) return errorResponse(result.error().message());
    json resp = jobToResponse(*result);
    resp["status"] = "triggered";
    return resp;
}

// ---------------------------------------------------------------------------
// Observability
// ---------------------------------------------------------------------------

json MaintenanceApiHandler::getStatus() {
    if (!orchestrator_) return errorResponse("Orchestrator not initialized");
    return orchestrator_->getStatus();
}

json MaintenanceApiHandler::getHealth() {
    if (!orchestrator_) return errorResponse("Orchestrator not initialized");
    return orchestrator_->getHealthReport().toJson();
}

json MaintenanceApiHandler::listTaskHandlers() {
    if (!orchestrator_) return errorResponse("Orchestrator not initialized");

    auto handlers = orchestrator_->listTaskHandlers();
    json arr = json::array();
    for (const auto& [task_type, handler_name] : handlers) {
        arr.push_back({{"task_type", task_type}, {"handler", handler_name}});
    }
    return {{"task_handlers", arr}, {"count", static_cast<int>(handlers.size())}};
}

} // namespace server
} // namespace themis
