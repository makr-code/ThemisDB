/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor/tensor_index_manager.h                      ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-05                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 EXPERIMENTAL — Phase 1 (Q3 2026)                         ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tensor/tensor_index_manager.h
 * @brief TensorIndexManager — lifecycle and routing for all TT-based indexes.
 *
 * ## Role in the SOC Architecture
 *
 * `TensorIndexManager` is the module-level registry for the `src/tensor/`
 * module, analogous to `IndexManager` in `src/index/`.  It:
 *
 *  1. Owns all `ITensorIndex` instances for the process lifetime.
 *  2. Routes incoming vectors to the correct index based on collection/field.
 *  3. Delegates storage to `TensorNetworkStorageEngine` (RocksDB-backed).
 *  4. Provides the `TensorRouter` decision boundary (HNSW vs. TT vs. HYBRID).
 *  5. Exports per-index statistics to the `TensorRagCostModel`.
 *
 * ## Multi-tenancy
 *
 * Each index is namespaced by `tenant_id`; key format:
 *   `__ttmgr__:<tenant_id>:<collection>:<field>`
 *
 * ## Thread safety
 *
 * All public methods are thread-safe.  Reads (search, stats) use a
 * shared_mutex; writes (create, drop) acquire exclusive ownership.
 */

#pragma once

