/**
 * @file tensor_index.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
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
#include <limits>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <unordered_map>

namespace themis {
namespace tensor {

namespace {

/**
 * @brief Infer Flat Mode Shape.
 * @param[in] dim Input parameter.
 * @return Return value.
 * @details Calls: std::sqrt().
 */
std::vector<size_t> inferFlatModeShape(size_t dim) {
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

// ============================================================================
// FlatTensorIndex — linear-scan ITensorIndex (Phase 1 reference impl)
// Internal to this TU; construct via createFlatTensorIndex().
// ============================================================================

class FlatTensorIndex final : public ITensorIndex {
public:
    FlatTensorIndex(size_t max_rank, double epsilon)
        : max_rank_(max_rank), epsilon_(epsilon) {}
    ~FlatTensorIndex() override = default;

    // -----------------------------------------------------------------------
    // Write path
    // -----------------------------------------------------------------------

    [[nodiscard]] bool add(int64_t id,
                           const storage::TTTrain& train) override {
        /**
         * @brief Lock.
         * @param[in] rw_mutex_ Input parameter.
         * @return Return value.
         */
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
        cfg.max_rank = max_rank_;
        cfg.eps      = epsilon_;

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
        /**
         * @brief Lock.
         * @param[in] rw_mutex_ Input parameter.
         * @return Return value.
         */
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
        /**
         * @brief Lock.
         * @param[in] rw_mutex_ Input parameter.
         * @return Return value.
         */
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
        if (k <= 0 || results.empty()) {
            return {};
        }
        const size_t actual_k = std::min(results.size(), static_cast<size_t>(k));
        std::partial_sort(results.begin(),
                          results.begin() + actual_k,
                          results.end(),
                          [](const TensorSearchResult& a,
                             const TensorSearchResult& b) {
                              return a.distance < b.distance;
                          });
        results.resize(actual_k);

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
        cfg.max_rank = max_rank_;
        cfg.eps      = epsilon_;
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
        /**
         * @brief Lock.
         * @param[in] rw_mutex_ Input parameter.
         * @return Return value.
         */
        std::shared_lock lock(rw_mutex_);
        auto it_a = store_.find(id_a);
        auto it_b = store_.find(id_b);
        if (it_a == store_.end() || it_b == store_.end()) {
          return std::nullopt;
        }
        return ttInnerProduct(it_a->second, it_b->second);
    }

    std::optional<float> norm(int64_t id) const override {
        /**
         * @brief Lock.
         * @param[in] rw_mutex_ Input parameter.
         * @return Return value.
         */
        std::shared_lock lock(rw_mutex_);
        auto it = store_.find(id);
        if (it == store_.end()) {
          return std::nullopt;
        }
        return ttNorm(it->second);
    }

    const storage::TTTrain* get(int64_t id) const override {
        /**
         * @brief Lock.
         * @param[in] rw_mutex_ Input parameter.
         * @return Return value.
         */
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
        /**
         * @brief Lock.
         * @param[in] rw_mutex_ Input parameter.
         * @return Return value.
         */
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
        // Parse into a local map; only replace store_ on complete success.
        std::unordered_map<int64_t, storage::TTTrain> new_store;
        try {
            if (!parseFile(path, new_store)) {
                return false;
            }
        } catch (const std::exception&) {
            // A corrupt or hostile file must never propagate bad_alloc /
            // length_error into server startup (createIndex restores on boot).
            return false;
        }

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
        /**
         * @brief Lock.
         * @param[in] rw_mutex_ Input parameter.
         * @return Return value.
         */
        std::shared_lock lock(rw_mutex_);
        return store_.size();
    }

    [[nodiscard]] TensorIndexStats stats() const override {
        /**
         * @brief Lock.
         * @param[in] rw_mutex_ Input parameter.
         * @return Return value.
         */
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
     * @brief Tt Inner Product.
     * @param[in] A Input parameter.
     * @param[in] B Input parameter.
     * @return Return value.
     * @details Calls: size(), T_new(), std::move(), empty().
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
     * @brief Tt Norm.
     * @param[in] T Input parameter.
     * @return Return value.
     * @details Calls: ttInnerProduct(), std::sqrt().
     */
    static float ttNorm(const storage::TTTrain& T) {
        float ip = ttInnerProduct(T, T);
        return (ip > 0.0f) ? std::sqrt(ip) : 0.0f;
    }

    // -----------------------------------------------------------------------
    // .ttidx parser — the file is untrusted, so every count is bounded by the
    // bytes still available before any allocation happens.
    // -----------------------------------------------------------------------

    /// Minimum on-disk bytes of one entry: id + n_modes + norm + eps + n_cores.
    static constexpr uint64_t kMinEntryBytes   = 8 + 4 + 8 + 8 + 4;
    /// On-disk bytes of a core header: r_left, n, r_right, n_floats.
    static constexpr uint64_t kCoreHeaderBytes = 4 * 8;
    /// Upper bound on TT order (modes / cores) accepted from disk.
    static constexpr uint32_t kMaxTtOrder      = 64;

    static bool checkedMul(uint64_t a, uint64_t b, uint64_t& out) {
        if (a != 0 && b > std::numeric_limits<uint64_t>::max() / a) {
            return false;
        }
        out = a * b;
        return true;
    }

    static bool parseFile(const std::string& path,
                          std::unordered_map<int64_t, storage::TTTrain>& out_store) {
        std::ifstream in(path, std::ios::binary | std::ios::ate);
        if (!in) {
            return false;
        }
        const auto end_pos = in.tellg();
        if (end_pos < 0) {
            return false;
        }
        const auto file_size = static_cast<uint64_t>(end_pos);
        in.seekg(0, std::ios::beg);

        uint64_t consumed = 0;
        auto remaining = [&]() { return file_size - consumed; };
        auto read = [&](void* dst, uint64_t bytes) -> bool {
            if (bytes > remaining()) {
                return false;
            }
            in.read(static_cast<char*>(dst), static_cast<std::streamsize>(bytes));
            if (in.fail()) {
                return false;
            }
            consumed += bytes;
            return true;
        };

        char magic[11];
        if (!read(magic, sizeof(magic)) ||
            std::memcmp(magic, kMagic, sizeof(kMagic)) != 0) {
            return false;
        }
        uint8_t version = 0;
        if (!read(&version, sizeof(version)) || version != kVersion) {
            return false;
        }
        uint64_t n = 0;
        if (!read(&n, sizeof(n)) ||
            n > static_cast<uint64_t>(std::numeric_limits<size_t>::max()) ||
            n > remaining() / kMinEntryBytes) {
            return false;
        }

        std::unordered_map<int64_t, storage::TTTrain> store;
        store.reserve(static_cast<size_t>(n));

        for (uint64_t i = 0; i < n; ++i) {
            int64_t id = 0;
            if (!read(&id, sizeof(id)) || store.count(id) != 0) {
                return false;
            }

            uint32_t nm = 0;
            if (!read(&nm, sizeof(nm)) || nm > kMaxTtOrder ||
                uint64_t{nm} * sizeof(uint64_t) > remaining()) {
                return false;
            }
            storage::TTTrain train;
            train.mode_sizes.resize(nm);
            for (auto& ms : train.mode_sizes) {
                uint64_t v = 0;
                if (!read(&v, sizeof(v)) ||
                    v > static_cast<uint64_t>(
                            std::numeric_limits<size_t>::max())) {
                    return false;
                }
                ms = static_cast<std::size_t>(v);
            }

            uint32_t nc = 0;
            if (!read(&train.original_norm, sizeof(double)) ||
                !read(&train.achieved_eps, sizeof(double)) ||
                !read(&nc, sizeof(nc))) {
                return false;
            }
            if (nc == 0 || nc > kMaxTtOrder ||
                uint64_t{nc} * kCoreHeaderBytes > remaining()) {
                return false;
            }

            train.cores.resize(nc);
            uint64_t prev_r_right = 1;  // TT boundary condition: r_0 == 1
            for (auto& core : train.cores) {
                uint64_t rl = 0, nn = 0, rr = 0, nf = 0;
                if (!read(&rl, sizeof(rl)) || !read(&nn, sizeof(nn)) ||
                    !read(&rr, sizeof(rr)) || !read(&nf, sizeof(nf))) {
                    return false;
                }
                if (rl == 0 || nn == 0 || rr == 0 || rl != prev_r_right) {
                    return false;
                }
                const auto max_size =
                    static_cast<uint64_t>(std::numeric_limits<size_t>::max());
                if (rl > max_size || nn > max_size || rr > max_size ||
                    nf > max_size) {
                    return false;
                }
                uint64_t expected = 0;
                uint64_t bytes    = 0;
                if (!checkedMul(rl, nn, expected) ||
                    !checkedMul(expected, rr, expected) || nf != expected ||
                    !checkedMul(nf, sizeof(float), bytes) || bytes > remaining()) {
                    return false;
                }
                core.r_left  = static_cast<std::size_t>(rl);
                core.n       = static_cast<std::size_t>(nn);
                core.r_right = static_cast<std::size_t>(rr);
                core.data.resize(static_cast<std::size_t>(nf));
                if (!read(core.data.data(), bytes)) {
                    return false;
                }
                prev_r_right = rr;
            }
            if (prev_r_right != 1) {  // TT boundary condition: r_d == 1
                return false;
            }
            store.emplace(id, std::move(train));
        }

        out_store = std::move(store);
        return true;
    }

    /**
     * @brief Estimate Bytes.
     * @param[in] t Input parameter.
     * @return Return value.
     * @details Calls: size().
     */
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
    const size_t                                       max_rank_;
    const double                                       epsilon_;
};

} // namespace

std::unique_ptr<ITensorIndex> createFlatTensorIndex(size_t max_rank, double epsilon) {
    if (!std::isfinite(epsilon) || epsilon <= 0.0 || epsilon >= 1.0) {
        throw std::invalid_argument(
            "createFlatTensorIndex: epsilon must be finite and in (0, 1)");
    }
    return std::make_unique<FlatTensorIndex>(max_rank, epsilon);
}

} // namespace tensor
} // namespace themis


