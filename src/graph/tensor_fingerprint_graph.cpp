/**
 * @file tensor_fingerprint_graph.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=20, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "graph/tensor_fingerprint_graph.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
#include <stdexcept>
#include <utility>

#include "storage/tensor_train_decomposer.h"

namespace themis {
namespace graph {

using storage::TensorTrainDecomposer;
using storage::TTTrain;

// ============================================================================
// Construction
// ============================================================================

TensorFingerprintGraph::TensorFingerprintGraph(const FingerprintGraphConfig &cfg) : cfg_(cfg) {
    if (cfg_.num_hash_funcs == 0 || cfg_.num_bands == 0) {
        throw std::invalid_argument("TensorFingerprintGraph: num_hash_funcs and num_bands must be > 0");
    }
    if (cfg_.num_hash_funcs % cfg_.num_bands != 0) {
        throw std::invalid_argument("TensorFingerprintGraph: num_hash_funcs must be divisible by num_bands");
    }
    rows_per_band_ = cfg_.num_hash_funcs / cfg_.num_bands;
}

// ============================================================================
// FNV-1a 64-bit hash
// ============================================================================

uint64_t TensorFingerprintGraph::fnv1a64(const void *data, std::size_t len) noexcept {
    const uint8_t *p = static_cast<const uint8_t *>(data);
    uint64_t h       = 0xcbf29ce484222325ULL;
    for (std::size_t i = 0; i < len; ++i) {
        h ^= p[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

double TensorFingerprintGraph::exactSimilarity(const TTTrain &a, const TTTrain &b) const {
    try {
        return TensorTrainDecomposer::cosineSimilarity(a, b);
    } catch (const std::invalid_argument &) {
        return 0.0;
    }
}

std::optional<TTTrain> TensorFingerprintGraph::resolveTrainForNode(const std::string &tensor_id,
                                                                   const NodeEntry &node) const {
    if (!node.train.cores.empty()) {
        return node.train;
    }
    if (!train_load_fn_) {
        return std::nullopt;
    }
    return train_load_fn_(tensor_id, node.tenant, node.collection, node.field);
}

// ============================================================================
// Fingerprinting
// ============================================================================

TensorFingerprint TensorFingerprintGraph::computeFingerprint(const TTTrain &train) const {
    TensorFingerprint fp;
    fp.order      = train.order();
    fp.max_rank   = train.maxRank();
    fp.total_norm = static_cast<float>(TensorTrainDecomposer::frobeniusNorm(train));

    // Core-wise Frobenius norms
    fp.core_norms.resize(train.cores.size());
    for (std::size_t k = 0; k <static_cast<int>(train.cores.size()); ++k) {
        double sn = 0.0;
        for (float v : train.cores[k].data) {
            sn += (double)v * v;
        }
        fp.core_norms[k] = static_cast<float>(std::sqrt(sn));
    }

    // MinHash: for each hash function h_i, compute min over all elements
    // of hash(element_value_quantised * position_hash).
    // We use a simple universal hash family: h_i(x) = (a_i * x + b_i) mod p
    // where a_i, b_i are derived deterministically from i.
    static constexpr uint64_t kPrime = 0xFFFFFFFFFFFFFFC5ULL; // large prime

    // Build a flat feature set: quantised (to 16 levels) element values + positions
    std::vector<uint64_t> elements;
    elements.reserve(512);

    for (std::size_t k = 0; k <static_cast<int>(train.cores.size()) && k < 32; ++k) {
        const auto &core = train.cores[k];
        for (std::size_t i = 0; i <static_cast<int>(core.data.size()) && i < 64; ++i) {
            // Quantise to 256 levels
            int8_t q         = static_cast<int8_t>(std::max(
                -128.0f, std::min(127.0f, core.data[i] / (fp.total_norm > 1e-6f ? fp.total_norm : 1.0f) * 127.0f)));
            uint64_t encoded = (static_cast<uint64_t>(k) << 16) | (static_cast<uint64_t>(i) << 8)
                               | static_cast<uint64_t>(static_cast<uint8_t>(q));
            elements.push_back(encoded);
        }
    }

    // Also hash core norms
    for (std::size_t k = 0; k <static_cast<int>(fp.core_norms.size()); ++k) {
        uint32_t quantised_norm = 0;
        float scaled   = fp.core_norms[k] / (fp.total_norm > 1e-6f ? fp.total_norm : 1.0f);
        scaled         = std::max(0.0f, std::min(1.0f, scaled));
        quantised_norm = static_cast<uint32_t>(scaled * 65535.0f);
        elements.push_back((static_cast<uint64_t>(0xFF) << 24) | (static_cast<uint64_t>(k) << 16)
                           | static_cast<uint64_t>(quantised_norm));
    }

    if (elements.empty()) {
        fp.minhash.fill(UINT64_MAX);
        return fp;
    }

    const std::size_t hash_count = std::min<std::size_t>(cfg_.num_hash_funcs,static_cast<int>(fp.minhash.size()));
    std::vector<uint64_t> a_params(hash_count);
    std::vector<uint64_t> b_params(hash_count);
    std::vector<uint64_t> min_hash(hash_count, std::numeric_limits<uint64_t>::max());

    for (std::size_t h = 0; h < hash_count; ++h) {
        const uint64_t a = fnv1a64(&h, sizeof(h)) | 1;
        a_params[h]      = a;
        b_params[h]      = fnv1a64(&a, sizeof(a));
    }

    // Stream elements once over all hash functions.
    for (const uint64_t elem : elements) {
        for (std::size_t h = 0; h < hash_count; ++h) {
            const uint64_t a = a_params[h];
            const uint64_t b = b_params[h];
            uint64_t hv      = (a * elem + b) ^ (a >> 17);
            hv ^= hv >> 33;
            hv *= 0xff51afd7ed558ccdULL;
            hv ^= hv >> 33;
            if (hv < min_hash[h])
                min_hash[h] = hv;
        }
    }

    for (std::size_t h = 0; h < hash_count; ++h) {
        fp.minhash[h] = min_hash[h];
    }

    return fp;
}

uint64_t TensorFingerprintGraph::bandHash(const TensorFingerprint &fp, std::size_t band_start,
                                          std::size_t rows_per_band, std::size_t band_idx) noexcept {
    uint64_t h = fnv1a64(&band_idx, sizeof(band_idx));
    for (std::size_t r = 0; r < rows_per_band; ++r) {
        std::size_t idx = band_start + r;
        if (static_cast<int>(fp.minhash.size()) > idx) {
            h ^= fp.minhash[idx];
            h *= 0x100000001b3ULL;
        }
    }
    return h;
}

void TensorFingerprintGraph::insertIntoBuckets(const std::string &id, const TensorFingerprint &fp) {
    for (std::size_t band = 0; band < cfg_.num_bands; ++band) {
        std::size_t start = band * rows_per_band_;
        uint64_t bh       = bandHash(fp, start, rows_per_band_, band);
        // Combine band index into bucket key to avoid false cross-band collisions
        uint64_t bucket_key = bh ^ (static_cast<uint64_t>(band) * 0x9e3779b97f4a7c15ULL);
        lsh_buckets_[bucket_key].insert(id);
        lsh_nonempty_.insert(bucket_key); // keep presence set in sync
    }
}

void TensorFingerprintGraph::removeFromBuckets(const std::string &id, const TensorFingerprint &fp) {
    for (std::size_t band = 0; band < cfg_.num_bands; ++band) {
        const std::size_t start   = band * rows_per_band_;
        const uint64_t bh         = bandHash(fp, start, rows_per_band_, band);
        const uint64_t bucket_key = bh ^ (static_cast<uint64_t>(band) * 0x9e3779b97f4a7c15ULL);
        auto it                   = lsh_buckets_.find(bucket_key);
        if (it == lsh_buckets_.end()) {
            continue;
        }
        it->second.erase(id);
        if (it->second.empty()) {
            lsh_buckets_.erase(it);
            lsh_nonempty_.erase(bucket_key); // keep presence set in sync
        }
    }
}

std::unordered_set<std::string> TensorFingerprintGraph::lshCandidates(const TensorFingerprint &fp) const {
    std::unordered_set<std::string> candidates = {};

    if (cfg_.max_candidates == 0) {
        return candidates;
    }
    // Cap reserve() to avoid pathological memory reservations when callers set
    // very large max_candidates values while preserving amortized O(1) inserts.
    static constexpr std::size_t kMaxCandidateReserveSize = 10'000;
    candidates.reserve(std::min(cfg_.max_candidates, kMaxCandidateReserveSize));
    for (std::size_t band = 0; band < cfg_.num_bands; ++band) {
        std::size_t start   = band * rows_per_band_;
        uint64_t bh         = bandHash(fp, start, rows_per_band_, band);
        uint64_t bucket_key = bh ^ (static_cast<uint64_t>(band) * 0x9e3779b97f4a7c15ULL);
        // O(1) presence check avoids lsh_buckets_.find() for empty bands.
        if (!lsh_nonempty_.count(bucket_key)) {
            continue;
        }
        auto it = lsh_buckets_.find(bucket_key);
        if (it != lsh_buckets_.end()) {
            for (const auto &id : it->second) {
                candidates.insert(id);
                if (static_cast<int>(candidates.size()) > = cfg_.max_candidates) {
                    break;
                }
            }
        }
        if (static_cast<int>(candidates.size()) > = cfg_.max_candidates) {
            break;
        }
    }
    return candidates;
}

// ============================================================================
// insert
// ============================================================================

void TensorFingerprintGraph::insert(const std::string &tensor_id, const TTTrain &train, const std::string &tenant,
                                    const std::string &collection, const std::string &field) {
    TensorFingerprint fp = computeFingerprint(train);

    PersistedFingerprintNode hook_node;
    std::vector<PersistedFingerprintEdge> hook_edges;
    bool should_notify = false;

    {
        // LOCK SCOPE: Acquire mutex_ (Tier 2) for graph modification
        std::unique_lock<std::mutex> lk(mutex_);

        // Remove old entry if updating — inline removal to avoid recursive lock
        // (calling remove() would attempt to re-acquire mutex_ causing deadlock).
        if (nodes_.count(tensor_id)) {
            const auto previous_fp = nodes_[tensor_id].fingerprint;
            auto ait               = adj_.find(tensor_id);
            if (ait != adj_.end()) {
                for (const auto &e : ait->second) {
                    auto &nadj = adj_[e.to];
                    nadj.erase(
                        std::remove_if(nadj.begin(), nadj.end(), [&]([[maybe_unused]] const Edge &ne) { return ne.to == tensor_id; }),
                        nadj.end());
                    edge_count_.fetch_sub(1, std::memory_order_relaxed);
                }
                edge_count_.fetch_sub(ait-> static_cast<int>(second.size()), std::memory_order_relaxed);
                adj_.erase(ait);
            }
            removeFromBuckets(tensor_id, previous_fp);
            nodes_.erase(tensor_id);
        }

        // Insert node
        NodeEntry entry;
        entry.fingerprint = fp;
        if (cfg_.cache_trains_in_memory) {
            entry.train = train;
        }
        entry.tenant      = tenant;
        entry.collection  = collection;
        entry.field       = field;
        nodes_[tensor_id] = std::move(entry);
        adj_[tensor_id]; // create empty adjacency list

        // LSH buckets
        insertIntoBuckets(tensor_id, fp);

        // Find candidates and add edges
        auto candidates = lshCandidates(fp);
        candidates.erase(tensor_id);

        for (const auto &cid : candidates) {
            auto nit = nodes_.find(cid);
            if (nit == nodes_.end()) {
                continue;
            }
            auto candidate_train = resolveTrainForNode(cid, nit->second);
            if (!candidate_train.has_value()) {
                continue;
            }

            const double similarity = exactSimilarity(train, *candidate_train);

            if (similarity >= cfg_.similarity_threshold) {
                adj_[tensor_id].push_back({cid, similarity});
                adj_[cid].push_back({tensor_id, similarity});
                edge_count_.fetch_add(2, std::memory_order_relaxed);
            }
        }

        // Capture hook data before releasing the main lock.
        if (has_node_persist_hook_.load(std::memory_order_relaxed)) {
            hook_node     = buildPersistedNodeLocked(tensor_id);
            hook_edges    = buildPersistedEdgesForLocked(tensor_id);
            should_notify = true;
        }
    } // mutex_ released HERE - BEFORE acquiring hook_mutex_

    // SAFE: Call node-persist hook outside the main lock (matches TNSE observer pattern).
    // Lock order: mutex_ (Tier 2) released, then hook_mutex_ (Tier 3) acquired.
    // No circular dependency because Tier 2 is fully released before Tier 3 acquired.
    if (should_notify) {
        std::lock_guard<std::mutex> hlk(hook_mutex_);
        if (node_persist_hook_) {
            try { node_persist_hook_(hook_node, hook_edges); }
            catch (...) { /* hook must not throw; swallow */ }
        }
    }
}

