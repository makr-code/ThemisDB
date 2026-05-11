/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adalora_tt_bridge.cpp                              ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-05                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 📋 Phase 1 (Q2 2027)                                        ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// STUB/SIMULATION NOTE:
// Purpose: Core conversion logic (exportToTT, importFromTT) is fully
//   implemented. Storage and findSimilarAdapters delegate to TensorNetwork-
//   StorageEngine and TensorFingerprintGraph which are Phase 2+ components.
// Activation: THEMIS_ENABLE_ADALORA_TT_BRIDGE=ON
// Production Delta: GgmlTensorBridge::mapAdapter() (Phase 3), training-loop
//   integration for roundAndReallocate (Phase 4).
// Removal Plan: Not removed — permanent bridge component.

#include "training/adalora_tt_bridge.h"
#include "graph/tensor_fingerprint_graph.h"
#include "utils/logger.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <mutex>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace training {

namespace {

// ---------------------------------------------------------------------------
// Column L2-norm of a flat row-major matrix (rows × cols), column index col
// ---------------------------------------------------------------------------
float colNorm(const std::vector<float>& mat, std::size_t rows,
              std::size_t cols, std::size_t col) noexcept
{
    float sum = 0.0f;
    for (std::size_t r = 0; r < rows; ++r) {
        float v = mat[r * cols + col];
        sum += v * v;
    }
    return std::sqrt(sum);
}

// Row L2-norm of a flat row-major matrix (rows × cols), row index row
float rowNorm(const std::vector<float>& mat, std::size_t rows,
              std::size_t cols, std::size_t row) noexcept
{
    float sum = 0.0f;
    for (std::size_t c = 0; c < cols; ++c) {
        float v = mat[row * cols + c];
        sum += v * v;
    }
    return std::sqrt(sum);
    (void)rows;
}

// ---------------------------------------------------------------------------
// Build TT-core G₀ (shape: 1 × d × r, flat: d*r floats)
// G₀[0, :, i] = B_col_i / ‖B_col_i‖ · √(λᵢ · scaling)
// ---------------------------------------------------------------------------
std::vector<float> buildG0(const std::vector<float>& B,
                            std::size_t d, std::size_t r,
                            const std::vector<float>& singular_values,
                            float scaling,
                            std::size_t active_rank,
                            bool normalise_sign) noexcept
{
    // G₀ is stored as d × r (contiguous in "mode-2 last" order)
    std::vector<float> G0(d * r, 0.0f);

    for (std::size_t i = 0; i < active_rank; ++i) {
        float lambda_i = singular_values[i];
        float scale    = std::sqrt(lambda_i * scaling);
        float norm_b   = colNorm(B, d, r, i);

        if (norm_b < 1e-12f || lambda_i < 1e-12f) continue;

        // Sign normalisation: first non-zero row of this column > 0
        float sign = 1.0f;
        if (normalise_sign) {
            for (std::size_t row = 0; row < d; ++row) {
                float v = B[row * r + i];
                if (std::fabs(v) > 1e-12f) {
                    sign = (v > 0.0f) ? 1.0f : -1.0f;
                    break;
                }
            }
        }

        for (std::size_t row = 0; row < d; ++row)
            G0[row * r + i] = sign * (B[row * r + i] / norm_b) * scale;
    }
    return G0;
}

// ---------------------------------------------------------------------------
// Build TT-core G₁ (shape: r × k × 1, flat: r*k floats)
// G₁[i, :, 0] = A_row_i / ‖A_row_i‖ · √(λᵢ · scaling)
// ---------------------------------------------------------------------------
std::vector<float> buildG1(const std::vector<float>& A,
                            std::size_t r, std::size_t k,
                            const std::vector<float>& singular_values,
                            float scaling,
                            std::size_t active_rank,
                            bool normalise_sign) noexcept
{
    std::vector<float> G1(r * k, 0.0f);

    for (std::size_t i = 0; i < active_rank; ++i) {
        float lambda_i = singular_values[i];
        float scale    = std::sqrt(lambda_i * scaling);
        float norm_a   = rowNorm(A, r, k, i);

        if (norm_a < 1e-12f || lambda_i < 1e-12f) continue;

        float sign = 1.0f;
        if (normalise_sign) {
            for (std::size_t col = 0; col < k; ++col) {
                float v = A[i * k + col];
                if (std::fabs(v) > 1e-12f) {
                    sign = (v > 0.0f) ? 1.0f : -1.0f;
                    break;
                }
            }
        }

        for (std::size_t col = 0; col < k; ++col)
            G1[i * k + col] = sign * (A[i * k + col] / norm_a) * scale;
    }
    return G1;
}

// ---------------------------------------------------------------------------
// Approximate singular values λᵢ ≈ ‖B[:,i]‖₂ · ‖A[i,:]‖₂
// (same heuristic used by AdaLoRAAdapter::updateImportance)
// ---------------------------------------------------------------------------
std::vector<float> approximateSingularValues(const std::vector<float>& B,
                                              const std::vector<float>& A,
                                              std::size_t d,
                                              std::size_t r,
                                              std::size_t k) noexcept
{
    std::vector<float> sv(r, 0.0f);
    for (std::size_t i = 0; i < r; ++i)
        sv[i] = colNorm(B, d, r, i) * rowNorm(A, r, k, i);
    return sv;
}

// ---------------------------------------------------------------------------
// Frobenius off-diagonal of P^T·P − I  (orthogonality check)
// Returns ‖P^T·P − I‖_F
// ---------------------------------------------------------------------------
double orthError(const std::vector<float>& P, std::size_t d, std::size_t r) noexcept
{
    double err = 0.0;
    for (std::size_t i = 0; i < r; ++i) {
        for (std::size_t j = 0; j < r; ++j) {
            double dot = 0.0;
            for (std::size_t row = 0; row < d; ++row)
                dot += static_cast<double>(P[row * r + i]) * P[row * r + j];
            double expected = (i == j) ? 1.0 : 0.0;
            double diff = dot - expected;
            err += diff * diff;
        }
    }
    return std::sqrt(err);
}

} // anonymous namespace

