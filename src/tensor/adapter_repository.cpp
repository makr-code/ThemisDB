/*
 * ThemisDB | File: adapter_repository.cpp | Version: 1.0.0 | Last Modified: 2026-05-24 14:31:17
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 89/100 | Lines: 398
 * Gap Summary: total=19; TODO=1, Stub=13, Unimpl=0, Mock=1, Sim=4, Debt=0, C=1, H=11, M=5, L=0
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file tensor/adapter_repository.cpp
 * @brief AdapterRepository implementation (Adapter Sovereignty).
 *
 * ### Stub log
 * - AR-01  loadAdapter() copies TT-core data from the backend into a heap
 *          allocation instead of using mmap(MAP_SHARED) for zero-copy.
 *          See STUB #265 in STUB_INVENTORY.md.
 * - AR-02  findSimilarAdapters() uses column-mean fingerprint cosine similarity
 *          ( inherited from STUB #276 in TensorFingerprintGraph).  Full TT
 *          inner-product deferred to Q3 2027 (Phase 4 AdaLoRA bridge).
 *          See STUB #266 in STUB_INVENTORY.md.
 *
 * STUB/SIMULATION NOTE (AR-01):
 * Purpose: Provide a fully functional adapter store/load cycle so that
 *          upstream callers (e.g. GgmlTensorBridge) can integrate with
 *          AdapterRepository before the mmap path is ready.
 * Activation: Always — no compile-time flag required.
 * Production Delta: GgmlCoreDescriptor::train.cores[k].data is a heap copy,
 *                   not a pointer into a mmap'd page.  This means one extra
 *                   memcpy per loadAdapter() call and no mlock() protection.
 *                   The adapter switch latency is O(totalParams) CPU copy vs.
 *                   the ≤ 5 ms page-pin target.
 * Removal Plan: Q1 2027 — replace backend->get() + deserialize() with
 *               mmap(MAP_SHARED) on the RocksDB SST backing file + mlock()
 *               to pin the pages; return raw float* into the mmap region.
 *
 * STUB/SIMULATION NOTE (AR-02 / STUB #266):
 * Purpose: Expose findSimilarAdapters() before full TT inner-product sweep
 *          is available in TensorFingerprintGraph.
 * Activation: Only when setFingerprintGraph() has been called with a non-null
 *             graph; otherwise findSimilarAdapters() returns an empty vector.
 * Production Delta: Similarity scores are based on cosine distance of the G_0
 *                   column-mean fingerprint, not the exact TT inner-product.
 *                   For high-rank adapters (G_0 energy < 60% of Frobenius
 *                   norm) the ranking may deviate from the exact result.
 * Removal Plan: Q3 2027 — wire TTTrain::innerProduct() per-pair and add HNSW
 *               indexing over fingerprints for sub-linear search.
 */

#include "tensor/adapter_repository.h"

#include <algorithm>
#include <cstdio>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace tensor {

namespace {
std::mutex& mmapLoadFnMutex() { static std::mutex m; return m; }
AdapterRepository::MmapLoadFn& mmapLoadFnStorage() {
    static AdapterRepository::MmapLoadFn fn;
    return fn;
}

std::mutex& exactSimilarityFnMutex() { static std::mutex m; return m; }
AdapterRepository::ExactSimilarityFn& exactSimilarityFnStorage() {
    static AdapterRepository::ExactSimilarityFn fn;
    return fn;
}
} // namespace

/*static*/
void AdapterRepository::setMmapLoadFn(MmapLoadFn fn) {
    std::lock_guard<std::mutex> lk(mmapLoadFnMutex());
    mmapLoadFnStorage() = std::move(fn);
}

/*static*/
void AdapterRepository::clearMmapLoadFn() {
    std::lock_guard<std::mutex> lk(mmapLoadFnMutex());
    mmapLoadFnStorage() = {};
}

/*static*/
void AdapterRepository::setExactSimilarityFn(ExactSimilarityFn fn) {
    std::lock_guard<std::mutex> lk(exactSimilarityFnMutex());
    exactSimilarityFnStorage() = std::move(fn);
}

/*static*/
void AdapterRepository::clearExactSimilarityFn() {
    std::lock_guard<std::mutex> lk(exactSimilarityFnMutex());
    exactSimilarityFnStorage() = {};
}

// ============================================================================
// Constructor
// ============================================================================

AdapterRepository::AdapterRepository(
    std::shared_ptr<storage::ITensorStorageBackend> backend,
    std::string                                     tenant_id)
    : backend_(std::move(backend))
    , tenant_id_(std::move(tenant_id))
{
    if (!backend_) {
        throw std::invalid_argument(
            "AdapterRepository: backend must not be null");
    }
}

