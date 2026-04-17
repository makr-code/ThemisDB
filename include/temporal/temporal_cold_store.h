/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            temporal_cold_store.h                              ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-17                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB Temporal Cold Store
 *
 * A sorted in-memory key-value store for historical VersionedDocument
 * entries that have been evicted from the hot tier of SystemVersionedTable.
 *
 * ## Key design
 *
 * Each stored version is addressed by a composite key:
 *
 *   <table_name> '\x01' <doc_key> '\x01' <16-char biased hex timestamp>
 *
 * The timestamp is encoded as a 16-character zero-padded hexadecimal
 * string of `(uint64_t)(sys_start + BIAS)` where
 * BIAS = (uint64_t)std::numeric_limits<int64_t>::max() + 1.
 *
 * This encoding is monotonically non-decreasing in byte-lexicographic
 * order, so a single `std::map::upper_bound(prefix + encode(t))` call
 * followed by a reverse iterator step resolves an AS-OF query in
 * O(log N) time.
 *
 * ## Integration with SystemVersionedTable
 *
 * Call `SystemVersionedTable::attachColdStore(store, threshold)` to
 * activate tiered storage.  After the hot tier for a key accumulates more
 * than `threshold` closed versions, `flushToCold(key)` offloads all but
 * the most-recent `threshold` closed versions to this store.
 *
 * The store is completely self-contained; no external dependencies beyond
 * the C++ standard library and nlohmann/json are required.  A production
 * deployment can swap the `std::map<std::string,std::string>` backing
 * store for RocksDB by keeping the same public API.
 *
 * Thread-safety: all public methods are thread-safe.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "temporal/temporal_types.h"
#include <atomic>
#include <map>
#include <optional>
#include <shared_mutex>
#include <string>
#include <vector>

namespace themisdb {
namespace temporal {

/**
 * @brief Statistics snapshot for a TemporalColdStore instance.
 */
struct ColdStoreStats {
    size_t total_versions{0};      ///< Total versions across all tables/keys
    size_t store_calls{0};         ///< Cumulative store() invocations
    size_t hit_getAsOf{0};         ///< AS-OF queries that found a result
    size_t miss_getAsOf{0};        ///< AS-OF queries that returned nullopt
    size_t total_getAll_results{0};///< Cumulative entries returned by getAll()

    nlohmann::json toJson() const {
        return {{"total_versions",       total_versions},
                {"store_calls",          store_calls},
                {"hit_getAsOf",          hit_getAsOf},
                {"miss_getAsOf",         miss_getAsOf},
                {"total_getAll_results", total_getAll_results}};
    }
};

/**
 * @brief Sorted in-memory cold-tier store for historical VersionedDocument
 *        entries.
 *
 * ## Complexity
 * | Operation                | Complexity          |
 * |--------------------------|---------------------|
 * | store()                  | O(log N)            |
 * | getAsOf(key, t)          | O(log N)            |
 * | getAll(key)              | O(log N + k)        |
 * | getRange(key, from, to)  | O(log N + k)        |
 * | versionCount(key)        | O(log N + k)        |
 * | remove(key)              | O(log N + k)        |
 * | totalVersionCount()      | O(1) — atomic       |
 *
 * where N is the total number of stored versions and k is the number of
 * versions for the queried (table, key) pair.
 */
class TemporalColdStore {
public:
    TemporalColdStore() = default;

    // Non-copyable; movable before any concurrent access.
    TemporalColdStore(const TemporalColdStore&)            = delete;
    TemporalColdStore& operator=(const TemporalColdStore&) = delete;

    // ── Mutation ─────────────────────────────────────────────────────────────

    /**
     * @brief Persist a historical version to the cold store.
     *
     * Storing a version that is still current (`isCurrent() == true`) is
     * explicitly rejected — only closed (non-current) versions are allowed
     * in the cold tier.
     *
     * @param table_name  Logical table the version belongs to.
     * @param doc         The historical VersionedDocument to store.
     * @return true on success; false if doc.isCurrent() is true.
     */
    bool store(const std::string& table_name, const VersionedDocument& doc);

    /**
     * @brief Remove all cold-tier versions for (table_name, doc_key).
     * @return Number of versions removed.
     */
    size_t remove(const std::string& table_name, const std::string& doc_key);

    /**
     * @brief Remove all cold-tier versions for table_name.
     * @return Number of versions removed.
     */
    size_t removeTable(const std::string& table_name);

    /** Remove everything from the store. */
    void clear();

    // ── Queries ───────────────────────────────────────────────────────────────

    /**
     * @brief Return the version of (table_name, doc_key) valid at timestamp
     *        as_of, i.e. v.sys_time.contains(as_of).
     *
     * Uses `upper_bound(prefix + encode(as_of))` and steps backwards — O(log N).
     *
     * @return nullopt if no matching version exists.
     */
    std::optional<VersionedDocument> getAsOf(const std::string& table_name,
                                             const std::string& doc_key,
                                             Timestamp as_of) const;

    /**
     * @brief Return all cold-tier versions for (table_name, doc_key),
     *        sorted ascending by sys_start.
     */
    std::vector<VersionedDocument> getAll(const std::string& table_name,
                                          const std::string& doc_key) const;

    /**
     * @brief Return cold-tier versions whose sys_time overlaps range,
     *        sorted ascending by sys_start.
     */
    std::vector<VersionedDocument> getRange(const std::string& table_name,
                                            const std::string& doc_key,
                                            const TimeRange& range) const;

    // ── Metadata ─────────────────────────────────────────────────────────────

    /** Number of cold-tier versions for (table_name, doc_key). */
    size_t versionCount(const std::string& table_name,
                        const std::string& doc_key) const;

    /** Total versions stored across all tables and keys. O(1). */
    size_t totalVersionCount() const noexcept;

    /** Snapshot of cumulative statistics. */
    ColdStoreStats stats() const;

    // ── Key encoding (public for tests / RocksDB migration) ──────────────────

    /**
     * @brief Encode a (table, doc_key, sys_start) triple into a sortable
     *        composite key.
     *
     * Format: `<table>\x01<doc_key>\x01<016llx biased timestamp>`
     */
    static std::string encodeKey(const std::string& table_name,
                                  const std::string& doc_key,
                                  Timestamp sys_start);

    /**
     * @brief Return the prefix string for all versions of (table, doc_key).
     *
     * Format: `<table>\x01<doc_key>\x01`
     */
    static std::string keyPrefix(const std::string& table_name,
                                  const std::string& doc_key);

    /**
     * @brief Return the prefix string for all versions of table_name.
     *
     * Format: `<table>\x01`
     */
    static std::string tablePrefix(const std::string& table_name);

private:
    // Bias added to int64_t sys_start before encoding to uint64_t hex.
    // Ensures all timestamps (including negative) encode as non-decreasing
    // byte strings when compared lexicographically.
    static constexpr uint64_t kTimestampBias =
        static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1ULL;

    static uint64_t biasedTimestamp(Timestamp t) noexcept;

    /// Helper: collect all versions in [lo_it, end) that share key_prefix.
    static std::vector<VersionedDocument>
    collectPrefix(const std::map<std::string, std::string>& map,
                  const std::string& prefix);

    // Primary storage: sorted composite key → JSON-serialised VersionedDocument
    std::map<std::string, std::string> store_;

    std::atomic<size_t> total_count_{0};

    mutable std::shared_mutex mutex_;
    mutable ColdStoreStats stats_;
};

} // namespace temporal
} // namespace themisdb
