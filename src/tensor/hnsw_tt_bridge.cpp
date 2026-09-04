/**
 * @file hnsw_tt_bridge.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=4, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "tensor/hnsw_tt_bridge.h"
#include <stdexcept>
#include "utils/logger.h"

#ifdef THEMIS_HNSW_ENABLED
#include <hnswlib/hnswlib.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

namespace themis {
namespace tensor {

// ============================================================================
// HnswLayer — hnswlib::HierarchicalNSW<float> when THEMIS_HNSW_ENABLED,
//             linear-scan fallback otherwise (HTB-01 resolved 2026-05-06).
//
// Design notes:
//   • hnswlib is lazily initialised on the first insert() call, at which
//     point the sketch dimension is known.
//   • hnswlib labels are assigned monotonically (next_label_++); id↔label
//     bi-maps allow int64_t IDs to be translated.
//   • Soft-deletion via markDelete() is used; active_count_ tracks the live
//     count because hnswlib doesn't expose it directly.
//   • Both space_ and appr_ are raw pointers managed by ~HnswLayer (RAII via
//     the struct destructor).  The space must outlive the HNSW graph.
//   • Thread safety is provided externally by HnswTTBridge::rw_mutex_.
// ============================================================================

struct HnswTTBridge::HnswLayer {

    // -----------------------------------------------------------------------
    // Linear-scan fallback (only populated when hnswlib is absent or errors)
    // -----------------------------------------------------------------------
    std::unordered_map<int64_t, std::vector<float>> sketches;

#ifdef THEMIS_HNSW_ENABLED
    // -----------------------------------------------------------------------
    // hnswlib resources (Phase 2 — HTB-01)
    // -----------------------------------------------------------------------
    hnswlib::SpaceInterface<float>*   space_   = nullptr;
    hnswlib::HierarchicalNSW<float>*  appr_    = nullptr;
    size_t active_count_    = 0;
    size_t next_label_      = 0;
    size_t M_               = 16;
    size_t ef_construction_ = 200;
    std::unordered_map<int64_t, size_t> id_to_label_;
    std::unordered_map<size_t, int64_t> label_to_id_;
    static constexpr size_t kInitialCapacity = 4096;

    /// Lazy initialisation — called on the first insert() once dim is known.
    /// Exception-safe: cleans up space_ on HierarchicalNSW ctor failure.
    void ensureInit([[maybe_unused]] size_t dim) {
        if (appr_) {
          return;
        }
        auto* sp = new hnswlib::L2Space(dim);
        try {
            appr_ = new hnswlib::HierarchicalNSW<float>(
                sp, kInitialCapacity, M_, ef_construction_);
        } catch (...) {
            delete sp;
            throw;
        }
        space_ = sp;
    }
#endif  // THEMIS_HNSW_ENABLED

    // -----------------------------------------------------------------------
    // Constructor / destructor
    // -----------------------------------------------------------------------

    explicit HnswLayer([[maybe_unused]] size_t M               = 16,
                       [[maybe_unused]] size_t ef_construction = 200) {
#ifdef THEMIS_HNSW_ENABLED
        M_               = M;
        ef_construction_ = ef_construction;
#endif
    }

    ~HnswLayer() {
#ifdef THEMIS_HNSW_ENABLED
        delete appr_;
        delete space_;
#endif
    }

    HnswLayer(const HnswLayer&)            = delete;
    HnswLayer& operator=(const HnswLayer&) = delete;

    // -----------------------------------------------------------------------
    // Write operations
    // -----------------------------------------------------------------------

    void insert(int64_t id, std::vector<float> sketch) {
#ifdef THEMIS_HNSW_ENABLED
        if (!sketch.empty()) {
            try {
                ensureInit(sketch.size());
                // Expand capacity if the pre-allocated pool is exhausted.
                if (next_label_ >= appr_->max_elements_) {
                    appr_->resizeIndex(appr_->max_elements_ * 2 + 1);
                }
                const size_t label = next_label_++;
                appr_->addPoint(sketch.data(), label);
                id_to_label_.emplace(id, label);
                label_to_id_.emplace(label, id);
                ++active_count_;
                return;  // hnswlib path taken — skip linear-scan map
            } catch (const std::exception& ex) {
                THEMIS_WARN("HnswLayer::insert: hnswlib error '{}'; "
                            "falling back to linear scan", ex.what());
            }
        }
#endif
        sketches.emplace(id, std::move(sketch));
    }

    void remove(int64_t id) {
#ifdef THEMIS_HNSW_ENABLED
        if (appr_) {
            auto it = id_to_label_.find(id);
            if (it != id_to_label_.end()) {
                try {
                    appr_->markDelete(it->second);
                } catch (...) {}
                label_to_id_.erase(it->second);
                id_to_label_.erase(it);
                if (active_count_ > 0) {
                  --active_count_;
                }
            }
            return;
        }
#endif
        sketches.erase(id);
    }

    // -----------------------------------------------------------------------
    // Read operations
    // -----------------------------------------------------------------------

    /// ANN search — returns up to `ef` candidate IDs.
    /// Uses hnswlib when available; falls back to linear scan otherwise.
    std::vector<int64_t> search(const std::vector<float>& query,
                                 size_t ef) const {
#ifdef THEMIS_HNSW_ENABLED
        if (appr_ && active_count_ > 0) {
            try {
                const size_t k = std::min(ef, active_count_);
                // hnswlib returns a max-heap keyed by distance (top = worst).
                auto pq = appr_->searchKnn(query.data(), k, nullptr);
                std::vector<int64_t> ids;
                ids.reserve(pq.size());
                while (!pq.empty()) {
                    const auto label = static_cast<size_t>(pq.top().second);
                    pq.pop();
                    auto it = label_to_id_.find(label);
                    if (it != label_to_id_.end())
                        ids.push_back(it->second);
                }
                return ids;
            } catch (const std::exception& ex) {
                THEMIS_WARN("HnswLayer::search: hnswlib error '{}'; "
                            "falling back to linear scan", ex.what());
            }
        }
#endif
        // Linear-scan fallback — O(n) but always correct.
        std::vector<std::pair<float, int64_t>> dist_ids;
        dist_ids.reserve(sketches.size());
        for (const auto& [id, sk] : sketches) {
            float d = 0.0f;
            const size_t dim = std::min(query.size(), sk.size());
            for (size_t i = 0; i < dim; ++i) {
                const float diff = query[i] - sk[i];
                d += diff * diff;
            }
            dist_ids.emplace_back(d, id);
        }
        const size_t take = std::min(ef, dist_ids.size());
        std::partial_sort(dist_ids.begin(), dist_ids.begin() + take,
                          dist_ids.end());
        std::vector<int64_t> ids;
        ids.reserve(take);
        for (size_t i = 0; i < take; ++i)
            ids.push_back(dist_ids[i].second);
        return ids;
    }

    size_t size() const {
#ifdef THEMIS_HNSW_ENABLED
        if (appr_) {
          return active_count_;
        }
#endif
        return sketches.size();
    }
};

// ============================================================================
// TTStore — in-memory TT storage (Phase 1)
// ============================================================================

struct HnswTTBridge::TTStore {
    std::unordered_map<int64_t, storage::TTTrain> trains;

    bool insert(int64_t id, storage::TTTrain t) {
        if (trains.count(id)) {
          return false;
        }
        trains.emplace(id, std::move(t));
        return true;
    }
    bool remove(int64_t id) { return trains.erase(id) > 0; }
    const storage::TTTrain* get(int64_t id) const {
        auto it = trains.find(id);
        return (it != trains.end()) ? &it->second : nullptr;
    }
    size_t size() const { return trains.size(); }
};

// ============================================================================
// HnswTTBridge — constructor / destructor
// ============================================================================

HnswTTBridge::HnswTTBridge(HnswTTConfig config, size_t dim)
    : cfg_(config)
    , dim_(dim)
    , hnsw_(std::make_unique<HnswLayer>(config.M, config.ef_construction))
    , tt_store_(std::make_unique<TTStore>()) {}

HnswTTBridge::~HnswTTBridge() = default;

// ============================================================================
// Write path
// ============================================================================

bool HnswTTBridge::add(int64_t id,
                        const storage::TTTrain& train) {
    std::unique_lock lock(rw_mutex_);
    if (!tt_store_->insert(id, train)) return false;  // duplicate

    auto sketch = extractSketch(train);
    hnsw_->insert(id, std::move(sketch));

    stats_.num_vectors++;
    size_t b = 0;
    for (const auto& c : train.cores) {
      b += c.data.size() * sizeof(float);
    }
    stats_.storage_bytes += b;

    if (dim_ == 0 && !train.cores.empty()) {
      dim_ = train.cores.front().n;
    }
    return true;
}

bool HnswTTBridge::addFlat(int64_t id,
                             const float* vector,
                             size_t dim) {
    if (!vector || dim == 0) {
        return false;
    }

    storage::TensorTrainDecomposer decomposer;
    storage::TensorTrainConfig cfg;
    cfg.max_rank = cfg_.max_tt_rank;
    cfg.eps      = cfg_.epsilon;

    std::vector<float> data(vector, vector + dim);
    std::vector<size_t> shape = { dim };
    try {
        auto [train, stats] = decomposer.decompose(data, shape, cfg);
        (void)stats;
        return add(id, train);
    } catch (...) {
        return false;
    }
}

bool HnswTTBridge::remove(int64_t id) {
    std::unique_lock lock(rw_mutex_);
    if (!tt_store_->remove(id)) {
      return false;
    }
    hnsw_->remove(id);
    stats_.num_vectors = (stats_.num_vectors > 0) ? stats_.num_vectors - 1 : 0;
    return true;
}

// ============================================================================
// Read path
// ============================================================================

std::vector<TensorSearchResult>
HnswTTBridge::search(const storage::TTTrain& query, int k) const {
    std::shared_lock lock(rw_mutex_);

    // Step 1: HNSW navigation on sketch
    auto sketch    = extractSketch(query);
    size_t ef      = std::max(static_cast<size_t>(k), cfg_.rerank_candidates);
    auto candidates = hnsw_->search(sketch, ef);

    // Step 2: TT re-rank
    float q_norm = ttNormFromTrain(query);
    std::vector<TensorSearchResult> results;
    results.reserve(candidates.size());

    for (int64_t cid : candidates) {
        const storage::TTTrain* t = tt_store_->get(cid);
        if (!t) {
          continue;
        }
        float ip  = ttInnerProductFromTrains(query, *t);
        float tn  = ttNormFromTrain(*t);
        float sim = (q_norm < 1e-12f || tn < 1e-12f)
                    ? 0.0f : ip / (q_norm * tn);
        results.push_back({ cid, 1.0f - sim, tn });
    }

    int actual_k = std::min(k, static_cast<int>(results.size()));
    std::partial_sort(results.begin(),
                      results.begin() + actual_k,
                      results.end(),
                      [](const TensorSearchResult& a,
                         const TensorSearchResult& b) {
                          return a.distance < b.distance;
                      });
    results.resize(static_cast<size_t>(actual_k));

    const_cast<HnswTTBridge*>(this)->stats_.total_searches++;
    return results;
}

std::vector<TensorSearchResult>
HnswTTBridge::searchFlat(const float* query, size_t dim, int k) const {
    if (!query || dim == 0) {
        return {};
    }

    storage::TensorTrainDecomposer decomposer;
    storage::TensorTrainConfig cfg;
    cfg.max_rank = cfg_.max_tt_rank;
    cfg.eps      = cfg_.epsilon;
    std::vector<float> data(query, query + dim);
    std::vector<size_t> shape = { dim };
    try {
        auto [train, stats] = decomposer.decompose(data, shape, cfg);
        (void)stats;
        return search(train, k);
    } catch (...) {
        return {};
    }
}

std::optional<float>
HnswTTBridge::innerProduct(int64_t id_a, int64_t id_b) const {
    std::shared_lock lock(rw_mutex_);
    const auto* a = tt_store_->get(id_a);
    const auto* b = tt_store_->get(id_b);
    if (!a || !b) {
      return std::nullopt;
    }
    return ttInnerProductFromTrains(*a, *b);
}

std::optional<float> HnswTTBridge::norm(int64_t id) const {
    std::shared_lock lock(rw_mutex_);
    const auto* t = tt_store_->get(id);
    if (!t) {
      return std::nullopt;
    }
    return ttNormFromTrain(*t);
}

const storage::TTTrain* HnswTTBridge::get(int64_t id) const {
    std::shared_lock lock(rw_mutex_);
    return tt_store_->get(id);
}

// ============================================================================
// Persistence — binary file format (Phase 1)
//
// File layout:
//   magic[11]        "THEMIS_HTB\0"
//   version: u8      = 1
//   n_entries: u64
//   For each entry (TTStore):
//     id: i64
//     n_mode_sizes: u32
//     mode_sizes[]: u64 × n_mode_sizes
//     original_norm: f64
//     achieved_eps:  f64
//     n_cores: u32
//     For each core k:
//       r_left, n, r_right: u64 each
//       n_floats: u64
//       float data[n_floats]
//
// On load, HNSW sketches are re-derived from the loaded TT-trains so that
// the full HNSW+TT state is restored from a single flat file.
// ============================================================================

namespace {
constexpr char kHtbMagic[11]  = "THEMIS_HTB";
constexpr uint8_t kHtbVersion = 1;
} // namespace

bool HnswTTBridge::save(const std::string& path) const {
    std::shared_lock lock(rw_mutex_);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
      return false;
    }

    out.write(kHtbMagic, 11);
    out.write(reinterpret_cast<const char*>(&kHtbVersion), 1);

    const uint64_t n = static_cast<uint64_t>(tt_store_->size());
    out.write(reinterpret_cast<const char*>(&n), sizeof(n));

    for (const auto& [id, train] : tt_store_->trains) {
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

bool HnswTTBridge::load(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
      return false;
    }

    char magic[11];
    in.read(magic, 11);
    if (in.fail() || std::memcmp(magic, kHtbMagic, 10) != 0) {
      return false;
    }

    uint8_t version;
    in.read(reinterpret_cast<char*>(&version), 1);
    if (in.fail() || version != kHtbVersion) {
      return false;
    }

    uint64_t n;
    in.read(reinterpret_cast<char*>(&n), sizeof(n));
    if (in.fail()) {
      return false;
    }

    // Build into local structures; apply atomically on complete success.
    auto new_hnsw = std::make_unique<HnswLayer>(cfg_.M, cfg_.ef_construction);
    auto new_tt   = std::make_unique<TTStore>();

    for (uint64_t i = 0; i < n; ++i) {
        int64_t id;
        in.read(reinterpret_cast<char*>(&id), sizeof(id));

        uint32_t nm;
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

        uint32_t nc;
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

        // Re-derive HNSW sketch from loaded train
        auto sketch = extractSketch(train);
        new_hnsw->insert(id, std::move(sketch));
        new_tt->insert(id, std::move(train));
    }

    if (in.fail()) {
      return false;
    }

    // Atomically swap in restored state
    std::unique_lock lock(rw_mutex_);
    tt_store_ = std::move(new_tt);
    hnsw_     = std::move(new_hnsw);
    stats_    = {};
    dim_      = 0;

    for (const auto& [id, t] : tt_store_->trains) {
        stats_.num_vectors++;
        size_t b = 0;
        for (const auto& c : t.cores) {
          b += c.data.size() * sizeof(float);
        }
        stats_.storage_bytes += b;
        if (dim_ == 0 && !t.cores.empty()) {
          dim_ = t.cores.front().n;
        }
    }
    return true;
}

// ============================================================================
// Diagnostics
// ============================================================================

size_t HnswTTBridge::size() const {
    std::shared_lock lock(rw_mutex_);
    return tt_store_->size();
}

TensorIndexStats HnswTTBridge::stats() const {
    std::shared_lock lock(rw_mutex_);
    TensorIndexStats s = stats_;
    s.dim = dim_;
    return s;
}

std::vector<float> HnswTTBridge::getSketch(int64_t id) const {
    std::shared_lock lock(rw_mutex_);
    const auto* t = tt_store_->get(id);
    if (!t) return {};
    return extractSketch(*t);
}

// ============================================================================
// Private helpers
// ============================================================================

std::vector<float>
HnswTTBridge::extractSketch(const storage::TTTrain& train) const {
    if (train.cores.empty()) return {};
    const auto& g0 = train.cores.front();
    // First core G_0: shape (1, n_0, r_0)  → flatten to n_0-dim sketch
    size_t sketch_len = (cfg_.sketch_dim > 0)
                        ? std::min(g0.n, cfg_.sketch_dim)
                        : g0.n;
    std::vector<float> sk(sketch_len);
    for (size_t i = 0; i < sketch_len; ++i) {
        // G_0(0, i, :) is a row of length r_right; take mean as sketch value
        float mean = 0.0f;
        for (size_t r = 0; r < g0.r_right; ++r) {
            mean += g0.data[i * g0.r_right + r];
        }
        sk[i] = (g0.r_right > 0) ? mean / static_cast<float>(g0.r_right) : 0.f;
    }
    return sk;
}

float HnswTTBridge::ttCosineSimilarity(const storage::TTTrain& a,
                                        const storage::TTTrain& b) const {
    float ip = ttInnerProductFromTrains(a, b);
    float na = ttNormFromTrain(a);
    float nb = ttNormFromTrain(b);
    if (na < 1e-12f || nb < 1e-12f) {
      return 0.0f;
    }
    return ip / (na * nb);
}

// TT inner-product — same sweep as FlatTensorIndex
float HnswTTBridge::ttInnerProductFromTrains(const storage::TTTrain& A,
                                              const storage::TTTrain& B) {
    const size_t d = A.cores.size();
    if (d == 0 || d != B.cores.size()) {
      return 0.0f;
    }

    std::vector<float> T = { 1.0f };

    for (size_t k = 0; k < d; ++k) {
        const auto& gA = A.cores[k];
        const auto& gB = B.cores[k];
        size_t rAl = gA.r_left, rAr = gA.r_right;
        size_t rBl = gB.r_left, rBr = gB.r_right;
        size_t n   = gA.n;

        std::vector<float> T_new(rAr * rBr, 0.0f);
        for (size_t i = 0; i < n; ++i) {
            for (size_t a = 0; a < rAr; ++a) {
                for (size_t b = 0; b < rBr; ++b) {
                    float acc = 0.0f;
                    for (size_t s = 0; s < rAl; ++s) {
                        for (size_t t = 0; t < rBl; ++t) {
                            size_t tIdx = s * rBl + t;
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
    return T.empty() ? 0.0f : T[0];
}

float HnswTTBridge::ttNormFromTrain(const storage::TTTrain& T) {
    float ip = ttInnerProductFromTrains(T, T);
    return (ip > 0.0f) ? std::sqrt(ip) : 0.0f;
}

} // namespace tensor
} // namespace themis


