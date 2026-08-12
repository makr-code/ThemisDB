/**
 * @file consumer_group.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB CDC Consumer Group Manager
 *
 * Implements consumer group semantics for the CDC changefeed:
 * - Durable per-group committed-offset tracking in RocksDB
 * - Key-hash partition assignment so consumers in a group each handle a
 *   disjoint subset of change events
 * - Resume-from-offset: consumers reconnect and continue from the last
 *   acknowledged sequence without scanning the full log
 *
 * RocksDB key layout:
 *   cdc_group:{group_id}:config   -> JSON-serialised ConsumerGroupConfig
 *   cdc_group:{group_id}:offset   -> uint64 committed sequence (decimal string)
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cdc/changefeed.h"
#include "cdc/cdc_error.h"

#include <cstdint>
#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <unordered_map>
#include <nlohmann/json.hpp>

// Forward declarations
namespace rocksdb {
    class TransactionDB;
    class ColumnFamilyHandle;
}

namespace themis {
namespace cdc {

// ============================================================
// Data structures
// ============================================================

/**
 * @brief Configuration for a consumer group.
 *
 * A consumer group divides the change log into @c consumer_count
 * partitions.  Each partition is owned by one consumer; partition
 * membership is determined by
 *   hash(event.key) % consumer_count == consumer_partition_index
 */
struct ConsumerGroupConfig {
    std::string group_id;       ///< Unique group identifier
    uint32_t consumer_count{1}; ///< Number of partitions (consumers) in this group

    nlohmann::json toJson() const {
        return {{"group_id", group_id}, {"consumer_count", consumer_count}};
    }

    static ConsumerGroupConfig fromJson(const nlohmann::json& j) {
        ConsumerGroupConfig cfg;
        cfg.group_id       = j.value("group_id", "");
        cfg.consumer_count = j.value("consumer_count", uint32_t{1});
        return cfg;
    }
};

/**
 * @brief Runtime information for a consumer group.
 */
struct ConsumerGroupInfo {
    ConsumerGroupConfig config;
    uint64_t committed_sequence{0}; ///< Last acked sequence for the group

    nlohmann::json toJson() const {
        return {
            {"config",             config.toJson()},
            {"committed_sequence", committed_sequence}
        };
    }
};

/**
 * @brief In-flight delivery statistics for a single consumer.
 *
 * "In-flight" events are those that have been delivered via
 * fetchEventsAtLeastOnce() but not yet acknowledged with acknowledgeEvents().
 */
struct InFlightStats {
    std::string group_id;
    std::string consumer_id;
    size_t   inflight_count{0};            ///< Total in-flight events
    size_t   overdue_count{0};             ///< In-flight events past ack timeout
    uint64_t oldest_inflight_sequence{0};  ///< Lowest in-flight sequence (0 if none)
};

// ============================================================
// ConsumerGroupManager
// ============================================================

/**
 * @brief Manages consumer groups for the CDC changefeed.
 *
 * Responsibilities:
 *  - Create / delete / enumerate groups (persisted to RocksDB)
 *  - Durable offset commit & retrieval
 *  - Partition assignment: maps a (group, consumer_id) pair to a
 *    partition index using a stable key-hash function
 *  - Fetch events that belong to a specific consumer's partition,
 *    resuming from the group's committed offset
 *
 * Thread-safety: all public methods are thread-safe.
 */
class ConsumerGroupManager {
public:
    /**
     * @brief Construct a ConsumerGroupManager.
     * @param db   RocksDB TransactionDB instance (not owned).
     * @param cf   Optional column family handle (nullptr = default CF).
     */
    explicit ConsumerGroupManager(rocksdb::TransactionDB* db,
                                  rocksdb::ColumnFamilyHandle* cf = nullptr);

    ~ConsumerGroupManager() = default;

    // Non-copyable, non-movable (owns mutex state)
    ConsumerGroupManager(const ConsumerGroupManager&) = delete;
    ConsumerGroupManager& operator=(const ConsumerGroupManager&) = delete;
    ConsumerGroupManager(ConsumerGroupManager&&) = delete;
    ConsumerGroupManager& operator=(ConsumerGroupManager&&) = delete;

    // --------------------------------------------------------
    // Group lifecycle
    // --------------------------------------------------------

