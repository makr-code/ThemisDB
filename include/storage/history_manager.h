// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "storage/hlc.h"
#include "storage/rocksdb_wrapper.h"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <memory>

namespace themis {

// ─────────────────────────────────────────────────────────────────────────────
// HistoryRecord
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief One immutable version entry in the history keyspace.
 *
 * Serialised as JSON with format version 1.  Fields:
 *   v        – format version (always 1)
 *   base_key – live key that was written/deleted
 *   ts       – HLC timestamp (raw uint64 value)
 *   op       – "put" or "del"
 *   value    – hex-encoded bytes for "put", empty for "del"
 *   txn_id   – originating transaction ID (0 if unknown)
 */
struct HistoryRecord {
    int         version{1};       ///< Format version
    std::string base_key;         ///< Live key (e.g. "entity:users:u1")
    HLCTimestamp timestamp;       ///< HLC timestamp of the write
    std::string op;               ///< "put" or "del"
    std::vector<uint8_t> value;   ///< Written bytes (empty for "del")
    uint64_t txn_id{0};           ///< Originating transaction ID
};

// ─────────────────────────────────────────────────────────────────────────────
// ConflictRecord
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Artifact capturing a write-write conflict.
 *
 * Persisted in the conflict keyspace when a transaction commit fails due to
 * an MVCC conflict.  Serialised as JSON with format version 1.
 *
 * base  – value at transaction start (snapshot read before writing)
 * ours  – value this transaction tried to write
 * theirs – value already committed by the conflicting transaction
 */
struct ConflictRecord {
    int         version{1};           ///< Format version
    std::string conflict_id;          ///< Unique ID (HLC timestamp string)
    std::string base_key;             ///< The conflicting live key
    HLCTimestamp detected_at;         ///< When the conflict was detected
    uint64_t txn_id{0};               ///< Transaction that encountered the conflict
    std::vector<uint8_t> base_value;  ///< Value at transaction start (may be empty)
    std::vector<uint8_t> ours_value;  ///< Value we tried to write (may be empty)
    std::vector<uint8_t> theirs_value;///< Value committed by conflicting transaction (may be empty)
};

// ─────────────────────────────────────────────────────────────────────────────
// HistoryManager
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Manages the immutable history keyspace.
 *
 * History keys are stored as:
 *   `hist:<base_key>\x00<8-byte-big-endian-HLC-ts>`
 *
 * This allows a prefix scan over `hist:<base_key>\x00` to iterate all
 * versions in chronological order, consistent with the existing MVCCStore
 * versioned-key scheme.
 *
 * @thread_safety
 * Read operations are thread-safe. Write operations must be performed within
 * a single-writer transaction to guarantee atomicity with live-key writes.
 */
class HistoryManager {
public:
    /**
     * @brief Construct a HistoryManager.
     *
     * @param db    Underlying RocksDB instance (shared ownership).
     * @param clock HLC clock used to generate timestamps (shared ownership).
     *              If null, a new HybridLogicalClock is created internally.
     */
    explicit HistoryManager(
        std::shared_ptr<RocksDBWrapper>     db,
        std::shared_ptr<HybridLogicalClock> clock = nullptr
    );

    // ── Key encoding helpers (public for testing) ────────────────────────────

    /**
     * @brief Build the history key for @p base_key at @p ts.
     *
     * Format: `hist:<base_key>\x00<8-byte-big-endian-ts>`
     */
    static std::string historyKey(std::string_view base_key, HLCTimestamp ts);

    /**
     * @brief Build the scan prefix for all history versions of @p base_key.
     *
     * Format: `hist:<base_key>\x00`
     */
    static std::string historyPrefix(std::string_view base_key);

    // ── Transactional write helpers ───────────────────────────────────────────

