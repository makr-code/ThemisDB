/**
 * @file adalora_tt_bridge.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "training/adalora_tt_bridge.h"
#include "graph/tensor_fingerprint_graph.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <mutex>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
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

/**
 * @brief Cache Key.
 * @param[in] tenant Input parameter.
 * @param[in] adapter_name Name of the adapter.
 * @return Return value.
 * @details Implements cacheKey without additional internal calls.
 */
std::string cacheKey(const std::string& tenant, const std::string& adapter_name) {
    return std::to_string(tenant.size()) + ":" + tenant +
        std::to_string(adapter_name.size()) + ":" + adapter_name;
}

std::string storageKey(const std::string& tenant, const std::string& adapter_name) {
    return "adalora_tt_bridge/" + cacheKey(tenant, adapter_name);
}

bool hasCoreShape(const storage::TTCore& core,
                  std::size_t r_left,
                  std::size_t n,
                  std::size_t r_right) noexcept {
    if (r_left == 0 || n == 0 || r_right == 0 ||
        r_left > std::numeric_limits<std::size_t>::max() / n ||
        r_left * n > std::numeric_limits<std::size_t>::max() / r_right) {
        return false;
    }
    return core.r_left == r_left && core.n == n && core.r_right == r_right &&
        core.data.size() == r_left * n * r_right;
}

bool orthonormalColumns(const std::vector<float>& matrix,
                        std::size_t rows,
                        std::size_t cols,
                        std::size_t count,
                        double epsilon) noexcept {
    if (rows == 0 || count == 0 || count > cols || matrix.size() != rows * cols) {
        return false;
    }

    double error_squared = 0.0;
    for (std::size_t left = 0; left < count; ++left) {
        double left_norm_squared = 0.0;
        for (std::size_t row = 0; row < rows; ++row) {
            const double value = matrix[row * cols + left];
            left_norm_squared += value * value;
        }
        if (left_norm_squared <= 1e-24) {
            return false;
        }
        for (std::size_t right = 0; right < count; ++right) {
            double right_norm_squared = 0.0;
            double dot = 0.0;
            for (std::size_t row = 0; row < rows; ++row) {
                const double left_value = matrix[row * cols + left];
                const double right_value = matrix[row * cols + right];
                dot += left_value * right_value;
                right_norm_squared += right_value * right_value;
            }
            if (right_norm_squared <= 1e-24) {
                return false;
            }
            const double normalized_dot =
                dot / std::sqrt(left_norm_squared * right_norm_squared);
            const double expected = left == right ? 1.0 : 0.0;
            const double delta = normalized_dot - expected;
            error_squared += delta * delta;
        }
    }
    return std::sqrt(error_squared) < epsilon;
}

bool orthonormalRows(const std::vector<float>& matrix,
                     std::size_t rows,
                     std::size_t cols,
                     std::size_t count,
                     double epsilon) noexcept {
    if (count == 0 || count > rows || cols == 0 ||
        rows > std::numeric_limits<std::size_t>::max() / cols ||
        matrix.size() != rows * cols) {
        return false;
    }

    double error_squared = 0.0;
    for (std::size_t left = 0; left < count; ++left) {
        double left_norm_squared = 0.0;
        for (std::size_t col = 0; col < cols; ++col) {
            const double value = matrix[left * cols + col];
            left_norm_squared += value * value;
        }
        if (left_norm_squared <= 1e-24) {
            return false;
        }
        for (std::size_t right = 0; right < count; ++right) {
            double right_norm_squared = 0.0;
            double dot = 0.0;
            for (std::size_t col = 0; col < cols; ++col) {
                const double left_value = matrix[left * cols + col];
                const double right_value = matrix[right * cols + col];
                dot += left_value * right_value;
                right_norm_squared += right_value * right_value;
            }
            if (right_norm_squared <= 1e-24) {
                return false;
            }
            const double normalized_dot =
                dot / std::sqrt(left_norm_squared * right_norm_squared);
            const double expected = left == right ? 1.0 : 0.0;
            const double delta = normalized_dot - expected;
            error_squared += delta * delta;
        }
    }
    return std::sqrt(error_squared) < epsilon;
}