#include "tensor/tensor_index.h"
#include "tensor/tensor_mmap_bridge.h"
#include "storage/tensor_router.h"
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {

// Forward declarations
class RocksDBWrapper;

namespace tensor {

// ============================================================================
// IndexHandle — lightweight descriptor returned by createIndex / getIndex
// ============================================================================

struct IndexHandle {
    std::string tenant_id;
    std::string collection;
    std::string field;
    storage::TensorRouter::Route route; ///< TENSOR_TRAIN, HNSW, or HYBRID
    ITensorIndex* index = nullptr;       ///< Owned by TensorIndexManager

    [[nodiscard]] std::string key() const {
        return "__ttmgr__:" + tenant_id + ":" + collection + ":" + field;
    }
};

// ============================================================================
// TensorIndexManager
// ============================================================================

/**
 * @brief Module-level lifecycle manager for ITensorIndex instances.
 *
 * ### Typical usage
 * ```cpp
 * auto mgr = TensorIndexManager::create(db);
 *
 * // Phase-1: decide routing
 * auto route = mgr->routeFor("tenant1", "llm_weights", "attention_k", 4096, 1e6);
 *
 * // Phase-2: create and use index
 * auto* idx = mgr->createIndex("tenant1", "llm_weights", "attention_k");
 * idx->addFlat(42, vec.data(), 4096);
 * auto results = idx->searchFlat(query.data(), 4096, 10);
 * ```
 */
class TensorIndexManager {
public:
    /// @brief Factory — creates a manager backed by the supplied RocksDB instance.
    static std::shared_ptr<TensorIndexManager>
        create(std::shared_ptr<RocksDBWrapper> db);

    ~TensorIndexManager();

    // ------------------------------------------------------------------
    // Routing decision (delegates to TensorRouter)
    // ------------------------------------------------------------------

    /**
     * @brief Ask the router whether data should use TT, HNSW, or HYBRID.
     *
     * @param tenant_id   Tenant namespace.
     * @param collection  Collection / table name.
     * @param field       Field / column name.
     * @param dim         Vector dimension.
     * @param num_vectors Estimated number of vectors.
     * @return            Routing decision (includes κ compressibility estimate).
     */
    storage::TensorRouter::Route routeFor(const std::string& tenant_id,
                                          const std::string& collection,
                                          const std::string& field,
                                          size_t dim,
                                          size_t num_vectors) const;

    // ------------------------------------------------------------------
    // Index lifecycle
    // ------------------------------------------------------------------

    /**
     * @brief Create (or open existing) TT index for a collection field.
     *
     * If the index already exists (persisted in RocksDB) it is loaded.
     * If the routing decision is HNSW the call succeeds but returns nullptr;
     * callers should then use the standard `src/index` path.
     *
     * @return Pointer to the ITensorIndex (owned by this manager), or nullptr
     *         on routing rejection or error.
     */
    ITensorIndex* createIndex(const std::string& tenant_id,
                               const std::string& collection,
                               const std::string& field,
                               size_t dim = 0,
                               size_t max_rank = 32,
                               double epsilon = 0.01);

    /**
     * @brief Retrieve an existing index without creating.
     *
     * @return Pointer, or nullptr if not found.
     */
    ITensorIndex* getIndex(const std::string& tenant_id,
                            const std::string& collection,
                            const std::string& field) const;

    /**
     * @brief Drop an index and delete its persisted data.
     *
     * @return true on success, false if not found.
     */
    bool dropIndex(const std::string& tenant_id,
                   const std::string& collection,
                   const std::string& field);

    /**
     * @brief Drop all indexes belonging to a tenant.
     */
    void dropTenantIndexes(const std::string& tenant_id);

    // ------------------------------------------------------------------
    // Introspection
    // ------------------------------------------------------------------

    /// List all (tenant, collection, field) triples managed.
    std::vector<IndexHandle> listIndexes() const;

    /// List indexes for a specific tenant.
    std::vector<IndexHandle> listIndexes(const std::string& tenant_id) const;

    /// Aggregate statistics across all TT indexes.
    TensorIndexStats aggregateStats() const;

    // ------------------------------------------------------------------
    // Zero-Copy GGML bridge integration point
    // ------------------------------------------------------------------

    /**
     * @brief Pin TT-core pages in RAM and return an RAII bridge object.
     *
     * Each TT-core for the requested vector is placed into a dedicated
     * `mmap(MAP_ANONYMOUS|MAP_PRIVATE)` region and locked via `mlock()`.
     * The returned `TensorMmapBridge` owns all regions; its destructor
     * calls `munlock()` + `munmap()` automatically.
     *
     * Clients should store the bridge object alive as long as any pointer
     * from `bridge->slices()` is being used (e.g. by a GGML graph node).
     *
     * @param tenant_id   Tenant namespace.
     * @param collection  Collection name.
     * @param field       Field name.
     * @param id          Vector ID.
     * @return            Owning bridge; nullptr if the vector does not exist.
     *
     * @note STUB #176 — currently uses MAP_ANONYMOUS + memcpy.
     *   Real zero-copy (MAP_SHARED on RocksDB SST pages) is deferred to
     *   Q1 2027 (see `src/tensor/FUTURE_ENHANCEMENTS.md`).
     */
    [[nodiscard]] std::unique_ptr<TensorMmapBridge>
        mapCores(const std::string& tenant_id,
                 const std::string& collection,
                 const std::string& field,
                 int64_t id) const;

    /**
     * @brief Raw-pointer variant (kept for backward compatibility).
     *
     * Prefer `mapCores()` for new code.  This method returns raw
     * `const float*` pointers that are valid only while the index is
     * alive; no mmap or mlock is performed.
     *
     * @deprecated Use mapCores() instead.
     *
     * @note
     * // STUB/SIMULATION NOTE:
     * // Purpose: backward-compatible raw-pointer bridge (Phase 3-A)
     * // Activation: always (no compile flag)
     * // Production Delta: no mmap / mlock; pointers invalidated on mutation
     * // Removal Plan: remove after all callers migrate to mapCores()
     */
    std::vector<std::pair<const float*, size_t>>
        ggmlCorePtrs(const std::string& tenant_id,
                     const std::string& collection,
                     const std::string& field,
                     int64_t id) const;

    // ------------------------------------------------------------------
    // File-based persistence
    // ------------------------------------------------------------------

    /**
     * @brief Configure the directory used to persist index files.
     *
     * When set, `createIndex()` attempts to restore an existing index from
     * `<data_dir>/<escaped_key>.ttidx`, and `dropIndex()` removes that file.
     * Call `flushAll()` to persist all in-memory indexes to disk.
     *
     * @param dir  Absolute or relative path to an existing directory.
     */
    void setDataDir(const std::string& dir);

    /**
     * @brief Save all currently loaded indexes to their respective files.
     *
     * Returns the number of indexes successfully saved.
     */
    size_t flushAll();

private:
    explicit TensorIndexManager(std::shared_ptr<RocksDBWrapper> db);

    /// Derive a safe filesystem path for an index with the given registry key.
    std::string indexFilePath(const std::string& key) const;

    std::shared_ptr<RocksDBWrapper> db_;
    std::string                     data_dir_;

    mutable std::shared_mutex registry_mutex_;
    std::unordered_map<std::string, std::unique_ptr<ITensorIndex>> indexes_;
    std::unordered_map<std::string, IndexHandle>                    handles_;
};

} // namespace tensor
} // namespace themis

