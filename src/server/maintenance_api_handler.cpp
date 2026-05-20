/*
 * ThemisDB | File: maintenance_api_handler.cpp | Version: 0.0.13
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=70, M=12, L=0
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file maintenance_api_handler.cpp
 * @brief Implementation of MaintenanceApiHandler.
 */

#include "server/maintenance_api_handler.h"
#include "utils/tracing.h"
#include "utils/input_validator.h"

#include <spdlog/spdlog.h>

using nlohmann::json;

namespace themis {
namespace server {

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------

namespace {

constexpr size_t kMaxMaintenanceIdentifierLength = 256;
constexpr size_t kMaxMaintenanceTenantIdLength = 256;

bool isValidMaintenanceIdentifier(std::string_view value) {
    themis::utils::InputValidator validator;
    return !value.empty() &&
           validator.validateStringLength(std::string(value), kMaxMaintenanceIdentifierLength) &&
           validator.validatePathSegment(std::string(value));
}

bool isValidTenantFilter(std::string_view value) {
    if (value.empty()) {
        return true;
    }

    themis::utils::InputValidator validator;
    return validator.validateStringLength(std::string(value), kMaxMaintenanceTenantIdLength) &&
           validator.validatePathSegment(std::string(value)) &&
           validator.validateHeaderValue(std::string(value));
}

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
    if (!isValidTenantFilter(tenant_id)) {
        span.setStatus(false, "Invalid tenant_id filter");
        return errorResponse("Invalid tenant_id filter");
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
    if (!isValidMaintenanceIdentifier(id)) {
        return errorResponse("Invalid schedule id");
    }

    auto result = orchestrator_->getSchedule(id);
    if (!result) return errorResponse(result.error().message());
    return scheduleToResponse(*result);
}

json MaintenanceApiHandler::updateSchedule(const std::string& id, const json& body) {
    if (!orchestrator_) return errorResponse("Orchestrator not initialized");
    if (id.empty())     return errorResponse("Schedule id must not be empty");
    if (!isValidMaintenanceIdentifier(id)) {
        return errorResponse("Invalid schedule id");
    }

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
    if (!isValidMaintenanceIdentifier(id)) {
        return errorResponse("Invalid schedule id");
    }

    auto result = orchestrator_->patchSchedule(id, patch);
    if (!result) return errorResponse(result.error().message());
    return scheduleToResponse(*result);
}

json MaintenanceApiHandler::deleteSchedule(const std::string& id) {
    if (!orchestrator_) return errorResponse("Orchestrator not initialized");
    if (id.empty())     return errorResponse("Schedule id must not be empty");
    if (!isValidMaintenanceIdentifier(id)) {
        return errorResponse("Invalid schedule id");
    }

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
    if (!isValidMaintenanceIdentifier(id)) {
        return errorResponse("Invalid job id");
    }

    auto result = orchestrator_->getJob(id);
    if (!result) return errorResponse(result.error().message());
    return jobToResponse(*result);
}

json MaintenanceApiHandler::cancelJob(const std::string& id) {
    if (!orchestrator_) return errorResponse("Orchestrator not initialized");
    if (id.empty())     return errorResponse("Job id must not be empty");
    if (!isValidMaintenanceIdentifier(id)) {
        return errorResponse("Invalid job id");
    }

    auto result = orchestrator_->cancelJob(id);
    if (!result) return errorResponse(result.error().message());
    return {{"status", "cancelled"}, {"id", id}};
}

json MaintenanceApiHandler::triggerNow(const std::string& schedule_id, bool force) {
    if (!orchestrator_) return errorResponse("Orchestrator not initialized");
    if (schedule_id.empty()) return errorResponse("Schedule id must not be empty");
    if (!isValidMaintenanceIdentifier(schedule_id)) {
        return errorResponse("Invalid schedule id");
    }

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

