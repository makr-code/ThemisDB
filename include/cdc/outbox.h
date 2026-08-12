/**
 * @file outbox.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Lifecycle state of an outbox record.
 */
enum class OutboxState {
    PENDING,    ///< Written to outbox; not yet forwarded to Changefeed
    PUBLISHED,  ///< Successfully forwarded to Changefeed and marked done
    FAILED      ///< Max relay attempts exhausted; record kept for inspection
};

// ============================================================
// OutboxRecord
// ============================================================

/**
 * @brief A single transactional outbox record.
 *
 * Written inside a user transaction alongside the application data.
 * Relay reads PENDING records and forwards them to Changefeed.
 */
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

    nlohmann::json toJson() const;
    static OutboxRecord fromJson(const nlohmann::json& j);
};

// ============================================================
// OutboxWriter
// ============================================================

/**
 * @brief Writes outbox records into a RocksDB transaction.
 *
 * Usage:
 * @code
 *   // Caller owns the RocksDB transaction; OutboxWriter participates in it.
 *   rocksdb::Transaction* txn = db->BeginTransaction(write_opts);
 *
 *   OutboxWriter writer(db, cf);
 *
 *   // Write application data inside the same transaction:
 *   txn->Put(cf, "orders:42", order_json);
 *
 *   // Enqueue a CDC outbox record in the same transaction:
 *   OutboxRecord rec;
 *   rec.collection = "orders";
 *   rec.key        = "orders:42";
 *   rec.value      = order_json;
 *   rec.event_type = Changefeed::ChangeEventType::EVENT_PUT;
 *   writer.writeToOutbox(txn, rec);
 *
 *   txn->Commit();   // both the app data and outbox record commit atomically
 * @endcode
 *
 * Thread-safety: writeToOutbox() is thread-safe (uses an internal sequence
 * mutex); multiple writers may share one OutboxWriter instance.
 */
class OutboxWriter {
public:
    /**
     * @param db  RocksDB TransactionDB instance (not owned).
     * @param cf  Optional column-family handle (nullptr = default CF).
     */
    explicit OutboxWriter(rocksdb::TransactionDB* db,
                          rocksdb::ColumnFamilyHandle* cf = nullptr);

    ~OutboxWriter() = default;

    OutboxWriter(const OutboxWriter&)            = delete;
    OutboxWriter& operator=(const OutboxWriter&) = delete;

    /**
     * @brief Write an outbox record into an existing RocksDB transaction.
     *
     * Assigns a sequence number, sets state = PENDING, records
     * created_at_ms, and calls txn->Put() to store the serialised record.
     * The sequence counter is also updated inside the transaction so it
     * commits atomically with the record.
     *
     * @param txn  Live RocksDB transaction to participate in (not owned).
     * @param rec  Outbox record to enqueue (mutated in place: sequence and
     *             created_at_ms are filled in by this method).
     * @return     Reference to the populated record (same as @p rec).
     * @throws CDCException (INVALID_ARGUMENT) if txn is null or key is empty.
     * @throws CDCException (DB_WRITE_FAILED)  on RocksDB write error.
     */
    OutboxRecord& writeToOutbox(rocksdb::Transaction* txn, OutboxRecord& rec);

private:
    rocksdb::TransactionDB*      db_;
    rocksdb::ColumnFamilyHandle* cf_;
    mutable std::mutex           sequence_mutex_;

    static constexpr const char* KEY_PREFIX    = "cdc_outbox:";
    static constexpr const char* SEQUENCE_KEY  = "cdc_outbox_sequence";

    std::string  makeKey(uint64_t seq) const;
    uint64_t     nextSequence();
};

// ============================================================
// OutboxRelayConfig
// ============================================================

/**
 * @brief Configuration for OutboxRelay.
 */
struct OutboxRelayConfig {
    /// How often the relay polls for PENDING records (default: 100 ms).
    std::chrono::milliseconds poll_interval{100};

    /// Maximum records to relay in a single poll cycle (default: 100).
    size_t batch_size{100};

    /// Maximum relay attempts before marking a record FAILED (0 = unlimited).
    int max_relay_attempts{5};
};

// ============================================================
// OutboxRelay
// ============================================================

/**
 * @brief Relay that forwards PENDING outbox records to a Changefeed.
 *
 * Runs a background thread that periodically scans the outbox for PENDING
 * records, records each one as a ChangeEvent in the provided Changefeed,
 * and then marks it PUBLISHED.  Records that fail after max_relay_attempts
 * are marked FAILED and left in the outbox for operator inspection.
 *
 * Usage:
 * @code
 *   OutboxRelayConfig cfg;
 *   cfg.poll_interval = std::chrono::milliseconds(200);
 *
 *   OutboxRelay relay(db, cf, changefeed, cfg);
 *   relay.start();
 *   // ... application runs ...
 *   relay.stop();
 * @endcode
 *
 * Thread-safety: all public methods are thread-safe.
 */
class OutboxRelay {
public:
    /**
     * @param db         RocksDB TransactionDB instance (not owned).
     * @param cf         Optional column-family handle (nullptr = default CF).
     * @param changefeed Changefeed to publish events to (not owned).
     * @param config     Relay configuration.
     */
    explicit OutboxRelay(rocksdb::TransactionDB* db,
                         rocksdb::ColumnFamilyHandle* cf,
                         Changefeed& changefeed,
                         OutboxRelayConfig config = {});

    ~OutboxRelay();

    OutboxRelay(const OutboxRelay&)            = delete;
    OutboxRelay& operator=(const OutboxRelay&) = delete;

    /**
     * @brief Start the background relay thread.
     *
     * Safe to call multiple times (second call is a no-op).
     */
    void start();

    /**
     * @brief Stop the background relay thread.
     *
     * Blocks until the thread exits.  After stop(), start() may be called again.
     */
    void stop();

    /**
     * @brief Perform one relay cycle synchronously (for testing / on-demand use).
     *
     * Fetches up to config.batch_size PENDING records, publishes them to
     * Changefeed, and updates their state to PUBLISHED or FAILED.
     *
     * @return Number of records successfully published in this cycle.
     */
    size_t relayOnce();

    /**
     * @brief Return all outbox records with the given state.
     *
     * @param state  Filter by state (PENDING, PUBLISHED, FAILED).
     * @param limit  Maximum entries to return (0 = unlimited).
     */
    std::vector<OutboxRecord> listRecords(OutboxState state,
                                          size_t limit = 0) const;

    /**
     * @brief Return all outbox records regardless of state.
     *
     * @param limit  Maximum entries to return (0 = unlimited).
     */
    std::vector<OutboxRecord> listAllRecords(size_t limit = 0) const;

    /**
     * @brief Remove a single outbox record by its sequence number.
     *
     * Intended for PUBLISHED or FAILED records that have been inspected.
     *
     * @return true if found and deleted, false if not found.
     */
    bool removeRecord(uint64_t outbox_sequence);

    /**
     * @brief Remove all PUBLISHED records (cleanup after successful delivery).
     *
     * @return Number of records removed.
     */
    size_t purgePublished();

    /**
     * @brief Total records relayed since construction.
     */
    uint64_t totalRelayed() const;

    /**
     * @brief Total relay failures since construction (records marked FAILED).
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

    std::string makeKey(uint64_t seq) const;
    void        relayThreadFunc();
    void        updateRecord(const OutboxRecord& rec);
    std::vector<OutboxRecord> scanRecords(size_t limit,
                                          OutboxState filter_state,
                                          bool all_states) const;
};

} // namespace cdc
} // namespace themis
