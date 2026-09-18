/**
 * @file consumer_group.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct ConsumerGroupConfig {
    std::string group_id;       ///< Unique group identifier
    uint32_t consumer_count{1}; ///< Number of partitions (consumers) in this group

    nlohmann::json toJson() const {
        return {{"group_id", group_id}, {"consumer_count", consumer_count}};
    }

    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     * @details Calls: value().
     */
    static ConsumerGroupConfig fromJson(const nlohmann::json& j) {
        ConsumerGroupConfig cfg;
        cfg.group_id       = j.value("group_id", "");
        cfg.consumer_count = j.value("consumer_count", uint32_t{1});
        return cfg;
    }
};

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

class ConsumerGroupManager {
public:
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
     * @brief Create Group.
     * @param[in] config Input parameter.
     */
    void createGroup(const ConsumerGroupConfig& config);

    /**
     * @brief Delete Group.
     * @param[in] group_id Identifier of the group.
     */
    void deleteGroup(const std::string& group_id);

    /**
     * @brief Group Exists.
     * @param[in] group_id Identifier of the group.
     * @return True when the operation succeeds.
     */
    bool groupExists(const std::string& group_id) const;

    /**
     * @brief Get Group Config.
     * @param[in] group_id Identifier of the group.
     * @return Return value.
     */
    ConsumerGroupConfig getGroupConfig(const std::string& group_id) const;

    /**
     * @brief Get Group Info.
     * @param[in] group_id Identifier of the group.
     * @return Return value.
     */
    ConsumerGroupInfo getGroupInfo(const std::string& group_id) const;

    /**
     * @brief List Groups.
     * @return Return value.
     */
    std::vector<std::string> listGroups() const;

    // --------------------------------------------------------
    // Offset tracking
    // --------------------------------------------------------

    /**
     * @brief Get Committed Offset.
     * @param[in] group_id Identifier of the group.
     * @return Return value.
     */
    uint64_t getCommittedOffset(const std::string& group_id) const;

    /**
     * @brief Commit Offset.
     * @param[in] group_id Identifier of the group.
     * @param[in] sequence Input parameter.
     */
    void commitOffset(const std::string& group_id, uint64_t sequence);

    // --------------------------------------------------------
    // Partition assignment
    // --------------------------------------------------------

    /**
     * @brief Get Consumer Partition.
     * @param[in] group_id Identifier of the group.
     * @param[in] consumer_id Identifier of the consumer.
     * @return Return value.
     */
    uint32_t getConsumerPartition(const std::string& group_id,
                                   const std::string& consumer_id) const;

    /**
     * @brief Consumer Handles Key.
     * @param[in] group_id Identifier of the group.
     * @param[in] consumer_id Identifier of the consumer.
     * @param[in] event_key Input parameter.
     * @return True when the operation succeeds.
     */
    bool consumerHandlesKey(const std::string& group_id,
                             const std::string& consumer_id,
                             const std::string& event_key) const;

    /**
     * @brief Get Partition For Key.
     * @param[in] group_id Identifier of the group.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    uint32_t getPartitionForKey(const std::string& group_id,
                                 const std::string& key) const;

    // --------------------------------------------------------
    // Event fetching
    // --------------------------------------------------------

    std::vector<Changefeed::ChangeEvent> fetchEvents(
        const std::string& group_id,
        const std::string& consumer_id,
        const Changefeed& changefeed,
        size_t limit = 100) const;

    // --------------------------------------------------------
    // At-least-once delivery
    // --------------------------------------------------------

    std::vector<Changefeed::ChangeEvent> fetchEventsAtLeastOnce(
        const std::string& group_id,
        const std::string& consumer_id,
        const Changefeed& changefeed,
        size_t limit = 100,
        uint32_t ack_timeout_ms = 30000);

    /**
     * @brief Acknowledge Events.
     * @param[in] group_id Identifier of the group.
     * @param[in] consumer_id Identifier of the consumer.
     * @param[in] up_to_sequence Input parameter.
     */
    void acknowledgeEvents(const std::string& group_id,
                           const std::string& consumer_id,
                           uint64_t up_to_sequence);

    /**
     * @brief Get In Flight Count.
     * @param[in] group_id Identifier of the group.
     * @param[in] consumer_id Identifier of the consumer.
     * @return Return value.
     */
    size_t getInFlightCount(const std::string& group_id,
                            const std::string& consumer_id) const;

    InFlightStats getInFlightStats(const std::string& group_id,
                                   const std::string& consumer_id,
                                   uint32_t ack_timeout_ms = 30000) const;

    /**
     * @brief -------------------------------------------------------- Static helpers (also useful for unit testing) --------------------------------------------------------
     * @param[in] s Input parameter.
     * @return Return value.
     */

    static uint32_t fnv1a32(const std::string& s);

    /**
     * @brief Partition For Key.
     * @param[in] key Input parameter.
     * @param[in] partition_count Input parameter.
     * @return Return value.
     */
    static uint32_t partitionForKey(const std::string& key,
                                     uint32_t partition_count);

    /**
     * @brief Partition For Consumer.
     * @param[in] consumer_id Identifier of the consumer.
     * @param[in] partition_count Input parameter.
     * @return Return value.
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

    /**
     * @brief Make Config Key.
     * @param[in] group_id Identifier of the group.
     * @return Return value.
     */
    std::string makeConfigKey(const std::string& group_id) const;
    /**
     * @brief Make Offset Key.
     * @param[in] group_id Identifier of the group.
     * @return Return value.
     */
    std::string makeOffsetKey(const std::string& group_id) const;

    /**
     * @brief Internal helpers (caller must hold mutex_)
     * @param[in] group_id Identifier of the group.
     * @return Return value.
     */
    ConsumerGroupConfig readConfigLocked(const std::string& group_id) const;
    /**
     * @brief Read Offset Locked.
     * @param[in] group_id Identifier of the group.
     * @return Return value.
     */
    uint64_t            readOffsetLocked(const std::string& group_id) const;
    /**
     * @brief Write Config Locked.
     * @param[in] config Input parameter.
     */
    void                writeConfigLocked(const ConsumerGroupConfig& config);
    /**
     * @brief Write Offset Locked.
     * @param[in] group_id Identifier of the group.
     * @param[in] sequence Input parameter.
     */
    void                writeOffsetLocked(const std::string& group_id, uint64_t sequence);

    // --------------------------------------------------------
    // In-flight tracking (in-memory; not persisted across restarts)
    // --------------------------------------------------------

    struct InFlightRecord {
        uint64_t sequence = 0;
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
