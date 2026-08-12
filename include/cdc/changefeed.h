/**
 * @file changefeed.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

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
#include <functional>
#include <unordered_map>
#include <nlohmann/json.hpp>

// Forward declarations for RocksDB types
namespace rocksdb {
    class TransactionDB;
    class ColumnFamilyHandle;
    class MergeOperator;
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
        uint64_t sequence = 0;            // Monotonic sequence number (0 = not persisted yet)
        ChangeEventType type = ChangeEventType::EVENT_PUT; // Event type
        std::string key;                  // Affected key
        std::optional<std::string> value; // Value (nullopt for DELETE)
        int64_t timestamp_ms = 0;         // Event timestamp
        nlohmann::json metadata;          // Additional metadata (tx_id, user, etc.)

        // Before/after document snapshots for change event enrichment.
        // before_snapshot: document state prior to this change (nullopt for INSERT).
        // after_snapshot:  document state after this change  (nullopt for DELETE).
        std::optional<std::string> before_snapshot;
        std::optional<std::string> after_snapshot;

        // Set to true when the value has been GDPR-redacted; preserves
        // sequence, type, key, and timestamp_ms for audit-trail integrity.
        bool redacted = false;

        // Serialization
        nlohmann::json toJson() const;
        static ChangeEvent fromJson(const nlohmann::json& j);
    };

    struct ListOptions {
        uint64_t from_sequence = 0;       // Start after this sequence (exclusive)
        uint64_t to_sequence = 0;         // Stop at this sequence (inclusive, 0 = no upper bound)
        size_t limit = 100;               // Max events to return (std::numeric_limits<size_t>::max() = no limit)
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

        static RetentionPolicy defaults() { return {}; }
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
     * @brief Create a RocksDB merge operator for atomic sequence increments.
     *
     * Callers that open the changefeed RocksDB column family should set this
     * operator via @c ColumnFamilyOptions::merge_operator before opening the DB:
     * @code
     *   rocksdb::Options opts;
     *   opts.merge_operator = Changefeed::makeSequenceMergeOperator();
     * @endcode
     * Without it, @c Merge() calls will be buffered but @c Get() on
     * @c SEQUENCE_KEY after a restart will fail.  The in-process atomic counter
     * (`sequence_counter_`) provides correctness within a single process
     * lifetime regardless.
     *
     * @return Shared pointer to a @c SequenceIncrementOperator instance.
     */
    static std::shared_ptr<rocksdb::MergeOperator> makeSequenceMergeOperator();

    /**
     * @brief Construct Changefeed
     * @param db RocksDB TransactionDB instance (not owned)
     * @param cf Optional column family handle (nullptr = default CF)
     * @param retention Retention policy (optional)
     */
    explicit Changefeed(rocksdb::TransactionDB* db, 
                        rocksdb::ColumnFamilyHandle* cf = nullptr,
                        RetentionPolicy retention = RetentionPolicy::defaults());

    /**
     * @brief Destructor - stops the retention cleanup worker.
     */
    ~Changefeed() noexcept;

    /**
     * @brief Record a change event
     * @param event Event to record (sequence will be auto-generated)
     * @return Event with assigned sequence number
     */
    ChangeEvent recordEvent(ChangeEvent event);

    /**
     * @brief List change events with default options.
     * @return Vector of change events.
     */
    std::vector<ChangeEvent> listEvents() const;
    /**
     * @brief List change events with optional filters.
     * @param options List options (pagination, filters, long-poll).
     * @return Vector of change events.
     */
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
     * @brief Result of a GDPR redaction pass
     */
    struct RedactionResult {
        size_t events_scanned = 0;   ///< Total events examined
        size_t events_redacted = 0;  ///< Events whose value field was scrubbed
        /// Unique event keys that were redacted (for Kafka tombstone propagation).
        std::vector<std::string> affected_keys;
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
     * @brief GDPR-aware in-place redaction of change log entries by key prefix
     *
     * Scans all stored change events and, for each event whose @p key field
     * starts with @p key_prefix, replaces the @p value, @p before_snapshot,
     * and @p after_snapshot fields with @c "[REDACTED]" / nullopt and sets
     * @c redacted = true.  The @p sequence, @p type, @p key, and
     * @p timestamp_ms fields are preserved for audit-trail integrity.
     *
     * Already-redacted events are skipped without error.
     *
     * @param key_prefix  Non-empty key prefix identifying the data subject
     *                    (e.g. @c "user:42").
     * @return RedactionResult with scan and redaction counts.
     * @throws CDCException if @p key_prefix is empty.
     */
    RedactionResult redactByKeyPrefix(const std::string& key_prefix);

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

    /**
     * @brief Check whether the background retention cleanup thread is running
     * @return true if the background thread is active
     */
    bool isRetentionCleanupRunning() const noexcept;

    // -----------------------------------------------------------------------
    // Push-based subscription API
    // -----------------------------------------------------------------------

    /**
     * @brief Filter for push-based change subscriptions.
     *
     * All fields are optional; an empty filter matches every event.
     */
    struct SubscriptionFilter {
        /// If non-empty, only events whose key starts with this prefix are delivered.
        std::string key_prefix;
        /// If non-empty, only events matching one of these types are delivered.
        std::set<ChangeEventType> event_types;

        bool matches(const ChangeEvent& ev) const noexcept;
    };

    /**
     * @brief Opaque subscription handle.
     *
     * Cancels the subscription on destruction (RAII).  Copy/assign are deleted;
     * move is supported.
     *
     * Usage:
     * ```cpp
     * auto h = feed.subscribe(filter, [](const ChangeEvent& ev) {
     *     // deliver ev to the client
     * });
     * // Subscription active while h is in scope.
     * ```
     */
    class SubscriptionHandle {
    public:
        SubscriptionHandle() = default;
        ~SubscriptionHandle() noexcept { cancel(); }

        SubscriptionHandle(const SubscriptionHandle&) = delete;
        SubscriptionHandle& operator=(const SubscriptionHandle&) = delete;

        SubscriptionHandle(SubscriptionHandle&& other) noexcept
            : feed_(other.feed_), id_(other.id_)
        {
            other.feed_ = nullptr;
            other.id_   = 0;
        }
        SubscriptionHandle& operator=(SubscriptionHandle&& other) noexcept {
            if (this != &other) {
                cancel();
                feed_ = other.feed_;
                id_   = other.id_;
                other.feed_ = nullptr;
                other.id_   = 0;
            }
            return *this;
        }

        /// Explicitly cancel the subscription before the handle goes out of scope.
        void cancel() noexcept;

        /// Return true if the subscription is still active.
        bool active() const noexcept { return feed_ != nullptr; }

        /// Return the subscription ID (debug / logging).
        uint64_t id() const noexcept { return id_; }

    private:
        friend class Changefeed;
        SubscriptionHandle(Changefeed* feed, uint64_t id) noexcept
            : feed_(feed), id_(id) {}

        Changefeed* feed_ = nullptr;
        uint64_t    id_   = 0;
    };

    /// Callback type invoked for every matching event.  Must be noexcept.
    using SubscriptionCallback = std::function<void(const ChangeEvent&)>;

    /**
     * @brief Register a push callback for CDC events matching @p filter.
     *
     * The @p callback is invoked synchronously during `recordEvent()` on the
     * thread that records the event.  Keep the callback lightweight (e.g. enqueue
     * the event into a per-connection queue and signal a worker thread).
     *
     * The subscription remains active until the returned @c SubscriptionHandle
     * is destroyed or `SubscriptionHandle::cancel()` is called.
     *
     * Thread-safe: may be called concurrently with `recordEvent()` and other
     * `subscribe()` calls.
     *
     * @param filter    Optional event filter (empty = all events).
     * @param callback  Callable invoked with each matching event.
     * @return RAII handle that cancels the subscription on destruction.
     */
    SubscriptionHandle subscribe(SubscriptionFilter filter,
                                 SubscriptionCallback callback);

    /**
     * @brief Unsubscribe by ID (called internally by SubscriptionHandle::cancel()).
     */
    void unsubscribe(uint64_t subscription_id) noexcept;

private:
    rocksdb::TransactionDB* db_;
    rocksdb::ColumnFamilyHandle* cf_;
    RetentionPolicy retention_policy_;

    static constexpr const char* KEY_PREFIX = "changefeed:";
    static constexpr const char* SEQUENCE_KEY = "changefeed_sequence";

    std::string makeKey(uint64_t sequence) const;
    uint64_t nextSequence();

    // Load the initial sequence counter value from RocksDB at construction.
    // Handles both the binary little-endian uint64 format (new) and the legacy
    // decimal-string format (old).  Falls back to scanning events when the DB
    // key cannot be read (e.g. unresolved Merge operands without a registered
    // merge operator).
    uint64_t loadInitialSequence() const;

    // Scan all stored changefeed events and return the maximum sequence number.
    // Used as a crash-recovery fallback when loadInitialSequence() cannot read
    // SEQUENCE_KEY directly.
    uint64_t scanMaxSequence() const;
    
    // Helper to wait for new events (for long-poll)
    bool waitForEvents(uint64_t from_sequence, uint32_t timeout_ms) const;
    
    // In-process atomic sequence counter.  Updated by fetch_add on every
    // nextSequence() call; persisted to RocksDB via Merge() for crash recovery.
    // Eliminates the need for sequence_mutex_ and a Get+Put round-trip per event.
    std::atomic<uint64_t> sequence_counter_{0};

    // Tracks the highest sequence known to be durably persisted. When RocksDB
    // Merge() is unavailable because no merge_operator was configured, we fall
    // back to a monotonic Put() path guarded by this mutex.
    std::atomic<uint64_t> persisted_sequence_{0};
    std::atomic<bool> sequence_merge_supported_{true};
    mutable std::mutex sequence_persist_mutex_;
    
    // Retention cleanup thread
    std::atomic<bool> retention_thread_running_{false};
    std::thread retention_thread_;
    std::condition_variable retention_cv_;
    mutable std::mutex retention_mutex_;  // also protects retention_policy_ reads
    
    void retentionCleanupThread();

    // Push-based subscriptions
    struct SubscriptionEntry {
        SubscriptionFilter   filter;
        SubscriptionCallback callback;
    };
    std::unordered_map<uint64_t, SubscriptionEntry> subscriptions_;
    mutable std::mutex subscriptions_mutex_;
    std::atomic<uint64_t> next_subscription_id_{1};
    std::atomic<size_t> subscription_count_{0};

    /// Notify all registered subscribers whose filter matches @p event.
    void notifySubscribers(const ChangeEvent& event);
};

} // namespace themis