nlohmann::json serializeExport(const AdaLoraTTExport& exp) {
    nlohmann::json layers = nlohmann::json::array();
    for (const auto& layer : exp.layers) {
        nlohmann::json cores = nlohmann::json::array();
        for (const auto& core : layer.train.cores) {
            cores.push_back({
                {"r_left", core.r_left},
                {"n", core.n},
                {"r_right", core.r_right},
                {"data", core.data}
            });
        }
        layers.push_back({
            {"layer_name", layer.layer_name},
            {"active_rank", layer.active_rank},
            {"scaling", layer.scaling},
            {"orthogonal_validated", layer.orthogonal_validated},
            {"mode_sizes", layer.train.mode_sizes},
            {"cores", std::move(cores)}
        });
    }
    return {
        {"adapter_name", exp.adapter_name},
        {"tenant", exp.tenant},
        {"layers", std::move(layers)}
    };
}

bool deserializeExport(const std::vector<std::uint8_t>& bytes,
                       AdaLoraTTExport& exp) noexcept {
    constexpr std::size_t max_serialized_bytes = 256 * 1024 * 1024;
    if (bytes.empty() || bytes.size() > max_serialized_bytes) {
        return false;
    }
    try {
        const auto json = nlohmann::json::parse(bytes.begin(), bytes.end());
        if (!json.is_object() || !json.contains("adapter_name") ||
            !json["adapter_name"].is_string() || !json.contains("tenant") ||
            !json["tenant"].is_string() || !json.contains("layers") ||
            !json["layers"].is_array() || json["layers"].empty()) {
            return false;
        }

        AdaLoraTTExport decoded;
        decoded.adapter_name = json["adapter_name"].get<std::string>();
        decoded.tenant = json["tenant"].get<std::string>();
        std::unordered_set<std::string> layer_names;
        for (const auto& layer_json : json["layers"]) {
            if (!layer_json.is_object() || !layer_json.contains("layer_name") ||
                !layer_json["layer_name"].is_string() || !layer_json.contains("active_rank") ||
                !layer_json["active_rank"].is_number_unsigned() || !layer_json.contains("scaling") ||
                !layer_json["scaling"].is_number() ||
                !layer_json.contains("orthogonal_validated") ||
                !layer_json["orthogonal_validated"].is_boolean() ||
                !layer_json.contains("mode_sizes") || !layer_json["mode_sizes"].is_array() ||
                !layer_json.contains("cores") || !layer_json["cores"].is_array() ||
                layer_json["cores"].size() != 2) {
                return false;
            }

            AdaLoraTTLayerExport layer;
            layer.layer_name = layer_json["layer_name"].get<std::string>();
            layer.active_rank = layer_json["active_rank"].get<std::size_t>();
            layer.scaling = layer_json["scaling"].get<float>();
            layer.orthogonal_validated =
                layer_json["orthogonal_validated"].get<bool>();
            layer.train.mode_sizes =
                layer_json["mode_sizes"].get<std::vector<std::size_t>>();
            if (layer.layer_name.empty() || layer.active_rank == 0 ||
                !std::isfinite(layer.scaling) || layer.scaling <= 0.0f ||
                layer.train.mode_sizes.size() != 2 ||
                !layer_names.insert(layer.layer_name).second) {
                return false;
            }

            for (const auto& core_json : layer_json["cores"]) {
                if (!core_json.is_object() || !core_json.contains("r_left") ||
                    !core_json.contains("n") || !core_json.contains("r_right") ||
                    !core_json.contains("data") || !core_json["data"].is_array()) {
                    return false;
                }
                storage::TTCore core;
                core.r_left = core_json["r_left"].get<std::size_t>();
                core.n = core_json["n"].get<std::size_t>();
                core.r_right = core_json["r_right"].get<std::size_t>();
                core.data = core_json["data"].get<std::vector<float>>();
                if (core.r_left == 0 || core.n == 0 || core.r_right == 0 ||
                    core.r_left > std::numeric_limits<std::size_t>::max() / core.n ||
                    core.r_left * core.n >
                        std::numeric_limits<std::size_t>::max() / core.r_right ||
                    core.data.size() != core.r_left * core.n * core.r_right ||
                    !std::all_of(core.data.begin(), core.data.end(),
                                 [](float value) { return std::isfinite(value); })) {
                    return false;
                }
                layer.train.cores.push_back(std::move(core));
            }
            if (layer.train.mode_sizes[0] != layer.train.cores[0].n ||
                layer.train.mode_sizes[1] != layer.train.cores[1].n ||
                layer.train.cores[0].r_left != 1 ||
                layer.train.cores[0].r_right != layer.train.cores[1].r_left ||
                layer.train.cores[1].r_right != 1 ||
                layer.active_rank > layer.train.cores[0].r_right) {
                return false;
            }
            decoded.layers.push_back(std::move(layer));
        }
        exp = std::move(decoded);
        return true;
    } catch (...) {
        return false;
    }
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
/**
 * @brief Training Step Fn Mutex.
 * @return Return value.
 * @details Implements trainingStepFnMutex without additional internal calls.
 */
std::mutex& trainingStepFnMutex() {
    static std::mutex m;
    return m;
}

/**
 * @brief Training Step Fn Storage.
 * @return Return value.
 * @details Implements trainingStepFnStorage without additional internal calls.
 */
AdaLoraTTBridge::TrainingStepFn& trainingStepFnStorage() {
    static AdaLoraTTBridge::TrainingStepFn fn;
    return fn;
}
} // namespace

