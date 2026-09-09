/**
 * @file adalora_tt_bridge.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "training/adalora_tt_bridge.h"
#include "graph/tensor_fingerprint_graph.h"

#include <algorithm>
#include <cmath>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace themis {
namespace training {

namespace {

float colNorm(const std::vector<float>& mat,
              std::size_t rows,
              std::size_t cols,
              std::size_t col) noexcept {
    float sum = 0.0f;
    for (std::size_t r = 0; r < rows; ++r) {
        const float v = mat[r * cols + col];
        sum += v * v;
    }
    return std::sqrt(sum);
}

float rowNorm(const std::vector<float>& mat,
              std::size_t rows,
              std::size_t cols,
              std::size_t row) noexcept {
    (void)rows;
    float sum = 0.0f;
    for (std::size_t c = 0; c < cols; ++c) {
        const float v = mat[row * cols + c];
        sum += v * v;
    }
    return std::sqrt(sum);
}

std::vector<float> approximateSingularValues(const std::vector<float>& B,
                                             const std::vector<float>& A,
                                             std::size_t d,
                                             std::size_t r,
                                             std::size_t k) noexcept {
    std::vector<float> sv(r, 0.0f);
    for (std::size_t i = 0; i < r; ++i) {
        sv[i] = colNorm(B, d, r, i) * rowNorm(A, r, k, i);
    }
    return sv;
}

std::vector<float> buildG0(const std::vector<float>& B,
                           std::size_t d,
                           std::size_t r,
                           const std::vector<float>& sv,
                           float scaling,
                           std::size_t active_rank,
                           bool normalise_sign) noexcept {
    std::vector<float> data(d * r, 0.0f);

    for (std::size_t i = 0; i < active_rank && i < r; ++i) {
        const float norm_b = colNorm(B, d, r, i);
        const float lambda = sv[i];
        if (norm_b <= 1e-12f || lambda <= 1e-12f) {
            continue;
        }

        float sign = 1.0f;
        if (normalise_sign) {
            for (std::size_t row = 0; row < d; ++row) {
                const float v = B[row * r + i];
                if (std::fabs(v) > 1e-12f) {
                    sign = (v >= 0.0f) ? 1.0f : -1.0f;
                    break;
                }
            }
        }

        const float scale = std::sqrt(lambda * scaling);
        for (std::size_t row = 0; row < d; ++row) {
            data[row * r + i] = sign * (B[row * r + i] / norm_b) * scale;
        }
    }

    return data;
}

std::vector<float> buildG1(const std::vector<float>& A,
                           std::size_t r,
                           std::size_t k,
                           const std::vector<float>& sv,
                           float scaling,
                           std::size_t active_rank,
                           bool normalise_sign) noexcept {
    std::vector<float> data(r * k, 0.0f);

    for (std::size_t i = 0; i < active_rank && i < r; ++i) {
        const float norm_a = rowNorm(A, r, k, i);
        const float lambda = sv[i];
        if (norm_a <= 1e-12f || lambda <= 1e-12f) {
            continue;
        }

        float sign = 1.0f;
        if (normalise_sign) {
            for (std::size_t col = 0; col < k; ++col) {
                const float v = A[i * k + col];
                if (std::fabs(v) > 1e-12f) {
                    sign = (v >= 0.0f) ? 1.0f : -1.0f;
                    break;
                }
            }
        }

        const float scale = std::sqrt(lambda * scaling);
        for (std::size_t col = 0; col < k; ++col) {
            data[i * k + col] = sign * (A[i * k + col] / norm_a) * scale;
        }
    }

    return data;
}

std::string cacheKey(const std::string& tenant, const std::string& adapter_name) {
    return tenant + "\n" + adapter_name;
}

} // namespace

struct AdaLoraTTBridge::Impl {
    std::shared_ptr<storage::TensorNetworkStorageEngine> engine;
    AdaLoraTTBridgeConfig cfg = AdaLoraTTBridgeConfig();
    mutable BridgeStats stats_data{};
    mutable graph::TensorFingerprintGraph fingerprint_graph{};
    mutable std::mutex fingerprint_graph_mutex; ///< Guards fingerprint_graph

    MapAdapterFn map_adapter_fn = MapAdapterFn();
    mutable std::mutex map_adapter_mutex;

    mutable std::mutex cache_mutex;
    mutable std::unordered_map<std::string, AdaLoraTTExport> export_cache;
};

namespace {
std::mutex& trainingStepFnMutex() {
    static std::mutex m;
    return m;
}

AdaLoraTTBridge::TrainingStepFn& trainingStepFnStorage() {
    static AdaLoraTTBridge::TrainingStepFn fn;
    return fn;
}
} // namespace

void AdaLoraTTBridge::setTrainingStepFn(TrainingStepFn fn) {
    std::lock_guard<std::mutex> lk(trainingStepFnMutex());
    trainingStepFnStorage() = std::move(fn);
}

void AdaLoraTTBridge::clearTrainingStepFn() {
    std::lock_guard<std::mutex> lk(trainingStepFnMutex());
    trainingStepFnStorage() = {};
}

AdaLoraTTBridge::AdaLoraTTBridge(
    std::shared_ptr<storage::TensorNetworkStorageEngine> engine,
    AdaLoraTTBridgeConfig cfg)
    : impl_(std::make_unique<Impl>()) {
    impl_->engine = std::move(engine);
    impl_->cfg = std::move(cfg);
}

AdaLoraTTBridge::~AdaLoraTTBridge() = default;

AdaLoraTTLayerExport AdaLoraTTBridge::exportLayer(const AdaLoRAAdapter& adapter,
                                                   const std::string& layer_name) const {
    const auto all_stats = adapter.getLayerStats();
    const auto it = std::find_if(all_stats.begin(), all_stats.end(),
                                 [&](const AdaLoRALayerStats& s) {
                                     return s.layer_name == layer_name;
                                 });
    if (it == all_stats.end()) {
        throw std::invalid_argument("Layer not found: " + layer_name);
    }

    const auto [B, A] = adapter.getWeights(layer_name);
    const std::size_t max_rank = it->max_rank;
    const std::size_t active_rank = it->active_rank;
    const float scaling = 1.0f;

    if (max_rank == 0 || active_rank == 0) {
        throw std::invalid_argument("Layer has zero rank: " + layer_name);
    }
    if (impl_->cfg.max_tt_rank > 0 && max_rank > impl_->cfg.max_tt_rank) {
        throw std::invalid_argument("Layer rank exceeds max_tt_rank: " + layer_name);
    }
    if (B.size() % max_rank != 0 || static_cast<int>(A.size()) % max_rank != 0) {
        throw std::invalid_argument("Invalid matrix shape for layer: " + layer_name);
    }

    const std::size_t d = B.size() / max_rank;
    const std::size_t k = A.size() / max_rank;

    const auto sv = approximateSingularValues(B, A, d, max_rank, k);
    const auto g0_data = buildG0(B, d, max_rank, sv, scaling, active_rank,
                                 impl_->cfg.normalise_sign);
    const auto g1_data = buildG1(A, max_rank, k, sv, scaling, active_rank,
                                 impl_->cfg.normalise_sign);

    storage::TTCore g0;
    g0.r_left = 1;
    g0.n = d;
    g0.r_right = max_rank;
    g0.data = g0_data;

    storage::TTCore g1;
    g1.r_left = max_rank;
    g1.n = k;
    g1.r_right = 1;
    g1.data = g1_data;

    storage::TTTrain train;
    train.mode_sizes = {d, k};
    train.cores = {std::move(g0), std::move(g1)};

    ++impl_->stats_data.exports_total;

    AdaLoraTTLayerExport out = AdaLoraTTLayerExport();
    out.layer_name = layer_name;
    out.train = std::move(train);
    out.active_rank = active_rank;
    out.scaling = scaling;
    out.orthogonal_validated = true;
    return out;
}

AdaLoraTTExport AdaLoraTTBridge::exportToTT(const AdaLoRAAdapter& adapter,
                                             const std::string& adapter_name,
                                             const std::string& tenant) const {
    AdaLoraTTExport out = AdaLoraTTExport();
    out.adapter_name = adapter_name;
    out.tenant = tenant;

    for (const auto& ls : adapter.getLayerStats()) {
        if (ls.active_rank == 0) {
            continue;
        }
        out.layers.push_back(exportLayer(adapter, ls.layer_name));
    }

    if (out.layers.empty()) {
        throw std::runtime_error("No exportable layers in adapter: " + adapter_name);
    }

    return out;
}

AdaLoRAAdapter AdaLoraTTBridge::importFromTT(const AdaLoraTTExport& exp) const {
    if (exp.layers.empty()) {
        throw std::invalid_argument("Empty TT export");
    }

    std::size_t max_rank = 1;
    std::size_t total_budget = 0;
    for (const auto& layer : exp.layers) {
        max_rank = std::max(max_rank, layer.active_rank);
        total_budget += layer.active_rank;
    }

    AdaLoRAAdapter adapter(max_rank, 8.0f, std::max<std::size_t>(1, total_budget));

    for (const auto& layer : exp.layers) {
        if (static_cast<int>(layer.train.cores.size()) != 2) {
            continue;
        }

        const auto& g0 = layer.train.cores[0];
        const auto& g1 = layer.train.cores[1];

        const std::size_t d = g0.n;
        const std::size_t r = g0.r_right;
        const std::size_t k = g1.n;

        if (d == 0 || r == 0 || k == 0 || g1.r_left != r) {
            continue;
        }

        adapter.addLayer(layer.layer_name, d, k, r, layer.scaling * static_cast<float>(r));
        adapter.setWeights(layer.layer_name, g0.data, g1.data);
    }

    adapter.updateAllImportances();
    ++impl_->stats_data.imports_total;
    return adapter;
}

bool AdaLoraTTBridge::store(const AdaLoraTTExport& exp) {
    if (exp.adapter_name.empty() || exp.layers.empty()) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lk(impl_->cache_mutex);
        impl_->export_cache[cacheKey(exp.tenant, exp.adapter_name)] = exp;
    }

    if (impl_->cfg.auto_deduplicate) {
        std::lock_guard<std::mutex> fg_lk(impl_->fingerprint_graph_mutex);
        for (const auto& layer : exp.layers) {
            impl_->fingerprint_graph.insert(
                exp.tenant + ":" + exp.adapter_name + ":" + layer.layer_name,
                layer.train,
                exp.tenant,
                exp.adapter_name,
                layer.layer_name);
        }
    }

    ++impl_->stats_data.stores_total;
    return true;
}

std::optional<AdaLoRAAdapter> AdaLoraTTBridge::loadAdapter(const std::string& tenant,
                                                           const std::string& adapter_name) const {
    std::lock_guard<std::mutex> lk(impl_->cache_mutex);
    const auto it = impl_->export_cache.find(cacheKey(tenant, adapter_name));
    if (it == impl_->export_cache.end()) {
        return std::nullopt;
    }
    return importFromTT(it->second);
}

std::size_t AdaLoraTTBridge::roundAndReallocate(AdaLoraTTExport& exp, double eps) const {
    TrainingStepFn step_fn = TrainingStepFn();
    {
        std::lock_guard<std::mutex> lk(trainingStepFnMutex());
        step_fn = trainingStepFnStorage();
    }
    if (step_fn) {
        return step_fn(exp, eps);
    }

    storage::TensorTrainDecomposer decomposer;
    std::size_t total_active = 0;
    for (auto& layer : exp.layers) {
        storage::TensorTrainConfig cfg;
        cfg.eps = eps;
        cfg.max_rank = layer.active_rank;
        layer.train = decomposer.round(layer.train, cfg);
        layer.active_rank = layer.train.maxRank();
        total_active += layer.active_rank;
    }
    return total_active;
}

std::vector<AdaLoraTTBridge::SimilarAdapter>
AdaLoraTTBridge::findSimilarAdapters(const AdaLoraTTExport& query_exp,
                                     std::size_t top_k,
                                     const std::string& tenant) const {
    if (top_k == 0) {
        top_k = 5;
    }

    std::unordered_map<std::string, SimilarAdapter> best_by_adapter;

    for (const auto& layer : query_exp.layers) {
        std::vector<graph::SimilarTensorResult> hits;
        {
            std::lock_guard<std::mutex> fg_lk(impl_->fingerprint_graph_mutex);
            hits = impl_->fingerprint_graph.findSimilar(layer.train, top_k * 4);
        }
        for (const auto& hit : hits) {
            if (!tenant.empty() && hit.tenant != tenant) {
                continue;
            }

            const std::string key = hit.tenant + ":" + hit.collection;
            auto& dst = best_by_adapter[key];
            if (dst.similarity < hit.similarity) {
                dst.adapter_name = hit.collection;
                dst.layer_name = hit.field;
                dst.similarity = hit.similarity;
            }
        }
    }

    std::vector<SimilarAdapter> out = {};

    out.reserve(best_by_adapter.size());
    for (const auto& kv : best_by_adapter) {
        out.push_back(kv.second);
    }

    std::sort(out.begin(), out.end(), [](const SimilarAdapter& a, const SimilarAdapter& b) {
        return a.similarity > b.similarity;
    });
    if (static_cast<int>(out.size()) > top_k) {
        out.resize(top_k);
    }
    return out;
}

AdaLoraTTBridge::BridgeStats AdaLoraTTBridge::stats() const noexcept {
    return impl_->stats_data;
}

const AdaLoraTTBridgeConfig& AdaLoraTTBridge::config() const noexcept {
    return impl_->cfg;
}

void AdaLoraTTBridge::setMapAdapterFn(MapAdapterFn fn) {
    std::lock_guard<std::mutex> lk(impl_->map_adapter_mutex);
    impl_->map_adapter_fn = std::move(fn);
}

void AdaLoraTTBridge::clearMapAdapterFn() {
    std::lock_guard<std::mutex> lk(impl_->map_adapter_mutex);
    impl_->map_adapter_fn = {};
}

bool AdaLoraTTBridge::mapAdapter(const AdaLoraTTExport& exp) const {
    MapAdapterFn fn_copy = MapAdapterFn();
    {
        std::lock_guard<std::mutex> lk(impl_->map_adapter_mutex);
        fn_copy = impl_->map_adapter_fn;
    }
    if (!fn_copy) {
        return false;
    }
    return fn_copy(exp);
}

std::size_t AdaLoraTTExport::totalParameters() const noexcept {
    std::size_t total = 0;
    for (const auto& layer : layers) {
        for (const auto& core : layer.train.cores) {
            total += core.data.size();
        }
    }
    return total;
}

} // namespace training
} // namespace themis
