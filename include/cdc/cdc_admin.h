/**
 * @file cdc_admin.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include "cdc/changefeed.h"
#include "cdc/cdc_metrics.h"
#include "nlohmann/json.hpp"

namespace themis {
// Forward declarations
class RocksDBWrapper;
namespace cdc {
class ICDCTransport;

// Forward declarations
class TenantBufferManager;

/**
 * Result of a purge operation
 */
struct PurgeResult {
    uint64_t events_deleted = 0;
    uint64_t elapsed_time_ms = 0;
    
    nlohmann::json toJson() const {
        return {
            {"events_deleted", events_deleted},
            {"elapsed_time_ms", elapsed_time_ms}
        };
    }
};

/**
 * Health status of CDC components
 */
struct HealthStatus {
    bool is_healthy = true;
    std::string message;
    
    // Component health
    bool changefeed_healthy = true;
    bool buffer_healthy = true;
    bool retention_healthy = true;
    
    // Metrics
    double buffer_utilization = 0.0;  // 0.0 to 1.0
    uint64_t error_count = 0;
    uint64_t error_rate_per_sec = 0;
    
    nlohmann::json toJson() const {
        return {
            {"is_healthy", is_healthy},
            {"message", message},
            {"components", {
                {"changefeed", changefeed_healthy},
                {"buffer", buffer_healthy},
                {"retention", retention_healthy}
            }},
            {"metrics", {
                {"buffer_utilization", buffer_utilization},
                {"error_count", error_count},
                {"error_rate_per_sec", error_rate_per_sec}
            }}
        };
    }
};

/**
 * Comprehensive diagnostics information
 */
struct DiagnosticsInfo {
    Changefeed::Watermarks watermarks;
    CDCMetrics metrics;
    HealthStatus health;
    
    // Additional stats
    uint64_t total_events = 0;
    uint64_t buffer_size = 0;
    std::chrono::system_clock::time_point uptime_start;
    
    nlohmann::json toJson() const {
        auto now = std::chrono::system_clock::now();
        auto uptime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - uptime_start).count();
        
        return {
            {"watermarks", {
                {"low_watermark", watermarks.low_watermark},
                {"high_watermark", watermarks.high_watermark},
                {"oldest_timestamp_ms", watermarks.oldest_timestamp_ms},
                {"newest_timestamp_ms", watermarks.newest_timestamp_ms}
            }},
            {"metrics", metrics.toJson()},
            {"health", health.toJson()},
            {"stats", {
                {"total_events", total_events},
                {"buffer_size", buffer_size},
                {"uptime_ms", uptime_ms}
            }}
        };
    }
};

/**
 * Status information for the current retention/compaction state
 */
struct RetentionStatus {
    // --- Current log status ---
    uint64_t total_events = 0;              ///< Current number of change events
    size_t   total_size_bytes = 0;          ///< Approximate log size in bytes
    int64_t  oldest_event_age_ms = 0;       ///< Age of the oldest event (ms)
    int64_t  oldest_timestamp_ms = 0;       ///< Absolute timestamp of oldest event
    int64_t  newest_timestamp_ms = 0;       ///< Absolute timestamp of newest event
    int64_t  next_cleanup_time_ms = 0;      ///< Estimated time of next scheduled cleanup (epoch ms)
    bool     compact_on_cleanup = false;    ///< Whether key compaction runs with each cleanup cycle

    // --- Current retention policy configuration ---
    bool     policy_enabled = false;                ///< Whether automatic retention cleanup is active
    uint32_t policy_max_age_hours = 168;            ///< Maximum event age before deletion (hours)
    uint64_t policy_max_event_count = 1000000;      ///< Maximum number of events to retain
    size_t   policy_max_size_bytes = Changefeed::RetentionPolicy::DEFAULT_MAX_SIZE_BYTES;  ///< Maximum log size in bytes
    uint32_t policy_cleanup_interval_minutes = 60;  ///< Interval between background cleanup runs
    bool     cleanup_thread_running = false;        ///< Whether the background cleanup thread is currently active

