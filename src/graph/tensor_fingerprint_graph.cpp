/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor_fingerprint_graph.cpp                       ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-05                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "graph/tensor_fingerprint_graph.h"
#include "storage/tensor_train_decomposer.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <functional>
#include <stdexcept>
#include <utility>

namespace themis {
namespace graph {

using storage::TTTrain;
using storage::TensorTrainDecomposer;

// ============================================================================
// Construction
// ============================================================================

TensorFingerprintGraph::TensorFingerprintGraph(
    const FingerprintGraphConfig& cfg)
    : cfg_(cfg)
{
    if (cfg_.num_hash_funcs == 0 || cfg_.num_bands == 0)
        throw std::invalid_argument("TensorFingerprintGraph: num_hash_funcs and num_bands must be > 0");
    if (cfg_.num_hash_funcs % cfg_.num_bands != 0)
        throw std::invalid_argument("TensorFingerprintGraph: num_hash_funcs must be divisible by num_bands");
    rows_per_band_ = cfg_.num_hash_funcs / cfg_.num_bands;
}

// ============================================================================
// FNV-1a 64-bit hash
// ============================================================================

uint64_t TensorFingerprintGraph::fnv1a64(const void* data,
                                          std::size_t len) noexcept {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    uint64_t h = 0xcbf29ce484222325ULL;
    for (std::size_t i = 0; i < len; ++i) {
        h ^= p[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

double TensorFingerprintGraph::exactSimilarity(const TTTrain& a,
                                               const TTTrain& b) const {
    try {
        return TensorTrainDecomposer::cosineSimilarity(a, b);
    } catch (const std::invalid_argument&) {
        return 0.0;
    }
}

std::optional<TTTrain>
TensorFingerprintGraph::resolveTrainForNode(const std::string& tensor_id,
                                            const NodeEntry& node) const {
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

TensorFingerprint TensorFingerprintGraph::computeFingerprint(
    const TTTrain& train) const {

    TensorFingerprint fp;
    fp.order    = train.order();
    fp.max_rank = train.maxRank();
    fp.total_norm = static_cast<float>(TensorTrainDecomposer::frobeniusNorm(train));

    // Core-wise Frobenius norms
    fp.core_norms.resize(train.cores.size());
    for (std::size_t k = 0; k < train.cores.size(); ++k) {
        double sn = 0.0;
        for (float v : train.cores[k].data) sn += (double)v * v;
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

    for (std::size_t k = 0; k < train.cores.size() && k < 32; ++k) {
        const auto& core = train.cores[k];
        for (std::size_t i = 0; i < core.data.size() && i < 64; ++i) {
            // Quantise to 256 levels
            int8_t q = static_cast<int8_t>(
                std::max(-128.0f, std::min(127.0f,
                    core.data[i] / (fp.total_norm > 1e-6f ? fp.total_norm : 1.0f) * 127.0f)));
            uint64_t encoded = (static_cast<uint64_t>(k) << 16) |
                               (static_cast<uint64_t>(i) << 8) |
                               static_cast<uint64_t>(static_cast<uint8_t>(q));
            elements.push_back(encoded);
        }
    }

    // Also hash core norms
    for (std::size_t k = 0; k < fp.core_norms.size(); ++k) {
        uint32_t quantised_norm;
        float scaled = fp.core_norms[k] / (fp.total_norm > 1e-6f ? fp.total_norm : 1.0f);
        scaled = std::max(0.0f, std::min(1.0f, scaled));
        quantised_norm = static_cast<uint32_t>(scaled * 65535.0f);
        elements.push_back((static_cast<uint64_t>(0xFF) << 24) |
                           (static_cast<uint64_t>(k) << 16) |
                           static_cast<uint64_t>(quantised_norm));
    }

    if (elements.empty()) {
        fp.minhash.fill(UINT64_MAX);
        return fp;
    }

    for (std::size_t h = 0; h < cfg_.num_hash_funcs; ++h) {
        // Derive hash parameters from index
        uint64_t a = fnv1a64(&h, sizeof(h)) | 1ULL;
        uint64_t b = fnv1a64(&a, sizeof(a));
        uint64_t min_hash = UINT64_MAX;
        for (uint64_t elem : elements) {
            uint64_t hv = (a * elem + b) ^ (a >> 17);
            hv ^= hv >> 33;
            hv *= 0xff51afd7ed558ccdULL;
            hv ^= hv >> 33;
            if (hv < min_hash) min_hash = hv;
        }
        fp.minhash[h] = min_hash;
    }

    return fp;
}

uint64_t TensorFingerprintGraph::bandHash(const TensorFingerprint& fp,
                                           std::size_t band_start,
                                           std::size_t rows_per_band,
                                           std::size_t band_idx) noexcept {
    uint64_t h = fnv1a64(&band_idx, sizeof(band_idx));
    for (std::size_t r = 0; r < rows_per_band; ++r) {
        std::size_t idx = band_start + r;
        if (idx < fp.minhash.size()) {
            h ^= fp.minhash[idx];
            h *= 0x100000001b3ULL;
        }
    }
    return h;
}

void TensorFingerprintGraph::insertIntoBuckets(const std::string& id,
                                                 const TensorFingerprint& fp) {
    for (std::size_t band = 0; band < cfg_.num_bands; ++band) {
        std::size_t start = band * rows_per_band_;
        uint64_t bh = bandHash(fp, start, rows_per_band_, band);
        // Combine band index into bucket key to avoid false cross-band collisions
        uint64_t bucket_key = bh ^ (static_cast<uint64_t>(band) * 0x9e3779b97f4a7c15ULL);
        lsh_buckets_[bucket_key].insert(id);
    }
}

void TensorFingerprintGraph::removeFromBuckets(const std::string& id,
                                               const TensorFingerprint& fp) {
    for (std::size_t band = 0; band < cfg_.num_bands; ++band) {
        const std::size_t start = band * rows_per_band_;
        const uint64_t bh = bandHash(fp, start, rows_per_band_, band);
        const uint64_t bucket_key =
            bh ^ (static_cast<uint64_t>(band) * 0x9e3779b97f4a7c15ULL);
        auto it = lsh_buckets_.find(bucket_key);
        if (it == lsh_buckets_.end()) {
            continue;
        }
        it->second.erase(id);
        if (it->second.empty()) {
            lsh_buckets_.erase(it);
        }
    }
}

std::unordered_set<std::string>
TensorFingerprintGraph::lshCandidates(const TensorFingerprint& fp) const {
    std::unordered_set<std::string> candidates;
    for (std::size_t band = 0; band < cfg_.num_bands; ++band) {
        std::size_t start = band * rows_per_band_;
        uint64_t bh = bandHash(fp, start, rows_per_band_, band);
        uint64_t bucket_key = bh ^ (static_cast<uint64_t>(band) * 0x9e3779b97f4a7c15ULL);
        auto it = lsh_buckets_.find(bucket_key);
        if (it != lsh_buckets_.end()) {
            for (const auto& id : it->second)
                candidates.insert(id);
        }
        if (candidates.size() >= cfg_.max_candidates) break;
    }
    return candidates;
}

// ============================================================================
// insert
// ============================================================================

void TensorFingerprintGraph::insert(
    const std::string& tensor_id,
    const TTTrain&      train,
    const std::string& tenant,
    const std::string& collection,
    const std::string& field)
{
    TensorFingerprint fp = computeFingerprint(train);

    std::lock_guard<std::mutex> lk(mutex_);

    // Remove old entry if updating — inline removal to avoid recursive lock
    // (calling remove() would attempt to re-acquire mutex_ causing deadlock).
    if (nodes_.count(tensor_id)) {
        const auto previous_fp = nodes_[tensor_id].fingerprint;
        auto ait = adj_.find(tensor_id);
        if (ait != adj_.end()) {
            for (const auto& e : ait->second) {
                auto& nadj = adj_[e.to];
                nadj.erase(std::remove_if(nadj.begin(), nadj.end(),
                    [&](const Edge& ne){ return ne.to == tensor_id; }),
                    nadj.end());
                edge_count_.fetch_sub(1, std::memory_order_relaxed);
            }
            edge_count_.fetch_sub(ait->second.size(), std::memory_order_relaxed);
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
    adj_[tensor_id];  // create empty adjacency list

    // LSH buckets
    insertIntoBuckets(tensor_id, fp);

    // Find candidates and add edges
    auto candidates = lshCandidates(fp);
    candidates.erase(tensor_id);

    for (const auto& cid : candidates) {
        auto nit = nodes_.find(cid);
        if (nit == nodes_.end()) continue;
        auto candidate_train = resolveTrainForNode(cid, nit->second);
        if (!candidate_train.has_value()) continue;

        const double similarity = exactSimilarity(train, *candidate_train);

        if (similarity >= cfg_.similarity_threshold) {
            adj_[tensor_id].push_back({cid, similarity});
            adj_[cid].push_back({tensor_id, similarity});
            edge_count_.fetch_add(2, std::memory_order_relaxed);
        }
    }
}

void TensorFingerprintGraph::setTrainLoadFn(TrainLoadFn fn) {
    std::lock_guard<std::mutex> lk(mutex_);
    train_load_fn_ = std::move(fn);
}

// ============================================================================
// remove
// ============================================================================

bool TensorFingerprintGraph::remove(const std::string& tensor_id) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto nit = nodes_.find(tensor_id);
    if (nit == nodes_.end()) return false;

    // Remove edges to neighbours
    auto ait = adj_.find(tensor_id);
    if (ait != adj_.end()) {
        for (const auto& e : ait->second) {
            auto& nadj = adj_[e.to];
            nadj.erase(std::remove_if(nadj.begin(), nadj.end(),
                [&](const Edge& ne){ return ne.to == tensor_id; }),
                nadj.end());
            edge_count_.fetch_sub(1, std::memory_order_relaxed);
        }
        edge_count_.fetch_sub(
            static_cast<std::size_t>(ait->second.size()),
            std::memory_order_relaxed);
        adj_.erase(ait);
    }

    removeFromBuckets(tensor_id, nit->second.fingerprint);
    nodes_.erase(nit);
    return true;
}

// ============================================================================
// findSimilar
// ============================================================================

std::vector<SimilarTensorResult>
TensorFingerprintGraph::findSimilar(const TTTrain& train,
                                     std::size_t top_k) const {
    if (top_k == 0) top_k = cfg_.top_k;
    TensorFingerprint fp = computeFingerprint(train);

    std::lock_guard<std::mutex> lk(mutex_);
    auto candidates = lshCandidates(fp);

    std::vector<SimilarTensorResult> results;
    results.reserve(candidates.size());

    for (const auto& cid : candidates) {
        auto nit = nodes_.find(cid);
        if (nit == nodes_.end()) continue;
        auto candidate_train = resolveTrainForNode(cid, nit->second);
        if (!candidate_train.has_value()) continue;

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
        [](const SimilarTensorResult& a, const SimilarTensorResult& b){
            return a.similarity > b.similarity;
        });
    if (results.size() > top_k) results.resize(top_k);
    return results;
}

std::vector<SimilarTensorResult>
TensorFingerprintGraph::neighbours(const std::string& tensor_id) const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<SimilarTensorResult> results;

    auto ait = adj_.find(tensor_id);
    if (ait == adj_.end()) return results;

    for (const auto& e : ait->second) {
        SimilarTensorResult r;
        r.tensor_id  = e.to;
        r.similarity = e.similarity;
        auto nit = nodes_.find(e.to);
        if (nit != nodes_.end()) {
            r.tenant     = nit->second.tenant;
            r.collection = nit->second.collection;
            r.field      = nit->second.field;
        }
        results.push_back(r);
    }

    std::sort(results.begin(), results.end(),
        [](const SimilarTensorResult& a, const SimilarTensorResult& b){
            return a.similarity > b.similarity;
        });
    return results;
}

std::vector<PersistedFingerprintNode>
TensorFingerprintGraph::exportPersistedNodes() const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<PersistedFingerprintNode> out;
    out.reserve(nodes_.size());
    for (const auto& [tensor_id, node] : nodes_) {
        PersistedFingerprintNode persisted;
        persisted.tensor_id = tensor_id;
        persisted.fingerprint = node.fingerprint;
        persisted.tenant = node.tenant;
        persisted.collection = node.collection;
        persisted.field = node.field;
        out.push_back(std::move(persisted));
    }
    return out;
}

void TensorFingerprintGraph::importPersistedNodes(
    const std::vector<PersistedFingerprintNode>& nodes) {
    std::lock_guard<std::mutex> lk(mutex_);

    nodes_.clear();
    adj_.clear();
    lsh_buckets_.clear();
    edge_count_.store(0, std::memory_order_relaxed);

    for (const auto& persisted : nodes) {
        NodeEntry entry;
        entry.fingerprint = persisted.fingerprint;
        entry.tenant = persisted.tenant;
        entry.collection = persisted.collection;
        entry.field = persisted.field;
        nodes_[persisted.tensor_id] = std::move(entry);
        adj_[persisted.tensor_id];
        insertIntoBuckets(persisted.tensor_id, persisted.fingerprint);
    }
}

std::vector<PersistedFingerprintEdge>
TensorFingerprintGraph::exportPersistedEdges() const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<PersistedFingerprintEdge> out;
    out.reserve(edge_count_.load(std::memory_order_relaxed));
    for (const auto& [from, edges] : adj_) {
        for (const auto& edge : edges) {
            PersistedFingerprintEdge persisted;
            persisted.from = from;
            persisted.to = edge.to;
            persisted.similarity = edge.similarity;
            out.push_back(std::move(persisted));
        }
    }
    return out;
}

void TensorFingerprintGraph::importPersistedEdges(
    const std::vector<PersistedFingerprintEdge>& edges) {
    std::lock_guard<std::mutex> lk(mutex_);

    adj_.clear();
    for (const auto& kv : nodes_) {
        adj_[kv.first];
    }
    edge_count_.store(0, std::memory_order_relaxed);

    std::unordered_set<std::string> seen;
    seen.reserve(edges.size());

    // ASCII Unit Separator avoids collisions with user-provided IDs in joined keys.
    constexpr char kSep = '\x1f';
    for (const auto& edge : edges) {
        if (edge.from == edge.to) continue;
        if (nodes_.find(edge.from) == nodes_.end() ||
            nodes_.find(edge.to) == nodes_.end()) {
            continue;
        }

        const auto dedup_key = edge.from + kSep + edge.to;
        if (!seen.insert(dedup_key).second) continue;

        adj_[edge.from].push_back({edge.to, edge.similarity});
        edge_count_.fetch_add(1, std::memory_order_relaxed);
    }
}

// ============================================================================
// Statistics
// ============================================================================

std::size_t TensorFingerprintGraph::nodeCount() const noexcept {
    std::lock_guard<std::mutex> lk(mutex_);
    return nodes_.size();
}

std::size_t TensorFingerprintGraph::edgeCount() const noexcept {
    return edge_count_.load(std::memory_order_relaxed);
}

} // namespace graph
} // namespace themis