// ============================================================================
// Impl
// ============================================================================

struct AdaLoraTTBridge::Impl {
    std::shared_ptr<storage::TensorNetworkStorageEngine> engine;
    AdaLoraTTBridgeConfig                                cfg;
    mutable BridgeStats                                  stats_data{};
    // TensorFingerprintGraph: keyed by "<tenant>:<adapter_name>:<layer_name>"
    mutable graph::TensorFingerprintGraph                fingerprint_graph{};

    // Phase 3 bridge — GgmlTensorBridge::mapAdapter() (STUB #271)
    MapAdapterFn map_adapter_fn;
    mutable std::mutex map_adapter_mutex;
};

// ─────────────────────────────────────────────────────────────────────────────
// Phase 4 (STUB #271) — training-step bridge storage (process-wide static)
// ─────────────────────────────────────────────────────────────────────────────

namespace {
std::mutex& trainingStepFnMutex() { static std::mutex m; return m; }
AdaLoraTTBridge::TrainingStepFn& trainingStepFnStorage() {
    static AdaLoraTTBridge::TrainingStepFn fn;
    return fn;
}
} // anonymous namespace

/*static*/
void AdaLoraTTBridge::setTrainingStepFn(TrainingStepFn fn) {
    std::lock_guard<std::mutex> lk(trainingStepFnMutex());
    trainingStepFnStorage() = std::move(fn);
}

/*static*/
void AdaLoraTTBridge::clearTrainingStepFn() {
    std::lock_guard<std::mutex> lk(trainingStepFnMutex());
    trainingStepFnStorage() = {};
}

// ============================================================================
// Construction
// ============================================================================