    nlohmann::json toJson() const {
        return {
            {"total_events",          total_events},
            {"total_size_bytes",      total_size_bytes},
            {"oldest_event_age_ms",   oldest_event_age_ms},
            {"oldest_timestamp_ms",   oldest_timestamp_ms},
            {"newest_timestamp_ms",   newest_timestamp_ms},
            {"next_cleanup_time_ms",  next_cleanup_time_ms},
            {"compact_on_cleanup",    compact_on_cleanup},
            {"cleanup_thread_running", cleanup_thread_running},
            {"policy", {
                {"enabled",                   policy_enabled},
                {"max_age_hours",             policy_max_age_hours},
                {"max_event_count",           policy_max_event_count},
                {"max_size_bytes",            policy_max_size_bytes},
                {"cleanup_interval_minutes",  policy_cleanup_interval_minutes}
            }}
        };
    }
};

/**
 * Result of a compaction operation
 */
using CompactionResult = Changefeed::CompactionResult;

/**
 * Result of a GDPR change-log redaction pass
 */
struct GDPRRedactionResult {
    size_t   events_scanned = 0;       ///< Total events examined
    size_t   events_redacted = 0;      ///< Events whose value was scrubbed
    uint64_t elapsed_time_ms = 0;      ///< Wall-clock time of the operation
    std::string key_prefix;            ///< Key prefix that was matched
    std::string tenant_id;             ///< Tenant context (for multi-tenant deployments)
    std::string operator_id;           ///< Identity of the requesting operator
    int64_t  timestamp_ms = 0;         ///< Epoch ms when redaction completed

    nlohmann::json toJson() const {
        return {
            {"events_scanned",  events_scanned},
            {"events_redacted", events_redacted},
            {"elapsed_time_ms", elapsed_time_ms},
            {"key_prefix",      key_prefix},
            {"tenant_id",       tenant_id},
            {"operator_id",     operator_id},
            {"timestamp_ms",    timestamp_ms}
        };
    }
};

/**
 * CDC Admin API for operational tasks
 * 
 * Provides administrative operations for CDC:
 * - Purge: Delete events by range, timestamp, or tenant
 * - Replay: Get events from a specific sequence
 * - Compact: Remove superseded entries to reclaim log space
 * - Health Check: Check system health status
 * - Diagnostics: Export complete diagnostics info
 */
class CDCAdmin {
public:
    /**
     * Create admin interface for a changefeed
     */
    explicit CDCAdmin(Changefeed* changefeed);
    
    /**
     * Create admin interface for tenant buffer manager
     */
    explicit CDCAdmin(TenantBufferManager* tenant_manager);
    
    ~CDCAdmin() = default;

    /**
     * @brief Wire a RocksDB storage backend for GDPR redaction audit logging.
     *
     * When set, every call to redactByKeyPrefix() writes a structured audit
     * record to the @c cdc_redactions column family of @p storage.  The audit
     * record contains:
     *   @code{"key_prefix":..., "redacted_count":..., "timestamp_ms":..., "operator":...}@endcode
     *
     * @param storage  RocksDBWrapper instance (not owned; must outlive CDCAdmin).
     *                 Pass @c nullptr to disable audit logging.
     */
    void setAuditStorage(RocksDBWrapper* storage) noexcept { audit_storage_ = storage; }

    /**
     * @brief Wire a CDC transport for Kafka tombstone propagation.
     *
     * When set, every call to redactByKeyPrefix() publishes a tombstone
     * (EVENT_DELETE ChangeEvent with null value) for each affected key via
     * @p transport.  This propagates GDPR erasure to downstream Kafka consumers.
     *
     * @param transport  CDC transport (not owned; must outlive CDCAdmin).
     *                   Pass @c nullptr to disable tombstone propagation.
     */
    void setTransport(ICDCTransport* transport) noexcept { transport_ = transport; }
    
    // Purge operations
    
    /**
     * Purge all events from the changefeed
     * WARNING: This deletes all data!
     */
    PurgeResult purgeAll();
    