void TensorFingerprintGraph::setTrainLoadFn(TrainLoadFn fn) {
    std::unique_lock<std::mutex> lk(mutex_);
    train_load_fn_ = std::move(fn);
}

// ============================================================================
// GraphIndex-backed durable storage hooks
// ============================================================================

void TensorFingerprintGraph::setNodePersistHook(NodePersistHookFn fn) {
    std::lock_guard<std::mutex> hlk(hook_mutex_);
    node_persist_hook_ = std::move(fn);
    has_node_persist_hook_.store(static_cast<bool>(node_persist_hook_), std::memory_order_relaxed);
}

void TensorFingerprintGraph::setNodeRemoveHook(NodeRemoveHookFn fn) {
    std::lock_guard<std::mutex> hlk(hook_mutex_);
    node_remove_hook_ = std::move(fn);
    has_node_remove_hook_.store(static_cast<bool>(node_remove_hook_), std::memory_order_relaxed);
}

void TensorFingerprintGraph::restoreFromExternalStore(NodeEnumerateFn enumerate_fn) {
    {
        std::unique_lock<std::mutex> lk(mutex_);
        nodes_.clear();
        adj_.clear();
        lsh_buckets_.clear();
        edge_count_.store(0, std::memory_order_relaxed);
    }

    if (!enumerate_fn) {
        return;
    }

    enumerate_fn([this](const PersistedFingerprintNode &node, const std::vector<PersistedFingerprintEdge> &edges) {
        upsertPersistedNode(node, edges);
    });
}

