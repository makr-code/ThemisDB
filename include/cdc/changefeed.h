/**
 * @file changefeed.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
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
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
        /**
         * @brief From Json.
         * @param[in] j Input parameter.
         * @return Return value.
         */
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

        /**
         * @brief Defaults.
         * @return Return value.
         * @details Implements defaults without additional internal calls.
         */
        static RetentionPolicy defaults() { return {}; }
    };
    
    struct Watermarks {
        uint64_t low_watermark = 0;          // Oldest event sequence
        uint64_t high_watermark = 0;         // Newest event sequence
        int64_t oldest_timestamp_ms = 0;     // Timestamp of oldest event
        int64_t newest_timestamp_ms = 0;     // Timestamp of newest event
    };

    struct Stats {
        uint64_t total_events = 0;
        uint64_t latest_sequence;
        size_t total_size_bytes;
        Watermarks watermarks;  // Watermark information
    };

    /**
     * @brief Make Sequence Merge Operator.
     * @return Return value.
     */
    static std::shared_ptr<rocksdb::MergeOperator> makeSequenceMergeOperator();

    explicit Changefeed(rocksdb::TransactionDB* db, 
                        rocksdb::ColumnFamilyHandle* cf = nullptr,
                        RetentionPolicy retention = RetentionPolicy::defaults());

    ~Changefeed() noexcept;

    /**
     * @brief Record Event.
     * @param[in] event Input parameter.
     * @return Return value.
     */
    ChangeEvent recordEvent(ChangeEvent event);

    /**
     * @brief List Events.
     * @return Return value.
     */
    std::vector<ChangeEvent> listEvents() const;
    /**
     * @brief List Events.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    std::vector<ChangeEvent> listEvents(const ListOptions& options) const;

    /**
     * @brief Get Latest Sequence.
     * @return Return value.
     */
    uint64_t getLatestSequence() const;

    /**
     * @brief Get Stats.
     * @return Return value.
     */
    Stats getStats() const;
    
    /**
     * @brief Get Watermarks.
     * @return Return value.
     */
    Watermarks getWatermarks() const;

    /**
     * @brief Clear.
     */
    void clear();

    /**
     * @brief Delete Old Events.
     * @param[in] before_sequence Input parameter.
     * @return Return value.
     */
    size_t deleteOldEvents(uint64_t before_sequence);

    /**
     * @brief Delete Old Events By Sequence.
     * @param[in] before_sequence Input parameter.
     * @return Return value.
     * @details Calls: deleteOldEvents().
     */
    size_t deleteOldEventsBySequence(uint64_t before_sequence) {
        return deleteOldEvents(before_sequence);
    }
    
    /**
     * @brief Delete Old Events By Timestamp.
     * @param[in] before_timestamp_ms Input parameter.
     * @return Return value.
     */
    size_t deleteOldEventsByTimestamp(int64_t before_timestamp_ms);

    /**
     * @brief Get Event.
     * @param[in] sequence Input parameter.
     * @return Return value.
     */
    ChangeEvent getEvent(uint64_t sequence) const;

    struct CompactionResult {
        size_t events_scanned = 0;   ///< Total events examined
        size_t events_deleted = 0;   ///< Superseded events removed
        size_t keys_compacted = 0;   ///< Distinct keys that had older entries removed
        size_t events_retained = 0;  ///< Events kept (latest per key + tombstones)
    };

    struct RedactionResult {
        size_t events_scanned = 0;   ///< Total events examined
        size_t events_redacted = 0;  ///< Events whose value field was scrubbed
        std::vector<std::string> affected_keys;
    };

    /**
     * @brief Compact By Key.
     * @return Return value.
     */
    CompactionResult compactByKey();

    /**
     * @brief Redact By Key Prefix.
     * @param[in] key_prefix Input parameter.
     * @return Return value.
     */
    RedactionResult redactByKeyPrefix(const std::string& key_prefix);

    /**
     * @brief Apply Retention Policy.
     * @return Return value.
     */
    size_t applyRetentionPolicy();
    
    /**
     * @brief Update Retention Policy.
     * @param[in] policy Input parameter.
     */
    void updateRetentionPolicy(const RetentionPolicy& policy);

    /**
     * @brief Get Retention Policy.
     * @return Return value.
     */
    RetentionPolicy getRetentionPolicy() const;
    
    /**
     * @brief Start Retention Cleanup.
     */
    void startRetentionCleanup();
    
    /**
     * @brief Stop Retention Cleanup.
     */
    void stopRetentionCleanup();

    /**
     * @brief Is Retention Cleanup Running.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool isRetentionCleanupRunning() const noexcept;

    // -----------------------------------------------------------------------
    // Push-based subscription API
    // -----------------------------------------------------------------------

    struct SubscriptionFilter {
        std::string key_prefix;
        std::set<ChangeEventType> event_types;

        /**
         * @brief Matches.
         * @param[in] ev Input parameter.
         * @return True when the operation succeeds.
         * @note Exception safety: noexcept.
         */
        bool matches(const ChangeEvent& ev) const noexcept;
    };

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

        /**
         * @brief Cancel.
         * @note Exception safety: noexcept.
         */
        void cancel() noexcept;

        bool active() const noexcept { return feed_ != nullptr; }

        uint64_t id() const noexcept { return id_; }

    private:
        friend class Changefeed;
        SubscriptionHandle(Changefeed* feed, uint64_t id) noexcept
            : feed_(feed), id_(id) {}

        Changefeed* feed_ = nullptr;
        uint64_t    id_   = 0;
    };

    using SubscriptionCallback = std::function<void(const ChangeEvent&)>;

    /**
     * @brief Subscribe.
     * @param[in] filter Input parameter.
     * @param[in] callback Input parameter.
     * @return Return value.
     */
    SubscriptionHandle subscribe(SubscriptionFilter filter,
                                 SubscriptionCallback callback);

    /**
     * @brief Unsubscribe.
     * @param[in] subscription_id Identifier of the subscription.
     * @note Exception safety: noexcept.
     */
    void unsubscribe(uint64_t subscription_id) noexcept;