AdaLoraTTBridge::AdaLoraTTBridge(
    std::shared_ptr<storage::TensorNetworkStorageEngine> engine,
    AdaLoraTTBridgeConfig cfg)
    : impl_(std::make_unique<Impl>())
{
    impl_->engine = std::move(engine);
    impl_->cfg    = std::move(cfg);
}

AdaLoraTTBridge::~AdaLoraTTBridge() = default;

// ============================================================================
// exportLayer — single-layer AdaLoRA → TT conversion (core algorithm)
// ============================================================================

AdaLoraTTLayerExport AdaLoraTTBridge::exportLayer(
    const AdaLoRAAdapter& adapter,
    const std::string&    layer_name) const
{
    auto [B, A] = adapter.getWeights(layer_name);

    if (B.empty() || A.empty())
        throw std::invalid_argument("Layer not found or has no weights: " + layer_name);

    const auto stats = adapter.getLayerStats(layer_name);
    const std::size_t max_rank    = stats.max_rank;
    const std::size_t active_rank = stats.active_rank;
    const float       scaling     = impl_->cfg.max_tt_rank > 0
                                  ? 1.0f : 1.0f;  // absorbed into cores

    if (max_rank == 0)
        throw std::invalid_argument("Layer has max_rank=0: " + layer_name);
    if (max_rank > impl_->cfg.max_tt_rank) {
        throw std::invalid_argument(
            "Layer rank " + std::to_string(max_rank) +
            " > max_tt_rank " + std::to_string(impl_->cfg.max_tt_rank) +
            " for layer " + layer_name + ". Use native storage.");
    }

    // Infer d and k from B (d×r) and A (r×k)
    const std::size_t d = B.size() / max_rank;
    const std::size_t k = A.size() / max_rank;

    if (d == 0 || k == 0)
        throw std::invalid_argument("Degenerate weight shape for layer: " + layer_name);

    // Approximate singular values λᵢ ≈ ‖B[:,i]‖ · ‖A[i,:]‖
    auto sv = approximateSingularValues(B, A, d, max_rank, k);

    // Build TT-cores G₀ (1×d×r) and G₁ (r×k×1)
    auto G0_data = buildG0(B, d, max_rank, sv, 1.0f, active_rank,
                            impl_->cfg.normalise_sign);
    auto G1_data = buildG1(A, max_rank, k, sv, 1.0f, active_rank,
                            impl_->cfg.normalise_sign);

    // Assemble TT-train (d=2 modes)
    storage::TTTrain train;
    train.mode_sizes = {d, k};
    train.ranks      = {1, static_cast<std::size_t>(max_rank), 1};

    storage::TTCore G0, G1;
    G0.shape = {1, d, max_rank};
    G0.data  = std::move(G0_data);
    G1.shape = {max_rank, k, 1};
    G1.data  = std::move(G1_data);

    train.cores = {std::move(G0), std::move(G1)};

    // Orthogonality validation
    bool orth_ok = true;
    if (active_rank > 0) {
        // Re-extract normalised P from G0 for orth check
        std::vector<float> P(d * active_rank);
        for (std::size_t row = 0; row < d; ++row)
            for (std::size_t i = 0; i < active_rank; ++i)
                P[row * active_rank + i] = train.cores[0].data[row * max_rank + i];

        double err = orthError(P, d, active_rank);
        orth_ok = (err < impl_->cfg.eps_orth * active_rank);
        if (!orth_ok) {
            ++impl_->stats_data.orth_violations;
            LOG_WARNING("adalora_tt_bridge: orthogonality error {:.4f} > eps "
                        "for layer {}", err, layer_name);
        }
    }

    ++impl_->stats_data.exports_total;

    return AdaLoraTTLayerExport{
        .layer_name             = layer_name,
        .train                  = std::move(train),
        .active_rank            = active_rank,
        .scaling                = scaling,
        .orthogonal_validated   = orth_ok,
    };
}

