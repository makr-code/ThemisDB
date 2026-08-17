/**
 * @file security_evidence_collector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "security/key_provider.h"
#include "security/rbac.h"
#include "utils/audit_logger.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <cstdint>
#include <mutex>

namespace themis {
namespace security {

// ============================================================================
// Supporting types
// ============================================================================

/**
 * @brief An exported snapshot of the tamper-evident audit log.
 *
 * Contains summary statistics and every raw log entry within the requested
 * time window, together with chain-integrity verification results.
 */
struct AuditLogExport {
    int64_t  from_ms = 0;             ///< Window start (Unix epoch ms)
    int64_t  to_ms   = 0;             ///< Window end   (Unix epoch ms)
    uint64_t total_events = 0;        ///< Total events in window
    uint64_t security_events = 0;     ///< Authentication / authorisation events
    uint64_t data_access_events = 0;  ///< Data read / write events
    uint64_t key_management_events = 0; ///< Key create / rotate / delete events
    bool     chain_intact = false;    ///< True when hash-chain is unbroken
    std::vector<nlohmann::json> entries; ///< Raw JSON records (chronological)

    nlohmann::json toJson() const;
};

/**
 * @brief Point-in-time metrics snapshot for SOC 2 capacity and availability controls.
 */
struct SecurityMetricsSnapshot {
    int64_t collected_at_ms = 0;  ///< Collection timestamp
    uint64_t active_keys = 0;     ///< Number of currently active encryption keys
    uint64_t total_key_versions = 0; ///< Sum of all key versions across all key IDs
    uint64_t deprecated_keys = 0; ///< Keys in DEPRECATED state (rotation in progress)
    uint64_t total_roles = 0;     ///< Number of RBAC roles registered
    uint64_t audit_log_entries = 0; ///< Approximate total entries in the audit log

    nlohmann::json toJson() const;
};

/**
 * @brief Record of a single key-rotation event for audit/compliance purposes.
 *
 * Populated from `KeyMetadata::created_at_ms` across successive versions —
 * version N+1 being created is the evidence that version N was rotated.
 */
struct KeyRotationRecord {
    std::string key_id;              ///< Logical key identifier
    uint32_t    from_version = 0;    ///< Previous version
    uint32_t    to_version   = 0;    ///< New version after rotation
    int64_t     rotated_at_ms = 0;   ///< When the new version was created
    std::string algorithm;           ///< Key algorithm (e.g. "AES-256-GCM")
    std::string status;              ///< Status of the old version after rotation

    nlohmann::json toJson() const;
};

/**
 * @brief Aggregate report of access-control configuration for a SOC 2 audit.
 *
 * Summarises the RBAC role registry so auditors can verify that least-privilege
 * principles and deny-by-default are in effect.
 */
struct AccessControlReport {
    int64_t generated_at_ms = 0;       ///< Report generation timestamp
    uint64_t total_roles = 0;          ///< Number of registered roles
    std::vector<std::string> role_names; ///< Sorted list of role names
    uint64_t total_permissions = 0;    ///< Total permission entries across all roles
    bool has_admin_role = false;       ///< True when at least one role has wildcard ("*:*")
    bool all_roles_have_permissions = true; ///< True when every role has ≥ 1 permission
    std::vector<std::string> empty_roles;   ///< Roles with no permissions (policy gap)

    nlohmann::json toJson() const;
};

/**
 * @brief Complete SOC 2 Type II evidence bundle for a given time window.
 *
 * Aggregates all four evidence categories required by the security ROADMAP:
 *   1. Audit log export
 *   2. Metrics snapshot
 *   3. Key-rotation records
 *   4. Access-control report
 *
 * The bundle includes a retention verification flag that indicates whether the
 * collected evidence falls within the configured 12-month retention window.
 */
/**
 * @brief Evidence of network control configuration for SOC 2 audit.
 *
 * Captures TLS cipher suites, mTLS-enabled shard count, and rate-limiter
 * configuration snapshot.
 */
struct NetworkControlsEvidence {
    std::vector<std::string> tls_cipher_suites;       ///< Configured TLS 1.3 cipher suite names
    int mtls_enabled_shard_count = 0;                 ///< Number of shards with mTLS enabled
    std::string rate_limiter_config_snapshot;         ///< JSON snapshot of rate limiter config

    nlohmann::json toJson() const;
};

/**
 * @brief Evidence of change management events for SOC 2 audit.
 *
 * Captures configuration audit trail entries and key rotation records
 * within a specified time window.
 */
struct ChangeManagementEvidence {
    std::vector<nlohmann::json> config_audit_trail;   ///< Config changes (last 30 days)
    std::vector<KeyRotationRecord> key_rotation_log;  ///< Key rotation events in window
    int64_t from_ms = 0;                              ///< Window start (Unix epoch ms)
    int64_t to_ms   = 0;                              ///< Window end   (Unix epoch ms)

    nlohmann::json toJson() const;
};

struct SecurityEvidenceBundle {
    std::string bundle_id;                   ///< Unique bundle identifier
    int64_t collected_at_ms = 0;             ///< Bundle collection timestamp
    int64_t window_from_ms = 0;              ///< Evidence window start
    int64_t window_to_ms   = 0;              ///< Evidence window end
    bool within_retention_window = true;     ///< True if window is within retention period
    AuditLogExport           audit_log;      ///< Audit log evidence
    SecurityMetricsSnapshot  metrics;        ///< Point-in-time metrics
    std::vector<KeyRotationRecord> key_rotations; ///< Key-rotation history
    AccessControlReport      access_control; ///< RBAC configuration snapshot
    NetworkControlsEvidence  network_controls;    ///< Network & TLS configuration evidence
    ChangeManagementEvidence change_management;   ///< Change management evidence

