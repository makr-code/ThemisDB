/**
 * @file project_lifecycle.h
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
#include <optional>
#include <shared_mutex>
#include <nlohmann/json.hpp>
#include "projects/DocumentManager/document_manager.h"
#include "storage/rocksdb_wrapper.h"

namespace themis {
namespace projects {

using json = nlohmann::json;

// ─── Project state machine ────────────────────────────────────────────────────

/**
 * @brief Valid states in the project lifecycle.
 *
 * Allowed transitions
 * ───────────────────
 *  CREATED  → ACTIVE
 *  ACTIVE   → ARCHIVED
 *  ACTIVE   → DELETED
 *  ARCHIVED → ACTIVE   (un-archive)
 *  ARCHIVED → DELETED
 *
 * The DELETED state is terminal: no further transitions are permitted.
 */
enum class ProjectState {
    CREATED,  ///< Project was initialised but not yet activated
    ACTIVE,   ///< Project is live and accepting reads/writes
    ARCHIVED, ///< Project is read-only; data is preserved
    DELETED,  ///< Project is logically deleted (terminal state)
};

/// Human-readable name for a ProjectState value.
const char* projectStateToString(ProjectState state) noexcept;

/// Parse a state string produced by projectStateToString().
/// Returns std::nullopt for unknown strings.
std::optional<ProjectState> projectStateFromString(const std::string& s) noexcept;

// ─── Audit record ─────────────────────────────────────────────────────────────

/**
 * @brief Append-only audit log entry for a single lifecycle transition.
 *
 * Audit log entries are written atomically with the state change and are
 * tamper-evident: deletion requires an explicit admin privilege check at
 * the implementation level.
 */
struct ProjectStateTransition {
    std::string  project_id;  ///< Project UUID
    ProjectState from_state;  ///< State before transition
    ProjectState to_state;    ///< State after transition
    int64_t      timestamp;   ///< Unix timestamp (seconds)
    std::string  actor;       ///< User / service that triggered the transition
    std::string  reason;      ///< Optional human-readable reason

    json toJson() const;
    static ProjectStateTransition fromJson(const json& j);
};

// ─── ProjectLifecycle ─────────────────────────────────────────────────────────

/**
 * @brief Manages atomic project state transitions with an append-only audit trail.
 *
 * State transitions are validated against the allowed transition table
 * before being committed.  Both the new state record and the audit-log
 * entry are written in a single RocksDB write batch to ensure atomicity.
 *
 * RocksDB key layout
 * ──────────────────
 *  lifecycle:<project_id>              → current ProjectState (string)
 *  lifecycle_log:<project_id>:<ts_ns>  → ProjectStateTransition JSON
 *
 * All methods are thread-safe (protected by an internal shared_mutex).
 */
class ProjectLifecycle {
public:
    explicit ProjectLifecycle(std::shared_ptr<RocksDBWrapper> storage);
    ~ProjectLifecycle() = default;

    /**
     * @brief Initialise lifecycle tracking for a newly created project.
     *
     * Sets the initial state to CREATED and records the first audit entry.
     * Returns an error if lifecycle state already exists for the project.
     *
     * @param project_id  Project UUID.
     * @param actor       Identity of the creator (optional).
     */
    Status initProject(
        const std::string& project_id,
        const std::string& actor = {}
    );

    /**
     * @brief Transition a project from CREATED or ARCHIVED to ACTIVE.
     *
     * @param project_id  Project UUID.
     * @param actor       Identity of the user performing the transition.
     */
    Status activate(
        const std::string& project_id,
        const std::string& actor = {}
    );

    /**
     * @brief Transition a project from ACTIVE to ARCHIVED.
     *
     * Archived projects become read-only; all data is preserved.
     *
     * @param project_id  Project UUID.
     * @param actor       Identity of the user archiving the project.
     * @param reason      Optional human-readable archival reason.
     */
    Status archive(
        const std::string& project_id,
        const std::string& actor = {},
        const std::string& reason = {}
    );

    /**
     * @brief Transition a project to the terminal DELETED state.
     *
     * Once deleted a project cannot be recovered through this interface.
     *
     * @param project_id  Project UUID.
     * @param actor       Identity of the user deleting the project.
     */
    Status deleteProject(
        const std::string& project_id,
        const std::string& actor = {}
    );

    /**
     * @brief Return the current state of a project.
     * @return ProjectState if found, std::nullopt if unknown project.
     */
    std::optional<ProjectState> getState(const std::string& project_id) const;

    /**
     * @brief Return the complete append-only audit trail for a project.
     *
     * Entries are ordered chronologically (oldest first).
     */
    std::vector<ProjectStateTransition> getAuditTrail(
        const std::string& project_id
    ) const;

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    mutable std::shared_mutex       mutex_;

    /// Validate whether a transition from → to is permitted.
    static bool isValidTransition(ProjectState from, ProjectState to) noexcept;

    /// Low-level helper: write new state + audit entry in one batch.
    Status applyTransition(
        const std::string&  project_id,
        ProjectState        to_state,
        const std::string&  actor,
        const std::string&  reason
    );
};

} // namespace projects
} // namespace themis