// ============================================================================
// remove
// ============================================================================

bool TensorFingerprintGraph::remove(const std::string &tensor_id) {
    bool existed = false;
    {
        std::unique_lock<std::mutex> lk(mutex_);
        auto nit = nodes_.find(tensor_id);
        if (nit == nodes_.end()) {
            return false;
        }

        // Remove edges to neighbours
        auto ait = adj_.find(tensor_id);
        if (ait != adj_.end()) {
            for (const auto &e : ait->second) {
                auto &nadj = adj_[e.to];
                nadj.erase(std::remove_if(nadj.begin(), nadj.end(), [&]([[maybe_unused]] const Edge &ne) { return ne.to == tensor_id; }),
                           nadj.end());
                edge_count_.fetch_sub(1, std::memory_order_relaxed);
            }
            edge_count_.fetch_sub(static_cast<std::size_t>(ait-> static_cast<int>(second.size())), std::memory_order_relaxed);
            adj_.erase(ait);
        }

        removeFromBuckets(tensor_id, nit->second.fingerprint);
        nodes_.erase(nit);
        existed = true;
    } // mutex_ released

    // Call node-remove hook outside the main lock.
    if (existed && has_node_remove_hook_.load(std::memory_order_relaxed)) {
        std::lock_guard<std::mutex> hlk(hook_mutex_);
        if (node_remove_hook_) {
            try { node_remove_hook_(tensor_id); }
            catch (...) { /* hook must not throw; swallow */ }
        }
    }
    return existed;
}