    /**
     * @brief Create or update a consumer group.
     *
     * If a group with @p config.group_id already exists the configuration
     * is updated but the committed offset is preserved.
     *
     * @throws CDCException (INVALID_ARGUMENT) if group_id is empty or
     *         consumer_count is zero.
     */
    void createGroup(const ConsumerGroupConfig& config);

    /**
     * @brief Delete a consumer group and its persisted offset.
     *
     * @throws CDCException (INVALID_ARGUMENT) if group_id is empty.
     */
    void deleteGroup(const std::string& group_id);

    /**
     * @brief Return true if a group with @p group_id exists.
     */
    bool groupExists(const std::string& group_id) const;

    /**
     * @brief Return the configuration for an existing group.
     *
     * @throws CDCException (INVALID_ARGUMENT) if the group does not exist.
     */
    ConsumerGroupConfig getGroupConfig(const std::string& group_id) const;

    /**
     * @brief Return runtime information (config + committed offset) for a group.
     *
     * @throws CDCException (INVALID_ARGUMENT) if the group does not exist.
     */
    ConsumerGroupInfo getGroupInfo(const std::string& group_id) const;

    /**
     * @brief List all known group IDs.
     */
    std::vector<std::string> listGroups() const;

    // --------------------------------------------------------
    // Offset tracking
    // --------------------------------------------------------

    /**
     * @brief Return the committed offset (last acked sequence) for a group.
     *
     * Returns 0 if no offset has been committed yet.
     *
     * @throws CDCException (INVALID_ARGUMENT) if the group does not exist.
     */
    uint64_t getCommittedOffset(const std::string& group_id) const;

    /**
     * @brief Advance the committed offset for a group to @p sequence.
     *
     * The commit is a no-op when @p sequence <= current committed offset
     * (offsets only move forward).
     *
     * @throws CDCException (INVALID_ARGUMENT) if group_id is empty.
     * @throws CDCException (DB_WRITE_FAILED) on RocksDB write failure.
     */
    void commitOffset(const std::string& group_id, uint64_t sequence);

    // --------------------------------------------------------
    // Partition assignment
    // --------------------------------------------------------

    /**
     * @brief Return the partition index for the given @p consumer_id inside
     *        the group identified by @p group_id.
     *
     * Partition index is derived deterministically:
     *   partition = fnv1a32(consumer_id) % consumer_count
     *
     * @throws CDCException (INVALID_ARGUMENT) if the group does not exist.
     */
    uint32_t getConsumerPartition(const std::string& group_id,
                                   const std::string& consumer_id) const;

    /**
     * @brief Return true when the given @p consumer_id should handle a
     *        change event with key @p event_key in @p group_id.
     *
     * @throws CDCException (INVALID_ARGUMENT) if the group does not exist.
     */
    bool consumerHandlesKey(const std::string& group_id,
                             const std::string& consumer_id,
                             const std::string& event_key) const;

    /**
     * @brief Return the partition index for a document key within a group.
     *
     * Partition = fnv1a32(key) % consumer_count
     *
     * @throws CDCException (INVALID_ARGUMENT) if the group does not exist.
     */
    uint32_t getPartitionForKey(const std::string& group_id,
                                 const std::string& key) const;

    // --------------------------------------------------------
    // Event fetching
    // --------------------------------------------------------

    /**
     * @brief Fetch change events for a specific consumer in a group.
     *
     * Retrieves up to @p limit events from the changefeed starting at
     * (group.committed_sequence + 1) and filters them to only those whose
     * key belongs to @p consumer_id's partition.
     *
     * This allows a consumer to resume from its last acknowledged position
     * after a disconnect without scanning the full log from the beginning.
     *
     * @param group_id    Consumer group identifier.
     * @param consumer_id Consumer identifier (used to derive partition).
     * @param changefeed  Changefeed to read events from (not owned).
     * @param limit       Maximum number of events to return (0 = use default 100).
     * @return Filtered list of change events for this consumer's partition.
     *
     * @throws CDCException (INVALID_ARGUMENT) if the group does not exist.
     */
    std::vector<Changefeed::ChangeEvent> fetchEvents(
        const std::string& group_id,
        const std::string& consumer_id,
        const Changefeed& changefeed,
        size_t limit = 100) const;

    // --------------------------------------------------------
    // At-least-once delivery
    // --------------------------------------------------------

