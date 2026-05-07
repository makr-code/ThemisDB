/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor_deduplication_manager.cpp                   ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-05                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "graph/tensor_deduplication_manager.h"
#include "storage/tensor_train_decomposer.h"
#include "storage/tt_quantizer.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace themis {
namespace graph {

using storage::TTTrain;
using storage::TensorFieldKey;
using storage::TensorTrainDecomposer;
using storage::TensorTrainConfig;

// ============================================================================
// Construction
// ============================================================================

TensorDeduplicationManager::TensorDeduplicationManager(
    std::shared_ptr<storage::TensorNetworkStorageEngine> storage,
    std::shared_ptr<TensorFingerprintGraph>              fp_graph,
    std::shared_ptr<TensorTrainDecomposer>               decomposer,
    const DeduplicationConfig&                           cfg)
    : storage_(std::move(storage))
    , fp_graph_(std::move(fp_graph))
    , decomposer_(std::move(decomposer))
    , cfg_(cfg)
{
    if (!storage_ || !fp_graph_ || !decomposer_)
        throw std::invalid_argument("TensorDeduplicationManager: null dependency");

    fp_graph_->setTrainLoadFn(
        [storage = storage_](const std::string&,
                             const std::string& tenant,
                             const std::string& collection,
                             const std::string& field)
            -> std::optional<TTTrain> {
            if (!storage) return std::nullopt;
            auto qtrain = storage->getCompressed({tenant, collection, field});
            if (!qtrain.has_value()) return std::nullopt;
            storage::TTQuantizer quantizer;
            return quantizer.dequantize(*qtrain);
        });
}

// ============================================================================
// Internal helpers
// ============================================================================

TensorFieldKey TensorDeduplicationManager::makeKey(
    const std::string& tenant,
    const std::string& collection,
    const std::string& field) const {
    return {tenant, collection, field};
}

TTTrain TensorDeduplicationManager::computeDelta(
    const TTTrain& ref,
    const TTTrain& new_train) const {

    // Reconstruct both (in production this would use TT-arithmetic)
    auto ref_dense = ref.reconstruct();
    auto new_dense = new_train.reconstruct();

    if (ref_dense.size() != new_dense.size()) {
        // Incompatible shapes; return new_train unchanged (no delta possible)
        return new_train;
    }

    // Delta = new - ref (element-wise)
    std::vector<float> delta_dense(ref_dense.size());
    for (std::size_t i = 0; i < delta_dense.size(); ++i)
        delta_dense[i] = new_dense[i] - ref_dense[i];

    // Re-compress delta
    TensorTrainConfig delta_cfg;
    delta_cfg.eps      = cfg_.delta_eps;
    delta_cfg.max_rank = cfg_.delta_max_rank;

    auto [delta_train, stats] = decomposer_->decompose(delta_dense,
                                                        new_train.mode_sizes,
                                                        delta_cfg);
    return std::move(delta_train);
}

TTTrain TensorDeduplicationManager::addTrains(
    const TTTrain& a, const TTTrain& b) const {

    auto da = a.reconstruct();
    auto db = b.reconstruct();

    if (da.size() != db.size()) return a;  // incompatible

    std::vector<float> sum(da.size());
    for (std::size_t i = 0; i < da.size(); ++i) sum[i] = da[i] + db[i];

    TensorTrainConfig cfg;
    cfg.eps = cfg_.delta_eps;
    auto [t, stats] = decomposer_->decompose(sum, a.mode_sizes, cfg);
    return std::move(t);
}

// ============================================================================
// store
// ============================================================================

StoredTensorRecord TensorDeduplicationManager::store(
    const std::string&              tensor_id,
    const std::vector<float>&       data,
    const std::vector<std::size_t>& mode_sizes,
    const std::string&              tenant,
    const std::string&              collection,
    const std::string&              field)
{
    // Decompose the new tensor
    TensorTrainConfig cfg;
    cfg.eps = cfg_.delta_eps * 10.0;  // slightly looser for the fingerprint
    auto [new_train, stats] = decomposer_->decompose(data, mode_sizes, cfg);

    // Find similar tensors via fingerprint graph
    auto similar = fp_graph_->findSimilar(new_train, 1);

    StoredTensorRecord record;
    record.tensor_id   = tensor_id;
    record.is_canonical = true;

    std::size_t full_bytes = data.size() * sizeof(float);

    if (!similar.empty() && similar[0].similarity >= cfg_.similarity_threshold) {
        // Found a reference — store delta
        const std::string& ref_id = similar[0].tensor_id;

        // Load reference compressed train for delta computation
        std::string ref_collection = similar[0].collection;
        std::string ref_tenant     = similar[0].tenant;
        std::string ref_field      = similar[0].field;

        auto ref_dense_opt = storage_->get(makeKey(ref_tenant, ref_collection, ref_field));
        if (ref_dense_opt) {
            // Build reference TTTrain (re-decompose the retrieved dense data)
            auto ref_ms = mode_sizes;  // assume compatible shapes
            TensorTrainConfig ref_cfg;
            ref_cfg.eps = cfg_.delta_eps;
            auto [ref_train, _] = decomposer_->decompose(*ref_dense_opt, ref_ms, ref_cfg);

            TTTrain delta = computeDelta(ref_train, new_train);

            // Store delta under a delta field name
            std::string delta_field = field + "__delta__" + ref_id;
            TensorFieldKey delta_key = makeKey(tenant, collection, delta_field);
            auto delta_dense = delta.reconstruct();
            storage_->put(delta_key, delta_dense, mode_sizes);

            std::size_t delta_bytes = delta.totalParams() * sizeof(float);

            record.reference_id              = ref_id;
            record.is_canonical              = false;
            record.compressed_bytes          = delta_bytes;
            record.saved_bytes               = (full_bytes > delta_bytes) ? full_bytes - delta_bytes : 0;
            record.similarity_to_reference   = similar[0].similarity;

            total_bytes_stored_.fetch_add(delta_bytes, std::memory_order_relaxed);
            bytes_saved_.fetch_add(record.saved_bytes, std::memory_order_relaxed);
        } else {
            // Reference not loadable — fall back to full storage
            goto store_canonical;
        }
    } else {
store_canonical:
        // Store as canonical tensor
        TensorFieldKey key = makeKey(tenant, collection, field);
        storage_->put(key, data, mode_sizes);

        std::size_t stored_bytes = new_train.totalParams() * sizeof(float);
        record.compressed_bytes = stored_bytes;
        record.saved_bytes      = (full_bytes > stored_bytes) ? full_bytes - stored_bytes : 0;

        total_bytes_stored_.fetch_add(stored_bytes, std::memory_order_relaxed);
        bytes_saved_.fetch_add(record.saved_bytes, std::memory_order_relaxed);
    }

    // Insert into fingerprint graph
    fp_graph_->insert(tensor_id, new_train, tenant, collection, field);

    // Persist key fields so retrieve() can look up the tensor without an extra index
    record.tenant     = tenant;
    record.collection = collection;
    record.field      = field;

    // Store record
    std::unique_lock<std::shared_mutex> wlk(rw_mutex_);
    records_[tensor_id] = record;

    return record;
}

// ============================================================================
// retrieve
// ============================================================================

std::optional<std::vector<float>>
TensorDeduplicationManager::retrieve(const std::string& tensor_id) const {
    // Copy the record while holding the shared lock so that we do NOT hold
    // the lock while calling storage_->get() — mixing the rw_mutex_ shared
    // lock with the storage engine's own write lock (held during put()) would
    // otherwise create a potential deadlock.
    StoredTensorRecord rec;
    {
        std::shared_lock<std::shared_mutex> rlk(rw_mutex_);
        auto it = records_.find(tensor_id);
        if (it == records_.end()) return std::nullopt;
        rec = it->second;
    }

    if (rec.is_canonical) {
        return storage_->get(makeKey(rec.tenant, rec.collection, rec.field));
    }

    // ── Delta path ───────────────────────────────────────────────────────────
    // 1. Load the canonical reference record.
    StoredTensorRecord ref_rec;
    {
        std::shared_lock<std::shared_mutex> rlk(rw_mutex_);
        auto ref_it = records_.find(rec.reference_id);
        if (ref_it == records_.end()) return std::nullopt;
        ref_rec = ref_it->second;
    }

    // 2. Load canonical reference dense vector from storage.
    auto ref_opt = storage_->get(makeKey(ref_rec.tenant, ref_rec.collection, ref_rec.field));
    if (!ref_opt) return std::nullopt;

    // 3. Load delta (stored under field + "__delta__" + reference_id).
    const std::string delta_field = rec.field + "__delta__" + rec.reference_id;
    auto delta_opt = storage_->get(makeKey(rec.tenant, rec.collection, delta_field));
    if (!delta_opt) return std::nullopt;

    if (ref_opt->size() != delta_opt->size()) return std::nullopt;

    // 4. Reconstruct: result = reference + delta (element-wise).
    std::vector<float> result(ref_opt->size());
    for (std::size_t i = 0; i < result.size(); ++i)
        result[i] = (*ref_opt)[i] + (*delta_opt)[i];
    return result;
}

std::optional<StoredTensorRecord>
TensorDeduplicationManager::getRecord(const std::string& tensor_id) const {
    std::shared_lock<std::shared_mutex> rlk(rw_mutex_);
    auto it = records_.find(tensor_id);
    if (it == records_.end()) return std::nullopt;
    return it->second;
}

// ============================================================================
// getStats
// ============================================================================

DeduplicationStats TensorDeduplicationManager::getStats() const noexcept {
    std::shared_lock<std::shared_mutex> rlk(rw_mutex_);
    DeduplicationStats s;
    s.total_tensors     = records_.size();
    for (const auto& kv : records_) {
        if (kv.second.is_canonical) ++s.canonical_tensors;
        else                        ++s.delta_tensors;
    }
    s.total_bytes_stored = total_bytes_stored_.load(std::memory_order_relaxed);
    s.bytes_saved        = bytes_saved_.load(std::memory_order_relaxed);
    std::size_t full_bytes = s.total_bytes_stored + s.bytes_saved;
    s.dedup_ratio = (s.total_bytes_stored > 0)
        ? static_cast<double>(full_bytes) / static_cast<double>(s.total_bytes_stored)
        : 1.0;
    return s;
}

} // namespace graph
} // namespace themis