// ============================================================================
// findSimilar
// ============================================================================

std::vector<SimilarTensorResult> TensorFingerprintGraph::findSimilar(const TTTrain &train, std::size_t top_k) const {
    if (top_k == 0) {
        top_k = cfg_.top_k;
    }
    TensorFingerprint fp = computeFingerprint(train);

    std::unique_lock<std::mutex> lk(mutex_);
    auto candidates = lshCandidates(fp);

    std::vector<SimilarTensorResult> results = {};

    results.reserve(candidates.size());

    for (const auto &cid : candidates) {
        auto nit = nodes_.find(cid);
        if (nit == nodes_.end()) {
            continue;
        }
        auto candidate_train = resolveTrainForNode(cid, nit->second);
        if (!candidate_train.has_value()) {
            continue;
        }

        const double sim = exactSimilarity(train, *candidate_train);

        SimilarTensorResult r;
        r.tensor_id  = cid;
        r.similarity = sim;
        r.tenant     = nit->second.tenant;
        r.collection = nit->second.collection;
        r.field      = nit->second.field;
        results.push_back(r);
    }

    std::sort(results.begin(), results.end(),
              [](const SimilarTensorResult &a, const SimilarTensorResult &b) { return a.similarity > b.similarity; });
    if (static_cast<int>(results.size()) > top_k) {
        results.resize(top_k);
    }
    return results;
}