    /**
     * @brief Fetch events with at-least-once delivery guarantee.
     *
     * Delivers events to a consumer and tracks them as in-flight.  If any
     * previously delivered events for this consumer have not been acknowledged
     * within @p ack_timeout_ms they are redelivered before new events are
     * returned.
     *
     * @param group_id       Consumer group identifier.
     * @param consumer_id    Consumer identifier.
     * @param changefeed     Changefeed to read events from (not owned).
     * @param limit          Maximum events to return (0 = default 100).
     * @param ack_timeout_ms Milliseconds before an unacknowledged event is
     *                       redelivered (0 = never redeliver in-session).
     * @return Events for this consumer's partition; may include redelivered events.
     *
     * @throws CDCException (INVALID_ARGUMENT) if the group does not exist.
     */
    std::vector<Changefeed::ChangeEvent> fetchEventsAtLeastOnce(
        const std::string& group_id,
        const std::string& consumer_id,
        const Changefeed& changefeed,
        size_t limit = 100,
        uint32_t ack_timeout_ms = 30000);

    /**
     * @brief Acknowledge events up to @p up_to_sequence.
     *
     * Removes in-flight records with sequence ≤ @p up_to_sequence and advances
     * the group's committed offset to @p up_to_sequence.
     *
     * @param group_id       Consumer group identifier.
     * @param consumer_id    Consumer identifier.
     * @param up_to_sequence Inclusive upper bound of the acknowledged range.
     *
     * @throws CDCException (INVALID_ARGUMENT) if the group does not exist.
     * @throws CDCException (DB_WRITE_FAILED) on RocksDB write failure.
     */
    void acknowledgeEvents(const std::string& group_id,
                           const std::string& consumer_id,
                           uint64_t up_to_sequence);

    /**
     * @brief Return the number of in-flight events for a consumer.
     *
     * @param group_id    Consumer group identifier.
     * @param consumer_id Consumer identifier.
     * @return Count of in-flight events (0 if none or consumer unknown).
     */
    size_t getInFlightCount(const std::string& group_id,
                            const std::string& consumer_id) const;

    /**
     * @brief Return in-flight statistics for a consumer.
     *
     * @param group_id       Consumer group identifier.
     * @param consumer_id    Consumer identifier.
     * @param ack_timeout_ms Timeout used to classify overdue events.
     * @return InFlightStats for this consumer.
     */
    InFlightStats getInFlightStats(const std::string& group_id,
                                   const std::string& consumer_id,
                                   uint32_t ack_timeout_ms = 30000) const;

    // --------------------------------------------------------
    // Static helpers (also useful for unit testing)
    // --------------------------------------------------------

    /**
     * @brief FNV-1a 32-bit hash of a string.
     */
    static uint32_t fnv1a32(const std::string& s);

    /**
     * @brief Derive partition index for a key given a partition count.
     */
    static uint32_t partitionForKey(const std::string& key,
                                     uint32_t partition_count);

    /**
     * @brief Derive partition index for a consumer_id given a partition count.
     */
    static uint32_t partitionForConsumer(const std::string& consumer_id,
                                          uint32_t partition_count);

private:
    rocksdb::TransactionDB*     db_;
    rocksdb::ColumnFamilyHandle* cf_;
    mutable std::mutex           mutex_;

    static constexpr const char* GROUP_KEY_PREFIX  = "cdc_group:";
    static constexpr const char* CONFIG_SUFFIX     = ":config";
    static constexpr const char* OFFSET_SUFFIX     = ":offset";

    std::string makeConfigKey(const std::string& group_id) const;
    std::string makeOffsetKey(const std::string& group_id) const;

    // Internal helpers (caller must hold mutex_)
    ConsumerGroupConfig readConfigLocked(const std::string& group_id) const;
    uint64_t            readOffsetLocked(const std::string& group_id) const;
    void                writeConfigLocked(const ConsumerGroupConfig& config);
    void                writeOffsetLocked(const std::string& group_id, uint64_t sequence);

    // --------------------------------------------------------
    // In-flight tracking (in-memory; not persisted across restarts)
    // --------------------------------------------------------

    struct InFlightRecord {
        uint64_t sequence;
        std::chrono::steady_clock::time_point delivered_at;
        uint32_t delivery_count{1};  ///< Times this event has been delivered
    };

    // group_id -> consumer_id -> in-flight records (protected by mutex_)
    std::unordered_map<std::string,
        std::unordered_map<std::string,
            std::vector<InFlightRecord>>> inflight_;
};

} // namespace cdc
} // namespace themis
