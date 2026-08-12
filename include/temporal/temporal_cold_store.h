/**
 * @file temporal_cold_store.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Temporal Cold Store
 *
 * Tiered cold storage for historical VersionedDocument entries that have
 * been evicted from the hot tier of SystemVersionedTable.
 *
 * ## Architecture
 *
 * The store separates the **RAM index** from the **value storage**:
 *
 *   RAM index   — `std::set<std::string>` of composite keys only.
 *                 Each entry costs ~60 bytes (key string) regardless of
 *                 payload size.  Enables O(log N) AS-OF and range queries
 *                 without touching disk.
 *
 *   Backend     — pluggable `IColdStoreBackend` interface.  Two built-in
 *                 implementations:
 *
 *                 * InMemoryBackend  (default) — stores values in a
 *                   `std::map<string,string>` in RAM.  Used in unit tests
 *                   and embedded builds.  RAM cost = index + values.
 *
 *                 * FileSystemBackend — stores each version as a JSON file:
 *                   `{base_dir}/{table}/{safe_key_hex}/{016x_ts}.json`
 *                   Values are NOT kept in RAM; only the key appears in the
 *                   RAM index (~60 B/version vs ~400 B/version in-memory).
 *                   The index is rebuilt from disk on construction.
 *
 * ## RAM savings (FileSystemBackend)
 *
 * | Scenario                   | InMemoryBackend  | FileSystemBackend |
 * |----------------------------|------------------|-------------------|
 * | 10 k versions (avg 400 B)  | ~4 MB            | ~600 KB (index)   |
 * | 100 k versions             | ~40 MB           | ~6 MB  (index)    |
 * | 1 M versions               | ~400 MB          | ~60 MB (index)    |
 *
 * ## Key encoding
 *
 * Composite key: `<table_name>\x01<doc_key>\x01<016llx biased timestamp>`
 * BIAS = uint64_max/2 + 1 so that negative timestamps sort before positive
 * ones in lexicographic order.
 *
 * ## Integration with SystemVersionedTable
 *
 *   auto cs = std::make_shared<TemporalColdStore>(
 *       std::make_unique<FileSystemBackend>("/var/lib/themisdb/cold"));
 *   table.attachColdStore(cs.get(), 500); // hot_threshold
 *   table.flushToCold("my_key");
 *
 * Thread-safety: all public methods are thread-safe.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "temporal/temporal_types.h"
#include <atomic>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <shared_mutex>
#include <string>
#include <vector>

namespace themisdb {
namespace temporal {

/**
 * @brief Statistics snapshot for a TemporalColdStore instance.
 */
struct ColdStoreStats {
    size_t total_versions{0};       ///< Total versions in the RAM index
    size_t store_calls{0};          ///< Cumulative store() invocations
    size_t hit_getAsOf{0};          ///< AS-OF queries that found a result
    size_t miss_getAsOf{0};         ///< AS-OF queries that returned nullopt
    size_t total_getAll_results{0}; ///< Cumulative entries returned by getAll()
    size_t backend_reads{0};        ///< Cumulative backend get() calls (disk I/O)
    size_t backend_writes{0};       ///< Cumulative backend put() calls (disk I/O)

    nlohmann::json toJson() const {
        return {{"total_versions",       total_versions},
                {"store_calls",          store_calls},
                {"hit_getAsOf",          hit_getAsOf},
                {"miss_getAsOf",         miss_getAsOf},
                {"total_getAll_results", total_getAll_results},
                {"backend_reads",        backend_reads},
                {"backend_writes",       backend_writes}};
    }
};

// ============================================================================
// IColdStoreBackend — pluggable value storage
// ============================================================================

/**
 * @brief Interface for cold-store value backends.
 *
 * The backend is responsible **only** for mapping composite keys to
 * JSON-serialised VersionedDocument strings.  All key ordering, range logic,
 * and RAM indexing live in TemporalColdStore; the backend sees only individual
 * put/get/del calls and prefix-based list operations.
 *
 * Implementations must be thread-safe.
 */
class IColdStoreBackend {
public:
    virtual ~IColdStoreBackend() = default;

    /** Persist key→value.  Returns false on I/O failure. */
    virtual bool put(const std::string& composite_key,
                     const std::string& json_value) = 0;

    /** Retrieve value for key.  Returns empty string if not found. */
    virtual std::string get(const std::string& composite_key) const = 0;

