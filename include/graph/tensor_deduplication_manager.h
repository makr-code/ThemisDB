/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor_deduplication_manager.h                     ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-05                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tensor_deduplication_manager.h
 * @brief Single-Instance-Storage for structurally similar TT-tensors.
 *
 * `TensorDeduplicationManager` sits between the write path and
 * `TensorNetworkStorageEngine`.  When a new tensor is stored it checks
 * the `TensorFingerprintGraph` for a sufficiently similar existing tensor.
 * If one exists, only a delta (residual TT-chain) is persisted:
 *
 *   stored = reference + delta
 *   where ‖delta‖_F / ‖stored‖_F ≤ (1 − similarity_threshold)
 *
 * The reference is stored once; all similar tensors store only their deltas,
 * significantly reducing storage for LLM weight repositories where Transformer
 * blocks are reused across layers and model versions.
 *
 * ### Delta encoding
 * Given canonical tensor C and new tensor N:
 *   delta = TT-rounding(N − C, eps=cfg.delta_eps)
 * Reconstruction: N ≈ C + delta.
 *
 * The TT-difference (N − C) is approximated element-wise via core unfolding;
 * the result is then re-compressed with TT-SVD.
 *
 * ### References
 * - Yadav, P. et al. (2023). TIES-Merging. NeurIPS 2023.
 * - Stoudenmire & Schwab (2016). Supervised Learning with Tensor Networks.
 * - RAID-6 / deduplication literature: Zhu, B. et al. (2008). Avoiding the
 *   Disk Bottleneck in the Data Domain Deduplication File System. FAST 2008.
 */

#pragma once

#include "graph/tensor_fingerprint_graph.h"
#include "storage/tensor_network_storage_engine.h"

#include <atomic>
#include <memory>
#include <mutex>
#include &lt;optional&gt;
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations for GraphIndex-backed journal wiring helper.
// Full definition in <index/graph_index.h>.
namespace themis {
class GraphIndexManager;
} // namespace themis

namespace themis {
namespace graph {

// ============================================================================
// DeduplicationConfig
// ============================================================================

/**
 * @brief Configuration for TensorDeduplicationManager.
 */
struct DeduplicationConfig {
    /// Cosine similarity threshold for treating two tensors as duplicates.
    /// Must match or exceed FingerprintGraphConfig::similarity_threshold.
    double similarity_threshold = 0.999;

    /// TT-rounding eps applied to the delta tensor.
    double delta_eps = 0.001;

    /// Maximum TT-rank for the stored delta (0 = unlimited).
    std::size_t delta_max_rank = 16;

    /// If true, the manager falls back to full storage when no suitable
    /// reference is found (disable for strict dedup-only mode).
    bool allow_full_storage_fallback = true;
};

// ============================================================================
// StoredTensorRecord
// ============================================================================

/**
 * @brief Metadata describing how a tensor is physically stored.
 */
struct StoredTensorRecord {
    std::string tensor_id;

    /// If non-empty, this tensor is stored as a delta relative to reference_id.
    std::string reference_id;

    /// True if stored in full TT form (no delta encoding).
    bool is_canonical = true;

    /// Compressed size in bytes (all cores combined).
    std::size_t compressed_bytes = 0;

    /// Deduplication savings vs. full storage in bytes.
    std::size_t saved_bytes = 0;

    /// Cosine similarity to the reference (0.0 for canonical tensors).
    double similarity_to_reference = 0.0;

    // ─── Storage key fields (populated by TensorDeduplicationManager::store) ──

    /// Tenant namespace used as the storage key.
    std::string tenant;

    /// Collection name used as the storage key.
    std::string collection;

    /// Field name used as the storage key.
    /// For delta-encoded tensors this is the *original* field name (the delta
    /// is stored under `field + "__delta__" + reference_id`).
    std::string field;
};

// ============================================================================
// DeduplicationStats
// ============================================================================

/**
 * @brief Aggregate statistics for the deduplication manager.
 */
struct DeduplicationStats {
    std::size_t total_tensors      = 0;
    std::size_t canonical_tensors  = 0;
    std::size_t delta_tensors      = 0;
    std::size_t total_bytes_stored = 0;
    std::size_t bytes_saved        = 0;
    double      dedup_ratio        = 1.0; ///< full_bytes / stored_bytes

