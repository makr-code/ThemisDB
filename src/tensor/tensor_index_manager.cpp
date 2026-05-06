/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor/tensor_index_manager.cpp                    ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-05                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                     ║
    • Maturity Level:  🟡 EXPERIMENTAL                                 ║
    • Open Issues:     Stubs: 2 (TIM-01, TIM-02)                       ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tensor/tensor_index_manager.cpp
 * @brief TensorIndexManager implementation.
 *
 * ### Stub log
 * - TIM-01  `ggmlCorePtrs()` — mmap bridge to GGML (Phase 3, Q1 2027)
 * - TIM-02  `dropTenantIndexes()` RocksDB prefix-delete (Phase 2, Q4 2026)
 */

#include "tensor/tensor_index_manager.h"
#include "utils/logger.h"

#include <shared_mutex>
#include <stdexcept>

// The FlatTensorIndex concrete class is defined in tensor_index.cpp and is
// not exposed in the header.  We forward-create it here via a factory lambda
// to keep the concrete type internal to this translation unit.
namespace themis::tensor {

// ---------------------------------------------------------------------------
// Forward-declare the concrete class from tensor_index.cpp so we can
// instantiate it.  In a real build both TUs are linked together.
// ---------------------------------------------------------------------------
class FlatTensorIndex;

} // pre-declare

#include "tensor/tensor_index.cpp"   // pull in FlatTensorIndex definition
                                      // (acceptable for single-TU builds;
                                      //  in multi-TU builds remove this line
                                      //  and link separately)

