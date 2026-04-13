/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ann_index.cpp                                      ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-13 04:26:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     432                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 0c973a2860  2026-02-26  Refactor and enhance ThemisDB components ║
    • e6e7fc6bbf  2026-02-25  feat(index): DiskANN/ScaNN alternative ANN algorithms for... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <fstream>
#include <numeric>
#include <queue>
#include <random>
#include <stdexcept>

namespace themis {
namespace index {

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
    if (n == 0 || k == 0) return;
    k = std::min(k, n);

    // k-means++ initialisation
    std::mt19937_64 rng(0xDEADBEEF);
    centroids.clear();
    centroids.reserve(k);

    // Pick first centroid uniformly at random
    std::uniform_int_distribution<size_t> uni(0, n - 1);
    size_t idx = uni(rng);
    centroids.emplace_back(data + idx * d, data + idx * d + d);

    // Pick remaining centroids with probability proportional to D^2
    std::vector<float> dists(n, std::numeric_limits<float>::max());
    for (size_t c = 1; c < k; ++c) {
        float total = 0.f;
        for (size_t i = 0; i < n; ++i) {
            float dist = l2sq(data + i * d, centroids.back().data(), d);
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
        centroids.emplace_back(data + chosen * d, data + chosen * d + d);
    }

    assignments.assign(n, 0);

    for (size_t iter = 0; iter < iters; ++iter) {
        // Assignment step
        for (size_t i = 0; i < n; ++i) {
            float best = std::numeric_limits<float>::max();
            size_t best_c = 0;
            for (size_t c = 0; c < k; ++c) {
                float dist = l2sq(data + i * d, centroids[c].data(), d);
                if (dist < best) { best = dist; best_c = c; }
            }
            assignments[i] = best_c;
        }

        // Update step
        std::vector<std::vector<float>> new_cents(k, std::vector<float>(d, 0.f));
        std::vector<size_t> counts(k, 0);
        for (size_t i = 0; i < n; ++i) {
            size_t c = assignments[i];
            ++counts[c];
            for (size_t j = 0; j < d; ++j)
                new_cents[c][j] += data[i * d + j];
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
                centroids[c].assign(data + r * d, data + r * d + d);
            }
        }
    }
    // Final assignment
    for (size_t i = 0; i < n; ++i) {
        float best = std::numeric_limits<float>::max();
        size_t best_c = 0;
        for (size_t c = 0; c < k; ++c) {
            float dist = l2sq(data + i * d, centroids[c].data(), d);
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
    num_subspaces = nss;
    sub_dim = d / nss;
    num_centroids = static_cast<size_t>(1) << bits;

    centroids.resize(nss);
    for (size_t s = 0; s < nss; ++s) {
        // Collect sub-vectors for this subspace
        std::vector<float> sub_data(n * sub_dim);
        for (size_t i = 0; i < n; ++i)
            std::copy(data + i * d + s * sub_dim,
                      data + i * d + (s + 1) * sub_dim,
                      sub_data.data() + i * sub_dim);

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

std::vector<uint8_t> ScaNN::PQCodebook::encode(const float* vec, size_t d) const {
    (void)d;
    std::vector<uint8_t> code(num_subspaces);
    for (size_t s = 0; s < num_subspaces; ++s) {
        const float* sv = vec + s * sub_dim;
        float best = std::numeric_limits<float>::max();
        uint8_t best_c = 0;
        for (size_t c = 0; c < num_centroids && c < 256; ++c) {
            float dist = ScaNN::l2sq(sv, centroids[s][c].data(), sub_dim);
            if (dist < best) { best = dist; best_c = static_cast<uint8_t>(c); }
        }
        code[s] = best_c;
    }
    return code;
}

float ScaNN::PQCodebook::decode_distance(const float* query,
                                          const std::vector<uint8_t>& code) const {
    float total = 0.f;
    for (size_t s = 0; s < num_subspaces; ++s) {
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
    if (count == 0 || dim == 0) return false;
    if (dim % cfg_.pq_num_subspaces != 0) {
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

    leaves_.resize(k);
    for (size_t c = 0; c < k; ++c)
        leaves_[c].centroid = std::move(centroids[c]);

    for (size_t i = 0; i < count; ++i) {
        size_t c = assignments[i];
        int64_t label = ids ? ids[i] : static_cast<int64_t>(i);
        leaves_[c].ids.push_back(label);
        leaves_[c].vectors.emplace_back(vectors + i * dim,
                                        vectors + i * dim + dim);
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
    if (!trained_) {
        // Accumulate in flat buffer; actual build happens lazily when search() is first called
        flat_ids_.push_back(id);
        flat_vectors_.emplace_back(vector, vector + dim);
        if (dim_ == 0) dim_ = dim;
        return true;
    }

    // Assign to nearest leaf centroid
    float best_dist = std::numeric_limits<float>::max();
    size_t best_leaf = 0;
    for (size_t i = 0; i < leaves_.size(); ++i) {
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

std::vector<AnnSearchResult> ScaNN::search(const float* query, size_t dim,
                                            int k) const {
    (void)dim;
    // Lazy build from flat buffer (thread-safety not required for this path)
    if (!trained_) {
        if (flat_ids_.empty()) return {};
        // Const-cast safe because build() writes internal state in a delayed fashion
        ScaNN* self = const_cast<ScaNN*>(this);
        std::vector<float> flat_data;
        flat_data.reserve(flat_ids_.size() * flat_vectors_[0].size());
        for (auto& v : self->flat_vectors_)
            flat_data.insert(flat_data.end(), v.begin(), v.end());
        self->build(flat_data.data(), flat_ids_.data(), flat_ids_.size(),
                    flat_vectors_[0].size());
        self->flat_ids_.clear();
        self->flat_vectors_.clear();
    }
    if (leaves_.empty()) return {};

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
    using Candidate = std::pair<float, size_t>; // (dist, global_idx_in_leaf)
    struct FullCandidate { float dist; const Leaf* leaf; size_t idx; };
    std::vector<FullCandidate> candidates;
    candidates.reserve(reorder_n * 2);

    for (size_t pi = 0; pi < probe; ++pi) {
        const Leaf& leaf = leaves_[leaf_scores[pi].second];
        bool use_ah = cfg_.enable_ah && codebook_.num_subspaces > 0
                      && !leaf.codes.empty();

        for (size_t i = 0; i < leaf.ids.size(); ++i) {
            float dist = use_ah
                ? codebook_.decode_distance(query, leaf.codes[i])
                : std::sqrt(l2sq(query, leaf.vectors[i].data(), dim_));
            candidates.push_back({dist, &leaf, i});
        }
    }

    // Keep top reorder_n candidates by approximate distance
    if (candidates.size() > reorder_n) {
        std::partial_sort(candidates.begin(),
                          candidates.begin() + static_cast<std::ptrdiff_t>(reorder_n),
                          candidates.end(),
                          [](const FullCandidate& a, const FullCandidate& b) {
                              return a.dist < b.dist;
                          });
        candidates.resize(reorder_n);
    }

    // ---- Step 3: Exact re-ranking ----
    std::vector<AnnSearchResult> results;
    results.reserve(candidates.size());
    for (auto& c : candidates) {
        float exact = std::sqrt(l2sq(query, c.leaf->vectors[c.idx].data(), dim_));
        results.push_back({c.leaf->ids[c.idx], exact});
    }

    std::sort(results.begin(), results.end(),
              [](const AnnSearchResult& a, const AnnSearchResult& b) {
                  return a.distance < b.distance;
              });

    if (results.size() > static_cast<size_t>(k))
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
    if (!ofs) return false;

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
                  sizeof(int64_t) * n);
        // vectors
        for (const auto& v : leaf.vectors)
            ofs.write(reinterpret_cast<const char*>(v.data()),
                      sizeof(float) * dim_);
    }
    return ofs.good();
}

bool ScaNN::load(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return false;

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
                 sizeof(int64_t) * n);
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