    nlohmann::json toJson() const;
};

// ============================================================================
// SecurityEvidenceCollector
// ============================================================================

/**
 * @brief SOC 2 Type II compliance evidence collector for the security module.
 *
 * Aggregates evidence from the audit log, key-management subsystem, and RBAC
 * registry into a structured bundle ready for external auditors.
 *
 * ## Retention policy
 * The default retention period is 365 days (12 months).  `collect()` sets
 * `SecurityEvidenceBundle::within_retention_window = false` when the requested
 * window extends beyond `now - retention_period`, signalling that evidence in
 * that range may have been purged.
 *
 * ## Thread safety
 * All public methods are thread-safe.
 *
 * ## Usage
 * @code
 * SecurityEvidenceCollector collector(
 *     {.retention_period = std::chrono::hours(365 * 24)},
 *     key_provider,
 *     rbac,
 *     &audit_logger
 * );
 *
 * auto from = std::chrono::system_clock::now() - std::chrono::hours(30 * 24);
 * auto to   = std::chrono::system_clock::now();
 * auto bundle = collector.collect(from, to);
 * collector.exportToFile(bundle, "/audit/soc2-evidence-2026-q1.json");
 * @endcode
 */
class SecurityEvidenceCollector {
public:
    /**
     * @brief Configuration for evidence collection and retention.
     */
    struct Config {
        /// How long evidence is retained.  Bundles outside this window are
        /// flagged as potentially incomplete. Default: 365 days.
        std::chrono::seconds retention_period{365 * 24 * 3600};

        /// Optional path for persisting exported bundles (empty = no persistence).
        std::string evidence_store_path;
    };

    /**
     * @brief Construct a SecurityEvidenceCollector.
     *
     * @param config        Retention and storage settings.
     * @param key_provider  Key-management provider (must not be null).
     * @param rbac          RBAC registry (may be null; access-control section will be empty).
     * @param audit_logger  Audit logger (may be null; audit-log section will be empty).
     */
    explicit SecurityEvidenceCollector(
        Config config,
        std::shared_ptr<KeyProvider> key_provider,
        RBAC*              rbac         = nullptr,
        utils::AuditLogger* audit_logger = nullptr);

    ~SecurityEvidenceCollector() = default;

    // Non-copyable, movable
    SecurityEvidenceCollector(const SecurityEvidenceCollector&) = delete;
    SecurityEvidenceCollector& operator=(const SecurityEvidenceCollector&) = delete;
    SecurityEvidenceCollector(SecurityEvidenceCollector&&) noexcept = default;
    SecurityEvidenceCollector& operator=(SecurityEvidenceCollector&&) noexcept = default;

    /**
     * @brief Collect all SOC 2 evidence for the given time window.
     *
     * @param from  Start of the evidence window (inclusive).
     * @param to    End of the evidence window (inclusive).
     * @return      Populated SecurityEvidenceBundle.
     */
    SecurityEvidenceBundle collect(
        std::chrono::system_clock::time_point from,
        std::chrono::system_clock::time_point to) const;

    /**
     * @brief Export a SecurityEvidenceBundle to a JSON file.
     *
     * The file is written atomically (written to a `.tmp` file first, then
     * renamed).  Existing files at @p path are overwritten.
     *
     * @param bundle  Bundle to export.
     * @param path    Destination file path.
     * @return true on success, false on I/O error.
     */
    bool exportToFile(const SecurityEvidenceBundle& bundle,
                      const std::string& path) const;

    /**
     * @brief Verify that the evidence store path contains bundles within the
     *        configured retention window.
     *
     * Checks that the oldest bundle in the store is not older than
     * `now - retention_period`.  A fresh store with no files passes.
     *
     * @param evidence_store_path  Directory containing exported bundle files.
     * @return true if retention is satisfied (no gaps detected), false otherwise.
     */
    bool verifyRetention(const std::string& evidence_store_path) const;

    const Config& config() const noexcept { return config_; }

private:
    Config config_;
    std::shared_ptr<KeyProvider> key_provider_;
    RBAC*               rbac_;
    utils::AuditLogger* audit_logger_;
    mutable std::mutex  mutex_;

    AuditLogExport         collectAuditLog(
        std::chrono::system_clock::time_point from,
        std::chrono::system_clock::time_point to) const;

    SecurityMetricsSnapshot collectMetrics(
        std::chrono::system_clock::time_point at) const;

    std::vector<KeyRotationRecord> collectKeyRotations(
        std::chrono::system_clock::time_point from,
        std::chrono::system_clock::time_point to) const;

    AccessControlReport collectAccessControl() const;

    NetworkControlsEvidence collectNetworkControls() const;

    ChangeManagementEvidence collectChangeManagement(
        std::chrono::system_clock::time_point from,
        std::chrono::system_clock::time_point to) const;

    static std::string generateBundleId();
    static int64_t toMs(std::chrono::system_clock::time_point tp) noexcept;
};

} // namespace security
} // namespace themis