/**
 * @brief Set Training Step Fn.
 * @param[in] fn Input parameter.
 * @details Calls: lk(), trainingStepFnMutex(), trainingStepFnStorage(), std::move().
 */
void AdaLoraTTBridge::setTrainingStepFn(TrainingStepFn fn) {
    std::lock_guard<std::mutex> lk(trainingStepFnMutex());
    trainingStepFnStorage() = std::move(fn);
}

/**
 * @brief Clear Training Step Fn.
 * @details Calls: lk(), trainingStepFnMutex(), trainingStepFnStorage().
 */
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
    if (max_rank == 0 || active_rank == 0 || active_rank > max_rank ||
        !std::isfinite(it->alpha) || it->alpha <= 0.0f ||
        !std::isfinite(impl_->cfg.eps_orth) || impl_->cfg.eps_orth <= 0.0) {
        throw std::invalid_argument("Invalid layer rank, scaling, or orthogonality tolerance: " +
                                    layer_name);
    }
    const float scaling = it->alpha / static_cast<float>(max_rank);
    if (!std::isfinite(scaling) || scaling <= 0.0f) {
        throw std::invalid_argument("Layer has invalid scaling: " + layer_name);
    }
    if (impl_->cfg.max_tt_rank > 0 && max_rank > impl_->cfg.max_tt_rank) {
        throw std::invalid_argument("Layer rank exceeds max_tt_rank: " + layer_name);
    }
    if (B.size() % max_rank != 0 || A.size() % max_rank != 0) {
        throw std::invalid_argument("Invalid matrix shape for layer: " + layer_name);
    }

    const std::size_t d = B.size() / max_rank;
    const std::size_t k = A.size() / max_rank;

    const auto sv = approximateSingularValues(B, A, d, max_rank, k);
    const auto g0_data = buildG0(B, d, max_rank, sv, 1.0f, active_rank,
                                 impl_->cfg.normalise_sign);
    const auto g1_data = buildG1(A, max_rank, k, sv, 1.0f, active_rank,
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
    out.orthogonal_validated =
        orthonormalColumns(B, d, max_rank, active_rank, impl_->cfg.eps_orth) &&
        orthonormalRows(A, max_rank, k, active_rank, impl_->cfg.eps_orth);
    if (!out.orthogonal_validated) {
        ++impl_->stats_data.orth_violations;
    }
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
    if (!std::isfinite(impl_->cfg.eps_orth) || impl_->cfg.eps_orth <= 0.0) {
        throw std::invalid_argument("eps_orth must be finite and positive");
    }

    std::size_t max_rank = 1;
    std::size_t total_budget = 0;
    for (const auto& layer : exp.layers) {
        if (layer.layer_name.empty() || layer.train.cores.size() != 2 ||
            layer.train.mode_sizes.size() != 2 || layer.active_rank == 0 ||
            !std::isfinite(layer.scaling) || layer.scaling <= 0.0f) {
            throw std::invalid_argument("Invalid TT export layer metadata");
        }
        const auto& g0 = layer.train.cores[0];
        const auto& g1 = layer.train.cores[1];
        if (!hasCoreShape(g0, 1, layer.train.mode_sizes[0], g1.r_left) ||
            !hasCoreShape(g1, g0.r_right, layer.train.mode_sizes[1], 1) ||
            layer.active_rank > g0.r_right ||
            !std::all_of(g0.data.begin(), g0.data.end(),
                         [](float value) { return std::isfinite(value); }) ||
            !std::all_of(g1.data.begin(), g1.data.end(),
                         [](float value) { return std::isfinite(value); }) ||
            (layer.orthogonal_validated &&
             !(orthonormalColumns(g0.data, g0.n, g0.r_right, layer.active_rank,
                                  impl_->cfg.eps_orth) &&
               orthonormalRows(g1.data, g1.r_left, g1.n, layer.active_rank,
                               impl_->cfg.eps_orth))) ||
            total_budget > std::numeric_limits<std::size_t>::max() - layer.active_rank) {
            throw std::invalid_argument("Invalid TT core shape or data");
        }
        max_rank = std::max(max_rank, layer.active_rank);
        total_budget += layer.active_rank;
    }

    AdaLoRAAdapter adapter(max_rank, 8.0f, std::max<std::size_t>(1, total_budget));

    for (const auto& layer : exp.layers) {
        const auto& g0 = layer.train.cores[0];
        const auto& g1 = layer.train.cores[1];

        const std::size_t d = g0.n;
        const std::size_t r = layer.active_rank;
        const std::size_t k = g1.n;
        std::vector<float> B(d * r, 0.0f);
        std::vector<float> A(r * k, 0.0f);
        for (std::size_t row = 0; row < d; ++row) {
            std::copy_n(g0.data.begin() + static_cast<std::ptrdiff_t>(row * g0.r_right),
                        r,
                        B.begin() + static_cast<std::ptrdiff_t>(row * r));
        }
        for (std::size_t component = 0; component < r; ++component) {
            std::copy_n(g1.data.begin() + static_cast<std::ptrdiff_t>(component * k),
                        k,
                        A.begin() + static_cast<std::ptrdiff_t>(component * k));
        }

        adapter.addLayer(layer.layer_name, d, k, r,
                         layer.scaling * static_cast<float>(r));
        adapter.setWeights(layer.layer_name, B, A);
    }

    adapter.updateAllImportances();
    ++impl_->stats_data.imports_total;
    return adapter;
}

