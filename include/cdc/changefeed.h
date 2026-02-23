/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            changefeed.h                                       ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     275                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a629043ab  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
    • 94f31dca3  2026-02-22  Cleanup: fix uninitialized Watermarks, unused variable, a... ║
    • d05084392  2026-02-22  Continue CDC compaction: GET/PUT retention endpoints, com... ║
    • 40dea3aaf  2026-02-22  Implement CDC log compaction, fix cdc_admin method discre... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#ifndef THEMIS_CHANGEFEED_H
#define THEMIS_CHANGEFEED_H

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <cstdint>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <set>
#include <nlohmann/json.hpp>

// Forward declarations for RocksDB types
namespace rocksdb {
    class TransactionDB;
    class ColumnFamilyHandle;
}

namespace themis {

/**
 * @brief Minimal Change Data Capture (CDC) implementation
 * 
 * Features:
 * - Sequence-based event tracking
 * - Long-polling support for real-time updates
 * - Event filtering by type/key prefix
 * 
 * Implementation: Simple key-value storage of change events with sequence numbers
 * Key format: "changefeed:{sequence_number}"
 * 
 * Note: This is a minimal MVP implementation. Production-grade CDC would use:
 * - RocksDB WAL tailing for lower latency
 * - Persistent subscriptions with checkpointing
 * - Materialized views with automatic updates
 * - Stream-table duality patterns
 */
class Changefeed {
public:
    enum class ChangeEventType {
        EVENT_PUT,
        EVENT_DELETE,
        EVENT_TRANSACTION_COMMIT,
        EVENT_TRANSACTION_ROLLBACK
    };

    struct ChangeEvent {
        uint64_t sequence;                // Monotonic sequence number
        ChangeEventType type;             // Event type
        std::string key;                  // Affected key
        std::optional<std::string> value; // Value (nullopt for DELETE)
        int64_t timestamp_ms;             // Event timestamp
        nlohmann::json metadata;          // Additional metadata (tx_id, user, etc.)

        // Serialization
        nlohmann::json toJson() const;
        static ChangeEvent fromJson(const nlohmann::json& j);
    };

    struct ListOptions {
        uint64_t from_sequence = 0;       // Start after this sequence
        size_t limit = 100;               // Max events to return
        uint32_t long_poll_ms = 0;        // Long-poll timeout (0 = immediate)
        std::optional<std::string> key_prefix; // Filter by key prefix
        std::optional<ChangeEventType> event_type;   // Filter by single event type (legacy; use event_types for multi-type)
        std::set<ChangeEventType> event_types; // Filter by one or more operation types (INSERT/UPDATE=PUT, DELETE); empty = no filter
    };
    
    struct RetentionPolicy {
        static constexpr size_t DEFAULT_MAX_SIZE_BYTES = 100ULL * 1024 * 1024 * 1024;  // 100GB
        
        bool enabled = false;                           // Enable automatic retention cleanup
        std::chrono::hours max_age_hours{168};          // Max age (default: 7 days)
        uint64_t max_event_count = 1000000;             // Max events (default: 1M)
        size_t max_size_bytes = DEFAULT_MAX_SIZE_BYTES; // Max size (default: 100GB)
        std::chrono::minutes cleanup_interval{60};      // Cleanup interval (default: 1 hour)
        bool compact_on_cleanup = false;                // Run key-based compaction after each cleanup cycle
    };
    
    struct Watermarks {
        uint64_t low_watermark = 0;          // Oldest event sequence
        uint64_t high_watermark = 0;         // Newest event sequence
        int64_t oldest_timestamp_ms = 0;     // Timestamp of oldest event
        int64_t newest_timestamp_ms = 0;     // Timestamp of newest event
    };

    struct Stats {
        uint64_t total_events;
        uint64_t latest_sequence;
        size_t total_size_bytes;
        Watermarks watermarks;  // Watermark information
    };

    /**
     * @brief Construct Changefeed
     * @param db RocksDB TransactionDB instance (not owned)
     * @param cf Optional column family handle (nullptr = default CF)
     * @param retention Retention policy (optional)
     */
    explicit Changefeed(rocksdb::TransactionDB* db, 
                        rocksdb::ColumnFamilyHandle* cf = nullptr,
                        RetentionPolicy retention = RetentionPolicy{});

