/**
 * @file project_lifecycle.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.3
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "projects/project_lifecycle.h"
#include <stdexcept>

#include <chrono>
#include <sstream>
#include <string>

namespace themis {
namespace projects {

// ─── Free helpers ─────────────────────────────────────────────────────────────

const char* projectStateToString(ProjectState state) noexcept {
    switch (state) {
        case ProjectState::CREATED:  return "created";
        case ProjectState::ACTIVE:   return "active";
        case ProjectState::ARCHIVED: return "archived";
        case ProjectState::DELETED:  return "deleted";
    }
    return "unknown";
}

std::optional<ProjectState> projectStateFromString(const std::string& s) noexcept {
    if (s == "created") {
      return ProjectState::CREATED;
    }
    if (s == "active") {
      return ProjectState::ACTIVE;
    }
    if (s == "archived") {
      return ProjectState::ARCHIVED;
    }
    if (s == "deleted") {
      return ProjectState::DELETED;
    }
    return std::nullopt;
}

// ─── ProjectStateTransition serialisation ─────────────────────────────────────

json ProjectStateTransition::toJson() const {
    return json{
        {"project_id",  project_id},
        {"from_state",  projectStateToString(from_state)},
        {"to_state",    projectStateToString(to_state)},
        {"timestamp",   timestamp},
        {"actor",       actor},
        {"reason",      reason},
    };
}

ProjectStateTransition ProjectStateTransition::fromJson(const json& j) {
    ProjectStateTransition t;
    t.project_id = j.value("project_id", std::string{});
    t.from_state = projectStateFromString(
        j.value("from_state", std::string{"created"}))
        .value_or(ProjectState::CREATED);
    t.to_state = projectStateFromString(
        j.value("to_state", std::string{"created"}))
        .value_or(ProjectState::CREATED);
    t.timestamp = j.value("timestamp", int64_t{0});
    t.actor     = j.value("actor", std::string{});
    t.reason    = j.value("reason", std::string{});
    return t;
}

// ─── ProjectLifecycle ─────────────────────────────────────────────────────────

ProjectLifecycle::ProjectLifecycle(std::shared_ptr<RocksDBWrapper> storage)
    : storage_(std::move(storage)) {}

// ── Transition table ─────────────────────────────────────────────────────────

bool ProjectLifecycle::isValidTransition(
    ProjectState from, ProjectState to) noexcept
{
    // DELETED is a terminal state — no outgoing transitions
    if (from == ProjectState::DELETED) {
      return false;
    }

    switch (to) {
        case ProjectState::ACTIVE:
            return from == ProjectState::CREATED ||
                   from == ProjectState::ARCHIVED;
        case ProjectState::ARCHIVED:
            return from == ProjectState::ACTIVE;
        case ProjectState::DELETED:
            return from == ProjectState::ACTIVE ||
                   from == ProjectState::ARCHIVED;
        default:
            return false;
    }
}

// ── Low-level helper ─────────────────────────────────────────────────────────

Status ProjectLifecycle::applyTransition(
    const std::string& project_id,
    ProjectState       to_state,
    const std::string& actor,
    const std::string& reason)
{
    // ── Entry validation: enforce bounded runtime contract ─────────────────────
    if (project_id.empty())
        return Status::Error("applyTransition: project_id must not be empty");
    if (actor.empty())
        return Status::Error("applyTransition: actor must not be empty");

    // Read current state (already held under unique_lock by callers)
    std::string state_str;
    std::optional<ProjectState> current = {};

    if (storage_->get("lifecycle:" + project_id, state_str))
        current = projectStateFromString(state_str);

    if (!current.has_value())
        return Status::Error("applyTransition: project lifecycle not found: " + project_id);

    if (!isValidTransition(*current, to_state)) {
        return Status::Error(
            std::string("applyTransition: invalid transition from ") +
            projectStateToString(*current) + " to " +
            projectStateToString(to_state));
    }

    const auto now = static_cast<int64_t>(
        std::chrono::system_clock::now().time_since_epoch() /
        std::chrono::seconds(1));

    // Nanosecond-resolution key for audit log ordering
    const auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    ProjectStateTransition transition;
    transition.project_id  = project_id;
    transition.from_state  = *current;
    transition.to_state    = to_state;
    transition.timestamp   = now;
    transition.actor       = actor;
    transition.reason      = reason;

    const std::string log_key =
        "lifecycle_log:" + project_id + ":" +
        std::to_string(now_ns);

    // Write new state
    if (!storage_->put("lifecycle:" + project_id,
                        projectStateToString(to_state)))
        return Status::Error("applyTransition: failed to persist lifecycle state");

    // Append audit entry
    if (!storage_->put(log_key, transition.toJson().dump())) {
        // Non-fatal: state was written; log entry is best-effort
    }

    return Status::OK();
}

// ── Public API ───────────────────────────────────────────────────────────────

Status ProjectLifecycle::initProject(
    const std::string& project_id,
    const std::string& actor)
{
    if (project_id.empty())
        return Status::Error("project_id must not be empty");

    std::unique_lock lock(mutex_);

    const std::string state_key = "lifecycle:" + project_id;
    std::string existing;
    if (storage_->get(state_key, existing))
        return Status::Error(
            "Lifecycle already initialised for project: " + project_id);

    const auto now = static_cast<int64_t>(
        std::chrono::system_clock::now().time_since_epoch() /
        std::chrono::seconds(1));
    const auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    if (!storage_->put(state_key, projectStateToString(ProjectState::CREATED)))
        return Status::Error("Failed to write initial lifecycle state");

    ProjectStateTransition t;
    t.project_id  = project_id;
    t.from_state  = ProjectState::CREATED; // sentinel — no prior state
    t.to_state    = ProjectState::CREATED;
    t.timestamp   = now;
    t.actor       = actor;
    t.reason      = "project initialised";

    storage_->put(
        "lifecycle_log:" + project_id + ":" + std::to_string(now_ns),
        t.toJson().dump());

    return Status::OK();
}

Status ProjectLifecycle::activate(
    const std::string& project_id,
    const std::string& actor)
{
    if (project_id.empty())
        return Status::Error("project_id must not be empty");
    std::unique_lock lock(mutex_);
    return applyTransition(project_id, ProjectState::ACTIVE, actor, {});
}

Status ProjectLifecycle::archive(
    const std::string& project_id,
    const std::string& actor,
    const std::string& reason)
{
    if (project_id.empty())
        return Status::Error("project_id must not be empty");
    std::unique_lock lock(mutex_);
    return applyTransition(project_id, ProjectState::ARCHIVED, actor, reason);
}

Status ProjectLifecycle::deleteProject(
    const std::string& project_id,
    const std::string& actor)
{
    if (project_id.empty())
        return Status::Error("project_id must not be empty");
    std::unique_lock lock(mutex_);
    return applyTransition(project_id, ProjectState::DELETED, actor, {});
}

std::optional<ProjectState> ProjectLifecycle::getState(
    const std::string& project_id) const
{
    std::shared_lock lock(mutex_);
    std::string val;
    if (!storage_->get("lifecycle:" + project_id, val))
        return std::nullopt;
    return projectStateFromString(val);
}

std::vector<ProjectStateTransition> ProjectLifecycle::getAuditTrail(
    const std::string& project_id) const
{
    std::shared_lock lock(mutex_);
    std::vector<ProjectStateTransition> trail;

    const std::string prefix = "lifecycle_log:" + project_id + ":";
    storage_->scanPrefix(prefix, [&](std::string_view, std::string_view val) {
        try {
            trail.push_back(
                ProjectStateTransition::fromJson(
                    json::parse(std::string(val))));
        } catch (const nlohmann::json::exception &) {
        } catch (const std::exception &) {
        } catch (const std::string &) {
        } catch (const char *) {
        }
        return true;
    });

    // Entries are already ordered by key (nanosecond timestamp suffix)
    return trail;
}

} // namespace projects
} // namespace themis