    /**
     * @brief Write a "put" history entry within an existing transaction.
     *
     * Both the live-key write and this history write share the same
     * @p txn so they are committed atomically.
     *
     * @param txn       Active RocksDB transaction.
     * @param base_key  The live key that was written.
     * @param value     The value that was written.
     * @param txn_id    Originating transaction ID (0 if unknown).
     * @return The HLC timestamp assigned to this history entry.
     */
    HLCTimestamp recordPut(
        RocksDBWrapper::TransactionWrapper& txn,
        std::string_view base_key,
        const std::vector<uint8_t>& value,
        uint64_t txn_id = 0
    );

    /**
     * @brief Write a tombstone "del" history entry within an existing transaction.
     *
     * @param txn      Active RocksDB transaction.
     * @param base_key The live key that was deleted.
     * @param txn_id   Originating transaction ID (0 if unknown).
     * @return The HLC timestamp assigned to this history entry.
     */
    HLCTimestamp recordDel(
        RocksDBWrapper::TransactionWrapper& txn,
        std::string_view base_key,
        uint64_t txn_id = 0
    );

    // ── Time-travel reads ─────────────────────────────────────────────────────

    /**
     * @brief Return the most-recent history record for @p base_key at or before @p ts.
     *
     * @return HistoryRecord, or std::nullopt if no version exists at or before @p ts.
     */
    std::optional<HistoryRecord> getAtTimestamp(
        std::string_view base_key,
        HLCTimestamp ts
    ) const;

    /**
     * @brief Return all history versions of @p base_key, oldest first.
     */
    std::vector<HistoryRecord> listVersions(std::string_view base_key) const;

    // ── Serialization helpers (public for testing) ────────────────────────────

    static std::vector<uint8_t> serializeHistoryRecord(const HistoryRecord& rec);
    static std::optional<HistoryRecord> deserializeHistoryRecord(std::string_view data);

private:
    std::shared_ptr<RocksDBWrapper>     db_;
    std::shared_ptr<HybridLogicalClock> clock_;
};

// ─────────────────────────────────────────────────────────────────────────────
// ConflictManager
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Persists and retrieves conflict artifacts.
 *
 * Conflict keys are stored as:
 *   `conflict:<conflict_id>`
 *
 * Writes use a non-transactional write batch so that they always succeed
 * even after a transaction has failed.
 *
 * @thread_safety Thread-safe for all operations.
 */
class ConflictManager {
public:
    /**
     * @brief Construct a ConflictManager.
     *
     * @param db    Underlying RocksDB instance.
     * @param clock HLC clock for generating unique conflict IDs.
     */
    explicit ConflictManager(
        std::shared_ptr<RocksDBWrapper>     db,
        std::shared_ptr<HybridLogicalClock> clock = nullptr
    );

    // ── Key encoding helpers (public for testing) ─────────────────────────────

    /**
     * @brief Build the storage key for a conflict record.
     *
     * Format: `conflict:<conflict_id>`
     */
    static std::string conflictKey(std::string_view conflict_id);

    // ── Write ─────────────────────────────────────────────────────────────────

    /**
     * @brief Persist a ConflictRecord using a fresh non-transactional write.
     *
     * Assigns a unique conflict_id to @p record if it is empty, then writes
     * it to the conflict keyspace.
     *
     * @return The assigned conflict_id.
     */
    std::string storeConflict(ConflictRecord& record);

    // ── Read ──────────────────────────────────────────────────────────────────

    /**
     * @brief Retrieve a specific ConflictRecord by ID.
     *
     * @return The record, or std::nullopt if not found.
     */
    std::optional<ConflictRecord> getConflict(std::string_view conflict_id) const;

    /**
     * @brief List all stored conflict records, most-recent first.
     *
     * Note: this is an O(N) scan over all conflict entries.
     */
    std::vector<ConflictRecord> listConflicts() const;

    // ── Serialization helpers (public for testing) ────────────────────────────

    static std::vector<uint8_t> serializeConflictRecord(const ConflictRecord& rec);
    static std::optional<ConflictRecord> deserializeConflictRecord(std::string_view data);

private:
    std::shared_ptr<RocksDBWrapper>     db_;
    std::shared_ptr<HybridLogicalClock> clock_;
};

} // namespace themis