// ============================================================================
// exportToTT — all layers
// ============================================================================

AdaLoraTTExport AdaLoraTTBridge::exportToTT(
    const AdaLoRAAdapter& adapter,
    const std::string&    adapter_name,
    const std::string&    tenant) const
{
    AdaLoraTTExport exp;
    exp.adapter_name = adapter_name;
    exp.tenant       = tenant;

    for (const auto& ls : adapter.getAllLayerStats()) {
        if (ls.active_rank == 0) continue;
        try {
            exp.layers.push_back(exportLayer(adapter, ls.layer_name));
        } catch (const std::invalid_argument& e) {
            LOG_WARNING("adalora_tt_bridge: skipping layer {}: {}", ls.layer_name, e.what());
        }
    }

    if (exp.layers.empty())
        throw std::runtime_error("No exportable layers in adapter: " + adapter_name);

    return exp;
}

// ============================================================================
// importFromTT — TT → AdaLoRA
// ============================================================================

AdaLoRAAdapter AdaLoraTTBridge::importFromTT(const AdaLoraTTExport& exp) const
{
    if (exp.layers.empty())
        throw std::invalid_argument("Empty TT-export: " + exp.adapter_name);

    // Determine global rank budget from layers
    std::size_t total_budget = 0;
    std::size_t default_rank = 0;
    for (const auto& l : exp.layers) {
        total_budget += l.active_rank;
        default_rank  = std::max(default_rank, l.active_rank);
    }

    AdaLoRAAdapter adapter(default_rank, 1.0f, total_budget);

    for (const auto& lexp : exp.layers) {
        const auto& G0 = lexp.train.cores.at(0);  // shape: 1 × d × r
        const auto& G1 = lexp.train.cores.at(1);  // shape: r × k × 1

        const std::size_t d = G0.shape.at(1);
        const std::size_t r = G0.shape.at(2);
        const std::size_t k = G1.shape.at(1);

        // Reconstruct B (d×r) and A (r×k) from TT-cores
        // B[:,i] = G₀[0,:,i]   A[i,:] = G₁[i,:,0]
        std::vector<float> B(d * r, 0.0f);
        std::vector<float> A(r * k, 0.0f);

        for (std::size_t row = 0; row < d; ++row)
            for (std::size_t i = 0; i < r; ++i)
                B[row * r + i] = G0.data[row * r + i];

        for (std::size_t i = 0; i < r; ++i)
            for (std::size_t col = 0; col < k; ++col)
                A[i * k + col] = G1.data[i * k + col];

        adapter.registerLayer(lexp.layer_name, d, k, r);
        adapter.setWeights(lexp.layer_name, B, A);
    }

    ++impl_->stats_data.imports_total;
    return adapter;
}

// ============================================================================
// store — persist TT-export in TensorNetworkStorageEngine
// ============================================================================

bool AdaLoraTTBridge::store(const AdaLoraTTExport& exp)
{
    if (!impl_->engine) return false;

    for (const auto& lexp : exp.layers) {
        // Key schema: __lora_adapters__:<tenant>:<adapter>:<layer>:G<0|1>
        auto make_key = [&](int core_idx) {
            return "__lora_adapters__:" + exp.tenant + ":"
                 + exp.adapter_name + ":" + lexp.layer_name
                 + ":G" + std::to_string(core_idx);
        };

        // Store each core via the storage engine
        impl_->engine->put(make_key(0), lexp.train.cores.at(0).data,
                           lexp.train.cores.at(0).shape);
        impl_->engine->put(make_key(1), lexp.train.cores.at(1).data,
                           lexp.train.cores.at(1).shape);

        // Register in fingerprint graph for cross-adapter similarity queries
        if (impl_->cfg.auto_deduplicate) {
            const std::string graph_id = exp.tenant + ":"
                                       + exp.adapter_name + ":"
                                       + lexp.layer_name;
            impl_->fingerprint_graph.insert(
                graph_id, lexp.train,
                exp.tenant, exp.adapter_name, lexp.layer_name);
        }
    }

    ++impl_->stats_data.stores_total;
    return true;
}

