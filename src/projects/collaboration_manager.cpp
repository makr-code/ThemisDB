// THEMIS_GAP_STATS: gaps=2 unimpl=2 stub=0 mock=0 sim=0 todo=0 debt=0 scanned=2026-05-18
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            collaboration_manager.cpp                          ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-15 18:50:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     230                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 600ad1dcea  2026-04-15  feat(projects): implement ProjectVersioning, ProjectDiff,... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "projects/collaboration_manager.h"

#include <chrono>
#include <random>

namespace themis {
namespace projects {

// ─── Change serialisation ─────────────────────────────────────────────────────

json Change::toJson() const {
    return json{
        {"project_id",  project_id},
        {"object_name", object_name},
        {"field_path",  field_path},
        {"old_value",   old_value},
        {"new_value",   new_value},
        {"timestamp",   timestamp},
        {"actor",       actor},
    };
}

Change Change::fromJson(const json& j) {
    Change c;
    c.project_id  = j.value("project_id",  std::string{});
    c.object_name = j.value("object_name", std::string{});
    c.field_path  = j.value("field_path",  std::string{});
    c.old_value   = j.value("old_value",   json{});
    c.new_value   = j.value("new_value",   json{});
    c.timestamp   = j.value("timestamp",   int64_t{0});
    c.actor       = j.value("actor",       std::string{});
    return c;
}

// ─── CollaborationManager ─────────────────────────────────────────────────────

CollaborationManager::CollaborationManager(
    std::shared_ptr<RocksDBWrapper> storage)
    : storage_(std::move(storage)) {}

// ── Permission helpers ────────────────────────────────────────────────────────

const char* CollaborationManager::permissionToString(Permission p) noexcept {
    switch (p) {
        case Permission::READ:  return kPermissionRead;
        case Permission::WRITE: return kPermissionWrite;
        case Permission::ADMIN: return kPermissionAdmin;
    }
    return kPermissionRead;
}

std::optional<Permission> CollaborationManager::permissionFromString(
    const std::string& s) noexcept
{
    if (s == kPermissionRead)  return Permission::READ;
    if (s == kPermissionWrite) return Permission::WRITE;
    if (s == kPermissionAdmin) return Permission::ADMIN;
    return std::nullopt;
}

// ── Sharing ───────────────────────────────────────────────────────────────────

Status CollaborationManager::shareProject(
    const std::string&       project_id,
    const std::vector<User>& users,
    Permission               permission)
{
    if (project_id.empty())
        return Status::Error("project_id must not be empty");

    for (const auto& user : users) {
        if (user.id.empty())
            return Status::Error("permission_denied: user id must not be empty");
    }

    for (const auto& user : users) {
        const std::string key =
            "collab_share:" + project_id + ":" + user.id;
        if (!storage_->put(key, permissionToString(permission)))
            return Status::Error(
                "Failed to persist share for user: " + user.id);
    }
    return Status::OK();
}

Status CollaborationManager::revokeAccess(
    const std::string& project_id,
    const std::string& user_id)
{
    if (project_id.empty() || user_id.empty())
        return Status::Error("project_id and user_id must not be empty");

    const std::string key = "collab_share:" + project_id + ":" + user_id;
    storage_->del(key);
    return Status::OK();
}

std::optional<Permission> CollaborationManager::getUserPermission(
    const std::string& project_id,
    const std::string& user_id) const
{
    const std::string key = "collab_share:" + project_id + ":" + user_id;
    std::string val;
    if (!storage_->get(key, val)) return std::nullopt;
    return permissionFromString(val);
}

// ── Event subscriptions ───────────────────────────────────────────────────────

void CollaborationManager::subscribe(ProjectEventCallback callback) {
    std::unique_lock lock(subscribers_mutex_);
    subscribers_.push_back(std::move(callback));
}

void CollaborationManager::unsubscribeAll() {
    std::unique_lock lock(subscribers_mutex_);
    subscribers_.clear();
}

// ── Optimistic locking ────────────────────────────────────────────────────────

Status CollaborationManager::lockObject(
    const std::string& project_id,
    const std::string& object_name,
    const std::string& locker_id)
{
    if (locker_id.empty())
        return Status::Error("locker_id must not be empty");

    std::unique_lock lock(locks_mutex_);
    const std::string composite = project_id + ":" + object_name;
    auto it = locks_.find(composite);
    if (it != locks_.end())
        return Status::Error(
            "Object already locked by: " + it->second);

    locks_[composite] = locker_id;
    return Status::OK();
}

Status CollaborationManager::unlockObject(
    const std::string& project_id,
    const std::string& object_name,
    const std::string& locker_id)
{
    std::unique_lock lock(locks_mutex_);
    const std::string composite = project_id + ":" + object_name;
    auto it = locks_.find(composite);
    if (it == locks_.end())
        return Status::Error("Object is not locked");
    if (it->second != locker_id)
        return Status::Error(
            "Cannot unlock: lock held by a different locker");

    locks_.erase(it);
    return Status::OK();
}

bool CollaborationManager::isLocked(
    const std::string& project_id,
    const std::string& object_name) const
{
    std::shared_lock lock(locks_mutex_);
    return locks_.count(project_id + ":" + object_name) > 0;
}

// ── Audit log DI ─────────────────────────────────────────────────────────────

void CollaborationManager::setAuditLog(std::shared_ptr<IProjectAuditLog> log)
{
    std::unique_lock lock(audit_mutex_);
    audit_log_ = std::move(log);
}

void CollaborationManager::clearAuditLog()
{
    std::unique_lock lock(audit_mutex_);
    audit_log_.reset();
}

void CollaborationManager::setMetrics(std::shared_ptr<ProjectMetrics> metrics)
{
    std::unique_lock lock(metrics_mutex_);
    metrics_ = std::move(metrics);
}

// ── Change feed ───────────────────────────────────────────────────────────────

std::vector<Change> CollaborationManager::getChanges(
    const std::string& project_id,
    int64_t            since_timestamp) const
{
    std::shared_lock lock(log_mutex_);
    std::vector<Change> result;
    for (const auto& c : change_log_) {
        if (c.project_id == project_id &&
            c.timestamp  >= since_timestamp)
        {
            result.push_back(c);
        }
    }
    return result;
}

void CollaborationManager::notifyChange(const Change& change) {
    // Persist to storage
    const auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const std::string key =
        "collab_change:" + change.project_id + ":" +
        std::to_string(now_ns);
    storage_->put(key, change.toJson().dump());

    // Append to in-memory log with eviction
    {
        std::unique_lock lock(log_mutex_);
        change_log_.push_back(change);
        if (change_log_.size() > kMaxLogEntries)
            change_log_.erase(change_log_.begin());
    }

    // Invoke subscribers (outside the log lock to avoid deadlock)
    std::shared_lock sub_lock(subscribers_mutex_);
    for (const auto& cb : subscribers_)
        cb(change);

    // Emit audit record if a sink is registered
    {
        std::shared_lock al_lock(audit_mutex_);
        if (audit_log_) {
            ProjectAuditEntry entry;
            entry.project_id   = change.project_id;
            entry.action       = ProjectAuditAction::DOCUMENT_UPDATED;
            entry.actor_id     = change.actor;
            entry.actor_type   = "user";
            entry.resource_id  = change.object_name;
            entry.resource_type = "object";
            entry.timestamp    = std::chrono::system_clock::now();
            entry.details["field_path"] = change.field_path;
            audit_log_->record(entry);
        }
    }

    // Increment collaboration-change metrics counter
    {
        std::shared_lock m_lock(metrics_mutex_);
        if (metrics_)
            metrics_->recordChange();
    }
}

} // namespace projects
} // namespace themis