    /// Reset to zero
    void reset() noexcept { *this = {}; }
};

// ============================================================================
// TensorDeduplicationManager
// ============================================================================

/**
 * @brief Manages Single-Instance-Storage for TT-compressed tensors.
 *
 * ### Write path
 * 1. Compute fingerprint and query `TensorFingerprintGraph`.
 * 2. If a reference with similarity ≥ threshold is found:
 *    a. Compute delta = TT-round(new − reference, delta_eps).
 *    b. Store delta in `TensorNetworkStorageEngine` under a delta key.
 *    c. Record the reference in `StoredTensorRecord`.
 * 3. Otherwise, store as a new canonical tensor.
 * 4. Insert into `TensorFingerprintGraph` in all cases.
 *
 * ### Read path
 * 1. Load `StoredTensorRecord` for the requested tensor_id.
 * 2. If `is_canonical`, load directly from storage engine.
 * 3. If delta: load canonical reference + load delta; reconstruct = ref + delta.
 *
 * ### Thread safety
 * Write operations (`store`) use an exclusive lock.
 * Read operations (`retrieve`, `getStats`) use a shared lock.
 */
class TensorDeduplicationManager {
public:
    /**
     * @brief Construct with all required dependencies.
     *
     * @param storage      Storage engine for TT tensors.
     * @param fp_graph     Fingerprint graph for similarity search.
     * @param decomposer   Decomposer for delta computation.
     * @param cfg          Deduplication configuration.
     * @throws std::invalid_argument if any pointer is null.
     */
    TensorDeduplicationManager(
        std::shared_ptr<storage::TensorNetworkStorageEngine> storage,
        std::shared_ptr<TensorFingerprintGraph>              fp_graph,
        std::shared_ptr<storage::TensorTrainDecomposer>      decomposer,
        const DeduplicationConfig&                           cfg = {});

    ~TensorDeduplicationManager();

    // ─── Write ────────────────────────────────────────────────────────────

    /**
     * @brief Store a tensor with automatic deduplication.
     *
     * @param tensor_id  Unique identifier for the tensor.
     * @param data       Flat float32 tensor data.
     * @param mode_sizes Mode sizes.
     * @param tenant     Tenant for storage namespace.
     * @param collection Collection name.
     * @param field      Field name.
     * @return           Record describing how the tensor was stored.
     * @throws std::invalid_argument on shape mismatch.
     */
    StoredTensorRecord store(const std::string&              tensor_id,
                             const std::vector<float>&       data,
                             const std::vector<std::size_t>& mode_sizes,
                             const std::string&              tenant     = "",
                             const std::string&              collection = "",
                             const std::string&              field      = "");

    // ─── Read ─────────────────────────────────────────────────────────────

    /**
     * @brief Retrieve and reconstruct a tensor.
     *
     * If the tensor is delta-encoded, loads and adds the canonical reference.
     *
     * @return Reconstructed float32 tensor, or std::nullopt if not found.
     */
    std::optional<std::vector<float>> retrieve(const std::string& tensor_id) const;

    /**
     * @brief Get storage record for a tensor_id without decompression.
     */
    std::optional<StoredTensorRecord>
    getRecord(const std::string& tensor_id) const;

    // ─── Statistics ───────────────────────────────────────────────────────

    /**
     * @brief Aggregate deduplication statistics.
     */
    DeduplicationStats getStats() const noexcept;

    // ─── Graph lifecycle (persistence) ────────────────────────────────────

    /**
     * @brief Persist the fingerprint graph state to the storage backend.
     *
     * Exports the full node+edge snapshot via `TensorFingerprintGraph::exportPersistedGraph()`
     * together with deduplication records, canonical key mappings, and persisted
     * byte counters, then stores the serialized payload as a raw metadata blob
     * under `snapshot_key`. A subsequent call to `restoreGraph(snapshot_key)`
     * can reload the snapshot without requiring any TT-trains to be loaded from
     * storage.
     *
     * @param snapshot_key  Logical name for the snapshot (default: "__tfg_default__").
     * @return True on success.
     */
    bool snapshotGraph(const std::string& snapshot_key = "__tfg_default__");

    /**
     * @brief Restore fingerprint graph state from a previously persisted snapshot.
     *
     * Loads the raw metadata blob stored under `snapshot_key`, deserializes the
     * persisted graph payload, restores deduplication records/counters, and calls
     * `TensorFingerprintGraph::importPersistedGraph()` to atomically replace graph
     * state. After restore the graph supports `neighbours()` and `findSimilar()`
     * without re-inserting tensors, and canonical storage observers continue to
     * update/remove the restored graph state.
     *
     * @param snapshot_key  Logical name matching a prior `snapshotGraph()` call.
     * @return True on success, false if no snapshot exists for the given key.
     */
    bool restoreGraph(const std::string& snapshot_key = "__tfg_default__");

    // ─── Per-entry journal storage hooks (GraphIndex-backed journaling) ────

    /// Persist or overwrite one journal entry for a given snapshot key.
    /// @param snapshot_key  Active snapshot key.
    /// @param tensor_id     Tensor this entry covers (used as the entry key).
    /// @param payload       Serialized entry bytes (single-entry journal blob).
    /// @return true on success.
    using JournalEntryPersistFn = std::function<bool(
        std::string_view snapshot_key,
        std::string_view tensor_id,
        const std::vector<uint8_t>& payload)>;

    /// Delete a single journal entry for a tensor within a snapshot.
    using JournalEntryDeleteFn = std::function<bool(
        std::string_view snapshot_key,
        std::string_view tensor_id)>;

    /// Enumerate all journal entries for a snapshot.
    /// The inner callback receives (tensor_id, payload) for each stored entry.
    using JournalEntryEnumerateFn = std::function<void(
        std::string_view snapshot_key,
        std::function<void(std::string_view tensor_id,
                           const std::vector<uint8_t>& payload)>)>;