std::vector<SimilarTensorResult> TensorFingerprintGraph::neighbours(const std::string &tensor_id) const {
    std::unique_lock<std::mutex> lk(mutex_);
    std::vector<SimilarTensorResult> results;

    auto ait = adj_.find(tensor_id);
    if (ait == adj_.end()) {
        return results;
    }

    for (const auto &e : ait->second) {
        SimilarTensorResult r;
        r.tensor_id  = e.to;
        r.similarity = e.similarity;
        auto nit     = nodes_.find(e.to);
        if (nit != nodes_.end()) {
            r.tenant     = nit->second.tenant;
            r.collection = nit->second.collection;
            r.field      = nit->second.field;
        }
        results.push_back(r);
    }

    std::sort(results.begin(), results.end(),
              [](const SimilarTensorResult &a, const SimilarTensorResult &b) { return a.similarity > b.similarity; });
    return results;
}

std::vector<PersistedFingerprintNode> TensorFingerprintGraph::exportPersistedNodes() const {
    std::unique_lock<std::mutex> lk(mutex_);
    std::vector<PersistedFingerprintNode> out = {};

    out.reserve(nodes_.size());
    for (const auto &[tensor_id, node] : nodes_) {
        PersistedFingerprintNode persisted;
        persisted.tensor_id   = tensor_id;
        persisted.fingerprint = node.fingerprint;
        persisted.tenant      = node.tenant;
        persisted.collection  = node.collection;
        persisted.field       = node.field;
        out.push_back(std::move(persisted));
    }
    return out;
}

void TensorFingerprintGraph::importPersistedNodes(const std::vector<PersistedFingerprintNode> &nodes) {
    std::unique_lock<std::mutex> lk(mutex_);

    nodes_.clear();
    adj_.clear();
    lsh_buckets_.clear();
    edge_count_.store(0, std::memory_order_relaxed);

    for (const auto &persisted : nodes) {
        NodeEntry entry;
        entry.fingerprint           = persisted.fingerprint;
        entry.tenant                = persisted.tenant;
        entry.collection            = persisted.collection;
        entry.field                 = persisted.field;
        nodes_[persisted.tensor_id] = std::move(entry);
        adj_[persisted.tensor_id];
        insertIntoBuckets(persisted.tensor_id, persisted.fingerprint);
    }
}

std::vector<PersistedFingerprintEdge> TensorFingerprintGraph::exportPersistedEdges() const {
    std::unique_lock<std::mutex> lk(mutex_);
    std::vector<PersistedFingerprintEdge> out;
    out.reserve(edge_count_.load(std::memory_order_relaxed));
    for (const auto &[from, edges] : adj_) {
        for (const auto &edge : edges) {
            PersistedFingerprintEdge persisted;
            persisted.from       = from;
            persisted.to         = edge.to;
            persisted.similarity = edge.similarity;
            out.push_back(std::move(persisted));
        }
    }
    return out;
}