namespace themis {
namespace tensor {

// ============================================================================
// TensorIndexManager — implementation
// ============================================================================

// static factory
std::shared_ptr<TensorIndexManager>
TensorIndexManager::create(std::shared_ptr<RocksDBWrapper> db) {
    return std::shared_ptr<TensorIndexManager>(
        new TensorIndexManager(std::move(db)));
}

TensorIndexManager::TensorIndexManager(std::shared_ptr<RocksDBWrapper> db)
    : db_(std::move(db)) {}

TensorIndexManager::~TensorIndexManager() = default;

// -----------------------------------------------------------------------
// Routing decision
// -----------------------------------------------------------------------

storage::TensorRouter::Route
TensorIndexManager::routeFor(const std::string& /*tenant_id*/,
                              const std::string& /*collection*/,
                              const std::string& /*field*/,
                              size_t dim,
                              size_t num_vectors) const {
    // Use the static heuristic (no TT-SVD pilot; no engine required).
    // κ estimates are dimension-derived following HNSW_FAISS_TT_BOUNDARY_ANALYSIS §3.2:
    //   dim ≥ 4096 → κ ≈ 4.5  (LLM attention, geodata — very compressible)
    //   dim ≥ 1024 → κ ≈ 2.8  (dense embeddings — moderately compressible)
    //   dim  < 1024 → κ ≈ 1.2  (low-dim / sparse — unlikely to compress well)
    storage::TensorRouter::DataProfile p;
    p.dim            = dim;
    p.num_vectors    = num_vectors;
    p.kappa_estimate = (dim >= 4096) ? 4.5 : (dim >= 1024 ? 2.8 : 1.2);
    return storage::TensorRouter::decide(p);
}

// -----------------------------------------------------------------------
// Index lifecycle
// -----------------------------------------------------------------------

ITensorIndex* TensorIndexManager::createIndex(const std::string& tenant_id,
                                               const std::string& collection,
                                               const std::string& field,
                                               size_t /*dim*/,
                                               size_t /*max_rank*/,
                                               double /*epsilon*/) {
    IndexHandle h;
    h.tenant_id  = tenant_id;
    h.collection = collection;
    h.field      = field;
    h.route      = storage::TensorRouter::Route::TENSOR_TRAIN;

    {
        std::shared_lock rlock(registry_mutex_);
        auto it = indexes_.find(h.key());
        if (it != indexes_.end()) return it->second.get();
    }

    auto idx = std::make_unique<FlatTensorIndex>();
    ITensorIndex* raw = idx.get();
    h.index = raw;

    std::unique_lock wlock(registry_mutex_);
    indexes_.emplace(h.key(), std::move(idx));
    handles_.emplace(h.key(), std::move(h));
    return raw;
}

ITensorIndex* TensorIndexManager::getIndex(const std::string& tenant_id,
                                             const std::string& collection,
                                             const std::string& field) const {
    IndexHandle probe;
    probe.tenant_id  = tenant_id;
    probe.collection = collection;
    probe.field      = field;

    std::shared_lock lock(registry_mutex_);
    auto it = indexes_.find(probe.key());
    return (it != indexes_.end()) ? it->second.get() : nullptr;
}

bool TensorIndexManager::dropIndex(const std::string& tenant_id,
                                    const std::string& collection,
                                    const std::string& field) {
    IndexHandle probe;
    probe.tenant_id  = tenant_id;
    probe.collection = collection;
    probe.field      = field;

    std::unique_lock lock(registry_mutex_);
    bool found = indexes_.erase(probe.key()) > 0;
    handles_.erase(probe.key());
    return found;
}

void TensorIndexManager::dropTenantIndexes(const std::string& tenant_id) {
    // STUB/SIMULATION NOTE:
    // Purpose: iterate and erase all indexes belonging to tenant_id
    // Activation: always
    // Production Delta: real impl also issues RocksDB prefix-delete for
    //   `__ttmgr__:<tenant_id>:` (requires Phase 2 persistence wire-up TIM-02)
    // Removal Plan: Phase 2 Q4 2026

    std::unique_lock lock(registry_mutex_);
    const std::string prefix = "__ttmgr__:" + tenant_id + ":";
    for (auto it = indexes_.begin(); it != indexes_.end(); ) {
        if (it->first.substr(0, prefix.size()) == prefix) {
            handles_.erase(it->first);
            it = indexes_.erase(it);
        } else {
            ++it;
        }
    }
    THEMIS_WARN("TensorIndexManager::dropTenantIndexes: RocksDB prefix-delete "
                "not yet implemented (TIM-02, Phase 2 Q4 2026)");
}

// -----------------------------------------------------------------------
// Introspection
// -----------------------------------------------------------------------

std::vector<IndexHandle> TensorIndexManager::listIndexes() const {
    std::shared_lock lock(registry_mutex_);
    std::vector<IndexHandle> out;
    out.reserve(handles_.size());
    for (const auto& [k, h] : handles_) out.push_back(h);
    return out;
}

std::vector<IndexHandle>
TensorIndexManager::listIndexes(const std::string& tenant_id) const {
    std::shared_lock lock(registry_mutex_);
    const std::string prefix = "__ttmgr__:" + tenant_id + ":";
    std::vector<IndexHandle> out;
    for (const auto& [k, h] : handles_) {
        if (k.substr(0, prefix.size()) == prefix) out.push_back(h);
    }
    return out;
}

TensorIndexStats TensorIndexManager::aggregateStats() const {
    std::shared_lock lock(registry_mutex_);
    TensorIndexStats agg;
    if (indexes_.empty()) return agg;

    for (const auto& [k, idx] : indexes_) {
        auto s = idx->stats();
        agg.num_vectors      += s.num_vectors;
        agg.storage_bytes    += s.storage_bytes;
        agg.total_searches   += s.total_searches;
        agg.avg_tt_rank      += s.avg_tt_rank;
        agg.avg_compress_ratio += s.avg_compress_ratio;
    }
    size_t n = indexes_.size();
    agg.avg_tt_rank       /= n;
    agg.avg_compress_ratio /= static_cast<double>(n);
    return agg;
}

// -----------------------------------------------------------------------
// GGML bridge (stub — Phase 3)
//
// STUB/SIMULATION NOTE:
// Purpose: expose raw TT-core pointers for zero-copy GGML injection
// Activation: called by GgmlTensorBridge when THEMIS_ENABLE_GGML_BRIDGE set
// Production Delta: real impl pins pages via mmap() and registers fence
// Removal Plan: implement in Phase 3 (Q1 2027) — TIM-01
// -----------------------------------------------------------------------

std::vector<std::pair<const float*, size_t>>
TensorIndexManager::ggmlCorePtrs(const std::string& tenant_id,
                                  const std::string& collection,
                                  const std::string& field,
                                  int64_t id) const {
    THEMIS_WARN("TensorIndexManager::ggmlCorePtrs() — mmap bridge stub "
                "(TIM-01, Phase 3 Q1 2027)");

    auto* idx = getIndex(tenant_id, collection, field);
    if (!idx) return {};

    const storage::TTTrain* train = idx->get(id);
    if (!train) return {};

    std::vector<std::pair<const float*, size_t>> ptrs;
    ptrs.reserve(train->cores.size());
    for (const auto& core : train->cores) {
        ptrs.emplace_back(core.data.data(),
                          core.data.size() * sizeof(float));
    }
    return ptrs;
}

} // namespace tensor
} // namespace themis