    /** Delete key.  Returns true if the key existed. */
    virtual bool del(const std::string& composite_key) = 0;

    /**
     * Enumerate all composite keys that start with prefix.
     * Used to rebuild the RAM index on startup.
     */
    virtual std::vector<std::string>
    listKeysWithPrefix(const std::string& prefix) const = 0;

    /** Remove all keys that start with prefix. */
    virtual size_t deletePrefix(const std::string& prefix) = 0;

    /** Remove all entries.  */
    virtual void clearAll() = 0;
};

// ============================================================================
// InMemoryBackend
// ============================================================================

/**
 * @brief In-process backend that keeps values in a `std::map`.
 *
 * Default backend — no disk I/O, suitable for unit tests and small embedded
 * deployments where RAM is not the bottleneck.
 *
 * RAM cost = RAM index (~60 B/version) + values (payload size, typically
 * 100–2000 B/version).
 */
class InMemoryBackend : public IColdStoreBackend {
public:
    bool put(const std::string& key, const std::string& value) override;
    std::string get(const std::string& key) const override;
    bool del(const std::string& key) override;
    std::vector<std::string>
    listKeysWithPrefix(const std::string& prefix) const override;
    size_t deletePrefix(const std::string& prefix) override;
    void clearAll() override;

private:
    mutable std::shared_mutex mutex_;
    std::map<std::string, std::string> data_;
};

// ============================================================================
// FileSystemBackend
// ============================================================================

/**
 * @brief Disk-backed backend that stores each version as a plain JSON file.
 *
 * ## File layout
 *
 *   {base_dir}/
 *     {table_name}/
 *       {percent_encoded_doc_key}/
 *         {016x_biased_sys_start}.json   ← one file per version
 *
 * The `{016x_biased_sys_start}` component is the last 16 characters of the
 * composite key, which already encodes the timestamp.  The directory
 * hierarchy mirrors the composite-key structure so that the RAM index can
 * be rebuilt by `fs::recursive_directory_iterator` in O(N) time.
 *
 * ## RAM savings vs. InMemoryBackend
 *
 * Each value (JSON payload ~400 B average) resides only on disk.  The
 * RAM index in TemporalColdStore holds only the composite key string
 * (~60 B average), yielding a ~6–7× RAM reduction for the typical workload.
 *
 * ## Crash safety
 *
 * Writes use an atomic rename pattern (write to `.tmp`, then rename) so
 * that a crash mid-write never produces a truncated value file.
 *
 * ## Construction
 *
 *   auto backend = std::make_unique<FileSystemBackend>("/var/lib/themisdb/cold");
 *
 * The constructor creates `base_dir` if it does not exist.  It does NOT
 * rebuild the RAM index — that is done by TemporalColdStore after calling
 * `rebuildIndexFromBackend()` during its own construction.
 */
class FileSystemBackend : public IColdStoreBackend {
public:
    /**
     * @param base_dir  Root directory for cold-tier data files.
     *                  Created automatically if it does not exist.
     */
    explicit FileSystemBackend(std::filesystem::path base_dir);

    bool put(const std::string& key, const std::string& value) override;
    std::string get(const std::string& key) const override;
    bool del(const std::string& key) override;
    std::vector<std::string>
    listKeysWithPrefix(const std::string& prefix) const override;
    size_t deletePrefix(const std::string& prefix) override;
    void clearAll() override;

    const std::filesystem::path& basePath() const noexcept { return base_dir_; }

private:
    std::filesystem::path base_dir_;
    mutable std::shared_mutex mutex_;

    /// Convert a composite key to a filesystem path under base_dir_.
    std::filesystem::path keyToPath(const std::string& composite_key) const;

    /// Percent-encode a string component so it is safe as a directory name.
    static std::string percentEncode(const std::string& s);

    /// Reverse of percentEncode.
    static std::string percentDecode(const std::string& s);

    /// Reconstruct the composite key from a file path relative to base_dir_.
    std::string pathToKey(const std::filesystem::path& rel_path) const;
};

// ============================================================================
// TemporalColdStore
// ============================================================================