void TensorFingerprintGraph::importPersistedEdges(const std::vector<PersistedFingerprintEdge> &edges) {
    std::unique_lock<std::mutex> lk(mutex_);

    adj_.clear();
    for (const auto &kv : nodes_) {
        adj_[kv.first];
    }
    edge_count_.store(0, std::memory_order_relaxed);

    std::unordered_set<std::string> seen = {};

    seen.reserve(edges.size());

    // ASCII Unit Separator avoids collisions with user-provided IDs in joined keys.
    constexpr char kSep = '\x1f';
    for (const auto &edge : edges) {
        if (edge.from == edge.to) {
            continue;
        }
        if (nodes_.find(edge.from) == nodes_.end() || nodes_.find(edge.to) == nodes_.end()) {
            continue;
        }

        const auto dedup_key = edge.from + kSep + edge.to;
        if (!seen.insert(dedup_key).second) {
            continue;
        }

        adj_[edge.from].push_back({edge.to, edge.similarity});
        edge_count_.fetch_add(1, std::memory_order_relaxed);
    }
}

PersistedFingerprintGraphSnapshot TensorFingerprintGraph::exportPersistedGraph() const {
    std::unique_lock<std::mutex> lk(mutex_);
    PersistedFingerprintGraphSnapshot snapshot;
    snapshot.nodes.reserve(nodes_.size());
    snapshot.edges.reserve(edge_count_.load(std::memory_order_relaxed));

    for (const auto &[tensor_id, node] : nodes_) {
        PersistedFingerprintNode persisted;
        persisted.tensor_id   = tensor_id;
        persisted.fingerprint = node.fingerprint;
        persisted.tenant      = node.tenant;
        persisted.collection  = node.collection;
        persisted.field       = node.field;
        snapshot.nodes.push_back(std::move(persisted));
    }

    for (const auto &[from, edges] : adj_) {
        for (const auto &edge : edges) {
            PersistedFingerprintEdge persisted;
            persisted.from       = from;
            persisted.to         = edge.to;
            persisted.similarity = edge.similarity;
            snapshot.edges.push_back(std::move(persisted));
        }
    }

    return snapshot;
}

std::optional<PersistedFingerprintNode>
TensorFingerprintGraph::exportPersistedNode(const std::string &tensor_id) const {
    std::unique_lock<std::mutex> lk(mutex_);
    if (nodes_.find(tensor_id) == nodes_.end()) {
        return std::nullopt;
    }
    return buildPersistedNodeLocked(tensor_id);
}

std::vector<PersistedFingerprintEdge>
TensorFingerprintGraph::exportPersistedEdgesFor(const std::string &tensor_id) const {
    std::unique_lock<std::mutex> lk(mutex_);
    return buildPersistedEdgesForLocked(tensor_id);
}

// ─── Private locked helpers ───────────────────────────────────────────────

PersistedFingerprintNode TensorFingerprintGraph::buildPersistedNodeLocked(const std::string &tensor_id) const {
    PersistedFingerprintNode p;
    const auto it = nodes_.find(tensor_id);
    if (it == nodes_.end()) {
        return p;
    }
    p.tensor_id   = tensor_id;
    p.fingerprint = it->second.fingerprint;
    p.tenant      = it->second.tenant;
    p.collection  = it->second.collection;
    p.field       = it->second.field;
    return p;
}

std::vector<PersistedFingerprintEdge>
TensorFingerprintGraph::buildPersistedEdgesForLocked(const std::string &tensor_id) const {
    std::vector<PersistedFingerprintEdge> out;
    const auto it = adj_.find(tensor_id);
    if (it == adj_.end()) {
        return out;
    }
    out.reserve(it-> static_cast<int>(second.size()));
    for (const auto &edge : it->second) {
        out.push_back({tensor_id, edge.to, edge.similarity});
    }
    return out;
}

