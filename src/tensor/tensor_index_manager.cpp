/**
 * @file tensor_index_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 93/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "tensor/tensor_index_manager.h"
#include "tensor/tensor_mmap_bridge.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/logger.h"

#include <filesystem>
#include <fstream>
#include <shared_mutex>
#include <stdexcept>
#include <unordered_map>

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

namespace {
[[nodiscard]] std::string makeLegacyBridgeKey(const std::string& tenant_id,
                                              const std::string& collection,
                                              const std::string& field,
                                              int64_t id) {
    return tenant_id + ":" + collection + ":" + field + ":" + std::to_string(id);
}

// Bounded concurrency control for concurrent workload hardening
constexpr size_t kMaxConcurrentCreates = 256;
constexpr size_t kMaxLegacyCacheSize   = 1000;
}

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
    //   dim ≥ 1024 → κ ≈ 1.5  (dense embeddings — moderately compressible)
    //   dim  < 1024 → κ ≈ 1.2  (low-dim / sparse — unlikely to compress well)
    storage::TensorRouter::DataProfile p;
    p.dim            = dim;
    p.num_vectors    = num_vectors;
    p.kappa_estimate = (dim >= 4096) ? 4.5 : (dim >= 1024 ? 1.5 : 1.2);
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
    // Bounded concurrency control: wait if too many creates in flight
    while (pending_operations_.load(std::memory_order_acquire) >= kMaxConcurrentCreates) {
        std::this_thread::yield();
    }
    pending_operations_.fetch_add(1, std::memory_order_release);
    
    struct OpGuard {
        std::atomic<size_t>& op_count;
        ~OpGuard() { op_count.fetch_sub(1, std::memory_order_release); }
    } op_guard{pending_operations_};

    IndexHandle h;
    h.tenant_id  = tenant_id;
    h.collection = collection;
    h.field      = field;
    h.route      = storage::TensorRouter::Route::TENSOR_TRAIN;

    {
        std::shared_lock rlock(registry_mutex_);
        auto it = indexes_.find(h.key());
        if (it != indexes_.end()) {
          return it->second.get();
        }
    }

    auto idx = std::make_unique<FlatTensorIndex>();

    // Restore persisted data when a data directory is configured.
    if (!data_dir_.empty()) {
        const std::string path = indexFilePath(h.key());
        if (std::filesystem::exists(path)) {
            if (!idx->load(path)) {
                THEMIS_WARN("TensorIndexManager: failed to load index from '{}'", path);
            }
        }
    }

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

    {
        std::unique_lock lock(registry_mutex_);
        bool found = indexes_.erase(probe.key()) > 0;
        handles_.erase(probe.key());
        if (!found) {
          return false;
        }
    }

    // Remove the persisted index file when a data directory is configured.
    if (!data_dir_.empty()) {
        const std::string path = indexFilePath(probe.key());
        std::error_code ec;
        std::filesystem::remove(path, ec);
        if (ec) {
            THEMIS_WARN("TensorIndexManager: could not remove index file '{}': {}",
                        path, ec.message());
        }
    }

    {
        std::lock_guard<std::mutex> lock(legacy_bridge_mutex_);
        const std::string prefix = tenant_id + ":" + collection + ":" + field + ":";
        for (auto it = legacy_bridge_cache_.begin(); it != legacy_bridge_cache_.end();) {
            if (it->first.rfind(prefix, 0) == 0) {
                it = legacy_bridge_cache_.erase(it);
            } else {
                ++it;
            }
        }
    }

    return true;
}

void TensorIndexManager::dropTenantIndexes(const std::string& tenant_id) {
    const std::string prefix = "__ttmgr__:" + tenant_id + ":";

    {
        std::unique_lock lock(registry_mutex_);
        for (auto it = indexes_.begin(); it != indexes_.end(); ) {
            if (it->first.substr(0, prefix.size()) == prefix) {
                handles_.erase(it->first);
                it = indexes_.erase(it);
            } else {
                ++it;
            }
        }
    }  // registry_mutex_ released before any I/O

    // Purge persisted TT data from RocksDB when a backing store is available.
    // Two key prefixes are cleaned up:
    //   __ttmgr__:<tenant>:  — index handle / metadata rows
    //   __ttidx__:<tenant>:  — serialised core rows (key schema §TTI)
    if (db_) {
        const std::string idx_prefix = "__ttidx__:" + tenant_id + ":";

        std::vector<std::string> to_del;
        for (const auto& pfx : {prefix, idx_prefix}) {
            db_->scanPrefix(pfx, [&](std::string_view k, std::string_view) -> bool {
                to_del.emplace_back(k);
                return true;
            });
        }
        for (const auto& key : to_del) {
            db_->del(key);
        }
    }

    {
        std::lock_guard<std::mutex> lock(legacy_bridge_mutex_);
        const std::string bridge_prefix = tenant_id + ":";
        for (auto it = legacy_bridge_cache_.begin(); it != legacy_bridge_cache_.end();) {
            if (it->first.rfind(bridge_prefix, 0) == 0) {
                it = legacy_bridge_cache_.erase(it);
            } else {
                ++it;
            }
        }
    }
}

// -----------------------------------------------------------------------
// Introspection
// -----------------------------------------------------------------------

std::vector<IndexHandle> TensorIndexManager::listIndexes() const {
    std::shared_lock lock(registry_mutex_);
    std::vector<IndexHandle> out;
    out.reserve(handles_.size());
    for (const auto& [k, h] : handles_) {
      out.push_back(h);
    }
    return out;
}

std::vector<IndexHandle>
TensorIndexManager::listIndexes(const std::string& tenant_id) const {
    std::shared_lock lock(registry_mutex_);
    const std::string prefix = "__ttmgr__:" + tenant_id + ":";
    std::vector<IndexHandle> out;
    for (const auto& [k, h] : handles_) {
        if (k.substr(0, prefix.size()) == prefix) {
          out.push_back(h);
        }
    }
    return out;
}

TensorIndexStats TensorIndexManager::aggregateStats() const {
    std::shared_lock lock(registry_mutex_);
    TensorIndexStats agg;
    if (indexes_.empty()) {
      return agg;
    }

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
// File-based persistence
// -----------------------------------------------------------------------

void TensorIndexManager::setDataDir(const std::string& dir) {
    data_dir_ = dir;
}

std::string TensorIndexManager::indexFilePath(const std::string& key) const {
    // Replace characters unsafe on most filesystems with '_'.
    std::string escaped;
    escaped.reserve(key.size());
    for (char c : key) {
        escaped += (c == ':' || c == '/' || c == '\\') ? '_' : c;
    }
    return data_dir_ + "/" + escaped + ".ttidx";
}

size_t TensorIndexManager::flushAll() {
    // Snapshot key→index pairs under read lock so I/O runs without holding it.
    std::vector<std::pair<std::string, ITensorIndex*>> snapshot;
    {
        std::shared_lock lock(registry_mutex_);
        snapshot.reserve(indexes_.size());
        for (const auto& [k, idx] : indexes_) {
            snapshot.emplace_back(k, idx.get());
        }
    }

    size_t saved = 0;
    for (const auto& [key, idx] : snapshot) {
        const std::string path = indexFilePath(key);
        if (idx->save(path)) {
            ++saved;
        } else {
            THEMIS_WARN("TensorIndexManager::flushAll(): failed to save '{}'", path);
        }
    }
    return saved;
}

// -----------------------------------------------------------------------
// mapCores() — Phase 3 mmap-pinned TT-core bridge (TIM-01, STUB #176)
// -----------------------------------------------------------------------

std::unique_ptr<TensorMmapBridge>
TensorIndexManager::mapCores(const std::string& tenant_id,
                              const std::string& collection,
                              const std::string& field,
                              int64_t id) const {
    auto* idx = getIndex(tenant_id, collection, field);
    if (!idx) {
      return nullptr;
    }

    const storage::TTTrain* train = idx->get(id);
    if (!train) {
      return nullptr;
    }

    return TensorMmapBridge::buildFromTrain(*train);
}

// -----------------------------------------------------------------------
// ggmlCorePtrs() — raw-pointer legacy bridge (kept for backward compat)
//
// Legacy compatibility path:
// preserve raw-pointer API while internally pinning cores through a cached
// TensorMmapBridge per vector ID.
// -----------------------------------------------------------------------

std::vector<std::pair<const float*, size_t>>
TensorIndexManager::ggmlCorePtrs(const std::string& tenant_id,
                                  const std::string& collection,
                                  const std::string& field,
                                  int64_t id) const {
    auto bridge = mapCores(tenant_id, collection, field, id);
    if (!bridge) return {};

    std::vector<std::pair<const float*, size_t>> ptrs;
    ptrs.reserve(bridge->slices().size());
    for (const auto& slice : bridge->slices()) {
        if (!slice.data || slice.bytes == 0) {
            continue;
        }
        ptrs.emplace_back(slice.data, slice.bytes);
    }

    if (!ptrs.empty()) {
        std::lock_guard<std::mutex> lock(legacy_bridge_mutex_);
        
        // Capacity guard for legacy bridge cache: when usage exceeds 90%,
        // evict arbitrary entries from the unordered_map until back to 50%.
        const size_t threshold_evict = (kMaxLegacyCacheSize * 9) / 10;
        if (legacy_bridge_cache_.size() >= threshold_evict) {
            // Evict 50% of entries to restore breathing room
            const size_t target_size = kMaxLegacyCacheSize / 2;
            while (legacy_bridge_cache_.size() > target_size) {
                auto it = legacy_bridge_cache_.begin();
                if (it != legacy_bridge_cache_.end()) {
                    legacy_bridge_cache_.erase(it);
                }
            }
        }
        
        legacy_bridge_cache_[makeLegacyBridgeKey(tenant_id, collection, field, id)] =
            std::shared_ptr<TensorMmapBridge>(std::move(bridge));
    }

    return ptrs;
}

} // namespace tensor
} // namespace themis
