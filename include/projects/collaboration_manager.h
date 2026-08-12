/**
 * @file collaboration_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.3
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <shared_mutex>
#include <nlohmann/json.hpp>
#include "projects/DocumentManager/document_manager.h"
#include "projects/project_audit_log.h"
#include "projects/project_metrics.h"
#include "storage/rocksdb_wrapper.h"

namespace themis {
namespace projects {

using json = nlohmann::json;

// ─── Permission enum ──────────────────────────────────────────────────────────

/**
 * @brief Access-level granted to a shared project user.
 */
enum class Permission {
    READ,  ///< Read-only access to project objects
    WRITE, ///< Read + write access; no admin operations
    ADMIN, ///< Full access including share/revoke
};

// ─── User ────────────────────────────────────────────────────────────────────

struct User {
    std::string id;   ///< Unique user identifier
    std::string name; ///< Display name
};

// ─── Change record ────────────────────────────────────────────────────────────

/**
 * @brief A single change event emitted to collaboration subscribers.
 *
 * Subscribers registered via @c CollaborationManager::subscribe() receive
 * one @c Change per committed write within the project.
 */
struct Change {
    std::string project_id;  ///< Project in which the change occurred
    std::string object_name; ///< Affected object key (e.g. doc UUID)
    std::string field_path;  ///< JSON Pointer path of the changed field
    json        old_value;   ///< Previous value (null for new fields)
    json        new_value;   ///< New value (null for deletions)
    int64_t     timestamp;   ///< Unix timestamp (seconds)
    std::string actor;       ///< User who made the change

    json toJson() const;
    static Change fromJson(const json& j);
};

/// Callback type invoked for each collaboration change event.
using ProjectEventCallback = std::function<void(const Change&)>;

// ─── CollaborationManager ────────────────────────────────────────────────────

/**
 * @brief Real-time collaboration and optimistic locking for shared projects.
 *
 * Features
 * ─────────
 * - Project sharing with per-user permission levels.
 * - In-process event callbacks (subscribers) invoked synchronously on
 *   each @c notifyChange() call with latency ≤ 5 ms under normal load.
 * - Optimistic object locking: a lock is held by a single @c locker_id
 *   and must be explicitly released.
 * - Append-only in-memory change log queryable by timestamp.
 *
 * Sharing model
 * ─────────────
 * Sharing requires an explicit user-consent grant.  Calling @c shareProject()
 * without a valid user @c User::id results in Status{false, "permission_denied"}.
 * Cross-project sharing permission delegation is planned for Q4 2026.
 *
 * Thread-safety
 * ─────────────
 * All methods are thread-safe.  @c subscribe() and @c notifyChange() use
 * separate fine-grained locks so that long-running callbacks do not block
 * incoming writes.
 *
 * RocksDB key layout (persistent entries)
 * ────────────────────────────────────────
 *  collab_share:<project_id>:<user_id>  → permission string
 *  collab_change:<project_id>:<ts_ns>   → Change JSON
 */
class CollaborationManager {
public:
    explicit CollaborationManager(std::shared_ptr<RocksDBWrapper> storage);
    ~CollaborationManager() = default;

    // ── Sharing ────────────────────────────────────────────────────────────

    /**
     * @brief Share a project with one or more users.
     *
     * Each user in @p users must have a non-empty @c User::id.
     * Sharing with an empty-id user returns Status{false, "permission_denied"}.
     *
     * @param project_id  Project UUID.
     * @param users       Users to grant access to.
     * @param permission  Access level granted to all listed users.
     */
    Status shareProject(
        const std::string&        project_id,
        const std::vector<User>&  users,
        Permission                permission
    );

    /**
     * @brief Revoke a user's access to a project.
     *
     * @param project_id  Project UUID.
     * @param user_id     User whose access should be revoked.
     */
    Status revokeAccess(
        const std::string& project_id,
        const std::string& user_id
    );

    /**
     * @brief Return the permission level of a user for a project.
     * @return Permission level if the user has access, std::nullopt otherwise.
     */
    std::optional<Permission> getUserPermission(
        const std::string& project_id,
        const std::string& user_id
    ) const;

    // ── Event subscriptions ────────────────────────────────────────────────

