/**
 * @file mvcc_store.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "storage/hlc.h"
#include "storage/rocksdb_wrapper.h"
#include <cstdint>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>

namespace themis {

/**
 * @brief MVCC Store – per-record multi-version concurrency control
 *
 * MVCCStore layers per-record versioning on top of RocksDBWrapper.  Each
 * write is stored under a versioned key that embeds an HLC timestamp, so
 * older versions are never overwritten in-place.  This enables:
 *
 *   • Snapshot reads  – read the most-recent version committed before a
 *     given timestamp without taking any lock.
 *   • Linearizable reads – read the absolutely latest committed version.
 *   • Version history – iterate all versions of a single key.
 *   • Garbage collection – remove versions older than a watermark.
 *
 * ## Key encoding
 *
 * Versioned keys are encoded as:
 *
 *     <base_key> '\x00' <8-byte-big-endian-HLC-timestamp>
 *
 * The null-byte separator `\x00` terminates the logical key, and the 8-byte
 * big-endian timestamp ensures that RocksDB's bytewise comparator sorts
 * versions in chronological order.  A prefix scan over `<base_key>\x00`
 * therefore traverses all versions of that key from oldest to newest.
 *
 * To find the latest version at-or-before timestamp T:
 *   1. Seek to `<base_key>\x00<T+1_be>` (exclusive upper bound).
 *   2. Step one entry backward with Prev().
 *   3. If the resulting key still starts with `<base_key>\x00`, it is the
 *      answer; otherwise the key has no version at or before T.
 *
 * ## Thread safety
 *
 * MVCCStore is thread-safe: the underlying RocksDBWrapper and HybridLogicalClock
 * are both internally synchronized.
 */
class MVCCStore {
public:
    /**
     * @brief Version entry returned by version-scan operations.
     */
    struct VersionEntry {
        HLCTimestamp  timestamp;
        std::vector<uint8_t> value;
    };

    /**
     * @brief Options for garbage collection.
     */
    struct GCOptions {
        /** Keep at least this many versions per key regardless of timestamp. */
        uint32_t min_versions_to_keep = 1;
    };

    /**
     * @brief Construct an MVCCStore backed by an existing RocksDBWrapper.
     *
     * @param db Shared ownership of the underlying RocksDB instance.
     *           Must already be open.
     * @param clock Shared ownership of the HLC clock.  If null, a new
     *              HybridLogicalClock is created internally.
     */
    explicit MVCCStore(
        std::shared_ptr<RocksDBWrapper> db,
        std::shared_ptr<HybridLogicalClock> clock = nullptr
    );

    // ─── Write ────────────────────────────────────────────────────────────────

    /**
     * @brief Write a new version of @p key with an auto-generated HLC timestamp.
     *
     * @return The HLC timestamp assigned to this version.
     */
    HLCTimestamp put(std::string_view key, const std::vector<uint8_t>& value);

    /**
     * @brief Write a versioned history entry for @p key within an existing transaction.
     *
     * Both the live-key write and this call share @p txn so they commit
     * atomically.  The HLC clock is advanced and the timestamp is returned.
     *
     * @param txn   Active RocksDB transaction.
     * @param key   The base (live) key whose value is being recorded.
     * @param value The value bytes to store in the history entry.
     * @return The HLC timestamp assigned to this history entry.
     */
    HLCTimestamp putInTxn(
        RocksDBWrapper::TransactionWrapper& txn,
        std::string_view key,
        const std::vector<uint8_t>& value
    );

    /**
     * @brief Write a tombstone history entry for @p key within an existing transaction.
     *
     * Records a deletion event in the history keyspace (op = "del").
     *
     * @param txn  Active RocksDB transaction.
     * @param key  The base (live) key that was deleted.
     * @return The HLC timestamp assigned to this tombstone entry.
     */
    HLCTimestamp delInTxn(
        RocksDBWrapper::TransactionWrapper& txn,
        std::string_view key
    );

    /**
     * @brief Write a new version of @p key with an explicit timestamp.
     *
     * Useful when the caller already holds a commit timestamp, e.g. from
     * a DistributedTimeCoordinator.  The clock is updated to stay ≥ ts.
     *
     * @param ts  Commit timestamp.  Must be strictly greater than any
     *            previously written version of the same key.
     */
    void putWithTimestamp(
        std::string_view key,
        const std::vector<uint8_t>& value,
        HLCTimestamp ts
    );

    // ─── Read ─────────────────────────────────────────────────────────────────

    /**
     * @brief Read the latest committed version of @p key.
     *
     * @return The value bytes, or std::nullopt if the key does not exist.
     */
    std::optional<std::vector<uint8_t>> getLatest(std::string_view key);

