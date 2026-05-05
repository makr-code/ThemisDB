/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor/tensor_index.cpp                            ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-05                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                     ║
    • Maturity Level:  🟡 EXPERIMENTAL                                 ║
    • Open Issues:     Stubs: 2 (TTI-01, TTI-02)                       ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tensor/tensor_index.cpp
 * @brief Flat-list ITensorIndex implementation — FlatTensorIndex.
 *
 * This file provides `FlatTensorIndex`, the canonical Phase-1 implementation
 * of `ITensorIndex`.  It stores all TT-trains in a hash-map (no HNSW layer)
 * and performs linear-scan search using TT-domain cosine similarity.
 *
 * ### Complexity
 * - add():     O(1)
 * - search():  O(n · d · r²)  — acceptable for n ≤ 50 K at r ≤ 32
 * - norm():    O(d · r³)
 *
 * For n > 50 K switch to `HnswTTBridge` (Phase 2) or pure `ITensorIndex`
 * with a custom ANN routing layer.
 *
 * ### Stub log
 * - TTI-01  `save()` — RocksDB persistence not yet wired (Phase 2, Q4 2026)
 * - TTI-02  `load()` — symmetric
 */

#include "tensor/tensor_index.h"
#include "storage/tensor_train_decomposer.h"
#include "utils/logger.h"

#include <algorithm>
#include <cmath>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <unordered_map>

namespace themis {
namespace tensor {

// ============================================================================
// FlatTensorIndex — linear-scan ITensorIndex (Phase 1 reference impl)
// ============================================================================

class FlatTensorIndex final : public ITensorIndex {
public:
    FlatTensorIndex() = default;
    ~FlatTensorIndex() override = default;

    // -----------------------------------------------------------------------
    // Write path
    // -----------------------------------------------------------------------

    [[nodiscard]] bool add(int64_t id,
                           const storage::TTTrain& train) override {
        std::unique_lock lock(rw_mutex_);
        if (store_.count(id)) return false;   // duplicate
        if (train.cores.empty()) return false;
        store_.emplace(id, train);
        // update dimension from first core shape
        if (dim_ == 0 && !train.cores.empty()) {
            dim_ = train.cores.front().n;
        }
        stats_.num_vectors++;
        stats_.storage_bytes += estimateBytes(train);
        return true;
    }

    [[nodiscard]] bool addFlat(int64_t id,
                               const float* vector,
                               size_t dim) override {
        storage::TensorTrainDecomposer decomposer;
        storage::TensorTrainDecomposer::Config cfg;
        cfg.max_rank = 32;
        cfg.epsilon  = 0.01;

        std::vector<size_t> shape = { dim };  // treat as 1-D for Phase 1
        auto result = decomposer.decompose(vector, shape, cfg);
        if (!result) return false;
        return add(id, *result);
    }

    bool remove(int64_t id) override {
        std::unique_lock lock(rw_mutex_);
        auto it = store_.find(id);
        if (it == store_.end()) return false;
        stats_.storage_bytes -= estimateBytes(it->second);
        store_.erase(it);
        stats_.num_vectors--;
        return true;
    }

    // -----------------------------------------------------------------------
    // Read path
    // -----------------------------------------------------------------------

    std::vector<TensorSearchResult> search(
            const storage::TTTrain& query, int k) const override {
        std::shared_lock lock(rw_mutex_);

        std::vector<TensorSearchResult> results;
        results.reserve(store_.size());

        float q_norm = ttNorm(query);
        if (q_norm < 1e-12f) return {};

        for (const auto& [id, train] : store_) {
            float ip   = ttInnerProduct(query, train);
            float t_n  = ttNorm(train);
            float sim  = (t_n < 1e-12f) ? 0.0f : ip / (q_norm * t_n);
            float dist = 1.0f - sim;   // cosine distance in [0, 2]
            results.push_back({ id, dist, t_n });
        }

        // partial sort — only need top k
        int actual_k = std::min(k, static_cast<int>(results.size()));
        std::partial_sort(results.begin(),
                          results.begin() + actual_k,
                          results.end(),
                          [](const TensorSearchResult& a,
                             const TensorSearchResult& b) {
                              return a.distance < b.distance;
                          });
        results.resize(static_cast<size_t>(actual_k));

        // update rolling average (const method — cast is intentional)
        auto* mutable_self = const_cast<FlatTensorIndex*>(this);
        mutable_self->stats_.total_searches++;

        return results;
    }

    std::vector<TensorSearchResult> searchFlat(
            const float* query, size_t dim, int k) const override {
        storage::TensorTrainDecomposer decomposer;
        storage::TensorTrainDecomposer::Config cfg;
        cfg.max_rank = 32;
        cfg.epsilon  = 0.01;
        std::vector<size_t> shape = { dim };
        auto result = decomposer.decompose(query, shape, cfg);
        if (!result) return {};
        return search(*result, k);
    }

    std::optional<float> innerProduct(int64_t id_a,
                                       int64_t id_b) const override {
        std::shared_lock lock(rw_mutex_);
        auto it_a = store_.find(id_a);
        auto it_b = store_.find(id_b);
        if (it_a == store_.end() || it_b == store_.end()) return std::nullopt;
        return ttInnerProduct(it_a->second, it_b->second);
    }

    std::optional<float> norm(int64_t id) const override {
        std::shared_lock lock(rw_mutex_);
        auto it = store_.find(id);
        if (it == store_.end()) return std::nullopt;
        return ttNorm(it->second);
    }