private:
    rocksdb::TransactionDB* db_;
    rocksdb::ColumnFamilyHandle* cf_;
    RetentionPolicy retention_policy_;

    static constexpr const char* KEY_PREFIX = "changefeed:";
    static constexpr const char* SEQUENCE_KEY = "changefeed_sequence";

    /**
     * @brief Make Key.
     * @param[in] sequence Input parameter.
     * @return Return value.
     */
    std::string makeKey(uint64_t sequence) const;
    /**
     * @brief Next Sequence.
     * @return Return value.
     */
    uint64_t nextSequence();

    /**
     * @brief Load the initial sequence counter value from RocksDB at construction.
     * @return Return value.
     * @details Handles both the binary little-endian uint64 format (new) and the legacy decimal-string format (old). Falls back to scanning events when the DB key cannot be read (e.g. unresolved Merge operands without a registered merge operator).
     */
    uint64_t loadInitialSequence() const;

    /**
     * @brief Scan all stored changefeed events and return the maximum sequence number.
     * @return Return value.
     * @details Used as a crash-recovery fallback when loadInitialSequence() cannot read SEQUENCE_KEY directly.
     */
    uint64_t scanMaxSequence() const;
    
    /**
     * @brief Helper to wait for new events (for long-poll)
     * @param[in] from_sequence Input parameter.
     * @param[in] timeout_ms Input parameter.
     * @return True when the operation succeeds.
     */
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
    
    /**
     * @brief Retention Cleanup Thread.
     */
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

    /**
     * @brief Notify Subscribers.
     * @param[in] event Input parameter.
     */
    void notifySubscribers(const ChangeEvent& event);
};

} // namespace themis