// ============================================================================
// makeKey()
// ============================================================================

std::string AdapterRepository::makeKey(const std::string& domain,
                                        const std::string& base_model_id) const {
    return "__adapters__:" + tenant_id_ + ":" + domain + ":" + base_model_id;
}

// ============================================================================
// store()
// ============================================================================

bool AdapterRepository::store(const std::string&      domain,
                               const std::string&      base_model_id,
                               const storage::TTTrain& adapter_train,
                               const AdapterMetadata&  /*meta*/) {
    if (domain.empty() || base_model_id.empty()) {
        return false;
    }

    // Serialise the TTTrain to bytes.
    const std::vector<uint8_t> bytes = adapter_train.serialize();
    if (bytes.empty()) {
        return false;
    }

    const std::string key = makeKey(domain, base_model_id);
    const bool ok = backend_->put(key, bytes);

    if (ok) {
        {
            std::unique_lock lock(stats_mutex_);
            ++stats_.total_adapters;
            stats_.total_param_bytes += adapter_train.totalParams() * sizeof(float);
        }

        // Register fingerprint in the graph (if wired).
        // Acquire graph_mutex_ AFTER releasing stats_mutex_ to preserve lock order.
        std::shared_ptr<TensorFingerprintGraph> graph;
        {
            std::shared_lock glock(graph_mutex_);
            graph = fingerprint_graph_;
        }
        if (graph) {
            graph->addAdapter(key, adapter_train, domain, base_model_id, tenant_id_);
        }
    }
    return ok;
}

// ============================================================================
// remove()
// ============================================================================

bool AdapterRepository::remove(const std::string& domain,
                                const std::string& base_model_id) {
    const std::string key = makeKey(domain, base_model_id);
    const bool existed = backend_->get(key).has_value();
    if (!existed) return false;

    const bool ok = backend_->del(key);
    if (ok) {
        {
            std::unique_lock lock(stats_mutex_);
            if (stats_.total_adapters > 0) --stats_.total_adapters;
        }

        // Deregister fingerprint from the graph (if wired).
        std::shared_ptr<TensorFingerprintGraph> graph;
        {
            std::shared_lock glock(graph_mutex_);
            graph = fingerprint_graph_;
        }
        if (graph) {
            graph->removeAdapter(key);
        }
    }
    return ok;
}

// ============================================================================
// loadAdapter()
// ============================================================================

GgmlCoreDescriptor
AdapterRepository::loadAdapter(const std::string& domain,
                                 const std::string& base_model_id) const {
    GgmlCoreDescriptor desc;
    desc.tenant_id      = tenant_id_;
    desc.domain         = domain;
    desc.base_model_id  = base_model_id;
    desc.adapter_key    = makeKey(domain, base_model_id);

    // Delegate to injected mmap-style loader backend when available (STUB #265).
    MmapLoadFn mmap_fn_copy;
    {
        std::lock_guard<std::mutex> lk(mmapLoadFnMutex());
        mmap_fn_copy = mmapLoadFnStorage();
    }
    if (mmap_fn_copy) {
        try {
            auto mapped = mmap_fn_copy(
                tenant_id_, domain, base_model_id, desc.adapter_key, backend_);
            if (mapped.tenant_id.empty()) mapped.tenant_id = tenant_id_;
            if (mapped.domain.empty()) mapped.domain = domain;
            if (mapped.base_model_id.empty()) mapped.base_model_id = base_model_id;
            if (mapped.adapter_key.empty()) mapped.adapter_key = desc.adapter_key;
            {
                std::unique_lock lock(stats_mutex_);
                if (mapped.valid) ++stats_.load_hits;
                else ++stats_.load_misses;
            }
            return mapped;
        } catch (const std::exception& e) {
            std::fprintf(stderr,
                "[ThemisDB][WARN] AdapterRepository::loadAdapter: injected "
                "MmapLoadFn failed (%s); using heap-deserialize fallback.\n",
                e.what());
            // Fail-closed to existing heap-deserialize fallback path below.
        } catch (...) {
            std::fprintf(stderr,
                "[ThemisDB][WARN] AdapterRepository::loadAdapter: injected "
                "MmapLoadFn failed (unknown exception); using heap-deserialize "
                "fallback.\n");
            // Fail-closed to existing heap-deserialize fallback path below.
        }
    }

    // STUB/SIMULATION NOTE (AR-01):
    // Retrieves bytes from the backend and deserialises into desc.train.
    // Production path: mmap() + mlock() for zero-copy page-pinning.
    const auto raw = backend_->get(desc.adapter_key);
    if (!raw.has_value() || raw->empty()) {
        std::unique_lock lock(stats_mutex_);
        ++stats_.load_misses;
        return desc;  // valid = false
    }

    auto train_opt = storage::TTTrain::deserialize(*raw);
    if (!train_opt.has_value()) {
        std::unique_lock lock(stats_mutex_);
        ++stats_.load_misses;
        return desc;
    }

    desc.train = std::move(*train_opt);
    desc.valid = true;

    {
        std::unique_lock lock(stats_mutex_);
        ++stats_.load_hits;
    }
    return desc;
}