/**
 * @brief Store.
 * @param[in] exp Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: empty(), lk(), cacheKey(), fg_lk(), insert().
 */
bool AdaLoraTTBridge::store(const AdaLoraTTExport& exp) {
    if (exp.adapter_name.empty() || exp.layers.empty() || !impl_->engine ||
        !std::isfinite(impl_->cfg.eps_orth) || impl_->cfg.eps_orth <= 0.0) {
        return false;
    }

    const auto json = serializeExport(exp);
    const std::string payload = json.dump();
    constexpr std::size_t max_serialized_bytes = 256 * 1024 * 1024;
    if (payload.size() > max_serialized_bytes) {
        return false;
    }
    const std::vector<std::uint8_t> bytes(payload.begin(), payload.end());
    AdaLoraTTExport validated;
    if (!deserializeExport(bytes, validated) ||
        validated.adapter_name != exp.adapter_name || validated.tenant != exp.tenant) {
        return false;
    }
    for (const auto& layer : validated.layers) {
        const auto& g0 = layer.train.cores[0];
        const auto& g1 = layer.train.cores[1];
        if (layer.orthogonal_validated &&
            !(orthonormalColumns(g0.data, g0.n, g0.r_right, layer.active_rank,
                                 impl_->cfg.eps_orth) &&
              orthonormalRows(g1.data, g1.r_left, g1.n, layer.active_rank,
                              impl_->cfg.eps_orth))) {
            return false;
        }
    }
    if (!impl_->engine->putRawMetadata(storageKey(exp.tenant, exp.adapter_name), bytes)) {
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
    AdaLoraTTExport exp;
    {
        std::lock_guard<std::mutex> lk(impl_->cache_mutex);
        const auto it = impl_->export_cache.find(cacheKey(tenant, adapter_name));
        if (it != impl_->export_cache.end()) {
            exp = it->second;
        }
    }
    if (exp.layers.empty()) {
        if (!impl_->engine) {
            return std::nullopt;
        }
        const auto bytes = impl_->engine->getRawMetadata(storageKey(tenant, adapter_name));
        if (!bytes || !deserializeExport(*bytes, exp) ||
            exp.tenant != tenant || exp.adapter_name != adapter_name) {
            return std::nullopt;
        }
        std::lock_guard<std::mutex> lk(impl_->cache_mutex);
        impl_->export_cache[cacheKey(tenant, adapter_name)] = exp;
    }
    return importFromTT(exp);
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
    if (out.size() > top_k) {
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

/**
 * @brief Set Map Adapter Fn.
 * @param[in] fn Input parameter.
 * @details Calls: lk(), std::move().
 */
void AdaLoraTTBridge::setMapAdapterFn(MapAdapterFn fn) {
    std::lock_guard<std::mutex> lk(impl_->map_adapter_mutex);
    impl_->map_adapter_fn = std::move(fn);
}

/**
 * @brief Clear Map Adapter Fn.
 * @details Calls: lk().
 */
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
