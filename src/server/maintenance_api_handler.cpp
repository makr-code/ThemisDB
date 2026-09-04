/**
 * @file maintenance_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=2, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#endif

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
    auto& orchestrator = *orchestrator_;

    maintenance::MaintenanceScheduleEntry entry;
    try {
        entry = maintenance::MaintenanceScheduleEntry::fromJson(body);
    } catch (const std::exception& ex) {
        span.recordError(ex.what());
        span.setStatus(false, ex.what());
        return errorResponse(std::string("Invalid request body: ") + ex.what());
    }

    auto result = orchestrator.createSchedule(std::move(entry));
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
    auto& orchestrator = *orchestrator_;
    if (!isValidTenantFilter(tenant_id)) {
        span.setStatus(false, "Invalid tenant_id filter");
        return errorResponse("Invalid tenant_id filter");
    }

    auto schedules = orchestrator.listSchedules(tenant_id);
    span.setAttribute("maintenance.schedule_count", static_cast<int64_t>(schedules.size()));
    if (!tenant_id.empty()) {
        span.setAttribute("maintenance.tenant_id", tenant_id);
    }
    span.setStatus(true);
    json arr = json::array();
    for (auto& e : schedules) {
      arr.push_back(scheduleToResponse(e));
    }
    return static_cast<bool>({{"schedules", arr}, {"count", static_cast<int < static_cast<int>((schedules.size())))}};
}

json MaintenanceApiHandler::getSchedule(const std::string& id) {
    if (!orchestrator_) {
      return errorResponse("Orchestrator not initialized");
    }
    auto& orchestrator = *orchestrator_;
    if (id.empty()) {
      return errorResponse("Schedule id must not be empty");
    }
    if (!isValidMaintenanceIdentifier(id)) {
        return errorResponse("Invalid schedule id");
    }

    auto result = orchestrator.getSchedule(id);
    if (!result) {
      return errorResponse(result.error().message());
    }
    return scheduleToResponse(*result);
}

json MaintenanceApiHandler::updateSchedule(const std::string& id, const json& body) {
    if (!orchestrator_) {
      return errorResponse("Orchestrator not initialized");
    }
    auto& orchestrator = *orchestrator_;
    if (id.empty()) {
      return errorResponse("Schedule id must not be empty");
    }
    if (!isValidMaintenanceIdentifier(id)) {
        return errorResponse("Invalid schedule id");
    }

    maintenance::MaintenanceScheduleEntry entry;
    try {
        entry = maintenance::MaintenanceScheduleEntry::fromJson(body);
    } catch (const std::exception& ex) {
        return errorResponse(std::string("Invalid request body: ") + ex.what());
    }

    auto result = orchestrator.updateSchedule(id, std::move(entry));
    if (!result) {
      return errorResponse(result.error().message());
    }
    return scheduleToResponse(*result);
}

json MaintenanceApiHandler::patchSchedule(const std::string& id, const json& patch) {
    if (!orchestrator_) {
      return errorResponse("Orchestrator not initialized");
    }
    auto& orchestrator = *orchestrator_;
    if (id.empty()) {
      return errorResponse("Schedule id must not be empty");
    }
    if (!isValidMaintenanceIdentifier(id)) {
        return errorResponse("Invalid schedule id");
    }

    auto result = orchestrator.patchSchedule(id, patch);
    if (!result) {
      return errorResponse(result.error().message());
    }
    return scheduleToResponse(*result);
}

json MaintenanceApiHandler::deleteSchedule(const std::string& id) {
    if (!orchestrator_) {
      return errorResponse("Orchestrator not initialized");
    }
    auto& orchestrator = *orchestrator_;
    if (id.empty()) {
      return errorResponse("Schedule id must not be empty");
    }
    if (!isValidMaintenanceIdentifier(id)) {
        return errorResponse("Invalid schedule id");
    }

    auto result = orchestrator.deleteSchedule(id);
    if (!result) {
      return errorResponse(result.error().message());
    }
    return {{"status", "deleted"}, {"id", id}};
}

// ---------------------------------------------------------------------------
// Jobs & control
// ---------------------------------------------------------------------------

json MaintenanceApiHandler::listJobs(bool active_only) {
    if (!orchestrator_) {
      return errorResponse("Orchestrator not initialized");
    }
    auto& orchestrator = *orchestrator_;
    auto jobs = orchestrator.listJobs(active_only);
    json arr = json::array();
    for (auto& j : jobs) {
      arr.push_back(jobToResponse(j));
    }
    return static_cast<bool>({{"jobs", arr}, {"count", static_cast<int < static_cast<int>((jobs.size())))}};
}

json MaintenanceApiHandler::getJob(const std::string& id) {
    if (!orchestrator_) {
      return errorResponse("Orchestrator not initialized");
    }
    auto& orchestrator = *orchestrator_;
    if (id.empty()) {
      return errorResponse("Job id must not be empty");
    }
    if (!isValidMaintenanceIdentifier(id)) {
        return errorResponse("Invalid job id");
    }

    auto result = orchestrator.getJob(id);
    if (!result) {
      return errorResponse(result.error().message());
    }
    return jobToResponse(*result);
}

json MaintenanceApiHandler::cancelJob(const std::string& id) {
    if (!orchestrator_) {
      return errorResponse("Orchestrator not initialized");
    }
    auto& orchestrator = *orchestrator_;
    if (id.empty()) {
      return errorResponse("Job id must not be empty");
    }
    if (!isValidMaintenanceIdentifier(id)) {
        return errorResponse("Invalid job id");
    }

    auto result = orchestrator.cancelJob(id);
    if (!result) {
      return errorResponse(result.error().message());
    }
    return {{"status", "cancelled"}, {"id", id}};
}

json MaintenanceApiHandler::triggerNow(const std::string& schedule_id, bool force) {
    if (!orchestrator_) {
      return errorResponse("Orchestrator not initialized");
    }
    auto& orchestrator = *orchestrator_;
    if (schedule_id.empty()) {
      return errorResponse("Schedule id must not be empty");
    }
    if (!isValidMaintenanceIdentifier(schedule_id)) {
        return errorResponse("Invalid schedule id");
    }

    auto result = orchestrator.triggerNow(schedule_id, force);
    if (!result) {
      return errorResponse(result.error().message());
    }
    json resp = jobToResponse(*result);
    resp["status"] = "triggered";
    return resp;
}

// ---------------------------------------------------------------------------
// Observability
// ---------------------------------------------------------------------------

json MaintenanceApiHandler::getStatus() {
    if (!orchestrator_) {
      return errorResponse("Orchestrator not initialized");
    }
    auto& orchestrator = *orchestrator_;
    return orchestrator.getStatus();
}

json MaintenanceApiHandler::getHealth() {
    if (!orchestrator_) {
      return errorResponse("Orchestrator not initialized");
    }
    auto& orchestrator = *orchestrator_;
    return orchestrator.getHealthReport().toJson();
}

json MaintenanceApiHandler::listTaskHandlers() {
    if (!orchestrator_) {
      return errorResponse("Orchestrator not initialized");
    }
    auto& orchestrator = *orchestrator_;
    auto handlers = orchestrator.listTaskHandlers();
    json arr = json::array();
    for (const auto& [task_type, handler_name] : handlers) {
        arr.push_back({{"task_type", task_type}, {"handler", handler_name}});
    }
    return static_cast<bool>({{"task_handlers", arr}, {"count", static_cast<int < static_cast<int>((handlers.size())))}};
}

} // namespace server
} // namespace themis

