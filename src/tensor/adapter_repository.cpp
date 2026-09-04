/**
 * @file adapter_repository.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: total=19; TODO=1, Stub=13, Unimpl=0, Mock=1, Sim=4, Debt=0, C=0, H=6, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "tensor/adapter_repository.h"
#include "index/ann_frontdoor.h"

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

themis::index::AnnScopeKind scopeKindForDomain(const std::string& domain) {
    if (domain.rfind("pkg:", 0) == 0 || domain.rfind("package:", 0) == 0) {
        return themis::index::AnnScopeKind::Package;
    }
    if (domain.rfind("shard:", 0) == 0 || domain.rfind("shard-summary:", 0) == 0) {
        return themis::index::AnnScopeKind::ShardSummary;
    }
    return themis::index::AnnScopeKind::Adapter;
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
    try {
        std::fprintf(stderr, "[AR] setExactSimilarityFn called -> stored=%d\n", (exactSimilarityFnStorage() ? 1 : 0));
    } catch (...) {}
}

/*static*/
void AdapterRepository::clearExactSimilarityFn() {
    std::lock_guard<std::mutex> lk(exactSimilarityFnMutex());
    exactSimilarityFnStorage() = {};
}

void AdapterRepository::setAnnFrontdoor(std::shared_ptr<index::AnnFrontdoor> frontdoor) {
    ann_frontdoor_ = std::move(frontdoor);
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
    const auto existing_raw = backend_->get(key);
    const bool replacing_existing = existing_raw.has_value() && !existing_raw->empty();

    std::size_t old_param_bytes = 0;
    if (replacing_existing) {
        auto old_train = storage::TTTrain::deserialize(*existing_raw);
        if (old_train.has_value()) {
            old_param_bytes = old_train->totalParams() * sizeof(float);
        }
    }

    const bool ok = backend_->put(key, bytes);

    if (ok) {
        {
            std::unique_lock lock(stats_mutex_);
            if (!replacing_existing) {
                ++stats_.total_adapters;
            }
            if (old_param_bytes <= stats_.total_param_bytes) {
                stats_.total_param_bytes -= old_param_bytes;
            } else {
                stats_.total_param_bytes = 0;
            }
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

        if (ann_frontdoor_) {
            ann_frontdoor_->registerScopeKind(key, scopeKindForDomain(domain));
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
    if (!existed) {
      return false;
    }

    const bool ok = backend_->del(key);
    if (ok) {
        {
            std::unique_lock lock(stats_mutex_);
            if (stats_.total_adapters > 0) {
              --stats_.total_adapters;
            }
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

        if (ann_frontdoor_) {
            ann_frontdoor_->registerScopeKind(key, scopeKindForDomain(domain));
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
            if (mapped.tenant_id.empty()) {
              mapped.tenant_id = tenant_id_;
            }
            if (mapped.domain.empty()) {
              mapped.domain = domain;
            }
            if (mapped.base_model_id.empty()) {
              mapped.base_model_id = base_model_id;
            }
            if (mapped.adapter_key.empty()) {
              mapped.adapter_key = desc.adapter_key;
            }
            {
                std::unique_lock lock(stats_mutex_);
                if (mapped.valid) {
                  ++stats_.load_hits;
                }
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

    if (ann_frontdoor_) {
        ann_frontdoor_->registerScopeKind(desc.adapter_key, scopeKindForDomain(domain));
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

    std::vector<std::string> domains = {};

    domains.reserve(keys.size());

    for (const auto& key : keys) {
        // Key format: __adapters__:<tenant>:<domain>:<base_model_id>
        // Strip the prefix and extract the domain part.
        if (key.size() <= prefix.size()) {
          continue;
        }
        const std::string rest = key.substr(prefix.size());
        const auto sep = rest.find(':');
        if (sep == std::string::npos) {
          continue;
        }
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
        if (key.size() <= prefix.size()) {
          continue;
        }
        const std::string rest = key.substr(prefix.size());
        const auto sep = rest.find(':');
        if (sep == std::string::npos || sep + 1 >= rest.size()) {
          continue;
        }
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

    // Diagnostic: always emit that this function was called so tests can
    // verify whether the injected exact-similarity override was considered.
    try {
        std::fprintf(stderr, "[AR] findSimilarAdapters called key='%s' k=%zu graph_present=%d\n",
                     key.c_str(), k, (fingerprint_graph_ ? 1 : 0));
    } catch (...) {}

    // Delegate to injected exact-similarity backend when available (STUB #266).
    ExactSimilarityFn exact_fn_copy;
    {
        std::lock_guard<std::mutex> lk(exactSimilarityFnMutex());
        exact_fn_copy = exactSimilarityFnStorage();
    }
    if (exact_fn_copy) {
        try {
            auto results = exact_fn_copy(key, k, backend_);
            std::fprintf(stderr, "[AR] exactSimilarityFn present for key='%s' -> returned=%zu\n", key.c_str(), results.size());
            if (ann_frontdoor_) {
                index::AnnQueryContext context;
                context.scope_id = key;
                context.dataset_size = results.size();
                context.hot_tier = true;
                (void)ann_frontdoor_->planRetrieval(context);
            }
            return results;
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

    if (ann_frontdoor_) {
        ann_frontdoor_->registerScopeKind(key, scopeKindForDomain(domain));
        index::AnnQueryContext context;
        context.scope_id = key;
        context.dataset_size = graph->size();
        context.hot_tier = true;
        (void)ann_frontdoor_->planRetrieval(context);
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

