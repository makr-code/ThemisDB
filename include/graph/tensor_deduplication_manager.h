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
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

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

    ~TensorDeduplicationManager() = default;

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

    /// Configuration.
    const DeduplicationConfig& config() const noexcept { return cfg_; }

private:
    std::shared_ptr<storage::TensorNetworkStorageEngine> storage_;
    std::shared_ptr<TensorFingerprintGraph>              fp_graph_;
    std::shared_ptr<storage::TensorTrainDecomposer>      decomposer_;
    DeduplicationConfig cfg_;

    mutable std::shared_mutex rw_mutex_;

    std::unordered_map<std::string, StoredTensorRecord> records_;

    std::atomic<std::size_t> total_bytes_stored_{0};
    std::atomic<std::size_t> bytes_saved_{0};

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
};

} // namespace graph
} // namespace themis