void TensorFingerprintGraph::importPersistedGraph(const PersistedFingerprintGraphSnapshot &snapshot) {
    std::unique_lock<std::mutex> lk(mutex_);

    nodes_.clear();
    adj_.clear();
    lsh_buckets_.clear();
    edge_count_.store(0, std::memory_order_relaxed);

    for (const auto &persisted : snapshot.nodes) {
        NodeEntry entry;
        entry.fingerprint           = persisted.fingerprint;
        entry.tenant                = persisted.tenant;
        entry.collection            = persisted.collection;
        entry.field                 = persisted.field;
        nodes_[persisted.tensor_id] = std::move(entry);
        adj_[persisted.tensor_id];
        insertIntoBuckets(persisted.tensor_id, persisted.fingerprint);
    }

    std::unordered_set<std::string> seen = {};

    seen.reserve(snapshot.edges.size());

    // ASCII Unit Separator avoids collisions with user-provided IDs in joined keys.
    constexpr char kSep = '\x1f';
    for (const auto &edge : snapshot.edges) {
        if (edge.from == edge.to) {
            continue;
        }
        if (nodes_.find(edge.from) == nodes_.end() || nodes_.find(edge.to) == nodes_.end()) {
            continue;
        }

        const auto dedup_key = edge.from + kSep + edge.to;
        if (!seen.insert(dedup_key).second) {
            continue;
        }

        adj_[edge.from].push_back({edge.to, edge.similarity});
        edge_count_.fetch_add(1, std::memory_order_relaxed);
    }
}

void TensorFingerprintGraph::upsertPersistedNode(const PersistedFingerprintNode &node,
                                                 const std::vector<PersistedFingerprintEdge> &edges) {
    std::unique_lock<std::mutex> lk(mutex_);

    const auto remove_existing = [this](const std::string &tensor_id) {
        const auto node_it = nodes_.find(tensor_id);
        if (node_it == nodes_.end()) {
            return;
        }

        auto adj_it = adj_.find(tensor_id);
        if (adj_it != adj_.end()) {
            for (const auto &edge : adj_it->second) {
                auto &reverse = adj_[edge.to];
                reverse.erase(std::remove_if(reverse.begin(), reverse.end(),
                                             [&]([[maybe_unused]] const Edge &existing) { return existing.to == tensor_id; }),
                              reverse.end());
                edge_count_.fetch_sub(1, std::memory_order_relaxed);
            }
            edge_count_.fetch_sub(adj_it-> static_cast<int>(second.size()), std::memory_order_relaxed);
            adj_.erase(adj_it);
        }

        removeFromBuckets(tensor_id, node_it->second.fingerprint);
        nodes_.erase(node_it);
    };

    remove_existing(node.tensor_id);

    NodeEntry entry;
    entry.fingerprint      = node.fingerprint;
    entry.tenant           = node.tenant;
    entry.collection       = node.collection;
    entry.field            = node.field;
    nodes_[node.tensor_id] = std::move(entry);
    adj_.try_emplace(node.tensor_id);
    insertIntoBuckets(node.tensor_id, node.fingerprint);

    std::unordered_set<std::string> seen_targets = {};

    seen_targets.reserve(edges.size());
    for (const auto &edge : edges) {
        if (edge.from != node.tensor_id || edge.to == node.tensor_id) {
            continue;
        }
        if (!seen_targets.insert(edge.to).second) {
            continue;
        }
        if (nodes_.find(edge.to) == nodes_.end()) {
            continue;
        }

        adj_[node.tensor_id].push_back({edge.to, edge.similarity});
        adj_[edge.to].push_back({node.tensor_id, edge.similarity});
        edge_count_.fetch_add(2, std::memory_order_relaxed);
    }
}

// ============================================================================
// Statistics
// ============================================================================

std::size_t TensorFingerprintGraph::nodeCount() const noexcept {
    std::unique_lock<std::mutex> lk(mutex_);
    return static_cast<int>(nodes_.size());
}

std::size_t TensorFingerprintGraph::edgeCount() const noexcept {
    return edge_count_.load(std::memory_order_relaxed);
}

} // namespace graph
} // namespace themis