// ============================================================================
// loadAdapter
// ============================================================================

std::optional<AdaLoRAAdapter> AdaLoraTTBridge::loadAdapter(
    const std::string& tenant,
    const std::string& adapter_name) const
{
    if (!impl_->engine) return std::nullopt;

    // Query all keys with prefix __lora_adapters__:<tenant>:<adapter_name>:
    const std::string prefix = "__lora_adapters__:" + tenant + ":"
                             + adapter_name + ":";

    auto keys = impl_->engine->listKeys(prefix);
    if (keys.empty()) return std::nullopt;

    // Rebuild AdaLoraTTExport from stored cores
    AdaLoraTTExport exp;
    exp.adapter_name = adapter_name;
    exp.tenant       = tenant;

    // Group keys by layer (each layer has G0 + G1)
    std::map<std::string, std::array<std::optional<storage::TTCore>, 2>> layer_cores;
    for (const auto& key : keys) {
        // Parse: __lora_adapters__:<tenant>:<adapter>:<layer>:G&lt;idx&gt;
        auto last_colon = key.rfind(':');
        if (last_colon == std::string::npos) continue;
        std::string core_tag   = key.substr(last_colon + 1);   // "G0" or "G1"
        std::string layer_part = key.substr(prefix.size(),
                                             last_colon - prefix.size());

        int core_idx = (core_tag == "G0") ? 0 : (core_tag == "G1") ? 1 : -1;
        if (core_idx < 0) continue;

        auto [data, shape] = impl_->engine->get(key);
        if (data.empty()) continue;

        storage::TTCore core;
        core.data  = std::move(data);
        core.shape = std::move(shape);
        layer_cores[layer_part][core_idx] = std::move(core);
    }

    for (auto& [layer_name, cores] : layer_cores) {
        if (!cores[0].has_value() || !cores[1].has_value()) continue;

        AdaLoraTTLayerExport lexp;
        lexp.layer_name  = layer_name;
        lexp.active_rank = cores[0]->shape.at(2);  // r = G₀ last dim
        lexp.scaling     = 1.0f;

        storage::TTTrain train;
        train.cores     = {std::move(*cores[0]), std::move(*cores[1])};
        train.mode_sizes = {train.cores[0].shape[1], train.cores[1].shape[1]};
        train.ranks      = {1, lexp.active_rank, 1};
        lexp.train       = std::move(train);

        exp.layers.push_back(std::move(lexp));
    }

    if (exp.layers.empty()) return std::nullopt;
    return importFromTT(exp);
}

// ============================================================================
// roundAndReallocate
// ============================================================================

std::size_t AdaLoraTTBridge::roundAndReallocate(AdaLoraTTExport& exp,
                                                  double            eps) const
{
    // STUB #271 (Phase 4): if a training-loop backend is injected, delegate to it.
    TrainingStepFn step_fn;
    {
        std::lock_guard<std::mutex> lk(trainingStepFnMutex());
        step_fn = trainingStepFnStorage();
    }
    if (step_fn) {
        return step_fn(exp, eps);
    }

    // Fallback: standalone TT-rounding via TensorTrainDecomposer (post-training
    // use-case — does not interface with a live training loop).
    storage::TensorTrainDecomposer decomposer;
    std::size_t total_active = 0;

    for (auto& lexp : exp.layers) {
        storage::TensorTrainConfig cfg;
        cfg.eps      = eps;
        cfg.max_rank = static_cast&lt;int&gt;(lexp.active_rank);

        auto [rounded, stats] = decomposer.round(lexp.train, cfg);
        lexp.train       = std::move(rounded);
        lexp.active_rank = stats.max_rank;
        total_active    += lexp.active_rank;
    }
    return total_active;
}

