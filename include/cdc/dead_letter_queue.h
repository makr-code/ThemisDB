/**
 * @file dead_letter_queue.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct DLQEntry {
    uint64_t    dlq_sequence = 0;       ///< DLQ-internal sequence (unique within DLQ)
    Changefeed::ChangeEvent event;  ///< Original change event that failed delivery
    std::string failure_reason;     ///< Human-readable reason (last error message)
    int         attempt_count;      ///< Number of delivery attempts that were made
    int64_t     enqueued_at_ms;     ///< Wall-clock timestamp when enqueued (ms since epoch)

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
    static DLQEntry fromJson(const nlohmann::json& j);
};

class DeadLetterQueue {
public:
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
     * @brief Enqueue.
     * @param[in] event Input parameter.
     * @param[in] failure_reason Input parameter.
     * @param[in] attempt_count Input parameter.
     * @return Return value.
     */
    DLQEntry enqueue(const Changefeed::ChangeEvent& event,
                     const std::string& failure_reason,
                     int attempt_count);

    /**
     * @brief Replay.
     * @param[in] dlq_sequence Input parameter.
     * @param[in,out] changefeed Input/output parameter.
     * @return Return value.
     */
    Changefeed::ChangeEvent replay(uint64_t dlq_sequence,
                                   Changefeed& changefeed);

    /**
     * @brief Remove.
     * @param[in] dlq_sequence Input parameter.
     * @return True when the operation succeeds.
     */
    bool remove(uint64_t dlq_sequence);

    /**
     * @brief Drain.
     * @return Return value.
     */
    size_t drain();

    // ------------------------------------------------------------------ //
    // Read operations
    // ------------------------------------------------------------------ //

    std::vector<DLQEntry> listEntries(size_t limit = 0) const;

    /**
     * @brief Get Entry.
     * @param[in] dlq_sequence Input parameter.
     * @return Return value.
     */
    DLQEntry getEntry(uint64_t dlq_sequence) const;

    /**
     * @brief Size.
     * @return Return value.
     */
    size_t size() const;

private:
    rocksdb::TransactionDB*     db_;
    rocksdb::ColumnFamilyHandle* cf_;

    static constexpr const char* KEY_PREFIX    = "dlq:";
    static constexpr const char* SEQUENCE_KEY  = "dlq_sequence";

    mutable std::mutex sequence_mutex_;

    /**
     * @brief Make Key.
     * @param[in] dlq_sequence Input parameter.
     * @return Return value.
     */
    std::string makeKey(uint64_t dlq_sequence) const;
    /**
     * @brief Next Sequence.
     * @return Return value.
     */
    uint64_t    nextSequence();
};

} // namespace cdc
} // namespace themis
