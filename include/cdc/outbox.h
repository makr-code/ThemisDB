/**
 * @file outbox.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB CDC Outbox Pattern
 *
 * Transactional outbox support for reliable change event publishing.
 *
 * The outbox pattern guarantees that application data mutations and their
 * corresponding CDC events are written atomically inside a single RocksDB
 * transaction.  A background relay then reads pending outbox records and
 * forwards them to the Changefeed, marking each record as PUBLISHED once the
 * forward succeeds.  This eliminates the dual-write problem where a process
 * crash between a DB commit and an event emit would silently drop events.
 *
 * Storage layout (RocksDB):
 *   Key:     "cdc_outbox:{20-digit-zero-padded-sequence}"
 *   Value:   JSON — OutboxRecord::toJson()
 *   Counter: "cdc_outbox_sequence"
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cdc/changefeed.h"
#include "cdc/cdc_error.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <nlohmann/json.hpp>

// Forward declarations for RocksDB types
namespace rocksdb {
    class TransactionDB;
    class ColumnFamilyHandle;
    class Transaction;
}

namespace themis {
namespace cdc {

// ============================================================
// OutboxState
// ============================================================

enum class OutboxState {
    PENDING,    ///< Written to outbox; not yet forwarded to Changefeed
    PUBLISHED,  ///< Successfully forwarded to Changefeed and marked done
    FAILED      ///< Max relay attempts exhausted; record kept for inspection
};

// ============================================================
// OutboxRecord
// ============================================================

struct OutboxRecord {
    uint64_t     outbox_sequence{0};  ///< Monotonic outbox-internal sequence
    std::string  collection;          ///< Target collection / table name
    std::string  key;                 ///< Document key
    std::optional<std::string> value; ///< Serialised document value (nullopt = DELETE)
    Changefeed::ChangeEventType event_type{Changefeed::ChangeEventType::EVENT_PUT};
    OutboxState  state{OutboxState::PENDING};
    int64_t      created_at_ms{0};    ///< Wall-clock ms when enqueued
    int64_t      published_at_ms{0};  ///< Wall-clock ms when published (0 = not yet)
    int          relay_attempts{0};   ///< How many times relay has tried this record
    std::string  failure_reason;      ///< Last relay error (FAILED state)
    nlohmann::json metadata;          ///< Caller-supplied metadata (tx_id, user, …)

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
    static OutboxRecord fromJson(const nlohmann::json& j);
};

// ============================================================
// OutboxWriter
// ============================================================

class OutboxWriter {
public:
    explicit OutboxWriter(rocksdb::TransactionDB* db,
                          rocksdb::ColumnFamilyHandle* cf = nullptr);

    ~OutboxWriter() = default;

    OutboxWriter(const OutboxWriter&)            = delete;
    OutboxWriter& operator=(const OutboxWriter&) = delete;

    /**
     * @brief Write To Outbox.
     * @param[in,out] txn Input/output parameter.
     * @param[in,out] rec Input/output parameter.
     * @return Return value.
     */
    OutboxRecord& writeToOutbox(rocksdb::Transaction* txn, OutboxRecord& rec);

private:
    rocksdb::TransactionDB*      db_;
    rocksdb::ColumnFamilyHandle* cf_;
    mutable std::mutex           sequence_mutex_;

    static constexpr const char* KEY_PREFIX    = "cdc_outbox:";
    static constexpr const char* SEQUENCE_KEY  = "cdc_outbox_sequence";

    /**
     * @brief Make Key.
     * @param[in] seq Input parameter.
     * @return Return value.
     */
    std::string  makeKey(uint64_t seq) const;
    /**
     * @brief Next Sequence.
     * @return Return value.
     */
    uint64_t     nextSequence();
};

// ============================================================
// OutboxRelayConfig
// ============================================================

struct OutboxRelayConfig {
    std::chrono::milliseconds poll_interval{100};

    size_t batch_size{100};

    int max_relay_attempts{5};
};

// ============================================================
// OutboxRelay
// ============================================================

class OutboxRelay {
public:
    explicit OutboxRelay(rocksdb::TransactionDB* db,
                         rocksdb::ColumnFamilyHandle* cf,
                         Changefeed& changefeed,
                         OutboxRelayConfig config = {});

    ~OutboxRelay();

    OutboxRelay(const OutboxRelay&)            = delete;
    OutboxRelay& operator=(const OutboxRelay&) = delete;

    /**
     * @brief Start.
     */
    void start();

    /**
     * @brief Stop.
     */
    void stop();

    /**
     * @brief Relay Once.
     * @return Return value.
     */
    size_t relayOnce();

    std::vector<OutboxRecord> listRecords(OutboxState state,
                                          size_t limit = 0) const;

    std::vector<OutboxRecord> listAllRecords(size_t limit = 0) const;

    /**
     * @brief Remove Record.
     * @param[in] outbox_sequence Input parameter.
     * @return True when the operation succeeds.
     */
    bool removeRecord(uint64_t outbox_sequence);

    /**
     * @brief Purge Published.
     * @return Return value.
     */
    size_t purgePublished();

    /**
     * @brief Total Relayed.
     * @return Return value.
     */
    uint64_t totalRelayed() const;

    /**
     * @brief Total Failed.
     * @return Return value.
     */
    uint64_t totalFailed() const;

private:
    rocksdb::TransactionDB*      db_;
    rocksdb::ColumnFamilyHandle* cf_;
    Changefeed&                  changefeed_;
    OutboxRelayConfig            config_;

    std::atomic<bool>       running_{false};
    std::thread             relay_thread_;
    std::condition_variable cv_;
    std::mutex              cv_mutex_;

    std::atomic<uint64_t>   total_relayed_{0};
    std::atomic<uint64_t>   total_failed_{0};

    static constexpr const char* KEY_PREFIX   = "cdc_outbox:";
    static constexpr const char* SEQUENCE_KEY = "cdc_outbox_sequence";

    /**
     * @brief Make Key.
     * @param[in] seq Input parameter.
     * @return Return value.
     */
    std::string makeKey(uint64_t seq) const;
    /**
     * @brief Relay Thread Func.
     */
    void        relayThreadFunc();
    /**
     * @brief Update Record.
     * @param[in] rec Input parameter.
     */
    void        updateRecord(const OutboxRecord& rec);
    /**
     * @brief Scan Records.
     * @param[in] limit Input parameter.
     * @param[in] filter_state Input parameter.
     * @param[in] all_states Input parameter.
     * @return Return value.
     */
    std::vector<OutboxRecord> scanRecords(size_t limit,
                                          OutboxState filter_state,
                                          bool all_states) const;
};

} // namespace cdc
} // namespace themis