// ============================================================================
// listDomains()
// ============================================================================

std::vector<std::string> AdapterRepository::listDomains() const {
    const std::string prefix = "__adapters__:" + tenant_id_ + ":";
    const auto keys = backend_->listKeys(prefix);

    std::vector<std::string> domains;
    domains.reserve(keys.size());

    for (const auto& key : keys) {
        // Key format: __adapters__:<tenant>:<domain>:<base_model_id>
        // Strip the prefix and extract the domain part.
        if (key.size() <= prefix.size()) continue;
        const std::string rest = key.substr(prefix.size());
        const auto sep = rest.find(':');
        if (sep == std::string::npos) continue;
        domains.push_back(rest.substr(0, sep));
    }

    // Sort and deduplicate.
    std::sort(domains.begin(), domains.end());
    domains.erase(std::unique(domains.begin(), domains.end()), domains.end());
    return domains;
}

// ============================================================================
// listAdapters()
// ============================================================================

std::vector<std::pair<std::string, std::string>>
AdapterRepository::listAdapters() const {
    const std::string prefix = "__adapters__:" + tenant_id_ + ":";
    const auto keys = backend_->listKeys(prefix);

    std::vector<std::pair<std::string, std::string>> result;
    result.reserve(keys.size());

    for (const auto& key : keys) {
        if (key.size() <= prefix.size()) continue;
        const std::string rest = key.substr(prefix.size());
        const auto sep = rest.find(':');
        if (sep == std::string::npos || sep + 1 >= rest.size()) continue;
        result.emplace_back(rest.substr(0, sep), rest.substr(sep + 1));
    }

    std::sort(result.begin(), result.end());
    return result;
}

// ============================================================================
// setFingerprintGraph()
// ============================================================================

void AdapterRepository::setFingerprintGraph(
        std::shared_ptr<TensorFingerprintGraph> graph) {
    std::unique_lock lock(graph_mutex_);
    fingerprint_graph_ = std::move(graph);
}

// ============================================================================
// findSimilarAdapters()
// ============================================================================

std::vector<SimilarityResult>
AdapterRepository::findSimilarAdapters(const std::string& domain,
                                        const std::string& base_model_id,
                                        std::size_t        k) const {
    if (k == 0 || domain.empty() || base_model_id.empty()) {
        return {};
    }

    const std::string key = makeKey(domain, base_model_id);

    // Delegate to injected exact-similarity backend when available (STUB #266).
    ExactSimilarityFn exact_fn_copy;
    {
        std::lock_guard<std::mutex> lk(exactSimilarityFnMutex());
        exact_fn_copy = exactSimilarityFnStorage();
    }
    if (exact_fn_copy) {
        try {
            return exact_fn_copy(key, k, backend_);
        } catch (const std::exception& e) {
            std::fprintf(stderr,
                "[ThemisDB][WARN] AdapterRepository::findSimilarAdapters: "
                "injected ExactSimilarityFn failed (%s); using fingerprint "
                "fallback.\n",
                e.what());
            // Fail-closed to fingerprint-graph path below.
        } catch (...) {
            std::fprintf(stderr,
                "[ThemisDB][WARN] AdapterRepository::findSimilarAdapters: "
                "injected ExactSimilarityFn failed (unknown exception); using "
                "fingerprint fallback.\n");
            // Fail-closed to fingerprint-graph path below.
        }
    }

    // STUB/SIMULATION NOTE (AR-02 / STUB #266):
    // Delegates to TensorFingerprintGraph::findSimilar() which uses
    // column-mean fingerprint cosine similarity (not full TT inner-product).
    std::shared_ptr<TensorFingerprintGraph> graph;
    {
        std::shared_lock lock(graph_mutex_);
        graph = fingerprint_graph_;
    }
    if (!graph) {
        return {};
    }

    return graph->findSimilar(key, k);
}

// ============================================================================
// stats()
// ============================================================================

AdapterRepository::RepositoryStats
AdapterRepository::stats() const noexcept {
    std::shared_lock lock(stats_mutex_);
    return stats_;
}

} // namespace tensor
} // namespace themis
