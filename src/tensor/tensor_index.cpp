/**
 * @file tensor_index.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "tensor/tensor_index.h"
#include "storage/tensor_train_decomposer.h"
#include "utils/logger.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <unordered_map>

namespace themis {
namespace tensor {

namespace {

std::vector<size_t> inferFlatModeShape([[maybe_unused]] size_t dim) {
    if (dim == 0) {
        return {1, 1};
    }
    auto rows = static_cast<size_t>(std::sqrt(static_cast<double>(dim)));
    if (rows == 0) {
        rows = 1;
    }
    while (rows > 1 && (dim % rows) != 0) {
        --rows;
    }
    const auto cols = dim / rows;
    return {rows, cols};
}

} // namespace

// ============================================================================
// FlatTensorIndex — linear-scan ITensorIndex (Phase 1 reference impl)
// ============================================================================

/** @brief FlatTensorIndex — linear-scan ITensorIndex (Phase 1 reference impl). */
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
        if (train.cores.empty()) {
          return false;
        }
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
        if (!vector || dim == 0) {
          return false;
        }

        storage::TensorTrainDecomposer decomposer;
        storage::TensorTrainConfig cfg;
        cfg.max_rank = 32;
        cfg.eps      = 0.01;

        std::vector<float> data(vector, vector + dim);
        const auto shape = inferFlatModeShape(dim);
        try {
            auto [train, stats] = decomposer.decompose(data, shape, cfg);
            (void)stats;
            return add(id, train);
        } catch (...) {
            return false;
        }
    }

    bool remove(int64_t id) override {
        std::unique_lock lock(rw_mutex_);
        auto it = store_.find(id);
        if (it == store_.end()) {
          return false;
        }
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

        std::vector<TensorSearchResult> results = {};

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
        if (!query || dim == 0) return {};

        storage::TensorTrainDecomposer decomposer;
        storage::TensorTrainConfig cfg;
        cfg.max_rank = 32;
        cfg.eps      = 0.01;
        const auto shape = inferFlatModeShape(dim);
        std::vector<float> data(query, query + dim);

        try {
            auto [train, stats] = decomposer.decompose(data, shape, cfg);
            (void)stats;
            return search(train, k);
        } catch (...) {
            return {};
        }
    }

    std::optional<float> innerProduct(int64_t id_a,
                                       int64_t id_b) const override {
        std::shared_lock lock(rw_mutex_);
        auto it_a = store_.find(id_a);
        auto it_b = store_.find(id_b);
        if (it_a == store_.end() || it_b == store_.end()) {
          return std::nullopt;
        }
        return ttInnerProduct(it_a->second, it_b->second);
    }

    std::optional<float> norm(int64_t id) const override {
        std::shared_lock lock(rw_mutex_);
        auto it = store_.find(id);
        if (it == store_.end()) {
          return std::nullopt;
        }
        return ttNorm(it->second);
    }

    const storage::TTTrain* get(int64_t id) const override {
        std::shared_lock lock(rw_mutex_);
        auto it = store_.find(id);
        return (it != store_.end()) ? &it->second : nullptr;
    }

    // -----------------------------------------------------------------------
    // Persistence — binary file format (Phase 1)
    //
    // File layout:
    //   magic[11]        "THEMIS_TTI\0"
    //   version: u8      = 1
    //   n_entries: u64
    //   For each entry:
    //     id: i64
    //     n_mode_sizes: u32
    //     mode_sizes[]: u64 × n_mode_sizes
    //     original_norm: f64
    //     achieved_eps:  f64
    //     n_cores: u32
    //     For each core k:
    //       r_left, n, r_right: u64 each
    //       n_floats: u64
    //       float data[n_floats] (4 bytes each)
    //
    // Phase 2 (Q4 2026) will layer a RocksDB key-value representation
    // `__ttidx__:<name>:<id>:G<k>` on top of or instead of this format.
    // -----------------------------------------------------------------------

    static constexpr char kMagic[11] = "THEMIS_TTI";  // 10 chars + '\0'
    static constexpr uint8_t kVersion = 1;

    bool save(const std::string& path) const override {
        std::shared_lock lock(rw_mutex_);
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out) {
          return false;
        }

        out.write(kMagic, 11);
        out.write(reinterpret_cast<const char*>(&kVersion), 1);

        const uint64_t n = static_cast<uint64_t>(store_.size());
        out.write(reinterpret_cast<const char*>(&n), sizeof(n));

        for (const auto& [id, train] : store_) {
            out.write(reinterpret_cast<const char*>(&id), sizeof(id));

            const uint32_t nm = static_cast<uint32_t>(train.mode_sizes.size());
            out.write(reinterpret_cast<const char*>(&nm), sizeof(nm));
            for (auto ms : train.mode_sizes) {
                const uint64_t v = static_cast<uint64_t>(ms);
                out.write(reinterpret_cast<const char*>(&v), sizeof(v));
            }

            out.write(reinterpret_cast<const char*>(&train.original_norm), sizeof(double));
            out.write(reinterpret_cast<const char*>(&train.achieved_eps),  sizeof(double));

            const uint32_t nc = static_cast<uint32_t>(train.cores.size());
            out.write(reinterpret_cast<const char*>(&nc), sizeof(nc));

            for (const auto& core : train.cores) {
                const uint64_t rl = static_cast<uint64_t>(core.r_left);
                const uint64_t nn = static_cast<uint64_t>(core.n);
                const uint64_t rr = static_cast<uint64_t>(core.r_right);
                const uint64_t nf = static_cast<uint64_t>(core.data.size());
                out.write(reinterpret_cast<const char*>(&rl), sizeof(rl));
                out.write(reinterpret_cast<const char*>(&nn), sizeof(nn));
                out.write(reinterpret_cast<const char*>(&rr), sizeof(rr));
                out.write(reinterpret_cast<const char*>(&nf), sizeof(nf));
                out.write(reinterpret_cast<const char*>(core.data.data()),
                          static_cast<std::streamsize>(nf * sizeof(float)));
            }
        }
        return out.good();
    }

    bool load(const std::string& path) override {
        std::ifstream in(path, std::ios::binary);
        if (!in) {
          return false;
        }

        char magic[11];
        in.read(magic, 11);
        if (in.fail() || std::memcmp(magic, kMagic, 10) != 0) {
          return false;
        }

        uint8_t version;
        in.read(reinterpret_cast<char*>(&version), 1);
        if (in.fail() || version != kVersion) {
          return false;
        }

        uint64_t n = {};
        in.read(reinterpret_cast<char*>(&n), sizeof(n));
        if (in.fail()) {
          return false;
        }

        // Build into a local map; only replace store_ on complete success.
        std::unordered_map<int64_t, storage::TTTrain> new_store;
        new_store.reserve(static_cast<size_t>(n));

        for (uint64_t i = 0; i < n; ++i) {
            int64_t id;
            in.read(reinterpret_cast<char*>(&id), sizeof(id));

            uint32_t nm = {};
            in.read(reinterpret_cast<char*>(&nm), sizeof(nm));
            if (in.fail()) {
              return false;
            }

            storage::TTTrain train;
            train.mode_sizes.resize(nm);
            for (uint32_t j = 0; j < nm; ++j) {
                uint64_t v = 0;
                in.read(reinterpret_cast<char*>(&v), sizeof(v));
                train.mode_sizes[j] = static_cast<std::size_t>(v);
            }

            in.read(reinterpret_cast<char*>(&train.original_norm), sizeof(double));
            in.read(reinterpret_cast<char*>(&train.achieved_eps),  sizeof(double));

            uint32_t nc = {};
            in.read(reinterpret_cast<char*>(&nc), sizeof(nc));
            if (in.fail()) {
              return false;
            }

            train.cores.resize(nc);
            for (uint32_t k = 0; k < nc; ++k) {
                uint64_t rl, nn, rr, nf;
                in.read(reinterpret_cast<char*>(&rl), sizeof(rl));
                in.read(reinterpret_cast<char*>(&nn), sizeof(nn));
                in.read(reinterpret_cast<char*>(&rr), sizeof(rr));
                in.read(reinterpret_cast<char*>(&nf), sizeof(nf));
                if (in.fail()) {
                  return false;
                }

                auto& core = train.cores[k];
                core.r_left  = static_cast<std::size_t>(rl);
                core.n       = static_cast<std::size_t>(nn);
                core.r_right = static_cast<std::size_t>(rr);
                core.data.resize(static_cast<std::size_t>(nf));
                in.read(reinterpret_cast<char*>(core.data.data()),
                        static_cast<std::streamsize>(nf * sizeof(float)));
                if (in.fail()) {
                  return false;
                }
            }
            new_store.emplace(id, std::move(train));
        }

        // Atomically swap in new data
        std::unique_lock lock(rw_mutex_);
        store_ = std::move(new_store);
        stats_ = {};
        dim_   = 0;
        for (const auto& [id, train] : store_) {
            stats_.num_vectors++;
            stats_.storage_bytes += estimateBytes(train);
        }
        if (!store_.empty()) {
            dim_ = store_.begin()->second.cores.front().n;
        }
        return true;
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
        if (d == 0 || d != B.cores.size()) {
          return 0.0f;
        }

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
                                if (tIdx >= T.size()) {
                                  continue;
                                }
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