/**
 * @brief Sorted cold-tier store for historical VersionedDocument entries.
 *
 * Maintains a sorted RAM index (`std::set<std::string>` of composite keys)
 * for O(log N) AS-OF and range query resolution without loading values into
 * RAM.  Value retrieval is delegated to the configured `IColdStoreBackend`.
 *
 * ## Complexity
 * | Operation                | Complexity                    |
 * |--------------------------|-------------------------------|
 * | store()                  | O(log N) + 1 backend write    |
 * | getAsOf(key, t)          | O(log N) + O(1) backend reads |
 * | getAll(key)              | O(log N + k) + k backend reads|
 * | getRange(key, from, to)  | O(log N + k) + k backend reads|
 * | versionCount(key)        | O(log N + k) — index only     |
 * | remove(key)              | O(log N + k) backend deletes  |
 * | totalVersionCount()      | O(1) — atomic                 |
 *
 * N = total stored versions, k = versions for the queried (table, key).
 *
 * Thread-safety: all public methods are thread-safe via std::shared_mutex.
 */
class TemporalColdStore {
public:
    /**
     * @brief Construct with a custom backend.
     *
     * If backend is nullptr an InMemoryBackend is created automatically.
     * When using FileSystemBackend the constructor calls
     * `rebuildIndexFromBackend()` to populate the RAM index from disk.
     */
    explicit TemporalColdStore(
        std::unique_ptr<IColdStoreBackend> backend = nullptr);

    // Non-copyable
    TemporalColdStore(const TemporalColdStore&)            = delete;
    TemporalColdStore& operator=(const TemporalColdStore&) = delete;

    // ── Mutation ─────────────────────────────────────────────────────────────

    /**
     * @brief Persist a historical version to the cold store.
     *
     * Storing a version that is still current (`isCurrent() == true`) is
     * explicitly rejected — only closed (non-current) versions belong in the
     * cold tier.
     *
     * @param table_name  Logical table the version belongs to.
     * @param doc         The historical VersionedDocument to store.
     * @return true on success; false if doc.isCurrent() is true or on
     *         backend write failure.
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

    /** Remove everything (index + backend). */
    void clear();

    // ── Queries ───────────────────────────────────────────────────────────────

    /**
     * @brief Return the version of (table_name, doc_key) valid at timestamp
     *        as_of, i.e. `v.sys_time.contains(as_of)`.
     *
     * Uses `upper_bound(prefix + encode(as_of))` on the RAM index — O(log N)
     * — then issues at most O(1) backend reads in the common case.
     *
     * @return nullopt if no matching cold-tier version exists.
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

    /** Number of cold-tier versions for (table_name, doc_key). O(log N + k). */
    size_t versionCount(const std::string& table_name,
                        const std::string& doc_key) const;

    /** Total versions in the RAM index. O(1). */
    size_t totalVersionCount() const noexcept;

    /** Snapshot of cumulative statistics. */
    ColdStoreStats stats() const;

    /** Access the underlying backend (for testing / inspection). */
    IColdStoreBackend& backend() noexcept { return *backend_; }
    const IColdStoreBackend& backend() const noexcept { return *backend_; }

    // ── Index maintenance ─────────────────────────────────────────────────────

    /**
     * @brief Rebuild the RAM index from the backend.
     *
     * Called automatically by the constructor when a non-null backend is
     * provided.  May also be called manually after crash recovery.
     *
     * For InMemoryBackend this is a no-op (the RAM map already holds all
     * keys).  For FileSystemBackend it scans `base_dir` and reconstructs
     * `key_index_` in O(N) time.
     */
    void rebuildIndexFromBackend();

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

    /** Prefix for all versions of (table_name, doc_key): `<table>\x01<key>\x01` */
    static std::string keyPrefix(const std::string& table_name,
                                  const std::string& doc_key);

    /** Prefix for all versions of table_name: `<table>\x01` */
    static std::string tablePrefix(const std::string& table_name);

private:
    static constexpr uint64_t kTimestampBias =
        static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1ULL;

    static uint64_t biasedTimestamp(Timestamp t) noexcept;

    /// Deserialise a JSON string into a VersionedDocument.
    static std::optional<VersionedDocument>
    parseDocument(const std::string& json_str);

    std::unique_ptr<IColdStoreBackend> backend_;

    /// Sorted set of composite keys — the RAM index.
    /// Holds only key strings, no values.  ~60 bytes per entry.
    std::set<std::string> key_index_;

    std::atomic<size_t> total_count_{0};

    mutable std::shared_mutex mutex_;
    mutable ColdStoreStats stats_;
};

} // namespace temporal
} // namespace themisdb