    const storage::TTTrain* get(int64_t id) const override {
        std::shared_lock lock(rw_mutex_);
        auto it = store_.find(id);
        return (it != store_.end()) ? &it->second : nullptr;
    }

    // -----------------------------------------------------------------------
    // Persistence
    //
    // STUB/SIMULATION NOTE:
    // Purpose: Phase-1 placeholder; RocksDB wire-up deferred to Phase 2
    // Activation: always (save/load are no-ops in Phase 1)
    // Production Delta: real impl uses TensorNetworkStorageEngine key schema
    //   `__ttidx__:<index_name>:<id>:G<k>:<version>`
    // Removal Plan: replace in Phase 2 (Q4 2026) — TTI-01 / TTI-02
    // -----------------------------------------------------------------------

    bool save(const std::string& /*path*/) const override {
        THEMIS_WARN("FlatTensorIndex::save() — RocksDB persistence not yet "
                    "implemented (TTI-01, Phase 2 Q4 2026)");
        return false;
    }

    bool load(const std::string& /*path*/) override {
        THEMIS_WARN("FlatTensorIndex::load() — RocksDB persistence not yet "
                    "implemented (TTI-02, Phase 2 Q4 2026)");
        return false;
    }

    // -----------------------------------------------------------------------
    // Diagnostics
    // -----------------------------------------------------------------------

    [[nodiscard]] size_t size() const override {
        std::shared_lock lock(rw_mutex_);
        return store_.size();
    }

    [[nodiscard]] TensorIndexStats stats() const override {
        std::shared_lock lock(rw_mutex_);
        TensorIndexStats s = stats_;
        s.dim = dim_;
        if (!store_.empty()) {
            size_t rank_sum = 0;
            for (const auto& [id, t] : store_) {
                for (const auto& c : t.cores) {
                    rank_sum += c.r_right;
                }
            }
            s.avg_tt_rank = rank_sum / (store_.size() *
                            (store_.begin()->second.cores.size() + 1));
            // compress ratio vs float32 flat
            float flat_bytes = static_cast<float>(dim_ * sizeof(float));
            float tt_bytes   = (store_.empty()) ? 1.0f
                : static_cast<float>(estimateBytes(store_.begin()->second));
            s.avg_compress_ratio = (tt_bytes > 0)
                ? static_cast<double>(flat_bytes / tt_bytes) : 1.0;
        }
        return s;
    }

private:
    // -----------------------------------------------------------------------
    // TT arithmetic helpers
    // -----------------------------------------------------------------------

    /**
     * @brief TT inner-product <A, B>_TT  — O(d · r³).
     *
     * Uses the left-to-right contraction sweep from Holtz et al. 2012.
     * Transfer matrix M_k = sum_{i_k} G_k^A(:,i_k,:) ⊗ G_k^B(:,i_k,:)
     * contracted left-to-right.
     */
    static float ttInnerProduct(const storage::TTTrain& A,
                                 const storage::TTTrain& B) {
        const size_t d = A.cores.size();
        if (d == 0 || d != B.cores.size()) return 0.0f;

        // Transfer vector T starts as scalar 1
        // At each step T (r_A × r_B) is updated via mode-k contraction.
        // For simplicity we implement the scalar-chain (valid for rank-r, n-mode).

        std::vector<float> T = { 1.0f };  // 1×1 matrix initially

        for (size_t k = 0; k < d; ++k) {
            const auto& gA = A.cores[k];  // r_left × n × r_right
            const auto& gB = B.cores[k];

            size_t rAl = gA.r_left, rAr = gA.r_right;
            size_t rBl = gB.r_left, rBr = gB.r_right;
            size_t n   = gA.n;

            // New transfer matrix T_new: (rAr × rBr)
            std::vector<float> T_new(rAr * rBr, 0.0f);

            for (size_t i = 0; i < n; ++i) {
                // slice gA[:, i, :] shape (rAl × rAr)
                // slice gB[:, i, :] shape (rBl × rBr)
                for (size_t a = 0; a < rAr; ++a) {
                    for (size_t b = 0; b < rBr; ++b) {
                        float acc = 0.0f;
                        for (size_t s = 0; s < rAl; ++s) {
                            for (size_t t = 0; t < rBl; ++t) {
                                size_t tIdx = s * rBl + t; // T is (rAl × rBl)
                                if (tIdx >= T.size()) continue;
                                acc += T[tIdx]
                                     * gA.data[s * n * rAr + i * rAr + a]
                                     * gB.data[t * n * rBr + i * rBr + b];
                            }
                        }
                        T_new[a * rBr + b] += acc;
                    }
                }
            }
            T = std::move(T_new);
        }

        // T should now be a 1×1 scalar
        return T.empty() ? 0.0f : T[0];
    }

    /**
     * @brief Frobenius norm via ‖T‖_F = sqrt(<T,T>_TT).
     */
    static float ttNorm(const storage::TTTrain& T) {
        float ip = ttInnerProduct(T, T);
        return (ip > 0.0f) ? std::sqrt(ip) : 0.0f;
    }

    static size_t estimateBytes(const storage::TTTrain& t) {
        size_t b = 0;
        for (const auto& c : t.cores) {
            b += c.data.size() * sizeof(float);
        }
        return b;
    }

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    mutable std::shared_mutex                          rw_mutex_;
    std::unordered_map<int64_t, storage::TTTrain>      store_;
    size_t                                             dim_ = 0;
    TensorIndexStats                                   stats_;
};

} // namespace tensor
} // namespace themis
