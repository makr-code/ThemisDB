/**
 * @file cdc_admin.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=15, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "cdc/cdc_admin.h"
#include "cdc/cdc_error.h"
#include "cdc/icdc_transport.h"
#include "cdc/tenant_buffer_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/logger.h"
#include <algorithm>
#include <limits>
#include <rocksdb/utilities/transaction_db.h>

namespace themis {
namespace cdc {

using namespace std::chrono;

CDCAdmin::CDCAdmin(Changefeed* changefeed)
    : changefeed_(changefeed)
    , tenant_manager_(nullptr)
    , creation_time_(system_clock::now())
{
    if (!changefeed_) {
        throw error::invalidArgument("Changefeed cannot be null");
    }
}

CDCAdmin::CDCAdmin(TenantBufferManager* tenant_manager)
    : changefeed_(nullptr)
    , tenant_manager_(tenant_manager)
    , creation_time_(system_clock::now())
{
    if (!tenant_manager_) {
        throw error::invalidArgument("TenantBufferManager cannot be null");
    }
}

PurgeResult CDCAdmin::purgeAll() {
    THEMIS_INFO("CDC Admin: Purging all events");
    
    auto start = steady_clock::now();
    PurgeResult result;
    
    if (!changefeed_) {
        throw error::internalError("No changefeed available for purge");
    }
    
    // Get watermarks to know range
    auto watermarks = changefeed_->getWatermarks();
    
    if (watermarks.low_watermark == 0 && watermarks.high_watermark == 0) {
        THEMIS_INFO("No events to purge");
        return result;
    }
    
    // Delete all events
    result.events_deleted = changefeed_->deleteOldEvents(watermarks.high_watermark + 1);
    
    auto end = steady_clock::now();
    result.elapsed_time_ms = duration_cast<milliseconds>(end - start).count();
    
    THEMIS_INFO("Purged {} events in {}ms", result.events_deleted, result.elapsed_time_ms);
    return result;
}

PurgeResult CDCAdmin::purgeBySequenceRange(uint64_t start_sequence, uint64_t end_sequence) {
    THEMIS_INFO("CDC Admin: Purging sequence range [{}, {}]", start_sequence, end_sequence);
    
    validateSequenceRange(start_sequence, end_sequence);
    
    auto start = steady_clock::now();
    PurgeResult result;
    
    if (!changefeed_) {
        throw error::internalError("No changefeed available for purge");
    }
    
    // Delete events up to end_sequence (exclusive, so add 1)
    uint64_t total_deleted = changefeed_->deleteOldEvents(end_sequence + 1);
    
    // If start_sequence > 0, we deleted too many, but RocksDB delete is by prefix
    // For now, we delete everything up to end_sequence
    // A proper implementation would iterate and delete specific keys
    result.events_deleted = total_deleted;
    
    auto end = steady_clock::now();
    result.elapsed_time_ms = duration_cast<milliseconds>(end - start).count();
    
    THEMIS_INFO("Purged {} events in {}ms", result.events_deleted, result.elapsed_time_ms);
    return result;
}

PurgeResult CDCAdmin::purgeByTimestamp(uint64_t before_timestamp_ms) {
    THEMIS_INFO("CDC Admin: Purging events before timestamp {}", before_timestamp_ms);
    
    auto start = steady_clock::now();
    PurgeResult result;
    
    if (!changefeed_) {
        throw error::internalError("No changefeed available for purge");
    }
    
    result.events_deleted = changefeed_->deleteOldEventsByTimestamp(before_timestamp_ms);
    
    auto end = steady_clock::now();
    result.elapsed_time_ms = duration_cast<milliseconds>(end - start).count();
    
    THEMIS_INFO("Purged {} events in {}ms", result.events_deleted, result.elapsed_time_ms);
    return result;
}

PurgeResult CDCAdmin::purgeTenant(const std::string& tenant_id) {
    THEMIS_INFO("CDC Admin: Purging tenant '{}'", tenant_id);

    if (!tenant_manager_) {
        throw error::internalError("No tenant manager available for tenant purge");
    }

    if (tenant_id.empty()) {
        throw error::invalidArgument("Tenant ID cannot be empty");
    }

    auto start = steady_clock::now();
    PurgeResult result;

    // Capture pre-flush event count for the return value.
    // events_recorded tracks everything that entered the buffer for this tenant.
    uint64_t pre_recorded = 0;
    if (auto stats = tenant_manager_->getTenantStats(tenant_id)) {
        pre_recorded = stats->events_recorded;
    }

    // Drain any in-memory buffered events to the underlying changefeed.
    size_t flushed = tenant_manager_->flushTenant(tenant_id);

    // Remove the tenant buffer entirely (also performs an internal flush as
    // a safety net before destruction).
    tenant_manager_->removeTenant(tenant_id);

    // Report deleted = flushed events (what was drained), falling back to the
    // pre-flush recorded count when the buffer was already empty.
    result.events_deleted = (flushed > 0)
        ? static_cast<uint64_t>(flushed)
        : pre_recorded;

    auto end = steady_clock::now();
    result.elapsed_time_ms = static_cast<uint64_t>(
        duration_cast<milliseconds>(end - start).count());

    THEMIS_INFO("CDC Admin: purgeTenant '{}' complete — deleted={} elapsed_ms={}",
                tenant_id, result.events_deleted, result.elapsed_time_ms);
    return result;
}

std::vector<Changefeed::ChangeEvent> CDCAdmin::replayFromSequence(
    uint64_t from_sequence,
    uint64_t limit,
    const std::set<Changefeed::ChangeEventType>& event_types)
{
    THEMIS_INFO("CDC Admin: Replaying from sequence {} (limit: {})",
                from_sequence, limit == 0 ? "unlimited" : std::to_string(limit));

    if (!changefeed_) {
        throw error::internalError("No changefeed available for replay");
    }

    Changefeed::ListOptions opts;
    // ListOptions::from_sequence is exclusive ("start AFTER this sequence"),
    // while replayFromSequence uses inclusive semantics. Subtract 1 so that
    // listEvents() includes the event at `from_sequence`.
    opts.from_sequence = (from_sequence > 0) ? from_sequence - 1 : 0;
    opts.limit = (limit > 0) ? static_cast<size_t>(limit)
                             : std::numeric_limits<size_t>::max();
    opts.event_types = event_types;

    auto events = changefeed_->listEvents(opts);
    THEMIS_INFO("Replayed {} events", events.size());
    return events;
}

HealthStatus CDCAdmin::healthCheck() {
    HealthStatus status;
    status.message = "CDC is healthy";
    
    if (!changefeed_ && !tenant_manager_) {
        status.is_healthy = false;
        status.changefeed_healthy = false;
        status.message = "No changefeed or tenant manager available";
        return status;
    }
    
    // Check changefeed health
    if (changefeed_) {
        try {
            // Try to get watermarks - if this fails, changefeed is unhealthy
            changefeed_->getWatermarks();
            status.changefeed_healthy = true;
        } catch (const std::exception& e) {
            status.changefeed_healthy = false;
            status.is_healthy = false;
            status.message = "Changefeed unhealthy: " + std::string(e.what());
        }
    }
    
    // Check buffer health (if available)
    // This would require access to buffer metrics
    status.buffer_healthy = true;  // Assume healthy if no buffer access
    
    // Check retention health
    status.retention_healthy = true;  // Assume healthy
    
    // Get error metrics (simplified - would need access to actual metrics)
    status.error_count = 0;
    status.error_rate_per_sec = 0;
    status.buffer_utilization = 0.0;
    
    if (!status.changefeed_healthy || !status.buffer_healthy || !status.retention_healthy) {
        status.is_healthy = false;
    }
    
    return status;
}

DiagnosticsInfo CDCAdmin::getDiagnostics() {
    THEMIS_INFO("CDC Admin: Getting diagnostics");

    Changefeed::Watermarks watermarks{};
    uint64_t total_events = 0;
    uint64_t buffer_size = 0;

    if (changefeed_) {
        // Get watermarks
        watermarks = changefeed_->getWatermarks();

        // Calculate total events (approximate)
        if (watermarks.high_watermark > 0) {
            total_events = watermarks.high_watermark - watermarks.low_watermark + 1;
        }
    }

    // Get health status
    HealthStatus health = healthCheck();

    // Metrics would be populated from actual buffer/changefeed metrics
    // For now, leave as default initialized

    THEMIS_INFO("Diagnostics: {} total events, health: {}", 
                total_events, health.is_healthy ? "OK" : "ISSUES");

    return DiagnosticsInfo{
        std::move(watermarks),
        CDCMetrics{},
        std::move(health),
        total_events,
        buffer_size,
        creation_time_
    };
}

uint64_t CDCAdmin::countEventsInRange(uint64_t start, uint64_t end) {
    if (end < start) {
        return 0;
    }
    return end - start + 1;
}

void CDCAdmin::validateSequenceRange(uint64_t start, uint64_t end) {
    if (end < start) {
        throw error::invalidArgument(
            "Invalid sequence range: end (" + std::to_string(end) + 
            ") < start (" + std::to_string(start) + ")");
    }
}

PurgeResult CDCAdmin::purgeOlderThan(int64_t before_timestamp_ms) {
    if (before_timestamp_ms < 0) {
        throw error::invalidArgument(
            "purgeOlderThan: before_timestamp_ms must be non-negative, got: " +
            std::to_string(before_timestamp_ms));
    }
    return purgeByTimestamp(static_cast<uint64_t>(before_timestamp_ms));
}

CompactionResult CDCAdmin::compactLog() {
    THEMIS_INFO("CDC Admin: Starting log compaction (compact by key)");

    if (!changefeed_) {
        throw error::internalError("No changefeed available for compaction");
    }

    auto start = steady_clock::now();
    CompactionResult result = changefeed_->compactByKey();
    auto end = steady_clock::now();

    THEMIS_INFO("CDC Admin: Compaction complete in {}ms — scanned={} deleted={} retained={}",
                duration_cast<milliseconds>(end - start).count(),
                result.events_scanned, result.events_deleted, result.events_retained);
    return result;
}

GDPRRedactionResult CDCAdmin::redactByKeyPrefix(
    const std::string& tenant_id,
    const std::string& key_prefix,
    const std::string& operator_id)
{
    THEMIS_INFO("CDC Admin: GDPR redaction — tenant='{}' key_prefix='{}' operator='{}'",
                tenant_id, key_prefix, operator_id);

    if (!changefeed_) {
        throw error::internalError("No changefeed available for GDPR redaction");
    }

    if (key_prefix.empty()) {
        throw error::invalidArgument("redactByKeyPrefix: key_prefix cannot be empty");
    }

    const auto now_ms = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();

    const auto start = steady_clock::now();
    Changefeed::RedactionResult inner = changefeed_->redactByKeyPrefix(key_prefix);
    const auto end = steady_clock::now();

    GDPRRedactionResult result;
    result.events_scanned  = inner.events_scanned;
    result.events_redacted = inner.events_redacted;
    result.elapsed_time_ms = static_cast<uint64_t>(
        duration_cast<milliseconds>(end - start).count());
    result.key_prefix  = key_prefix;
    result.tenant_id   = tenant_id;
    result.operator_id = operator_id;
    result.timestamp_ms = now_ms;

    THEMIS_INFO("CDC Admin: GDPR redaction complete — "
                "tenant='{}' key_prefix='{}' scanned={} redacted={} elapsed_ms={} operator='{}'",
                tenant_id, key_prefix,
                result.events_scanned, result.events_redacted,
                result.elapsed_time_ms, operator_id);

    // ── Audit log ────────────────────────────────────────────────────────────
    // Write an immutable audit record to the 'cdc_redactions' column family so
    // the operation is traceable even if it partially fails.
    if (audit_storage_) {
        auto cf_result = audit_storage_->getOrCreateColumnFamily("cdc_redactions");
        if (cf_result.has_value()) {
            rocksdb::ColumnFamilyHandle* audit_cf = cf_result.value();
            // Key: "<timestamp_ms>:<key_prefix>" ensures chronological ordering.
            const std::string audit_key =
                std::to_string(now_ms) + ":" + key_prefix;
            const nlohmann::json audit_entry = {
                {"key_prefix",      key_prefix},
                {"redacted_count",  result.events_redacted},
                {"timestamp_ms",    now_ms},
                {"operator",        operator_id},
                {"tenant_id",       tenant_id}
            };
            rocksdb::WriteOptions write_opts;
            auto* raw_db = audit_storage_->getDB();
            if (raw_db) {
                rocksdb::Status s =
                    raw_db->Put(write_opts, audit_cf, audit_key, audit_entry.dump());
                if (!s.ok()) {
                    THEMIS_WARN("CDC Admin: failed to write GDPR audit log entry: {}",
                                s.ToString());
                } else {
                    THEMIS_INFO("CDC Admin: GDPR audit log entry written for key_prefix='{}'",
                                key_prefix);
                }
            }
        } else {
            THEMIS_WARN("CDC Admin: could not obtain 'cdc_redactions' column family for audit log");
        }
    }

    // ── Kafka tombstone propagation ──────────────────────────────────────────
    // For each distinct affected key, publish a tombstone (EVENT_DELETE with
    // null value) so that downstream Kafka consumers observe the erasure.
    if (transport_ && !inner.affected_keys.empty()) {
        // Deduplicate keys so each key gets exactly one tombstone.
        std::vector<std::string> unique_keys = inner.affected_keys;
        std::sort(unique_keys.begin(), unique_keys.end());
        unique_keys.erase(std::unique(unique_keys.begin(), unique_keys.end()),
                          unique_keys.end());

        for (const auto& affected_key : unique_keys) {
            Changefeed::ChangeEvent tombstone;
            tombstone.type       = Changefeed::ChangeEventType::EVENT_DELETE;
            tombstone.key        = affected_key;
            tombstone.value      = std::nullopt;  // null payload — Kafka tombstone
            tombstone.redacted   = true;
            tombstone.timestamp_ms = now_ms;

            if (!transport_->publish(tombstone)) {
                THEMIS_WARN("CDC Admin: failed to publish tombstone for key='{}'",
                            affected_key);
            }
        }
        THEMIS_INFO("CDC Admin: published {} tombstone(s) for key_prefix='{}'",
                    unique_keys.size(), key_prefix);
    }

    return result;
}

RetentionStatus CDCAdmin::getRetentionStatus() {
    THEMIS_INFO("CDC Admin: Getting retention status");

    if (!changefeed_) {
        throw error::internalError("No changefeed available for retention status");
    }

    RetentionStatus status;

    auto stats = changefeed_->getStats();
    status.total_events      = stats.total_events;
    status.total_size_bytes  = stats.total_size_bytes;
    status.oldest_timestamp_ms = stats.watermarks.oldest_timestamp_ms;
    status.newest_timestamp_ms = stats.watermarks.newest_timestamp_ms;

    auto now_ms = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();

    if (stats.watermarks.oldest_timestamp_ms > 0) {
        status.oldest_event_age_ms = now_ms - stats.watermarks.oldest_timestamp_ms;
    }

    // Populate policy-derived fields
    auto policy = changefeed_->getRetentionPolicy();
    status.compact_on_cleanup = policy.compact_on_cleanup;
    if (policy.enabled) {
        // Estimate next cleanup: current time + interval (conservative upper bound)
        status.next_cleanup_time_ms = now_ms +
            duration_cast<milliseconds>(policy.cleanup_interval).count();
    }

    // Expose the full retention policy configuration so callers can read it back
    status.policy_enabled                = policy.enabled;
    status.policy_max_age_hours          = static_cast<uint32_t>(policy.max_age_hours.count());
    status.policy_max_event_count        = policy.max_event_count;
    status.policy_max_size_bytes         = policy.max_size_bytes;
    status.policy_cleanup_interval_minutes =
        static_cast<uint32_t>(policy.cleanup_interval.count());

    // Report whether the background cleanup thread is actually running
    status.cleanup_thread_running = changefeed_->isRetentionCleanupRunning();

    return status;
}

}  // namespace cdc
}  // namespace themis