    /**
     * Purge events in a sequence range (inclusive)
     * @param start_sequence First sequence to delete (inclusive)
     * @param end_sequence Last sequence to delete (inclusive)
     */
    PurgeResult purgeBySequenceRange(uint64_t start_sequence, uint64_t end_sequence);
    
    /**
     * Purge events older than a timestamp
     * @param before_timestamp_ms Delete events with timestamp < this value
     */
    PurgeResult purgeByTimestamp(uint64_t before_timestamp_ms);

    /**
     * Purge events older than a timestamp (alias for purgeByTimestamp)
     * @param before_timestamp_ms Delete events with timestamp < this value
     */
    PurgeResult purgeOlderThan(int64_t before_timestamp_ms);
    
    /**
     * Purge all events for a specific tenant
     * Only works with TenantBufferManager
     */
    PurgeResult purgeTenant(const std::string& tenant_id);
    
    // Replay operations
    
    /**
     * Replay events starting from a specific sequence
     * @param from_sequence Start from this sequence (inclusive)
     * @param limit Maximum number of events to return (0 = no limit)
     * @param event_types Optional set of event types to filter (empty = all types)
     * @return Vector of change events
     */
    std::vector<Changefeed::ChangeEvent> replayFromSequence(
        uint64_t from_sequence,
        uint64_t limit = 0,
        const std::set<Changefeed::ChangeEventType>& event_types = {});

    // Compaction

    /**
     * Compact the change log by removing superseded entries per key.
     *
     * Keeps only the latest event per document key, removing older events that
     * have been superseded by a newer one.  DELETE tombstones are always kept.
     *
     * @return CompactionResult with counts of scanned/deleted/retained events
     */
    CompactionResult compactLog();

    // GDPR / data-subject operations

    /**
     * @brief Redact PII from all change log entries matching a key prefix.
     *
     * Implements the GDPR "right to erasure" for the change log: all stored
     * events whose @p key starts with @p key_prefix have their @p value,
     * @p before_snapshot, and @p after_snapshot fields replaced with
     * @c "[REDACTED]" and @c redacted = true.  Audit-critical fields
     * (@p sequence, @p type, @p key, @p timestamp_ms) are preserved.
     *
     * The operation and its outcome are recorded at INFO level in the
     * structured application log (tenant, key_prefix, counts, operator).
     *
     * @param tenant_id    Tenant scope (informational; used in the audit log).
     * @param key_prefix   Non-empty key prefix identifying the data subject.
     * @param operator_id  Identity of the requesting operator (audit record).
     * @return GDPRRedactionResult with scan, redaction counts and timing.
     * @throws CDCException if @p key_prefix is empty or no changefeed is set.
     */
    GDPRRedactionResult redactByKeyPrefix(const std::string& tenant_id,
                                          const std::string& key_prefix,
                                          const std::string& operator_id = "");

    // Retention status

    /**
     * Get current retention/compaction status information
     * @return RetentionStatus describing current log state
     */
    RetentionStatus getRetentionStatus();
    
    // Health & diagnostics
    
    /**
     * Perform health check on CDC components
     * @return Health status with component details
     */
    HealthStatus healthCheck();
    
    /**
     * Get comprehensive diagnostics information
     * @return Complete diagnostics including metrics, watermarks, health
     */
    DiagnosticsInfo getDiagnostics();
    
private:
    Changefeed* changefeed_;
    TenantBufferManager* tenant_manager_;
    std::chrono::system_clock::time_point creation_time_;

    /// Optional RocksDB backend for writing GDPR redaction audit log entries.
    RocksDBWrapper* audit_storage_ = nullptr;
    /// Optional CDC transport for publishing Kafka tombstones after redaction.
    ICDCTransport* transport_ = nullptr;
    
    // Helper methods
    uint64_t countEventsInRange(uint64_t start, uint64_t end);
    void validateSequenceRange(uint64_t start, uint64_t end);
};

}  // namespace cdc
}  // namespace themis
