/**
 * @file tensor_deduplication_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

struct DeduplicationConfig {
    double similarity_threshold = 0.999;

    double delta_eps = 0.001;

    std::size_t delta_max_rank = 16;

    bool allow_full_storage_fallback = true;
};

// ============================================================================
// StoredTensorRecord
// ============================================================================

struct StoredTensorRecord {
    std::string tensor_id;

    std::string reference_id;

    bool is_canonical = true;

    std::size_t compressed_bytes = 0;

    std::size_t saved_bytes = 0;

    double similarity_to_reference = 0.0;

    // ─── Storage key fields (populated by TensorDeduplicationManager::store) ──

    std::string tenant;

    std::string collection;

    std::string field;
};

// ============================================================================
// DeduplicationStats
// ============================================================================

struct DeduplicationStats {
    std::size_t total_tensors      = 0;
    std::size_t canonical_tensors  = 0;
    std::size_t delta_tensors      = 0;
    std::size_t total_bytes_stored = 0;
    std::size_t bytes_saved        = 0;
    double      dedup_ratio        = 1.0; ///< full_bytes / stored_bytes

    void reset() noexcept { *this = {}; }
};

// ============================================================================
// TensorDeduplicationManager
// ============================================================================

class TensorDeduplicationManager {
public:
    TensorDeduplicationManager(
        std::shared_ptr<storage::TensorNetworkStorageEngine> storage,
        std::shared_ptr<TensorFingerprintGraph>              fp_graph,
        std::shared_ptr<storage::TensorTrainDecomposer>      decomposer,
        const DeduplicationConfig&                           cfg = {});

    ~TensorDeduplicationManager();

    // ─── Write ────────────────────────────────────────────────────────────

    StoredTensorRecord store(const std::string&              tensor_id,
                             const std::vector<float>&       data,
                             const std::vector<std::size_t>& mode_sizes,
                             const std::string&              tenant     = "",
                             const std::string&              collection = "",
                             const std::string&              field      = "");

    /**
     * @brief ─── Read ─────────────────────────────────────────────────────────────
     * @param[in] tensor_id Identifier of the tensor.
     * @return Return value.
     */

    std::optional<std::vector<float>> retrieve(const std::string& tensor_id) const;

    std::optional<StoredTensorRecord>
    getRecord(const std::string& tensor_id) const;

    /**
     * @brief ─── Statistics ───────────────────────────────────────────────────────
     * @return Return value.
     * @note Exception safety: noexcept.
     */

    DeduplicationStats getStats() const noexcept;

    // ─── Graph lifecycle (persistence) ────────────────────────────────────

    bool snapshotGraph(const std::string& snapshot_key = "__tfg_default__");

    bool restoreGraph(const std::string& snapshot_key = "__tfg_default__");

    // ─── Per-entry journal storage hooks (GraphIndex-backed journaling) ────

    using JournalEntryPersistFn = std::function<bool(
        std::string_view snapshot_key,
        std::string_view tensor_id,
        const std::vector<uint8_t>& payload)>;

    using JournalEntryDeleteFn = std::function<bool(
        std::string_view snapshot_key,
        std::string_view tensor_id)>;

    using JournalEntryEnumerateFn = std::function<void(
        std::string_view snapshot_key,
        std::function<void(std::string_view tensor_id,
                           const std::vector<uint8_t>& payload)>)>;

    using JournalEntryClearFn = std::function<bool(std::string_view snapshot_key)>;

    /**
     * @brief Set Journal Entry Hooks.
     * @param[in] persist_fn Input parameter.
     * @param[in] delete_fn Input parameter.
     * @param[in] enumerate_fn Input parameter.
     * @param[in] clear_fn Input parameter.
     */
    void setJournalEntryHooks(JournalEntryPersistFn persist_fn,
                               JournalEntryDeleteFn  delete_fn,
                               JournalEntryEnumerateFn enumerate_fn,
                               JournalEntryClearFn  clear_fn);

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

    /**
     * @brief Has Journal Entry Hooks.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool hasJournalEntryHooks() const noexcept;

    /**
     * @brief ─── Internal helpers ─────────────────────────────────────────────────
     * @param[in] ref Input parameter.
     * @param[in] new_train Input parameter.
     * @return Return value.
     */

    storage::TTTrain computeDelta(const storage::TTTrain& ref,
                                  const storage::TTTrain& new_train) const;

    /**
     * @brief Add Trains.
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return Return value.
     */
    storage::TTTrain addTrains(const storage::TTTrain& a,
                               const storage::TTTrain& b) const;

    /**
     * @brief Make Key.
     * @param[in] tenant Input parameter.
     * @param[in] collection Input parameter.
     * @param[in] field Input parameter.
     * @return Return value.
     */
    storage::TensorFieldKey makeKey(const std::string& tenant,
                                    const std::string& collection,
                                    const std::string& field) const;

    /**
     * @brief Make Key Index.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    std::string makeKeyIndex(const storage::TensorFieldKey& key) const;
    /**
     * @brief Clear Mapping For Tensor Id Locked.
     * @param[in] tensor_id Identifier of the tensor.
     */
    void clearMappingForTensorIdLocked(const std::string& tensor_id);
    /**
     * @brief Replay Mutation Journal.
     * @param[in] snapshot_key Input parameter.
     * @return True when the operation succeeds.
     */
    bool replayMutationJournal(const std::string& snapshot_key);
    /**
     * @brief Activate Snapshot Key.
     * @param[in] snapshot_key Input parameter.
     */
    void activateSnapshotKey(const std::string& snapshot_key) const;
    /**
     * @brief Clear Mutation Journal.
     * @param[in] snapshot_key Input parameter.
     */
    void clearMutationJournal(const std::string& snapshot_key) const;
    /**
     * @brief Persist Upsert Journal Entry.
     * @param[in] record Input parameter.
     * @param[in] total_bytes_stored Input parameter.
     * @param[in] bytes_saved Input parameter.
     */
    void persistUpsertJournalEntry(const StoredTensorRecord& record,
                                   std::size_t total_bytes_stored,
                                   std::size_t bytes_saved) const;
    /**
     * @brief Persist Delete Journal Entry.
     * @param[in] tensor_id Identifier of the tensor.
     * @param[in] total_bytes_stored Input parameter.
     * @param[in] bytes_saved Input parameter.
     */
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
 * @brief Wire Graph Index Journal Hooks.
 * @param[in,out] tdm Input/output parameter.
 * @param[in,out] graph_idx Input/output parameter.
 * @param[in] snapshot_key Input parameter.
 */
void wireGraphIndexJournalHooks(TensorDeduplicationManager& tdm,
                                 GraphIndexManager&           graph_idx,
                                 const std::string&           snapshot_key);

} // namespace graph
} // namespace themis
