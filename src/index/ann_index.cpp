/**
 * @file ann_index.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=14, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ScaNN (Scalable Nearest Neighbors) – self-contained production implementation
//
// Based on:
//   "Accelerating Large-Scale Inference with Anisotropic Vector Quantization"
//   Ruiqi Guo, Philip Sun, Erik Lindgren, Quan Geng, David Simcha,
//   Felix Chern, Sanjiv Kumar – ICML 2020
//   https://arxiv.org/abs/1908.10396
//
// Algorithm:
//   build()  →  K-means partitioning (Lloyd's algorithm)  +  PQ codebook training
//   search() →  centroid scoring  →  AH scan of best leaves  →  exact re-ranking

#include "index/ann_index.h"
#include "utils/logger.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <numeric>
#include <queue>
#include <random>
#include <stdexcept>

namespace themis {
namespace index {

namespace {

bool checkedMultiply(size_t lhs, size_t rhs, size_t& out) {
    if (lhs == 0 || rhs == 0) {
        out = 0;
        return true;
    }

    if (lhs > std::numeric_limits<size_t>::max() / rhs) {
        return false;
    }

    out = lhs * rhs;
    return true;
}

const float* checkedRow(const float* data, size_t rows, size_t dim, size_t row_index) {
    if (data == nullptr || dim == 0 || row_index >= rows) {
        return nullptr;
    }

    size_t total_values = 0;
    size_t offset = 0;
    if (!checkedMultiply(rows, dim, total_values) ||
        !checkedMultiply(row_index, dim, offset) ||
        offset > total_values ||
        dim > total_values - offset) {
        return nullptr;
    }

    return data + offset;
}

template <typename Value>
constexpr size_t elementSize() {
    return sizeof(Value);
}

} // namespace

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

float ScaNN::l2sq(const float* a, const float* b, size_t d) {
    float s = 0.f;
    for (size_t i = 0; i < d; ++i) {
        float diff = a[i] - b[i];
        s += diff * diff;
    }
    return s;
}

// Simple Lloyd's k-means (k iterations, seeded with k-means++)
void ScaNN::kmeans(const float* data, size_t n, size_t d,
                   size_t k, size_t iters,
                   std::vector<std::vector<float>>& centroids,
                   std::vector<size_t>& assignments) {
    if (data == nullptr || n == 0 || d == 0 || k == 0) {
      return;
    }
    k = std::min(k, n);

    // k-means++ initialisation
    std::mt19937_64 rng(0xDEADBEEF);
    centroids.clear();
    centroids.reserve(k);

    // Pick first centroid uniformly at random
    std::uniform_int_distribution<uint64_t> uni(0, n - 1);
    size_t idx = static_cast<size_t>(uni(rng));
    if (const float* first_row = checkedRow(data, n, d, idx)) {
        centroids.emplace_back(first_row, first_row + d);
    } else {
        return;
    }

    // Pick remaining centroids with probability proportional to D^2
    std::vector<float> dists(n, std::numeric_limits<float>::max());
    for (size_t c = 1; c < k; ++c) {
        float total = 0.f;
        for (size_t i = 0; i < n; ++i) {
            const float* row = checkedRow(data, n, d, i);
            if (row == nullptr) {
                return;
            }
            float dist = l2sq(row, centroids.back().data(), d);
            dists[i] = std::min(dists[i], dist);
            total += dists[i];
        }
        std::uniform_real_distribution<float> real_dis(0.f, total);
        float threshold = real_dis(rng);
        float cumsum = 0.f;
        size_t chosen = n - 1;
        for (size_t i = 0; i < n; ++i) {
            cumsum += dists[i];
            if (cumsum >= threshold) { chosen = i; break; }
        }
        if (const float* chosen_row = checkedRow(data, n, d, chosen)) {
            centroids.emplace_back(chosen_row, chosen_row + d);
        } else {
            return;
        }
    }

    assignments.assign(n, 0);

    for (size_t iter = 0; iter < iters; ++iter) {
        // Assignment step
        for (size_t i = 0; i < n; ++i) {
            float best = std::numeric_limits<float>::max();
            size_t best_c = 0;
            const float* row = checkedRow(data, n, d, i);
            if (row == nullptr) {
                return;
            }
            for (size_t c = 0; c < k; ++c) {
                float dist = l2sq(row, centroids[c].data(), d);
                if (dist < best) { best = dist; best_c = c; }
            }
            assignments[i] = best_c;
        }

        // Update step
        std::vector<std::vector<float>> new_cents(k, std::vector<float>(d, 0.f));
        std::vector<size_t> counts(k, 0);
        for (size_t i = 0; i < n; ++i) {
            size_t c = assignments[i];
            const float* row = checkedRow(data, n, d, i);
            if (c >= new_cents.size() || row == nullptr) {
                return;
            }
            ++counts[c];
            for (size_t j = 0; j < d; ++j)
                new_cents[c][j] += row[j];
        }
        for (size_t c = 0; c < k; ++c) {
            if (counts[c] > 0) {
                for (size_t j = 0; j < d; ++j)
                    new_cents[c][j] /= static_cast<float>(counts[c]);
                centroids[c] = std::move(new_cents[c]);
            }
            // If a centroid got no assignment, re-seed it with a random point
            else {
                size_t r = uni(rng);
                if (const float* reseed_row = checkedRow(data, n, d, r)) {
                    centroids[c].assign(reseed_row, reseed_row + d);
                } else {
                    return;
                }
            }
        }
    }
    // Final assignment
    for (size_t i = 0; i < n; ++i) {
        float best = std::numeric_limits<float>::max();
        size_t best_c = 0;
        const float* row = checkedRow(data, n, d, i);
        if (row == nullptr) {
            return;
        }
        for (size_t c = 0; c < k; ++c) {
            float dist = l2sq(row, centroids[c].data(), d);
            if (dist < best) { best = dist; best_c = c; }
        }
        assignments[i] = best_c;
    }
}

// ---------------------------------------------------------------------------
// PQCodebook
// ---------------------------------------------------------------------------

void ScaNN::PQCodebook::train(const float* data, size_t n, size_t d,
                               size_t nss, size_t bits, size_t iters) {
    if (data == nullptr || n == 0 || d == 0 || nss == 0 || d % nss != 0) {
        num_subspaces = 0;
        sub_dim = 0;
        centroids.clear();
        return;
    }

    num_subspaces = nss;
    sub_dim = d / nss;
    if (bits >= sizeof(size_t) * 8) {
        num_subspaces = 0;
        sub_dim = 0;
        centroids.clear();
        return;
    }
    num_centroids = static_cast<size_t>(1) << bits;

    centroids.resize(nss);
    for (size_t s = 0; s < nss; ++s) {
        // Collect sub-vectors for this subspace
        size_t subspace_size = 0;
        if (!checkedMultiply(n, sub_dim, subspace_size)) {
            THEMIS_ERROR("ScaNN::PQCodebook::train - size overflow when computing subspace_size (n={} sub_dim={})", n, sub_dim);
            num_subspaces = 0;
            sub_dim = 0;
            centroids.clear();
            return;
        }

        std::vector<float> sub_data(subspace_size);
        for (size_t i = 0; i < n; ++i) {
            const float* row = checkedRow(data, n, d, i);
            if (row == nullptr) {
                THEMIS_ERROR("ScaNN::PQCodebook::train - checkedRow returned nullptr for index {}", i);
                num_subspaces = 0;
                sub_dim = 0;
                centroids.clear();
                return;
            }
            std::copy_n(row + s * sub_dim, sub_dim, sub_data.data() + i * sub_dim);
        }

        std::vector<std::vector<float>> cents;
        std::vector<size_t> asgn;
        kmeans(sub_data.data(), n, sub_dim, num_centroids, iters, cents, asgn);

        // Pad to exactly num_centroids (k-means may produce fewer)
        while (cents.size() < num_centroids) {
            cents.push_back(std::vector<float>(sub_dim, 0.f));
        }
        centroids[s] = std::move(cents);
    }
}

std::vector<uint8_t> ScaNN::PQCodebook::encode(const float* vec, [[maybe_unused]] size_t d) const {
    const size_t expected_dim = num_subspaces * sub_dim;
    if (vec == nullptr || num_subspaces == 0 || sub_dim == 0  || static_cast<size_t>(d) < expected_dim || centroids.size() < num_subspaces) {
        THEMIS_WARN("ScaNN::PQCodebook::encode: invalid input or uninitialized codebook (num_subspaces={} sub_dim={} d={} expected_dim={})",
                    num_subspaces, sub_dim, d, expected_dim);
        return {};
    }

    std::vector<uint8_t> code(num_subspaces);
    for (size_t s = 0; s < num_subspaces; ++s) {
        if (centroids[s].empty()) {
            THEMIS_WARN("ScaNN::PQCodebook::encode: centroid subspace {} empty", s);
            return {};
        }
        const float* sv = vec + s * sub_dim;
        float best = std::numeric_limits<float>::max();
        uint8_t best_c = 0;
        const size_t centroid_count = std::min({num_centroids, centroids[s].size(), static_cast<size_t>(256)});
        for (size_t c = 0; c < centroid_count; ++c) {
            float dist = ScaNN::l2sq(sv, centroids[s][c].data(), sub_dim);
            if (dist < best) { best = dist; best_c = static_cast<uint8_t>(c); }
        }
        code[s] = best_c;
    }
    return code;
}

float ScaNN::PQCodebook::decode_distance(const float* query,
                                          const std::vector<uint8_t>& code) const {
    if (query == nullptr || code.size() < num_subspaces || centroids.size() < num_subspaces) {
        return std::numeric_limits<float>::max();
    }

    float total = 0.f;
    for (size_t s = 0; s < num_subspaces; ++s) {
        if (code[s] >= centroids[s].size()) {
            return std::numeric_limits<float>::max();
        }
        const float* sq = query + s * sub_dim;
        const float* sc = centroids[s][code[s]].data();
        total += ScaNN::l2sq(sq, sc, sub_dim);
    }
    return std::sqrt(total);
}

// ---------------------------------------------------------------------------
// ScaNN public API
// ---------------------------------------------------------------------------

ScaNN::ScaNN(ScaNNConfig cfg) : cfg_(std::move(cfg)) {}

bool ScaNN::build(const float* vectors, const int64_t* ids,
                  size_t count, size_t dim) {
    if (vectors == nullptr || count == 0 || dim == 0 || cfg_.num_leaves == 0) {
      return false;
    }
    if (cfg_.enable_ah && cfg_.pq_num_subspaces == 0) {
      return false;
    }
    if (cfg_.pq_num_subspaces != 0 && dim % cfg_.pq_num_subspaces != 0) {
        // Adjust pq_num_subspaces to be a divisor of dim
        for (size_t s = cfg_.pq_num_subspaces; s >= 1; --s) {
            if (dim % s == 0) { cfg_.pq_num_subspaces = s; break; }
        }
    }
    dim_ = dim;

    // ---- Phase 1: K-means partitioning ----
    size_t k = std::min(cfg_.num_leaves, count);
    std::vector<std::vector<float>> centroids;
    std::vector<size_t> assignments;
    kmeans(vectors, count, dim, k, cfg_.kmeans_iters, centroids, assignments);
    if (static_cast<int>(centroids.size()) != k || assignments.size() != count) {
        return false;
    }

    leaves_.resize(k);
    for (size_t c = 0; c < k; ++c)
        leaves_[c].centroid = std::move(centroids[c]);

    for (size_t i = 0; i < count; ++i) {
        size_t c = assignments[i];
        const float* row = checkedRow(vectors, count, dim, i);
        if (c >= leaves_.size() || row == nullptr) {
            return false;
        }
        int64_t label = ids ? ids[i] : static_cast<int64_t>(i);
        leaves_[c].ids.push_back(label);
        leaves_[c].vectors.emplace_back(row, row + dim);
    }

    // ---- Phase 2: Train PQ codebook & encode leaf vectors ----
    if (cfg_.enable_ah && count >= cfg_.pq_num_subspaces) {
        codebook_.train(vectors, count, dim,
                        cfg_.pq_num_subspaces,
                        cfg_.pq_bits_per_subspace,
                        cfg_.kmeans_iters);

        for (auto& leaf : leaves_) {
            leaf.codes.resize(leaf.vectors.size());
            for (size_t i = 0; i < leaf.vectors.size(); ++i)
                leaf.codes[i] = codebook_.encode(leaf.vectors[i].data(), dim);
        }
    }

    trained_ = true;
    return true;
}

bool ScaNN::add(int64_t id, const float* vector, size_t dim) {
    if (vector == nullptr || dim == 0) {
        return false;
    }

    if (!trained_) {
        // Accumulate in flat buffer; actual build happens lazily when search() is first called
        if (dim_ != 0 && dim != dim_) {
            return false;
        }
        flat_ids_.push_back(id);
        flat_vectors_.emplace_back(vector, vector + dim);
        if (dim_ == 0) {
          dim_ = dim;
        }
        return true;
    }

    if (dim != dim_ || leaves_.empty()) {
        return false;
    }

    // Assign to nearest leaf centroid
    float best_dist = std::numeric_limits<float>::max();
    size_t best_leaf = 0;
    for (size_t i = 0; i < leaves_.size(); ++i) {
        if (leaves_[i].centroid.size() != dim_) {
            THEMIS_WARN("ScaNN::addToLeaf: leaf {} centroid size {} != dim_ {}", i, leaves_[i].centroid.size(), dim_);
            return false;
        }
        float d = l2sq(vector, leaves_[i].centroid.data(), dim_);
        if (d < best_dist) { best_dist = d; best_leaf = i; }
    }

    leaves_[best_leaf].ids.push_back(id);
    leaves_[best_leaf].vectors.emplace_back(vector, vector + dim);
    if (cfg_.enable_ah && codebook_.num_subspaces > 0)
        leaves_[best_leaf].codes.push_back(
            codebook_.encode(vector, dim));

    return true;
}

std::vector<AnnSearchResult> ScaNN::search(const float* query, [[maybe_unused]] size_t dim,
                                            int k) const {
    if (query == nullptr || k <= 0) {
        THEMIS_WARN("ScaNN::search: invalid arguments (query==nullptr={}, k={})", query == nullptr, k);
        return {};
    }

    // Lazy build from flat buffer (thread-safety not required for this path)
    if (!trained_) {
        if (flat_ids_.empty()) { THEMIS_DEBUG("ScaNN::search: flat_ids_ empty while not trained"); return {}; }
        if (flat_vectors_.empty() || flat_vectors_[0].empty()) { THEMIS_DEBUG("ScaNN::search: flat_vectors_ empty while not trained"); return {}; }
        // Const-cast safe because build() writes internal state in a delayed fashion
        ScaNN* self = const_cast<ScaNN*>(this);
        std::vector<float> flat_data = {};

        flat_data.reserve(flat_ids_.size() * flat_vectors_[0].size());
        for (auto& v : self->flat_vectors_)
            flat_data.insert(flat_data.end(), v.begin(), v.end());
        self->build(flat_data.data(), flat_ids_.data(), flat_ids_.size(),
                    flat_vectors_[0].size());
        self->flat_ids_.clear();
        self->flat_vectors_.clear();
    }
    if (leaves_.empty()) { THEMIS_WARN("ScaNN::search: no leaves available"); return {}; }
    if (dim != dim_) { THEMIS_WARN("ScaNN::search: query dim {} != index dim {}", dim, dim_); return {}; }

    // ---- Step 1: Score leaf centroids ----
    size_t probe = std::min(cfg_.num_leaves_to_search, leaves_.size());
    using LeafScore = std::pair<float, size_t>;
    std::vector<LeafScore> leaf_scores(leaves_.size());
    for (size_t i = 0; i < leaves_.size(); ++i)
        leaf_scores[i] = { l2sq(query, leaves_[i].centroid.data(), dim_), i };

    std::partial_sort(leaf_scores.begin(),
                      leaf_scores.begin() + static_cast<std::ptrdiff_t>(probe),
                      leaf_scores.end(),
                      [](const LeafScore& a, const LeafScore& b) {
                          return a.first < b.first;
                      });

    // ---- Step 2: AH scan within selected leaves ----
    size_t reorder_n = std::max(static_cast<size_t>(k), cfg_.reorder_num_neighbors);
    struct FullCandidate { float dist; const Leaf* leaf; size_t idx; };
    std::vector<FullCandidate> candidates;
    candidates.reserve(reorder_n * 2);

    for (size_t pi = 0; pi < probe; ++pi) {
        const Leaf& leaf = leaves_[leaf_scores[pi].second];
        bool use_ah = cfg_.enable_ah && codebook_.num_subspaces > 0
                      && leaf.codes.size() == leaf.vectors.size()
                      && !leaf.codes.empty();

        const size_t scan_count = std::min(leaf.ids.size(), leaf.vectors.size());
        for (size_t i = 0; i < scan_count; ++i) {
            float dist = use_ah
                ? codebook_.decode_distance(query, leaf.codes[i])
                : std::sqrt(l2sq(query, leaf.vectors[i].data(), dim_));
            candidates.push_back({dist, &leaf, i});
        }
    }

    // Keep top reorder_n candidates by approximate distance
    if (static_cast<int>(candidates.size()) > reorder_n) {
        std::partial_sort(candidates.begin(),
                          candidates.begin() + static_cast<std::ptrdiff_t>(reorder_n),
                          candidates.end(),
                          [](const FullCandidate& a, const FullCandidate& b) {
                              return a.dist < b.dist;
                          });
        candidates.resize(reorder_n);
    }

    // ---- Step 3: Exact re-ranking ----
    std::vector<AnnSearchResult> results = {};

    results.reserve(candidates.size());
    for (auto& c : candidates) {
        float exact = std::sqrt(l2sq(query, c.leaf->vectors[c.idx].data(), dim_));
        results.push_back({c.leaf->ids[c.idx], exact});
    }

    std::sort(results.begin(), results.end(),
              [](const AnnSearchResult& a, const AnnSearchResult& b) {
                  return a.distance < b.distance;
              });

    if (static_cast<int>(results.size()) > static_cast<size_t>(k))
        results.resize(static_cast<size_t>(k));

    return results;
}

size_t ScaNN::size() const {
    size_t total = flat_ids_.size();
    for (const auto& leaf : leaves_)
        total += leaf.ids.size();
    return total;
}

bool ScaNN::save(const std::string& path) const {
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs) {
      return false;
    }

    // Write header
    ofs.write(reinterpret_cast<const char*>(&dim_), sizeof(dim_));
    size_t num_leaves = leaves_.size();
    ofs.write(reinterpret_cast<const char*>(&num_leaves), sizeof(num_leaves));

    for (const auto& leaf : leaves_) {
        // centroid
        ofs.write(reinterpret_cast<const char*>(leaf.centroid.data()),
                  sizeof(float) * dim_);
        // ids
        size_t n = leaf.ids.size();
        ofs.write(reinterpret_cast<const char*>(&n), sizeof(n));
        ofs.write(reinterpret_cast<const char*>(leaf.ids.data()),
                  elementSize<std::vector<int64_t>::value_type>() * n);
        // vectors
        for (const auto& v : leaf.vectors)
            ofs.write(reinterpret_cast<const char*>(v.data()),
                      sizeof(float) * dim_);
    }
    return ofs.good();
}

bool ScaNN::load(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
      return false;
    }

    ifs.read(reinterpret_cast<char*>(&dim_), sizeof(dim_));
    size_t num_leaves = 0;
    ifs.read(reinterpret_cast<char*>(&num_leaves), sizeof(num_leaves));

    leaves_.resize(num_leaves);
    for (auto& leaf : leaves_) {
        leaf.centroid.resize(dim_);
        ifs.read(reinterpret_cast<char*>(leaf.centroid.data()),
                 sizeof(float) * dim_);
        size_t n = 0;
        ifs.read(reinterpret_cast<char*>(&n), sizeof(n));
        leaf.ids.resize(n);
        ifs.read(reinterpret_cast<char*>(leaf.ids.data()),
                 elementSize<std::vector<int64_t>::value_type>() * n);
        leaf.vectors.resize(n);
        for (auto& v : leaf.vectors) {
            v.resize(dim_);
            ifs.read(reinterpret_cast<char*>(v.data()), sizeof(float) * dim_);
        }
    }

    trained_ = ifs.good();
    return trained_;
}

} // namespace index
} // namespace themis