    ~Changefeed();

    /**
     * @brief Record a change event
     * @param event Event to record (sequence will be auto-generated)
     * @return Event with assigned sequence number
     */
    ChangeEvent recordEvent(ChangeEvent event);

    /**
     * @brief List change events with optional filters
     * @param options List options (pagination, filters, long-poll)
     * @return Vector of change events
     */
    std::vector<ChangeEvent> listEvents() const;
    std::vector<ChangeEvent> listEvents(const ListOptions& options) const;

    /**
     * @brief Get the latest sequence number
     * @return Latest sequence, or 0 if no events
     */
    uint64_t getLatestSequence() const;

    /**
     * @brief Get changefeed statistics
     * @return Stats struct
     */
    Stats getStats() const;
    
    /**
     * @brief Get watermark information
     * @return Watermarks struct
     */
    Watermarks getWatermarks() const;

    /**
     * @brief Clear all events (admin operation)
     */
    void clear();

    /**
     * @brief Delete events older than a given sequence (retention policy)
     * @param before_sequence Delete events with sequence < this value
     * @return Number of events deleted
     */
    size_t deleteOldEvents(uint64_t before_sequence);

    /**
     * @brief Alias for deleteOldEvents (sequence-based)
     * @param before_sequence Delete events with sequence < this value
     * @return Number of events deleted
     */
    size_t deleteOldEventsBySequence(uint64_t before_sequence) {
        return deleteOldEvents(before_sequence);
    }
    
    /**
     * @brief Delete events older than given timestamp
     * @param before_timestamp_ms Delete events with timestamp < this value
     * @return Number of events deleted
     */
    size_t deleteOldEventsByTimestamp(int64_t before_timestamp_ms);

    /**
     * @brief Get a single event by sequence number
     * @param sequence The sequence number to look up
     * @return The change event (throws on not found or error)
     */
    ChangeEvent getEvent(uint64_t sequence) const;

    /**
     * @brief Result of a compaction operation
     */
    struct CompactionResult {
        size_t events_scanned = 0;   ///< Total events examined
        size_t events_deleted = 0;   ///< Superseded events removed
        size_t keys_compacted = 0;   ///< Distinct keys that had older entries removed
        size_t events_retained = 0;  ///< Events kept (latest per key + tombstones)
    };

    /**
     * @brief Compact the change log by removing superseded entries per key
     *
     * For each document key, retains only the latest change event and removes
     * all earlier events that have been superseded by a newer one.  A DELETE
     * event is never discarded so that consumers can still observe tombstones.
     *
     * @return CompactionResult describing what was removed
     */
    CompactionResult compactByKey();
    
    /**
     * @brief Apply retention policy (delete old events based on configured policy)
     * @return Number of events deleted
     */
    size_t applyRetentionPolicy();
    
    /**
     * @brief Update the retention policy at runtime
     * @param policy New retention policy to apply
     */
    void updateRetentionPolicy(const RetentionPolicy& policy);

    /**
     * @brief Get the current retention policy
     * @return Current retention policy
     */
    RetentionPolicy getRetentionPolicy() const;
    
    /**
     * @brief Start background retention cleanup thread
     */
    void startRetentionCleanup();
    
    /**
     * @brief Stop background retention cleanup thread
     */
    void stopRetentionCleanup();

private:
    rocksdb::TransactionDB* db_;
    rocksdb::ColumnFamilyHandle* cf_;
    RetentionPolicy retention_policy_;

    static constexpr const char* KEY_PREFIX = "changefeed:";
    static constexpr const char* SEQUENCE_KEY = "changefeed_sequence";

    std::string makeKey(uint64_t sequence) const;
    uint64_t nextSequence();
    
    // Helper to wait for new events (for long-poll)
    bool waitForEvents(uint64_t from_sequence, uint32_t timeout_ms) const;
    
    // Mutex to protect sequence generation (prevents race conditions in read-modify-write)
    mutable std::mutex sequence_mutex_;
    
    // Retention cleanup thread
    std::atomic<bool> retention_thread_running_{false};
    std::thread retention_thread_;
    std::condition_variable retention_cv_;
    mutable std::mutex retention_mutex_;  // also protects retention_policy_ reads
    
    void retentionCleanupThread();
};

} // namespace themis

#endif // THEMIS_CHANGEFEED_H
