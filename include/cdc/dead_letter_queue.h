/**
 * @file dead_letter_queue.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB CDC Dead-Letter Queue
 *
 * Persistent storage for CDC change events that could not be delivered
 * after exhausting all retry attempts. Entries can be inspected, replayed
 * (re-recorded to the originating Changefeed), or removed.
 *
 * Storage layout (RocksDB):
 *   Key:   "dlq:{20-digit-zero-padded-sequence}"
 *   Value: JSON — DLQEntry::toJson()
 *   Counter key: "dlq_sequence"
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cdc/changefeed.h"
#include <cstdint>
#include <string>
#include <vector>
#include <mutex>

// Forward declarations for RocksDB types
namespace rocksdb {
    class TransactionDB;
    class ColumnFamilyHandle;
}

namespace themis {
namespace cdc {

/**
 * @brief A single dead-letter queue entry wrapping a failed ChangeEvent.
 */
struct DLQEntry {
    uint64_t    dlq_sequence;       ///< DLQ-internal sequence (unique within DLQ)
    Changefeed::ChangeEvent event;  ///< Original change event that failed delivery
    std::string failure_reason;     ///< Human-readable reason (last error message)
    int         attempt_count;      ///< Number of delivery attempts that were made
    int64_t     enqueued_at_ms;     ///< Wall-clock timestamp when enqueued (ms since epoch)

    nlohmann::json toJson() const;
    static DLQEntry fromJson(const nlohmann::json& j);
};

/**
 * @brief Dead-letter queue for failed CDC event deliveries.
 *
 * Thread-safe. Backed by the same RocksDB instance as the Changefeed
 * (or a dedicated one), using the key prefix "dlq:".
 *
 * Typical lifecycle:
 * 1. ChangefeedBuffer exhausts retries → calls enqueue().
 * 2. Operator inspects entries via listEntries().
 * 3. Operator replays individual entries via replay() after fixing the root cause.
 * 4. Successfully replayed entries are removed via remove(); drain() clears all.
 */
class DeadLetterQueue {
public:
    /**
     * @brief Construct a DeadLetterQueue backed by the given RocksDB instance.
     *
     * @param db   RocksDB TransactionDB instance (not owned).
     * @param cf   Optional column-family handle (nullptr = default CF).
     */
    explicit DeadLetterQueue(rocksdb::TransactionDB* db,
                             rocksdb::ColumnFamilyHandle* cf = nullptr);

    ~DeadLetterQueue() = default;

    // Non-copyable, non-movable (contains a mutex)
    DeadLetterQueue(const DeadLetterQueue&)             = delete;
    DeadLetterQueue& operator=(const DeadLetterQueue&)  = delete;
    DeadLetterQueue(DeadLetterQueue&&)                  = delete;
    DeadLetterQueue& operator=(DeadLetterQueue&&)       = delete;

    // ------------------------------------------------------------------ //
    // Write operations
    // ------------------------------------------------------------------ //

    /**
     * @brief Enqueue a failed event.
     *
     * @param event          The ChangeEvent that could not be delivered.
     * @param failure_reason Human-readable description of the last error.
     * @param attempt_count  Total number of delivery attempts that were made.
     * @return               The DLQEntry as stored (with assigned dlq_sequence).
     */
    DLQEntry enqueue(const Changefeed::ChangeEvent& event,
                     const std::string& failure_reason,
                     int attempt_count);

    /**
     * @brief Replay a DLQ entry by re-recording it to the provided Changefeed.
     *
     * On success the entry is removed from the DLQ automatically.
     *
     * @param dlq_sequence   The DLQ sequence of the entry to replay.
     * @param changefeed     Target Changefeed instance to re-record into.
     * @return               The newly recorded ChangeEvent (with fresh sequence).
     * @throws CDCException  If the entry is not found or re-recording fails.
     */
    Changefeed::ChangeEvent replay(uint64_t dlq_sequence,
                                   Changefeed& changefeed);

    /**
     * @brief Remove a single DLQ entry.
     *
     * @param dlq_sequence   The DLQ sequence of the entry to delete.
     * @return               true if found and deleted, false if not found.
     */
    bool remove(uint64_t dlq_sequence);

    /**
     * @brief Remove all DLQ entries.
     *
     * @return Number of entries deleted.
     */
    size_t drain();

    // ------------------------------------------------------------------ //
    // Read operations
    // ------------------------------------------------------------------ //

    /**
     * @brief List DLQ entries in enqueue order.
     *
     * @param limit  Maximum number of entries to return (0 = unlimited).
     * @return       Vector of DLQEntry, oldest first.
     */
    std::vector<DLQEntry> listEntries(size_t limit = 0) const;

    /**
     * @brief Fetch a single DLQ entry by its sequence number.
     *
     * @param dlq_sequence  DLQ sequence to look up.
     * @return              The DLQEntry.
     * @throws CDCException If not found.
     */
    DLQEntry getEntry(uint64_t dlq_sequence) const;

    /**
     * @brief Return the number of entries currently in the DLQ.
     */
    size_t size() const;

private:
    rocksdb::TransactionDB*     db_;
    rocksdb::ColumnFamilyHandle* cf_;

    static constexpr const char* KEY_PREFIX    = "dlq:";
    static constexpr const char* SEQUENCE_KEY  = "dlq_sequence";

    mutable std::mutex sequence_mutex_;

    std::string makeKey(uint64_t dlq_sequence) const;
    uint64_t    nextSequence();
};

} // namespace cdc
} // namespace themis