    /**
     * @brief Register a callback invoked for every change event.
     *
     * Callbacks are invoked synchronously inside @c notifyChange().
     * Long-running callbacks will delay subsequent notifications.
     * Callbacks must be non-blocking (hand off to a worker thread if needed).
     *
     * @param callback  Callable(const Change&); must be copyable.
     */
    void subscribe(ProjectEventCallback callback);

    /**
     * @brief Remove all registered event callbacks.
     */
    void unsubscribeAll();

    // ── Optimistic locking ─────────────────────────────────────────────────

    /**
     * @brief Acquire a lock on a named object within a project.
     *
     * @param project_id  Project UUID.
     * @param object_name Object key to lock.
     * @param locker_id   Unique identifier of the lock holder.
     * @return Status{true} on success, Status{false, reason} if already locked.
     */
    Status lockObject(
        const std::string& project_id,
        const std::string& object_name,
        const std::string& locker_id
    );

    /**
     * @brief Release a previously acquired object lock.
     *
     * Only the current lock holder (matching @p locker_id) may release the lock.
     *
     * @param project_id  Project UUID.
     * @param object_name Object key to unlock.
     * @param locker_id   Must match the current lock holder.
     * @return Status{true} on success.
     */
    Status unlockObject(
        const std::string& project_id,
        const std::string& object_name,
        const std::string& locker_id
    );

    /**
     * @brief Check whether an object is currently locked.
     *
     * @param project_id  Project UUID.
     * @param object_name Object key to query.
     */
    bool isLocked(
        const std::string& project_id,
        const std::string& object_name
    ) const;

    // ── Audit log DI ─────────────────────────────────────────────────────

    /**
     * @brief Inject an audit log sink.
     *
     * When set, `notifyChange()` records a `DOCUMENT_UPDATED` entry for
     * every change event.  Pass `nullptr` to disable.
     *
     * Thread-safe.
     */
    void setAuditLog(std::shared_ptr<IProjectAuditLog> log);

    /**
     * @brief Remove the audit log sink (no-op if not set).
     */
    void clearAuditLog();

    // ── Metrics DI ────────────────────────────────────────────────────────

    /**
     * @brief Inject a metrics sink.
     *
     * When set, `notifyChange()` increments
     * `ProjectMetrics::recordChange()` for every committed event.
     * Pass `nullptr` to disable.
     *
     * Thread-safe.
     */
    void setMetrics(std::shared_ptr<ProjectMetrics> metrics);

    // ── Change feed ────────────────────────────────────────────────────────

    /**
     * @brief Return all changes to a project recorded after @p since_timestamp.
     *
     * Changes are ordered chronologically.  The in-memory log is bounded to
     * the most recent 10 000 entries per @c CollaborationManager instance.
     *
     * @param project_id      Project UUID.
     * @param since_timestamp Unix timestamp (seconds); 0 returns all entries.
     */
    std::vector<Change> getChanges(
        const std::string& project_id,
        int64_t            since_timestamp
    ) const;

    /**
     * @brief Record a change event and invoke all registered subscribers.
     *
     * Called by writers after committing a mutation.  Persists the change
     * to RocksDB and appends it to the in-memory log before invoking
     * callbacks.
     *
     * @param change  The change to record and broadcast.
     */
    void notifyChange(const Change& change);

private:
    std::shared_ptr<RocksDBWrapper> storage_;

    mutable std::shared_mutex audit_mutex_;
    std::shared_ptr<IProjectAuditLog> audit_log_;

    mutable std::shared_mutex metrics_mutex_;
    std::shared_ptr<ProjectMetrics> metrics_;

    mutable std::shared_mutex subscribers_mutex_;
    std::vector<ProjectEventCallback> subscribers_;

    mutable std::shared_mutex locks_mutex_;
    /// Composite key "<project_id>:<object_name>" → locker_id
    std::unordered_map<std::string, std::string> locks_;

    mutable std::shared_mutex log_mutex_;
    std::vector<Change> change_log_;
    static constexpr size_t kMaxLogEntries = 10'000;

    static constexpr const char* kPermissionRead  = "read";
    static constexpr const char* kPermissionWrite = "write";
    static constexpr const char* kPermissionAdmin = "admin";

    static const char* permissionToString(Permission p) noexcept;
    static std::optional<Permission> permissionFromString(
        const std::string& s) noexcept;
};

} // namespace projects
} // namespace themis