// ============================================================================
// findSimilarAdapters
// ============================================================================

std::vector<AdaLoraTTBridge::SimilarAdapter>
AdaLoraTTBridge::findSimilarAdapters(const AdaLoraTTExport& query_exp,
                                      std::size_t            top_k,
                                      const std::string&     tenant) const
{
    if (top_k == 0) top_k = 5;

    // Per-adapter aggregate score: adapter_key → {max_similarity, layer_name}
    struct AdapterScore {
        double      max_sim  = 0.0;
        std::string best_layer;
    };
    std::map<std::string, AdapterScore> adapter_scores;

    // Query the fingerprint graph for each layer in the query export
    for (const auto& lexp : query_exp.layers) {
        auto hits = impl_->fingerprint_graph.findSimilar(
            lexp.train, top_k * 4);

        for (const auto& hit : hits) {
            // hit.collection = adapter_name, hit.field = layer_name
            // hit.tenant = tenant of the stored adapter
            if (!tenant.empty() && hit.tenant != tenant) continue;

            // Aggregate by (tenant, adapter_name)
            const std::string adapter_key = hit.tenant + ":" + hit.collection;
            auto& sc = adapter_scores[adapter_key];
            if (hit.similarity > sc.max_sim) {
                sc.max_sim    = hit.similarity;
                sc.best_layer = hit.field;
            }
        }
    }

    // Build sorted result list
    std::vector<SimilarAdapter> results;
    results.reserve(adapter_scores.size());
    for (const auto& [key, sc] : adapter_scores) {
        // Parse "tenant:adapter_name" back out
        auto colon = key.find(':');
        std::string adapter_name = (colon != std::string::npos)
                                 ? key.substr(colon + 1) : key;
        SimilarAdapter sa;
        sa.adapter_name = adapter_name;
        sa.layer_name   = sc.best_layer;
        sa.similarity   = sc.max_sim;
        results.push_back(sa);
    }

    std::sort(results.begin(), results.end(),
        [](const SimilarAdapter& a, const SimilarAdapter& b){
            return a.similarity > b.similarity;
        });
    if (results.size() > top_k) results.resize(top_k);
    return results;
}

// ============================================================================
// Accessors
// ============================================================================

AdaLoraTTBridge::BridgeStats AdaLoraTTBridge::stats() const noexcept {
    return impl_->stats_data;
}

const AdaLoraTTBridgeConfig& AdaLoraTTBridge::config() const noexcept {
    return impl_->cfg;
}

// ============================================================================
// Phase 3 bridge — mapAdapter() (STUB #271)
// ============================================================================

void AdaLoraTTBridge::setMapAdapterFn(MapAdapterFn fn) {
    std::lock_guard<std::mutex> lk(impl_->map_adapter_mutex);
    impl_->map_adapter_fn = std::move(fn);
}

void AdaLoraTTBridge::clearMapAdapterFn() {
    std::lock_guard<std::mutex> lk(impl_->map_adapter_mutex);
    impl_->map_adapter_fn = {};
}

bool AdaLoraTTBridge::mapAdapter(const AdaLoraTTExport& exp) const {
    MapAdapterFn fn_copy;
    {
        std::lock_guard<std::mutex> lk(impl_->map_adapter_mutex);
        fn_copy = impl_->map_adapter_fn;
    }
    // STUB #271 (Phase 3): if no bridge fn is set, return false — the caller
    // should detect this and fall back to native GgmlTensorBridge::mapAdapter().
    if (!fn_copy) return false;
    return fn_copy(exp);
}

// ============================================================================
// AdaLoraTTExport::totalParameters
// ============================================================================

std::size_t AdaLoraTTExport::totalParameters() const noexcept {
    std::size_t total = 0;
    for (const auto& l : layers) {
        for (const auto& core : l.train.cores)
            total += core.data.size();
    }
    return total;
}

} // namespace training
} // namespace themis
