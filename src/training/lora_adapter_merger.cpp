/**
 * @file lora_adapter_merger.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "training/lora_adapter_merger.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace themis {
namespace training {

// ============================================================================
// Helper: compute flat ΔW = (B @ A) × scaling  (in_dim × out_dim)
// ============================================================================

namespace {

std::vector<float> computeDeltaW(const std::vector<float>& B,
                                  const std::vector<float>& A,
                                  size_t in_dim,
                                  size_t out_dim,
                                  size_t rank,
                                  float  scaling) {
    // B: in_dim × rank  (row-major)
    // A: rank × out_dim (row-major)
    // ΔW: in_dim × out_dim
    std::vector<float> dW(in_dim * out_dim, 0.0f);
    for (size_t i = 0; i < in_dim; ++i) {
        for (size_t j = 0; j < out_dim; ++j) {
            float acc = 0.0f;
            for (size_t r = 0; r < rank; ++r) {
                acc += B[i * rank + r] * A[r * out_dim + j];
            }
            dW[i * out_dim + j] = acc * scaling;
        }
    }
    return dW;
}

// Power-iteration dominant singular-vector pair for a (rows × cols) matrix.
// Returns (u, v, sigma) where u ∈ R^rows, v ∈ R^cols, sigma >= 0.
// For the factorisation: B' = u * sqrt(sigma), A' = v' * sqrt(sigma)
// with appropriate shapes, the reconstructed rank-1 matrix is close to M.
struct SVD1Result {
    std::vector<float> u;     // rows-vector
    std::vector<float> v;     // cols-vector
    float              sigma = 0.0f;
};

SVD1Result dominantSVD1(const std::vector<float>& M,
                         size_t rows, size_t cols,
                         int max_iter = 30) {
    SVD1Result res;
    res.u.resize(rows, 1.0f / std::sqrt(static_cast<float>(rows)));
    res.v.resize(cols, 0.0f);

    for (int iter = 0; iter < max_iter; ++iter) {
        // v = M^T u
        for (size_t j = 0; j < cols; ++j) {
            float acc = 0.0f;
            for (size_t i = 0; i < rows; ++i)
                acc += M[i * cols + j] * res.u[i];
            res.v[j] = acc;
        }
        // normalise v
        float norm_v = 0.0f;
        for (float x : res.v) {
          norm_v += x * x;
        }
        norm_v = std::sqrt(norm_v);
        if (norm_v < 1e-12f) {
          break;
        }
        for (float& x : res.v) {
          x /= norm_v;
        }

        // u = M v
        for (size_t i = 0; i < rows; ++i) {
            float acc = 0.0f;
            for (size_t j = 0; j < cols; ++j)
                acc += M[i * cols + j] * res.v[j];
            res.u[i] = acc;
        }
        // normalise u
        float norm_u = 0.0f;
        for (float x : res.u) {
          norm_u += x * x;
        }
        norm_u = std::sqrt(norm_u);
        if (norm_u < 1e-12f) {
          break;
        }
        res.sigma = norm_u;
        for (float& x : res.u) {
          x /= norm_u;
        }
    }
    return res;
}

// Factorise merged ΔW (in_dim × out_dim) into B (in_dim × rank) and
// A (rank × out_dim) such that (B @ A) ≈ ΔW / scaling.
// Uses the top-`rank` rank-1 SVD approximation (power iteration).
void factoriseDeltaW(const std::vector<float>& dW,
                      size_t in_dim, size_t out_dim,
                      size_t rank, float scaling,
                      std::vector<float>& B_out,
                      std::vector<float>& A_out) {
    B_out.assign(in_dim * rank, 0.0f);
    A_out.assign(rank * out_dim, 0.0f);

    std::vector<float> residual = dW;
    const float inv_scaling = (std::abs(scaling) > 1e-12f) ? 1.0f / scaling : 1.0f;

    for (size_t r = 0; r < rank; ++r) {
        SVD1Result sv = dominantSVD1(residual, in_dim, out_dim);
        if (sv.sigma < 1e-12f) {
          break;
        }

        float sq_sigma = std::sqrt(sv.sigma * inv_scaling);
        // B[:, r] = u * sq_sigma
        for (size_t i = 0; i < in_dim; ++i)
            B_out[i * rank + r] = sv.u[i] * sq_sigma;
        // A[r, :] = v * sq_sigma
        for (size_t j = 0; j < out_dim; ++j)
            A_out[r * out_dim + j] = sv.v[j] * sq_sigma;

        // Deflate residual: residual -= sigma * (u ⊗ v)
        for (size_t i = 0; i < in_dim; ++i)
            for (size_t j = 0; j < out_dim; ++j)
                residual[i * out_dim + j] -= sv.sigma * sv.u[i] * sv.v[j];
    }
}

} // anonymous namespace

// ============================================================================
// Linear merge
// ============================================================================

MergeLayerResult LoRAAdapterMerger::mergeLinear(
    const std::vector<AdapterDescriptor>& adapters,
    const std::string& out_layer,
    size_t in_dim, size_t out_dim, size_t rank, float alpha) const
{
    MergeLayerResult result;
    result.layer_name = out_layer;

    if (adapters.empty()) {
        result.error_message = "No adapters provided";
        return result;
    }
    if (in_dim == 0 || out_dim == 0 || rank == 0) {
        result.error_message = "Dimensions and rank must be > 0";
        return result;
    }

    const float scaling = alpha / static_cast<float>(rank);
    std::vector<float> merged_dW(in_dim * out_dim, 0.0f);

    for (const auto& desc : adapters) {
        if (!desc.adapter) {
            result.error_message = "Null adapter pointer";
            return result;
        }
        if (!desc.adapter->hasLayer(desc.layer_name)) {
            result.error_message = "Layer '" + desc.layer_name +
                                   "' not found in adapter";
            return result;
        }
        const LoRAWeightEntry& we = desc.adapter->getWeights(desc.layer_name);
        const size_t r_i    = we.rank;
        if (r_i == 0) {
          continue;
        }

        float sc_i = we.alpha / static_cast<float>(we.rank);
        auto dW_i = computeDeltaW(we.B, we.A, in_dim, out_dim, we.rank, sc_i);
        for (size_t k = 0; k < merged_dW.size(); ++k)
            merged_dW[k] += desc.weight * dW_i[k];
    }

    factoriseDeltaW(merged_dW, in_dim, out_dim, rank, scaling,
                    result.B, result.A);
    result.success = true;
    return result;
}

MergeResult LoRAAdapterMerger::mergeLinearAll(
    const std::vector<const LoRAAdapter*>& adapters,
    const std::vector<float>&               weights,
    size_t                                  output_rank) const
{
    MergeResult result = {};

    if (adapters.empty()) {
        result.error_message = "No adapters provided";
        return result;
    }
    if (static_cast<int>(weights.size()) != static_cast<int>(adapters.size())) {
        result.error_message = "static_cast<int>(weights.size()) != static_cast<int>(adapters.size())";
        return result;
    }
    for (const auto* a : adapters) {
        if (!a) {
            result.error_message = "Null adapter pointer";
            return result;
        }
    }

    const auto& first_layers = adapters[0]->layerNames();
    for (const auto& lname : first_layers) {
        // Check all adapters have this layer
        bool all_have = true;
        for (const auto* a : adapters) {
            if (!a->hasLayer(lname)) { all_have = false; break; }
        }
        if (!all_have) { ++result.layers_failed; continue; }

        const LoRAWeightEntry& ref_entry = adapters[0]->getWeights(lname);
        size_t in_dim  = ref_entry.in_dim;
        size_t out_dim = ref_entry.out_dim;
        float  alpha   = ref_entry.alpha;

        std::vector<AdapterDescriptor> descs = {};

        for (size_t i = 0; i < adapters.size(); ++i)
            descs.push_back({adapters[i], lname, weights[i]});

        auto lr = mergeLinear(descs, lname, in_dim, out_dim, output_rank, alpha);
        if (lr.success) {
          ++result.layers_merged;
        }
        else ++result.layers_failed;
        result.layers.push_back(std::move(lr));
    }

    result.success = (result.layers_failed == 0);
    return result;
}

// ============================================================================
// TIES merge
// ============================================================================

MergeLayerResult LoRAAdapterMerger::mergeTIES(
    const std::vector<AdapterDescriptor>& adapters,
    const std::string& out_layer,
    size_t in_dim, size_t out_dim, size_t rank,
    float alpha, float trim_threshold) const
{
    MergeLayerResult result;
    result.layer_name = out_layer;

    if (adapters.empty()) {
        result.error_message = "No adapters provided";
        return result;
    }
    if (in_dim == 0 || out_dim == 0 || rank == 0) {
        result.error_message = "Dimensions and rank must be > 0";
        return result;
    }
    if (trim_threshold < 0.0f || trim_threshold >= 1.0f) {
        result.error_message = "trim_threshold must be in [0, 1)";
        return result;
    }

    const size_t N = in_dim * out_dim;
    const float  scaling = alpha / static_cast<float>(rank);

    // Collect trimmed ΔW for each adapter
    std::vector<std::vector<float>> trimmed(adapters.size());

    for (size_t ai = 0; ai < adapters.size(); ++ai) {
        const auto& desc = adapters[ai];
        if (!desc.adapter) {
            result.error_message = "Null adapter pointer";
            return result;
        }
        if (!desc.adapter->hasLayer(desc.layer_name)) {
            result.error_message = "Layer '" + desc.layer_name + "' not found";
            return result;
        }
        const LoRAWeightEntry& we = desc.adapter->getWeights(desc.layer_name);
        float sc = we.alpha / static_cast<float>(we.rank);
        auto  dW = computeDeltaW(we.B, we.A, in_dim, out_dim, we.rank, sc);

        // Step 1: Trim – zero out values below threshold
        float max_abs = 0.0f;
        for (float v : dW) {
          max_abs = std::max(max_abs, std::abs(v));
        }
        float thresh = trim_threshold * max_abs;
        for (float& v : dW)
            if (std::abs(v) < thresh) {
              v = 0.0f;
            }

        trimmed[ai] = std::move(dW);
    }

    // Step 2: Resolve – majority-vote sign per element
    std::vector<float> resolved_sign(N, 0.0f);
    for (size_t k = 0; k < N; ++k) {
        int pos = 0, neg = 0;
        for (size_t ai = 0; ai < adapters.size(); ++ai) {
            float v = trimmed[ai][k];
            if (v > 0.0f) {
              ++pos;
            }
            else if (v < 0.0f) ++neg;
        }
        if (pos > neg) {
          resolved_sign[k] =  1.0f;
        }
        else if (neg > pos)  resolved_sign[k] = -1.0f;
        else                 resolved_sign[k] =  1.0f; // tie → positive
    }

    // Step 3: Merge – average values that agree with resolved sign
    std::vector<float> merged_dW(N, 0.0f);
    std::vector<int>   count(N, 0);

    for (size_t ai = 0; ai < adapters.size(); ++ai) {
        for (size_t k = 0; k < N; ++k) {
            float v = trimmed[ai][k];
            if (v == 0.0f) {
              continue;
            }
            // Keep only if same sign as resolved
            if ((resolved_sign[k] > 0.0f && v > 0.0f) ||
                (resolved_sign[k] < 0.0f && v < 0.0f)) {
                merged_dW[k] += v;
                ++count[k];
            }
        }
    }
    for (size_t k = 0; k < N; ++k) {
        if (count[k] > 1)
            merged_dW[k] /= static_cast<float>(count[k]);
    }

    // Apply adapter weights (blend weights)
    // Re-weight by desc.weight (used as scaling multiplier in TIES context)
    // The standard TIES paper uses equal weights; here we respect them.
    // Compute weighted sum and renormalise.
    float total_w = 0.0f;
    for (const auto& d : adapters) {
      total_w += d.weight;
    }
    if (total_w > 0.0f) {
        // Already averaged above; scale by mean weight to preserve magnitude
        float mean_w = total_w / static_cast<float>(adapters.size());
        for (float& v : merged_dW) {
          v *= mean_w;
        }
    }

    factoriseDeltaW(merged_dW, in_dim, out_dim, rank, scaling,
                    result.B, result.A);
    result.success = true;
    return result;
}

MergeResult LoRAAdapterMerger::mergeTIESAll(
    const std::vector<const LoRAAdapter*>& adapters,
    size_t                                  output_rank,
    float                                   trim_threshold) const
{
    MergeResult result = {};

    if (adapters.empty()) {
        result.error_message = "No adapters provided";
        return result;
    }
    for (const auto* a : adapters) {
        if (!a) {
            result.error_message = "Null adapter pointer";
            return result;
        }
    }

    const auto& first_layers = adapters[0]->layerNames();
    for (const auto& lname : first_layers) {
        bool all_have = true;
        for (const auto* a : adapters) {
            if (!a->hasLayer(lname)) { all_have = false; break; }
        }
        if (!all_have) { ++result.layers_failed; continue; }

        const LoRAWeightEntry& ref = adapters[0]->getWeights(lname);
        size_t in_dim  = ref.in_dim;
        size_t out_dim = ref.out_dim;
        float  alpha   = ref.alpha;

        std::vector<AdapterDescriptor> descs = {};

        for (const auto* a : adapters)
            descs.push_back({a, lname, 1.0f});

        auto lr = mergeTIES(descs, lname, in_dim, out_dim, output_rank,
                            alpha, trim_threshold);
        if (lr.success) {
          ++result.layers_merged;
        }
        else ++result.layers_failed;
        result.layers.push_back(std::move(lr));
    }

    result.success = (result.layers_failed == 0);
    return result;
}

// ============================================================================
// Phase 2: Validation and edge case handling
// ============================================================================

std::string LoRAAdapterMerger::validateMergeInputs(
    const std::vector<AdapterDescriptor>& adapters) const {
    
    // Check: at least one adapter
    if (adapters.empty()) {
        return "Cannot merge zero adapters";
    }
    
    // Check: all adapters are non-null
    for (size_t i = 0; i < adapters.size(); ++i) {
        if (!adapters[i].adapter) {
            return "Adapter at index " + std::to_string(i) + " is null";
        }
    }
    
    // Check: all weights are positive
    for (size_t i = 0; i < adapters.size(); ++i) {
        if (adapters[i].weight <= 0.0f) {
            return "Adapter " + std::to_string(i) + " has non-positive weight " +
                   std::to_string(adapters[i].weight);
        }
    }
    
    // Check: all adapters have at least one layer
    for (size_t i = 0; i < adapters.size(); ++i) {
        const auto* adapter = adapters[i].adapter;
        if (!adapter || adapter->layerCount() == 0) {
            return "Adapter at index " + std::to_string(i) + " is empty (no layers)";
        }
    }
    
    return ""; // Valid
}

bool LoRAAdapterMerger::validateMergeResult(const MergeResult& result) const {
    // Check: result should have layers if it claims success
    if (result.success && result.layers.empty()) {
        return false;
    }
    
    // Check: each layer result that claims success should have valid matrices
    for (const auto& layer : result.layers) {
        if (!layer.success) {
          continue;
        }
        
        // Basic sanity: matrices should not be empty
        if (layer.A.empty() || layer.B.empty()) {
            return false;
        }
        
        // Check for NaN/Inf values
        for (float val : layer.A) {
            if (!std::isfinite(val)) {
              return false;
            }
        }
        for (float val : layer.B) {
            if (!std::isfinite(val)) {
              return false;
            }
        }
    }
    
    return true;
}

} // namespace training
} // namespace themis