    /**
     * @brief Read the most-recent version of @p key committed at or before @p ts.
     *
     * This provides a consistent snapshot read without acquiring any lock.
     *
     * @param key  The logical record key.
     * @param ts   The read timestamp (snapshot point).
     * @return The value bytes, or std::nullopt if no version exists at or
     *         before @p ts.
     */
    std::optional<std::vector<uint8_t>> getAtTimestamp(
        std::string_view key,
        HLCTimestamp ts
    );

    // ─── Version history ──────────────────────────────────────────────────────

    /**
     * @brief Enumerate all stored versions of @p key, oldest first.
     *
     * @param key      The logical record key.
     * @param callback Called once per version in ascending timestamp order.
     *                 Return @c false to stop iteration early.
     */
    void scanVersions(
        std::string_view key,
        std::function<bool(const VersionEntry&)> callback
    );

    // ─── Garbage collection ───────────────────────────────────────────────────

    /**
     * @brief Remove all versions of @p key that are strictly older than @p min_ts.
     *
     * Versions at or after @p min_ts are kept.  The most-recent version is
     * always retained regardless of @p opts.min_versions_to_keep.
     *
     * @param key     The logical record key.
     * @param min_ts  All versions with timestamp < min_ts may be deleted.
     * @param opts    GC tuning options (e.g., always keep N recent versions).
     * @return Number of version entries deleted.
     */
    uint64_t gcVersionsBefore(
        std::string_view key,
        HLCTimestamp min_ts,
        GCOptions opts
    );

    /** @overload Uses default GCOptions (min_versions_to_keep = 1). */
    uint64_t gcVersionsBefore(std::string_view key, HLCTimestamp min_ts) {
        return gcVersionsBefore(key, min_ts, GCOptions{});
    }

    /**
     * @brief Run GC across all keys in the store.
     *
     * Iterates the entire keyspace and applies @p min_ts + @p opts to every
     * versioned key.  This is an O(N) scan – prefer per-key GC in hot paths.
     *
     * @return Total number of version entries deleted.
     */
    uint64_t gcAllBefore(HLCTimestamp min_ts, GCOptions opts);

    /** @overload Uses default GCOptions. */
    uint64_t gcAllBefore(HLCTimestamp min_ts) {
        return gcAllBefore(min_ts, GCOptions{});
    }

    /**
     * @brief Enumerate every distinct base key that has at least one versioned
     *        entry in the store.
     *
     * Performs a single O(N) full scan.  The callback receives each unique
     * base key exactly once.  Return @c false from the callback to stop
     * iteration early.
     *
     * Intended for use by MVCCChainPruner::pruneAll() so that the pruner can
     * migrate-then-delete per-key without duplicating the key-discovery logic.
     */
    void scanBaseKeys(std::function<bool(std::string_view base_key)> callback);

    // ─── Clock access ─────────────────────────────────────────────────────────

    /** Return the current HLC timestamp without advancing it. */
    HLCTimestamp currentTimestamp() const;

    /** Advance the HLC after receiving a remote timestamp. */
    HLCTimestamp updateClock(HLCTimestamp received);

    // ─── Key encoding helpers (public for testing) ────────────────────────────

    /**
     * @brief Build the versioned storage key for @p base_key at @p ts.
     *
     * Format: `<base_key>'\\0'<8-byte-big-endian-ts>`
     */
    static std::string encodeVersionedKey(std::string_view base_key, HLCTimestamp ts);

    /**
     * @brief Build the prefix used to scan all versions of @p base_key.
     *
     * Format: `<base_key>'\\0'`
     */
    static std::string encodeVersionPrefix(std::string_view base_key);

    /**
     * @brief Extract the timestamp from a versioned key produced by encodeVersionedKey().
     *
     * The timestamp occupies the last 8 bytes of a versioned key.  This function
     * uses a fixed-width offset from the end of the key rather than searching for
     * the '\\0' separator, because the 8-byte big-endian timestamp can itself
     * contain '\\0' bytes.
     *
     * @return The decoded HLCTimestamp, or a zero-valued timestamp if @p key
     *         is shorter than 9 bytes (and therefore cannot be a valid versioned key).
     */
    static HLCTimestamp decodeTimestamp(std::string_view versioned_key);

private:
    std::shared_ptr<RocksDBWrapper>     db_;
    std::shared_ptr<HybridLogicalClock> clock_;

    // F-010: Latest-version cache — maps base key → HLC timestamp of the most
    // recent non-transactional write.  Allows getLatest() to perform a direct
    // db_->get() point-read instead of creating a RocksDB iterator every time.
    //
    // Invariant: latest_ts_map_[key] == ts of the last put()/putWithTimestamp()
    // call for that key, provided ts >= any previous value in the map.
    // Keys written exclusively via putInTxn()/delInTxn() will NOT be in the
    // map; getLatest() falls back to the iterator path for those (safe).
    mutable std::shared_mutex                    latest_mu_;
    std::unordered_map<std::string, HLCTimestamp> latest_ts_map_;
};

} // namespace themis

