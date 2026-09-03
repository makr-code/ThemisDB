/**
 * @file rocksdb_wiki_store.h
 * @brief RocksDB-backed key-value store for LLM Wiki page persistence.
 *
 * Provides `RocksDbWikiStore`, a production-grade RAII wrapper around a
 * `rocksdb::DB` instance.  Each wiki page is stored as a JSON-serialised
 * string value keyed by a stable string key derived from the page slug.
 *
 * ## Guard
 *
 * The entire implementation is conditionally compiled under
 * `#ifdef THEMIS_USE_ROCKSDB`.  When RocksDB is not available the header
 * still compiles cleanly, but `RocksDbWikiStore` is not defined.  All call
 * sites that reference `RocksDbWikiStore` must therefore also be guarded.
 *
 * When RocksDB is NOT available the existing in-memory fallback in
 * `LLMWikiPluginImpl` remains active and is documented with the updated
 * STUB/SIMULATION NOTE below.
 *
 * ## STUB/SIMULATION NOTE (Wave-B in-memory backend fallback):
 * Purpose: Fallback when THEMIS_USE_ROCKSDB is not defined or db_path is
 *          not configured.
 * Activation: When THEMIS_LLM_WIKI_BACKEND=mock OR when RocksDB is
 *             unavailable.
 * Production Delta: In-memory backend loses all data on restart; RocksDB
 *                   path is persistent.
 * Removal Plan: In-memory fallback retained for test environments; production
 *               must use RocksDB path.
 *               Target for mandatory RocksDB enforcement: Q1 2027.
 *
 * @version 0.1.0
 * @date    2026-08-26
 * @note    Wave-B gap closure — LW1 (RocksDB backend)
 * @see     src/llm_wiki/rocksdb_wiki_store.cpp
 * @see     tests/llm/test_wave_next_llm_wiki_rocksdb.cpp
 */

#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Status comes from the lightweight header so this file does not pull in
// the full plugin interface (which transitively includes llm/wiki_index_store.h
// → TBB → CUDA headers).
// ─────────────────────────────────────────────────────────────────────────────
#include "llm_wiki/llm_wiki_status.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#ifdef THEMIS_USE_ROCKSDB

#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/status.h>

namespace themis {
namespace plugins {
namespace llm_wiki {

/**
 * @brief RAII wrapper around a `rocksdb::DB` for LLM Wiki page persistence.
 *
 * Stores wiki page JSON blobs keyed by a stable slug string.
 * Thread-safe for concurrent `get()` / `scan()` after `open()`.
 * `put()` and `remove()` serialise through the underlying RocksDB
 * write path (RocksDB itself is thread-safe).
 *
 * ### Lifecycle
 * ```cpp
 * RocksDbWikiStore store;
 * auto st = store.open("/var/lib/themisdb/wiki_store");
 * if (!st.ok()) { return; }   // handle error
 * store.put("page:hnsw", page_json);
 * auto [get_st, json] = store.get("page:hnsw");
 * store.close();
 * ```
 */
class RocksDbWikiStore {
 public:
    RocksDbWikiStore()  = default;
    ~RocksDbWikiStore() { close(); }

    // Non-copyable; movable.
    RocksDbWikiStore(const RocksDbWikiStore&)            = delete;
    RocksDbWikiStore& operator=(const RocksDbWikiStore&) = delete;
    RocksDbWikiStore(RocksDbWikiStore&&)                 = default;
    RocksDbWikiStore& operator=(RocksDbWikiStore&&)      = default;

    // ── Open / Close ──────────────────────────────────────────────────────

    /**
     * @brief Open or create the RocksDB store at the given path.
     *
     * Creates the directory (and any missing parents) if it does not exist.
     * If `open()` fails after a partial open, the internal `db_` pointer is
     * reset to nullptr so `isOpen()` returns false.
     *
     * @param db_path  Filesystem path for the RocksDB directory.
     * @return         Status::Ok on success, Status::Error on failure.
     */
    Status open(const std::string& db_path);

    /**
     * @brief Close the database, flushing any pending WAL entries.
     *
     * Safe to call multiple times; subsequent calls are no-ops.
     */
    void close();

    /// @return True if the store is currently open.
    [[nodiscard]] bool isOpen() const noexcept { return db_ != nullptr; }

    // ── CRUD ──────────────────────────────────────────────────────────────

    /**
     * @brief Store a wiki page as a JSON string.
     *
     * @param key         Stable slug key, e.g., `"page:hnsw-algorithm"`.
     * @param value_json  Serialised JSON representation of the page.
     * @return            Status::Ok on success, Status::Error on failure.
     */
    Status put(const std::string& key, const std::string& value_json);

    /**
     * @brief Retrieve a wiki page by key.
     *
     * @param key  Slug key.
     * @return     Pair of Status and JSON string.
     *             If the key is not found, `Status::Error("not_found")` is
     *             returned with an empty value string.
     */
    [[nodiscard]] std::pair<Status, std::string> get(const std::string& key) const;

    /**
     * @brief Delete a wiki page by key.
     *
     * Deleting a non-existent key is treated as a success (idempotent).
     *
     * @param key  Slug key.
     * @return     Status::Ok on success, Status::Error on failure.
     */
    Status remove(const std::string& key);

    // ── Scan ──────────────────────────────────────────────────────────────

    /**
     * @brief Iterate all stored pages for index rebuild or migration.
     *
     * The callback receives `(key, value_json)` for each page.  Iteration
     * order is lexicographic by key.  The store must be open; calling
     * `scan()` on a closed store is a no-op.
     *
     * @param cb  Callback invoked once per stored entry.
     */
    void scan(std::function<void(std::string_view key,
                                 std::string_view value)> cb) const;

 private:
    rocksdb::DB*                 db_ = nullptr;
    rocksdb::Options             options_;
    std::string                  db_path_;
};

}  // namespace llm_wiki
}  // namespace plugins
}  // namespace themis

#endif  // THEMIS_USE_ROCKSDB
