/**
 * @file maintenance_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
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

/**
 * @brief Is Valid Maintenance Identifier.
 * @param[in] value Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: empty(), validateStringLength(), std::string(), validatePathSegment().
 */
bool isValidMaintenanceIdentifier(std::string_view value) {
    themis::utils::InputValidator validator;
    return !value.empty() &&
           validator.validateStringLength(std::string(value), kMaxMaintenanceIdentifierLength) &&
           validator.validatePathSegment(std::string(value));
}

/**
 * @brief Is Valid Tenant Filter.
 * @param[in] value Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: empty(), validateStringLength(), std::string(), validatePathSegment(), validateHeaderValue().
 */
bool isValidTenantFilter(std::string_view value) {
    if (value.empty()) {
        return true;
    }

    themis::utils::InputValidator validator;
    return validator.validateStringLength(std::string(value), kMaxMaintenanceTenantIdLength) &&
           validator.validatePathSegment(std::string(value)) &&
           validator.validateHeaderValue(std::string(value));
}

/**
 * @brief Schedule To Response.
 * @param[in] e Input parameter.
 * @return Return value.
 * @details Calls: toJson().
 */
json scheduleToResponse(const maintenance::MaintenanceScheduleEntry& e) {
    return e.toJson();
}

/**
 * @brief Job To Response.
 * @param[in] j Input parameter.
 * @return Return value.
 * @details Calls: toJson().
 */
json jobToResponse(const maintenance::OrchestratorJob& j) {
    return j.toJson();
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Schedule CRUD
// ---------------------------------------------------------------------------

/**
 * @brief Create Schedule.
 * @param[in] body Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), setStatus(), errorResponse(), maintenance::MaintenanceScheduleEntry::fromJson(), recordError(), what(), std::string(), std::move().
 */
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

/**
 * @brief List Schedules.
 * @param[in] tenant_id Identifier of the tenant.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), setStatus(), errorResponse(), isValidTenantFilter(), setAttribute(), size(), empty(), json::array().
 */
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
    return {{"schedules", arr}, {"count", schedules.size()}};
}

/**
 * @brief Get Schedule.
 * @param[in] id Input parameter.
 * @return Return value.
 * @details Calls: errorResponse(), empty(), isValidMaintenanceIdentifier(), error(), message(), scheduleToResponse().
 */
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

/**
 * @brief Update Schedule.
 * @param[in] id Input parameter.
 * @param[in] body Input parameter.
 * @return Return value.
 * @details Calls: errorResponse(), empty(), isValidMaintenanceIdentifier(), maintenance::MaintenanceScheduleEntry::fromJson(), std::string(), what(), std::move(), error().
 */
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

/**
 * @brief Patch Schedule.
 * @param[in] id Input parameter.
 * @param[in] patch Input parameter.
 * @return Return value.
 * @details Calls: errorResponse(), empty(), isValidMaintenanceIdentifier(), error(), message(), scheduleToResponse().
 */
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

/**
 * @brief Delete Schedule.
 * @param[in] id Input parameter.
 * @return Return value.
 * @details Calls: errorResponse(), empty(), isValidMaintenanceIdentifier(), error(), message().
 */
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

/**
 * @brief List Jobs.
 * @param[in] active_only Input parameter.
 * @return Return value.
 * @details Calls: errorResponse(), json::array(), push_back(), jobToResponse(), size().
 */
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
    return {{"jobs", arr}, {"count", jobs.size()}};
}

/**
 * @brief Get Job.
 * @param[in] id Input parameter.
 * @return Return value.
 * @details Calls: errorResponse(), empty(), isValidMaintenanceIdentifier(), error(), message(), jobToResponse().
 */
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

/**
 * @brief Cancel Job.
 * @param[in] id Input parameter.
 * @return Return value.
 * @details Calls: errorResponse(), empty(), isValidMaintenanceIdentifier(), error(), message().
 */
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

/**
 * @brief Trigger Now.
 * @param[in] schedule_id Identifier of the schedule.
 * @param[in] force Input parameter.
 * @return Return value.
 * @details Calls: errorResponse(), empty(), isValidMaintenanceIdentifier(), error(), message(), jobToResponse().
 */
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

/**
 * @brief Get Status.
 * @return Return value.
 * @details Calls: errorResponse().
 */
json MaintenanceApiHandler::getStatus() {
    if (!orchestrator_) {
      return errorResponse("Orchestrator not initialized");
    }
    auto& orchestrator = *orchestrator_;
    return orchestrator.getStatus();
}

/**
 * @brief Get Health.
 * @return Return value.
 * @details Calls: errorResponse(), getHealthReport(), toJson().
 */
json MaintenanceApiHandler::getHealth() {
    if (!orchestrator_) {
      return errorResponse("Orchestrator not initialized");
    }
    auto& orchestrator = *orchestrator_;
    return orchestrator.getHealthReport().toJson();
}

/**
 * @brief List Task Handlers.
 * @return Return value.
 * @details Calls: errorResponse(), json::array(), push_back(), size().
 */
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
    return {{"task_handlers", arr}, {"count", handlers.size()}};
}

} // namespace server
} // namespace themis