    /// Clear all journal entries for a snapshot key.
    using JournalEntryClearFn = std::function<bool(std::string_view snapshot_key)>;

    /**
     * @brief Inject per-entry journal storage hooks for GraphIndex-backed journaling.
     *
     * When all four hooks are non-null, individual journal entries are stored and
     * retrieved via the injected callbacks instead of the monolithic-blob approach.
     * This enables O(1) amortized journal writes (overwriting an existing entry
     * acts as natural compaction) without reading and rewriting the entire blob.
     *
     * Pass all nullptr (or call with no arguments) to revert to the blob approach.
     *
     * @param persist_fn   Stores/overwrites one entry.
     * @param delete_fn    Deletes one entry (optional; used on explicit removal).
     * @param enumerate_fn Enumerates all entries for a snapshot key.
     * @param clear_fn     Clears all entries for a snapshot key.
     */
    void setJournalEntryHooks(JournalEntryPersistFn persist_fn,
                               JournalEntryDeleteFn  delete_fn,
                               JournalEntryEnumerateFn enumerate_fn,
                               JournalEntryClearFn  clear_fn);

    /// Configuration.
    const DeduplicationConfig& config() const noexcept { return cfg_; }

private:
    std::shared_ptr<storage::TensorNetworkStorageEngine> storage_;
    std::shared_ptr<TensorFingerprintGraph>              fp_graph_;
    std::shared_ptr<storage::TensorTrainDecomposer>      decomposer_;
    DeduplicationConfig cfg_;

    mutable std::shared_mutex rw_mutex_;

    std::unordered_map<std::string, StoredTensorRecord> records_;
    std::unordered_map<std::string, std::string> key_to_tensor_id_;
    std::unordered_map<std::string, std::string> tensor_id_to_key_;

    std::atomic<std::size_t> total_bytes_stored_{0};
    std::atomic<std::size_t> bytes_saved_{0};

    // ─── Per-entry journal hooks ──────────────────────────────────────────
    mutable std::mutex journal_hooks_mutex_;
    JournalEntryPersistFn   journal_entry_persist_fn_;
    JournalEntryDeleteFn    journal_entry_delete_fn_;
    JournalEntryEnumerateFn journal_entry_enumerate_fn_;
    JournalEntryClearFn     journal_entry_clear_fn_;

    /// Returns true when all four per-entry journal hooks are set.
    bool hasJournalEntryHooks() const noexcept;

    // ─── Internal helpers ─────────────────────────────────────────────────

    /// Compute TT-train for delta = new - reference (reconstructed subtraction).
    storage::TTTrain computeDelta(const storage::TTTrain& ref,
                                  const storage::TTTrain& new_train) const;

    /// Add TT-train element-wise: result[i] = a[i] + b[i] (dense, then recompress).
    storage::TTTrain addTrains(const storage::TTTrain& a,
                               const storage::TTTrain& b) const;

    storage::TensorFieldKey makeKey(const std::string& tenant,
                                    const std::string& collection,
                                    const std::string& field) const;

    std::string makeKeyIndex(const storage::TensorFieldKey& key) const;
    void clearMappingForTensorIdLocked(const std::string& tensor_id);
    bool replayMutationJournal(const std::string& snapshot_key);
    void activateSnapshotKey(const std::string& snapshot_key) const;
    void clearMutationJournal(const std::string& snapshot_key) const;
    void persistUpsertJournalEntry(const StoredTensorRecord& record,
                                   std::size_t total_bytes_stored,
                                   std::size_t bytes_saved) const;
    void persistDeleteJournalEntry(const std::string& tensor_id,
                                   std::size_t total_bytes_stored,
                                   std::size_t bytes_saved) const;
};

} // namespace graph
} // namespace themis

// ============================================================================
// wireGraphIndexJournalHooks — non-member wiring helper
// ============================================================================

namespace themis {
namespace graph {

/**
 * @brief Wire per-entry journal hooks backed by GraphIndexManager edge storage.
 *
 * Each journal entry is stored as one GraphIndex edge from a virtual anchor
 * node (`"__tfgj_anchor__:<snapshot_key>"`) to the tensor id, with payload
 * encoded into edge fields.
 *
 * This is a durable alternative to the TNSE `putRawMetadata` approach. The
 * hook contract is identical: per-tensor journal entries are independently
 * stored and deleted without reading or rewriting the entire monolithic blob.
 *
 * ### Typical usage
 * @code
 *   wireGraphIndexJournalHooks(tdm, graph_idx, "__tfg_default__");
 *   tdm.snapshotGraph("__tfg_default__");
 * @endcode
 *
 * @param tdm           Dedup manager to configure.
 * @param graph_idx     GraphIndexManager used for adjacency-based listing.
 * @param snapshot_key  Active snapshot key (must match `snapshotGraph` call).
 */
void wireGraphIndexJournalHooks(TensorDeduplicationManager& tdm,
                                 GraphIndexManager&           graph_idx,
                                 const std::string&           snapshot_key);

} // namespace graph
} // namespace themis
