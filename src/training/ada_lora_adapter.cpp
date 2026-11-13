/**
 * @file ada_lora_adapter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "training/ada_lora_adapter.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <unordered_map>

namespace themis {
namespace training {

// ============================================================================
// Kaiming uniform initialiser (same convention as lora_adapter.cpp)
// ============================================================================

namespace {

std::vector<float> kaimingUniform(size_t rows, size_t cols, uint32_t seed = 0) {
    const float bound = std::sqrt(2.0f / static_cast<float>(rows));
    /**
     * @brief Gen.
     * @param[in] seed Input parameter.
     * @return Return value.
     */
    std::mt19937 gen(seed);
    std::uniform_real_distribution<float> dist(-bound, bound);
    /**
     * @brief W.
     * @param[in,out] cols Input/output parameter.
     * @return Return value.
     */
    std::vector<float> w(rows * cols);
    for (auto& v : w) {
      v = dist(gen);
    }
    return w;
}

/**
 * @brief Zeros.
 * @param[in] count Input parameter.
 * @return Return value.
 * @details Implements zeros without additional internal calls.
 */
std::vector<float> zeros(size_t count) {
    return std::vector<float>(count, 0.0f);
}

} // anonymous namespace

// ============================================================================
// Impl
// ============================================================================

class AdaLoRAAdapter::Impl {
public:
    // Per-layer state
    struct Layer {
        std::string layer_name;
        size_t      in_dim    = 0;
        size_t      out_dim   = 0;
        size_t      max_rank  = 0;
        size_t      active_rank = 0;
        float       alpha     = 8.0f;   ///< scaling = alpha / max_rank
        float       importance = 0.0f;
        std::vector<float> B; ///< in_dim × max_rank
        std::vector<float> A; ///< max_rank × out_dim
    };

    /**
     * @brief Impl.
     * @param[in] default_rank Input parameter.
     * @param[in] default_alpha Input parameter.
     * @param[in] rank_budget Input parameter.
     * @return Return value.
     */
    explicit Impl(size_t default_rank, float default_alpha, size_t rank_budget)
        : default_rank_(default_rank > 0 ? default_rank : 4)
        , default_alpha_(default_alpha > 0.0f ? default_alpha : 8.0f)
        , rank_budget_(rank_budget > 0 ? rank_budget : 64)
    {}

    // -------------------------------------------------------------------------
    // Layer management
    // -------------------------------------------------------------------------

    /**
     * @brief Add Layer.
     * @param[in] name Input parameter.
     * @param[in] in_dim Input parameter.
     * @param[in] out_dim Input parameter.
     * @param[in] max_rank Input parameter.
     * @param[in] alpha Input parameter.
     * @throws std::invalid_argument if an error occurs.
     * @details Calls: empty(), count(), kaimingUniform(), zeros(), std::move(), push_back().
     */
    void addLayer(const std::string& name, size_t in_dim, size_t out_dim,
                  size_t max_rank, float alpha) {
        if (name.empty())
            throw std::invalid_argument("Layer name must not be empty");
        if (in_dim == 0 || out_dim == 0)
            throw std::invalid_argument("Dimensions must be > 0");
        if (layers_.count(name))
            throw std::invalid_argument("Layer '" + name + "' already exists");

        size_t r = (max_rank > 0) ? max_rank : default_rank_;
        float  a = (alpha > 0.0f)  ? alpha    : default_alpha_;

        Layer lay;
        lay.layer_name  = name;
        lay.in_dim      = in_dim;
        lay.out_dim     = out_dim;
        lay.max_rank    = r;
        lay.active_rank = r;   // start fully active
        lay.alpha       = a;
        lay.importance  = 0.0f;
        // seed based on name hash for determinism
        uint32_t seed = static_cast<uint32_t>(std::hash<std::string>{}(name));
        lay.B = kaimingUniform(in_dim, r, seed);
        lay.A = zeros(r * out_dim);

        layers_[name]       = std::move(lay);
        insertion_order_.push_back(name);
    }

    /**
     * @brief Remove Layer.
     * @param[in] name Input parameter.
     * @return True when the operation succeeds.
     * @details Calls: find(), end(), erase(), std::remove(), begin().
     */
    bool removeLayer(const std::string& name) {
        auto it = layers_.find(name);
        if (it == layers_.end()) {
          return false;
        }
        layers_.erase(it);
        insertion_order_.erase(
            std::remove(insertion_order_.begin(), insertion_order_.end(), name),
            insertion_order_.end());
        return true;
    }

    bool hasLayer(const std::string& name) const {
        return layers_.count(name) > 0;
    }

    std::vector<std::string> layerNames() const {
        return insertion_order_;
    }

    size_t layerCount() const { return layers_.size(); }

    // -------------------------------------------------------------------------
    // Importance update
    // -------------------------------------------------------------------------

    /**
     * @brief Importance ≈ mean_i( ||B[:,i]||_F^2 * ||A[i,:]||_F^2 ) for i in active_rank.
     * @param[in] name Input parameter.
     * @details This is a rank-component outer-product approximation to the nuclear-norm importance used in the original AdaLoRA paper. Calls: getLayer().
     */
    void updateImportance(const std::string& name) {
        Layer& lay = getLayer(name);
        const size_t r = lay.active_rank;
        if (r == 0) {
            lay.importance = 0.0f;
            return;
        }
        const size_t D_in  = lay.in_dim;
        const size_t D_out = lay.out_dim;

        float total = 0.0f;
        for (size_t i = 0; i < r; ++i) {
            // ||B[:, i]||^2
            float norm_B = 0.0f;
            for (size_t d = 0; d < D_in; ++d) {
                float v = lay.B[d * lay.max_rank + i];
                norm_B += v * v;
            }
            // ||A[i, :]||^2
            float norm_A = 0.0f;
            for (size_t d = 0; d < D_out; ++d) {
                float v = lay.A[i * D_out + d];
                norm_A += v * v;
            }
            total += norm_B * norm_A;
        }
        lay.importance = (r > 0) ? total / static_cast<float>(r) : 0.0f;
    }

    /**
     * @brief Update All Importances.
     * @details Calls: updateImportance().
     */
    void updateAllImportances() {
        for (auto& [name, _] : layers_) {
            updateImportance(name);
        }
    }

    // -------------------------------------------------------------------------
    // Rank reallocation
    // -------------------------------------------------------------------------

    /**
     * @brief Reallocate Ranks.
     * @param[in] total_budget Input parameter.
     * @return Return value.
     * @throws std::invalid_argument if an error occurs.
     * @details Calls: empty(), size(), std::min(), allocs(), at(), std::round().
     */
    ReallocResult reallocateRanks(size_t total_budget) {
        if (total_budget == 0)
            throw std::invalid_argument("total_budget must be > 0");
        if (layers_.empty()) return {};

        // Compute total importance
        float total_importance = 0.0f;
        for (const auto& name : insertion_order_)
            total_importance += layers_.at(name).importance;

        ReallocResult result = {};

        if (total_importance <= 0.0f) {
            // No importance data yet; distribute budget evenly
            size_t n = layers_.size();
            size_t per_layer = std::max<size_t>(1, total_budget / n);
            for (const auto& name : insertion_order_) {
                auto& lay = layers_.at(name);
                size_t new_rank = std::min(per_layer, lay.max_rank);
                new_rank        = std::max<size_t>(1, new_rank);
                if (new_rank < lay.active_rank) {
                  ++result.layers_pruned;
                }
                else if (new_rank > lay.active_rank) ++result.layers_expanded;
                lay.active_rank = new_rank;
                result.total_active_rank += new_rank;
            }
            return result;
        }

        // Proportional allocation
        std::vector<std::string>& order = insertion_order_;
        std::vector<size_t> allocs(order.size());
        size_t allocated = 0;

        for (size_t i = 0; i < order.size(); ++i) {
            Layer& lay = layers_.at(order[i]);
            float  frac = lay.importance / total_importance;
            size_t raw  = static_cast<size_t>(std::round(frac * static_cast<float>(total_budget)));
            raw = std::max<size_t>(1, raw);
            raw = std::min(raw, lay.max_rank);
            allocs[i] = raw;
            allocated += raw;
        }

        // Adjust for rounding errors: redistribute surplus/deficit from/to
        // the most-important layer.
        if (allocated != total_budget && !order.empty()) {
            // Find layer with highest importance
            size_t best = 0;
            float  best_imp = -1.0f;
            for (size_t i = 0; i < order.size(); ++i) {
                if (layers_.at(order[i]).importance > best_imp) {
                    best_imp = layers_.at(order[i]).importance;
                    best     = i;
                }
            }
            if (allocated > total_budget) {
                size_t excess = allocated - total_budget;
                allocs[best] = (allocs[best] > excess + 1)
                               ? allocs[best] - excess : 1;
            } else {
                size_t deficit = total_budget - allocated;
                allocs[best] = std::min(allocs[best] + deficit,
                                        layers_.at(order[best]).max_rank);
            }
        }

        // Apply
        for (size_t i = 0; i < order.size(); ++i) {
            Layer& lay      = layers_.at(order[i]);
            size_t new_rank = allocs[i];
            if (new_rank < lay.active_rank) {
              ++result.layers_pruned;
            }
            else if (new_rank > lay.active_rank) ++result.layers_expanded;
            lay.active_rank = new_rank;
            result.total_active_rank += new_rank;
        }
        return result;
    }

    // -------------------------------------------------------------------------
    // Accessors
    // -------------------------------------------------------------------------

    size_t getActiveRank(const std::string& name) const {
        return getLayerConst(name).active_rank;
    }

    size_t getMaxRank(const std::string& name) const {
        return getLayerConst(name).max_rank;
    }

    float getImportance(const std::string& name) const {
        return getLayerConst(name).importance;
    }

    std::vector<AdaLoRALayerStats> getLayerStats() const {
        std::vector<AdaLoRALayerStats> stats = {};

        stats.reserve(insertion_order_.size());
        for (const auto& name : insertion_order_) {
            const Layer& lay = layers_.at(name);
            AdaLoRALayerStats s;
            s.layer_name   = lay.layer_name;
            s.max_rank     = lay.max_rank;
            s.active_rank  = lay.active_rank;
            s.importance   = lay.importance;
            s.alpha        = lay.alpha;
            stats.push_back(s);
        }
        return stats;
    }

    size_t totalActiveParameterCount() const {
        size_t total = 0;
        for (const auto& [_, lay] : layers_)
            total += lay.active_rank * (lay.in_dim + lay.out_dim);
        return total;
    }

    /**
     * @brief Set Weights.
     * @param[in] name Input parameter.
     * @param[in] B Input parameter.
     * @param[in] A Input parameter.
     * @throws std::invalid_argument if an error occurs.
     * @details Calls: getLayer(), size().
     */
    void setWeights(const std::string& name,
                    const std::vector<float>& B,
                    const std::vector<float>& A) {
        Layer& lay = getLayer(name);
        if (B.size() != lay.in_dim * lay.max_rank)
            throw std::invalid_argument("B size mismatch");
        if (A.size() != lay.max_rank * lay.out_dim)
            throw std::invalid_argument("A size mismatch");
        lay.B = B;
        lay.A = A;
    }

    std::pair<std::vector<float>, std::vector<float>>
    getWeights(const std::string& name) const {
        const Layer& lay = getLayerConst(name);
        return {lay.B, lay.A};
    }

    // -------------------------------------------------------------------------
    // Forward pass (active rank only)
    // -------------------------------------------------------------------------

    std::vector<float> forward(const std::string& name,
                               const std::vector<float>& input,
                               size_t batch_size) const {
        const Layer& lay = getLayerConst(name);
        const size_t D_in  = lay.in_dim;
        const size_t D_out = lay.out_dim;
        const size_t r     = lay.active_rank;

        if (input.size() != batch_size * D_in)
            throw std::invalid_argument("Input size mismatch");

        const float scaling = lay.alpha / static_cast<float>(lay.max_rank);

        // hidden = input @ B[:, :r]   → batch_size × r
        std::vector<float> hidden(batch_size * r, 0.0f);
        for (size_t b = 0; b < batch_size; ++b) {
            for (size_t i = 0; i < r; ++i) {
                float acc = 0.0f;
                for (size_t d = 0; d < D_in; ++d) {
                    acc += input[b * D_in + d] * lay.B[d * lay.max_rank + i];
                }
                hidden[b * r + i] = acc;
            }
        }

        // output = hidden @ A[:r, :]  → batch_size × D_out
        std::vector<float> output(batch_size * D_out, 0.0f);
        for (size_t b = 0; b < batch_size; ++b) {
            for (size_t d = 0; d < D_out; ++d) {
                float acc = 0.0f;
                for (size_t i = 0; i < r; ++i) {
                    acc += hidden[b * r + i] * lay.A[i * D_out + d];
                }
                output[b * D_out + d] = acc * scaling;
            }
        }
        return output;
    }

    AdaLoRATrainStepResult trainBatch(const std::string& name,
                                      const std::vector<float>& input,
                                      const std::vector<float>& targets,
                                      size_t batch_size,
                                      float learning_rate) {
        Layer& lay = getLayer(name);
        if (batch_size == 0 || input.size() % batch_size != 0 ||
            input.size() / batch_size != lay.in_dim ||
            targets.size() % batch_size != 0 ||
            targets.size() / batch_size != lay.out_dim) {
            throw std::invalid_argument("AdaLoRAAdapter::trainBatch: activation or target shape mismatch");
        }
        if (!std::isfinite(learning_rate) || learning_rate <= 0.0f) {
            throw std::invalid_argument("AdaLoRAAdapter::trainBatch: learning_rate must be positive and finite");
        }

        const size_t rank = lay.active_rank;
        const size_t sample_width = batch_size * lay.out_dim;
        const float scaling = lay.alpha / static_cast<float>(lay.max_rank);
        const float normalization = 1.0f / static_cast<float>(sample_width);
        std::vector<float> grad_a(lay.max_rank * lay.out_dim, 0.0f);
        std::vector<float> grad_b(lay.in_dim * lay.max_rank, 0.0f);
        std::vector<float> hidden(rank, 0.0f);
        std::vector<float> grad_hidden(rank, 0.0f);
        AdaLoRATrainStepResult result;
        result.sample_count = batch_size;

        for (size_t sample = 0; sample < batch_size; ++sample) {
            const float* sample_input = input.data() + sample * lay.in_dim;
            const float* sample_target = targets.data() + sample * lay.out_dim;
            std::fill(hidden.begin(), hidden.end(), 0.0f);
            std::fill(grad_hidden.begin(), grad_hidden.end(), 0.0f);

            for (size_t component = 0; component < rank; ++component) {
                for (size_t feature = 0; feature < lay.in_dim; ++feature) {
                    hidden[component] += sample_input[feature] * lay.B[feature * lay.max_rank + component];
                }
            }

            for (size_t output = 0; output < lay.out_dim; ++output) {
                float prediction = 0.0f;
                for (size_t component = 0; component < rank; ++component) {
                    prediction += hidden[component] * lay.A[component * lay.out_dim + output];
                }
                const float error = (prediction * scaling) - sample_target[output];
                result.loss += 0.5f * error * error * normalization;
                const float scaled_error = error * scaling * normalization;
                for (size_t component = 0; component < rank; ++component) {
                    grad_a[component * lay.out_dim + output] += hidden[component] * scaled_error;
                    grad_hidden[component] += lay.A[component * lay.out_dim + output] * scaled_error;
                }
            }

            for (size_t component = 0; component < rank; ++component) {
                for (size_t feature = 0; feature < lay.in_dim; ++feature) {
                    grad_b[feature * lay.max_rank + component] +=
                        sample_input[feature] * grad_hidden[component];
                }
            }
        }

        for (size_t component = 0; component < rank; ++component) {
            for (size_t output = 0; output < lay.out_dim; ++output) {
                const size_t index = component * lay.out_dim + output;
                result.gradient_norm += grad_a[index] * grad_a[index];
                lay.A[index] -= learning_rate * grad_a[index];
            }
            for (size_t feature = 0; feature < lay.in_dim; ++feature) {
                const size_t index = feature * lay.max_rank + component;
                result.gradient_norm += grad_b[index] * grad_b[index];
                lay.B[index] -= learning_rate * grad_b[index];
            }
        }
        result.gradient_norm = std::sqrt(result.gradient_norm);
        updateImportance(name);
        return result;
    }

    AdaLoRATrainStepResult trainCausalLmBatch(
        const std::vector<float>& input,
        const std::vector<float>& base_logits,
        const std::vector<size_t>& target_token_ids,
        const std::vector<float>& sample_weights,
        size_t batch_size,
        float learning_rate,
        float max_gradient_norm) {
        if (insertion_order_.empty()) {
            throw std::invalid_argument("AdaLoRAAdapter::trainCausalLmBatch: no output shards registered");
        }
        if (batch_size == 0 || !std::isfinite(learning_rate) || learning_rate <= 0.0f ||
            !std::isfinite(max_gradient_norm) || max_gradient_norm <= 0.0f) {
            throw std::invalid_argument("AdaLoRAAdapter::trainCausalLmBatch: invalid batch or optimizer values");
        }

        const size_t input_dim = layers_.at(insertion_order_.front()).in_dim;
        size_t vocabulary_size = 0;
        for (const auto& name : insertion_order_) {
            const auto& layer = layers_.at(name);
            if (layer.in_dim != input_dim || layer.active_rank == 0 ||
                vocabulary_size > std::numeric_limits<size_t>::max() - layer.out_dim) {
                throw std::invalid_argument("AdaLoRAAdapter::trainCausalLmBatch: incompatible output shards");
            }
            vocabulary_size += layer.out_dim;
        }
        if (batch_size > std::numeric_limits<size_t>::max() / input_dim ||
            batch_size > std::numeric_limits<size_t>::max() / vocabulary_size ||
            input.size() != batch_size * input_dim ||
            base_logits.size() != batch_size * vocabulary_size ||
            target_token_ids.size() != batch_size ||
            (!sample_weights.empty() && sample_weights.size() != batch_size)) {
            throw std::invalid_argument("AdaLoRAAdapter::trainCausalLmBatch: batch shape mismatch");
        }
        const auto all_finite = [](const std::vector<float>& values) {
            return std::all_of(values.begin(), values.end(), [](float value) {
                return std::isfinite(value);
            });
        };
        if (!all_finite(input) || !all_finite(base_logits)) {
            throw std::invalid_argument("AdaLoRAAdapter::trainCausalLmBatch: inputs must be finite");
        }
        if (std::any_of(target_token_ids.begin(), target_token_ids.end(),
                        [vocabulary_size](size_t token) { return token >= vocabulary_size; })) {
            throw std::invalid_argument("AdaLoRAAdapter::trainCausalLmBatch: target token is outside vocabulary");
        }
        double weight_sum = 0.0;
        if (sample_weights.empty()) {
            weight_sum = static_cast<double>(batch_size);
        } else {
            for (const float weight : sample_weights) {
                if (!std::isfinite(weight) || weight <= 0.0f) {
                    throw std::invalid_argument("AdaLoRAAdapter::trainCausalLmBatch: sample weights must be positive and finite");
                }
                weight_sum += weight;
            }
        }

        std::vector<std::vector<float>> grad_a;
        std::vector<std::vector<float>> grad_b;
        grad_a.reserve(insertion_order_.size());
        grad_b.reserve(insertion_order_.size());
        for (const auto& name : insertion_order_) {
            const auto& layer = layers_.at(name);
            grad_a.emplace_back(layer.max_rank * layer.out_dim, 0.0f);
            grad_b.emplace_back(layer.in_dim * layer.max_rank, 0.0f);
        }

        AdaLoRATrainStepResult result;
        result.sample_count = batch_size;
        std::vector<float> logits(vocabulary_size);
        std::vector<std::vector<float>> hidden(insertion_order_.size());
        std::vector<size_t> shard_offsets(insertion_order_.size());
        size_t offset = 0;
        for (size_t shard = 0; shard < insertion_order_.size(); ++shard) {
            shard_offsets[shard] = offset;
            offset += layers_.at(insertion_order_[shard]).out_dim;
        }

        for (size_t row = 0; row < batch_size; ++row) {
            std::copy_n(base_logits.data() + row * vocabulary_size, vocabulary_size, logits.begin());
            const float* features = input.data() + row * input_dim;
            for (size_t shard = 0; shard < insertion_order_.size(); ++shard) {
                auto& layer = layers_.at(insertion_order_[shard]);
                const size_t rank = layer.active_rank;
                auto& shard_hidden = hidden[shard];
                shard_hidden.assign(rank, 0.0f);
                const float scaling = layer.alpha / static_cast<float>(layer.max_rank);
                for (size_t component = 0; component < rank; ++component) {
                    for (size_t feature = 0; feature < input_dim; ++feature) {
                        shard_hidden[component] += features[feature] *
                            layer.B[feature * layer.max_rank + component];
                    }
                }
                for (size_t output = 0; output < layer.out_dim; ++output) {
                    float residual = 0.0f;
                    for (size_t component = 0; component < rank; ++component) {
                        residual += shard_hidden[component] * layer.A[component * layer.out_dim + output];
                    }
                    logits[shard_offsets[shard] + output] += residual * scaling;
                }
            }

            const float max_logit = *std::max_element(logits.begin(), logits.end());
            const float row_weight = sample_weights.empty() ? 1.0f : sample_weights[row];
            const float batch_scale = static_cast<float>(static_cast<double>(row_weight) / weight_sum);
            double exp_sum = 0.0;
            for (const float logit : logits) {
                exp_sum += std::exp(static_cast<double>(logit - max_logit));
            }
            if (!std::isfinite(exp_sum) || exp_sum <= 0.0) {
                throw std::runtime_error("AdaLoRAAdapter::trainCausalLmBatch: invalid softmax normalization");
            }
            const size_t target = target_token_ids[row];
            result.loss += static_cast<float>(
                (static_cast<double>(max_logit) + std::log(exp_sum) - logits[target]) * batch_scale);

            for (size_t shard = 0; shard < insertion_order_.size(); ++shard) {
                auto& layer = layers_.at(insertion_order_[shard]);
                const size_t rank = layer.active_rank;
                const size_t shard_offset = shard_offsets[shard];
                const float scaling = layer.alpha / static_cast<float>(layer.max_rank);
                std::vector<float> grad_hidden(rank, 0.0f);
                for (size_t output = 0; output < layer.out_dim; ++output) {
                    const size_t token = shard_offset + output;
                    const double probability = std::exp(static_cast<double>(logits[token] - max_logit)) / exp_sum;
                    const float grad_logit = static_cast<float>(
                        (probability - (token == target ? 1.0 : 0.0)) * batch_scale);
                    for (size_t component = 0; component < rank; ++component) {
                        grad_a[shard][component * layer.out_dim + output] +=
                            hidden[shard][component] * grad_logit * scaling;
                        grad_hidden[component] += layer.A[component * layer.out_dim + output] *
                            grad_logit * scaling;
                    }
                }
                for (size_t component = 0; component < rank; ++component) {
                    for (size_t feature = 0; feature < input_dim; ++feature) {
                        grad_b[shard][feature * layer.max_rank + component] +=
                            features[feature] * grad_hidden[component];
                    }
                }
            }
        }

        double squared_norm = 0.0;
        for (size_t shard = 0; shard < insertion_order_.size(); ++shard) {
            const auto& layer = layers_.at(insertion_order_[shard]);
            for (size_t component = 0; component < layer.active_rank; ++component) {
                for (size_t output = 0; output < layer.out_dim; ++output) {
                    const float gradient = grad_a[shard][component * layer.out_dim + output];
                    squared_norm += static_cast<double>(gradient) * gradient;
                }
                for (size_t feature = 0; feature < input_dim; ++feature) {
                    const float gradient = grad_b[shard][feature * layer.max_rank + component];
                    squared_norm += static_cast<double>(gradient) * gradient;
                }
            }
        }
        result.gradient_norm = static_cast<float>(std::sqrt(squared_norm));
        const float clip_scale = result.gradient_norm > max_gradient_norm
            ? max_gradient_norm / result.gradient_norm
            : 1.0f;
        for (size_t shard = 0; shard < insertion_order_.size(); ++shard) {
            auto& layer = layers_.at(insertion_order_[shard]);
            for (size_t component = 0; component < layer.active_rank; ++component) {
                for (size_t output = 0; output < layer.out_dim; ++output) {
                    const size_t index = component * layer.out_dim + output;
                    layer.A[index] -= learning_rate * clip_scale * grad_a[shard][index];
                }
                for (size_t feature = 0; feature < input_dim; ++feature) {
                    const size_t index = feature * layer.max_rank + component;
                    layer.B[index] -= learning_rate * clip_scale * grad_b[shard][index];
                }
            }
        }
        updateAllImportances();
        return result;
    }

    size_t rankBudget() const { return rank_budget_; }
    /**
     * @brief Set Rank Budget.
     * @param[in] b Input parameter.
     * @details Implements setRankBudget without additional internal calls.
     */
    void setRankBudget(size_t b) { rank_budget_ = b; }

private:
    friend class AdaLoRAAdapter;
    size_t default_rank_ = {};
    float  default_alpha_ = {};
    size_t rank_budget_ = {};

    std::unordered_map<std::string, Layer> layers_;
    std::vector<std::string> insertion_order_;

    /**
     * @brief Get Layer.
     * @param[in] name Input parameter.
     * @return Return value.
     * @throws std::out_of_range if an error occurs.
     * @details Calls: find(), end().
     */
    Layer& getLayer(const std::string& name) {
        auto it = layers_.find(name);
        if (it == layers_.end())
            throw std::out_of_range("AdaLoRAAdapter: unknown layer '" + name + "'");
        return it->second;
    }

    const Layer& getLayerConst(const std::string& name) const {
        auto it = layers_.find(name);
        if (it == layers_.end())
            throw std::out_of_range("AdaLoRAAdapter: unknown layer '" + name + "'");
        return it->second;
    }
};

// ============================================================================
// Public API implementation
// ============================================================================

AdaLoRAAdapter::AdaLoRAAdapter(size_t default_rank, float default_alpha, size_t rank_budget)
    : impl_(std::make_unique<Impl>(default_rank, default_alpha, rank_budget))
{}

AdaLoRAAdapter::~AdaLoRAAdapter() = default;
AdaLoRAAdapter::AdaLoRAAdapter(AdaLoRAAdapter&&) noexcept = default;
AdaLoRAAdapter& AdaLoRAAdapter::operator=(AdaLoRAAdapter&&) noexcept = default;

/**
 * @brief Add Layer.
 * @param[in] layer_name Name of the layer.
 * @param[in] in_dim Input parameter.
 * @param[in] out_dim Input parameter.
 * @param[in] max_rank Input parameter.
 * @param[in] alpha Input parameter.
 * @details Implements addLayer without additional internal calls.
 */
void AdaLoRAAdapter::addLayer(const std::string& layer_name,
                              size_t in_dim, size_t out_dim,
                              size_t max_rank, float alpha) {
    impl_->addLayer(layer_name, in_dim, out_dim, max_rank, alpha);
}

/**
 * @brief Remove Layer.
 * @param[in] layer_name Name of the layer.
 * @return True when the operation succeeds.
 * @details Implements removeLayer without additional internal calls.
 */
bool AdaLoRAAdapter::removeLayer(const std::string& layer_name) {
    return impl_->removeLayer(layer_name);
}

bool AdaLoRAAdapter::hasLayer(const std::string& layer_name) const {
    return impl_->hasLayer(layer_name);
}

std::vector<std::string> AdaLoRAAdapter::layerNames() const {
    return impl_->layerNames();
}

size_t AdaLoRAAdapter::layerCount() const {
    return impl_->layerCount();
}

/**
 * @brief Update Importance.
 * @param[in] layer_name Name of the layer.
 * @details Implements updateImportance without additional internal calls.
 */
void AdaLoRAAdapter::updateImportance(const std::string& layer_name) {
    impl_->updateImportance(layer_name);
}

/**
 * @brief Update All Importances.
 * @details Implements updateAllImportances without additional internal calls.
 */
void AdaLoRAAdapter::updateAllImportances() {
    impl_->updateAllImportances();
}

/**
 * @brief Reallocate Ranks.
 * @param[in] total_budget Input parameter.
 * @return Return value.
 * @details Implements reallocateRanks without additional internal calls.
 */
ReallocResult AdaLoRAAdapter::reallocateRanks(size_t total_budget) {
    return impl_->reallocateRanks(total_budget);
}

/**
 * @brief Reallocate Ranks.
 * @return Return value.
 * @details Calls: rankBudget().
 */
ReallocResult AdaLoRAAdapter::reallocateRanks() {
    return impl_->reallocateRanks(impl_->rankBudget());
}

size_t AdaLoRAAdapter::getActiveRank(const std::string& layer_name) const {
    return impl_->getActiveRank(layer_name);
}

size_t AdaLoRAAdapter::getMaxRank(const std::string& layer_name) const {
    return impl_->getMaxRank(layer_name);
}

float AdaLoRAAdapter::getImportance(const std::string& layer_name) const {
    return impl_->getImportance(layer_name);
}

std::vector<AdaLoRALayerStats> AdaLoRAAdapter::getLayerStats() const {
    return impl_->getLayerStats();
}

size_t AdaLoRAAdapter::totalActiveParameterCount() const {
    return impl_->totalActiveParameterCount();
}

/**
 * @brief Set Weights.
 * @param[in] layer_name Name of the layer.
 * @param[in] B Input parameter.
 * @param[in] A Input parameter.
 * @details Implements setWeights without additional internal calls.
 */
void AdaLoRAAdapter::setWeights(const std::string& layer_name,
                                const std::vector<float>& B,
                                const std::vector<float>& A) {
    impl_->setWeights(layer_name, B, A);
}

std::pair<std::vector<float>, std::vector<float>>
AdaLoRAAdapter::getWeights(const std::string& layer_name) const {
    return impl_->getWeights(layer_name);
}

std::vector<float> AdaLoRAAdapter::forward(const std::string& layer_name,
                                           const std::vector<float>& input,
                                           size_t batch_size) const {
    return impl_->forward(layer_name, input, batch_size);
}

AdaLoRATrainStepResult AdaLoRAAdapter::trainBatch(
    const std::string& layer_name,
    const std::vector<float>& input,
    const std::vector<float>& targets,
    size_t batch_size,
    float learning_rate) {
    return impl_->trainBatch(layer_name, input, targets, batch_size, learning_rate);
}

AdaLoRATrainStepResult AdaLoRAAdapter::trainCausalLmBatch(
    const std::vector<float>& input,
    const std::vector<float>& base_logits,
    const std::vector<size_t>& target_token_ids,
    size_t batch_size,
    float learning_rate,
    float max_gradient_norm) {
    return impl_->trainCausalLmBatch(
        input, base_logits, target_token_ids, {}, batch_size, learning_rate, max_gradient_norm);
}

AdaLoRATrainStepResult AdaLoRAAdapter::trainCausalLmBatch(
    const std::vector<float>& input,
    const std::vector<float>& base_logits,
    const std::vector<size_t>& target_token_ids,
    const std::vector<float>& sample_weights,
    size_t batch_size,
    float learning_rate,
    float max_gradient_norm) {
    return impl_->trainCausalLmBatch(
        input, base_logits, target_token_ids, sample_weights,
        batch_size, learning_rate, max_gradient_norm);
}

size_t AdaLoRAAdapter::rankBudget() const {
    return impl_->rankBudget();
}

/**
 * @brief Set Rank Budget.
 * @param[in] budget Input parameter.
 * @details Implements setRankBudget without additional internal calls.
 */
void AdaLoRAAdapter::setRankBudget(size_t budget) {
    impl_->setRankBudget(budget);
}

// ============================================================================
// Persistence helpers — binary checkpoint format
// ============================================================================

namespace {

// File format constants
constexpr std::array<char, 8> kMagic = {'A','D','A','L','O','R','A','\0'};
constexpr uint32_t kFormatVersion = 2;
constexpr size_t   kFingerprintBytes = 64; // 64-char hex SHA-256 + NUL-pad
constexpr uint64_t kMaxCheckpointBytes = 256ull * 1024 * 1024;
constexpr uint32_t kMaxLayers = 65536;
constexpr uint32_t kMaxNameBytes = 4096;

void addSize(uint64_t& total, uint64_t count) {
    if (count > kMaxCheckpointBytes - total)
        throw std::runtime_error("AdaLoRA checkpoint: size limit exceeded");
    total += count;
}

bool finiteFloat(float value) {
    static_assert(sizeof(float) == sizeof(uint32_t), "checkpoint requires 32-bit floats");
    static_assert(std::numeric_limits<float>::is_iec559, "checkpoint requires IEEE-754 floats");
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return (bits & 0x7f800000u) != 0x7f800000u;
}

bool validFloats(const std::vector<float>& values) {
    return std::all_of(values.begin(), values.end(),
                       [](float x) { return finiteFloat(x); });
}

size_t checkedSize(uint64_t value) {
    if (value > std::numeric_limits<size_t>::max())
        throw std::runtime_error("AdaLoRA checkpoint: size is not representable");
    return static_cast<size_t>(value);
}

size_t checkedProduct(size_t a, size_t b) {
    if (a == 0 || b == 0 || a > std::numeric_limits<size_t>::max() / b)
        throw std::runtime_error("AdaLoRA checkpoint: invalid dimensions");
    return a * b;
}

// FNV-1a detects accidental corruption, not malicious tampering.
uint64_t updateChecksum(uint64_t hash, const char* data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        hash ^= static_cast<unsigned char>(data[i]);
        hash *= 1099511628211ull;
    }
    return hash;
}

class CheckpointWriter {
public:
    explicit CheckpointWriter(std::ostream& stream) : stream_(stream) {}
    void write(const char* data, size_t bytes) {
        stream_.write(data, static_cast<std::streamsize>(bytes));
        if (!stream_) throw std::runtime_error("AdaLoRA checkpoint: write error");
        hash_ = updateChecksum(hash_, data, bytes);
    }
    template <typename T> void value(T value) {
        write(reinterpret_cast<const char*>(&value), sizeof(value));
    }
    void floats(const std::vector<float>& values) {
        if (!values.empty())
            write(reinterpret_cast<const char*>(values.data()), values.size() * sizeof(float));
    }
    uint64_t hash() const { return hash_; }
private:
    std::ostream& stream_;
    uint64_t hash_ = 14695981039346656037ull;
};

class CheckpointReader {
public:
    CheckpointReader(std::istream& stream, uint64_t bytes)
        : stream_(stream), remaining_(bytes) {}

    void require(uint64_t bytes) const {
        if (bytes > remaining_)
            throw std::runtime_error("AdaLoRA checkpoint: truncated or invalid data");
    }

    void read(char* destination, size_t bytes) {
        require(bytes);
        stream_.read(destination, static_cast<std::streamsize>(bytes));
        if (!stream_) throw std::runtime_error("AdaLoRA checkpoint: truncated data");
        hash_ = updateChecksum(hash_, destination, bytes);
        remaining_ -= bytes;
    }

    template <typename T> T value() {
        T result{};
        read(reinterpret_cast<char*>(&result), sizeof(result));
        return result;
    }

    std::vector<float> floats(size_t count) {
        require(static_cast<uint64_t>(count) * sizeof(float));
        std::vector<float> result(count);
        if (count) read(reinterpret_cast<char*>(result.data()), count * sizeof(float));
        return result;
    }

    uint64_t remaining() const { return remaining_; }
    uint64_t hash() const { return hash_; }

private:
    std::istream& stream_;
    uint64_t remaining_;
    uint64_t hash_ = 14695981039346656037ull;
};

template <typename T>
T readNative(std::istream& is) {
    T val{};
    is.read(reinterpret_cast<char*>(&val), sizeof(T));
    if (!is) {
      throw std::runtime_error("AdaLoRAAdapter::loadFromFile: unexpected EOF");
    }
    return val;
}

} // anonymous namespace

// ─── saveToFile ──────────────────────────────────────────────────────────────

void AdaLoRAAdapter::saveToFile(const std::string& path,
                                const std::string& model_fingerprint) const {
    if (model_fingerprint.size() > kFingerprintBytes ||
        model_fingerprint.find('\0') != std::string::npos)
        throw std::runtime_error("AdaLoRA checkpoint: invalid fingerprint");
    if (impl_->default_rank_ == 0 || !finiteFloat(impl_->default_alpha_) ||
        impl_->default_alpha_ <= 0 || impl_->insertion_order_.size() > kMaxLayers)
        throw std::runtime_error("AdaLoRA checkpoint: invalid configuration");
    uint64_t size = 8 + 4 + kFingerprintBytes + 8 + 4 + 8 + 4 + 8;
    float total_importance = 0.0f;
    for (const auto& name : impl_->insertion_order_) {
        const auto& layer = impl_->layers_.at(name);
        if (name.empty() || name.size() > kMaxNameBytes || name.find('\0') != std::string::npos ||
            layer.active_rank == 0 || layer.active_rank > layer.max_rank ||
            !finiteFloat(layer.alpha) || layer.alpha <= 0 ||
            !finiteFloat(layer.importance) || layer.importance < 0 ||
            checkedProduct(layer.in_dim, layer.max_rank) != layer.B.size() ||
            checkedProduct(layer.out_dim, layer.max_rank) != layer.A.size() ||
            !validFloats(layer.B) || !validFloats(layer.A))
            throw std::runtime_error("AdaLoRA checkpoint: invalid layer");
        total_importance += layer.importance;
        if (!finiteFloat(total_importance))
            throw std::runtime_error("AdaLoRA checkpoint: importance sum overflow");
        addSize(size, 4 + name.size() + 4 * sizeof(uint64_t) + 2 * sizeof(float));
        if (layer.B.size() > kMaxCheckpointBytes / sizeof(float) ||
            layer.A.size() > kMaxCheckpointBytes / sizeof(float))
            throw std::runtime_error("AdaLoRA checkpoint: weights exceed size limit");
        addSize(size, layer.B.size() * sizeof(float));
        addSize(size, layer.A.size() * sizeof(float));
    }

    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs)
        throw std::runtime_error("AdaLoRAAdapter::saveToFile: cannot open '" + path + "'");
    CheckpointWriter writer(ofs);

    // Magic
    writer.write(kMagic.data(), kMagic.size());

    // Version
    writer.value<uint32_t>(kFormatVersion);

    // Fingerprint — exactly kFingerprintBytes, NUL-padded
    char fp_buf[kFingerprintBytes] = {};
    std::memcpy(fp_buf, model_fingerprint.data(), model_fingerprint.size());
    writer.write(fp_buf, kFingerprintBytes);

    writer.value<uint64_t>(impl_->default_rank_);
    writer.value<float>(impl_->default_alpha_);
    writer.value<uint64_t>(impl_->rank_budget_);
    writer.value<uint32_t>(static_cast<uint32_t>(impl_->insertion_order_.size()));

    for (const auto& name : impl_->insertion_order_) {
        const auto& layer = impl_->layers_.at(name);
        const auto name_len = static_cast<uint32_t>(name.size());
        writer.value<uint32_t>(name_len);
        writer.write(name.data(), name.size());
        writer.value<uint64_t>(layer.in_dim);
        writer.value<uint64_t>(layer.out_dim);
        writer.value<uint64_t>(layer.max_rank);
        writer.value<uint64_t>(layer.active_rank);
        writer.value<float>(layer.alpha);
        writer.value<float>(layer.importance);
        writer.floats(layer.B);
        writer.floats(layer.A);
    }
    const auto digest = writer.hash();
    ofs.write(reinterpret_cast<const char*>(&digest), sizeof(digest));
    ofs.flush();
    if (!ofs)
        throw std::runtime_error("AdaLoRAAdapter::saveToFile: write error on '" + path + "'");
    ofs.close();
    if (!ofs)
        throw std::runtime_error("AdaLoRAAdapter::saveToFile: close error on '" + path + "'");
}

/**
 * @brief ─── loadFromFile ─────────────────────────────────────────────────────────────
 * @param[in] path Input parameter.
 * @return Return value.
 * @throws std::runtime_error if an error occurs.
 * @details Validates a temporary implementation before replacing current state.
 */

std::string AdaLoRAAdapter::loadFromFile(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs)
        throw std::runtime_error("AdaLoRAAdapter::loadFromFile: cannot open '" + path + "'");
    const auto end = ifs.tellg();
    constexpr uint64_t minimum_size = 8 + 4 + kFingerprintBytes + 8 + 4 + 8 + 4 + 8;
    if (end < 0 || static_cast<uint64_t>(end) < 8 + sizeof(uint32_t) ||
        static_cast<uint64_t>(end) > kMaxCheckpointBytes)
        throw std::runtime_error("AdaLoRA checkpoint: invalid file size");
    ifs.seekg(0);
    char magic_buf[8] = {};
    ifs.read(magic_buf, sizeof(magic_buf));
    if (!ifs || std::memcmp(magic_buf, kMagic.data(), 8) != 0)
        throw std::runtime_error("AdaLoRAAdapter::loadFromFile: bad magic in '" + path + "'");
    const uint32_t version = readNative<uint32_t>(ifs);
    if (version != kFormatVersion)
        throw std::runtime_error("AdaLoRAAdapter::loadFromFile: unsupported version " +
                                 std::to_string(version) + " in '" + path + "'");
    if (static_cast<uint64_t>(end) < minimum_size)
        throw std::runtime_error("AdaLoRA checkpoint: invalid file size");

    // Hash and validate the bytes actually parsed, including the header.
    ifs.seekg(0);
    CheckpointReader reader(ifs, static_cast<uint64_t>(end) - sizeof(uint64_t));
    reader.read(magic_buf, sizeof(magic_buf));
    if (std::memcmp(magic_buf, kMagic.data(), sizeof(magic_buf)) != 0 ||
        reader.value<uint32_t>() != kFormatVersion)
        throw std::runtime_error("AdaLoRA checkpoint: invalid header");

    char fp_buf[kFingerprintBytes + 1] = {};
    reader.read(fp_buf, kFingerprintBytes);
    const auto fp_end = std::find(fp_buf, fp_buf + kFingerprintBytes, '\0');
    if (!std::all_of(fp_end, fp_buf + kFingerprintBytes, [](char c) { return c == '\0'; }))
        throw std::runtime_error("AdaLoRA checkpoint: invalid fingerprint padding");
    std::string fingerprint(fp_buf, fp_end);
    const size_t default_rank = checkedSize(reader.value<uint64_t>());
    const float default_alpha = reader.value<float>();
    const size_t rank_budget = checkedSize(reader.value<uint64_t>());
    if (default_rank == 0 || !finiteFloat(default_alpha) || default_alpha <= 0)
        throw std::runtime_error("AdaLoRA checkpoint: invalid configuration");
    auto replacement = std::make_unique<Impl>(default_rank, default_alpha, rank_budget);
    // The setter permits zero, whereas the constructor substitutes a default.
    replacement->rank_budget_ = rank_budget;
    const uint32_t layer_count = reader.value<uint32_t>();
    if (layer_count > kMaxLayers)
        throw std::runtime_error("AdaLoRA checkpoint: too many layers");
    reader.require(static_cast<uint64_t>(layer_count) * (4 + 1 + 32 + 8 + 8));
    float total_importance = 0.0f;
    for (uint32_t i = 0; i < layer_count; ++i) {
        const uint32_t name_len = reader.value<uint32_t>();
        if (name_len == 0 || name_len > kMaxNameBytes)
            throw std::runtime_error("AdaLoRA checkpoint: invalid name length");
        reader.require(static_cast<uint64_t>(name_len) + 32 + 8);
        std::string name(name_len, '\0');
        reader.read(name.data(), name_len);
        if (name.find('\0') != std::string::npos || replacement->layers_.count(name))
            throw std::runtime_error("AdaLoRA checkpoint: invalid or duplicate layer name");

        Impl::Layer layer;
        layer.layer_name = name;
        layer.in_dim = checkedSize(reader.value<uint64_t>());
        layer.out_dim = checkedSize(reader.value<uint64_t>());
        layer.max_rank = checkedSize(reader.value<uint64_t>());
        layer.active_rank = checkedSize(reader.value<uint64_t>());
        layer.alpha = reader.value<float>();
        layer.importance = reader.value<float>();
        if (layer.active_rank == 0 || layer.active_rank > layer.max_rank ||
            !finiteFloat(layer.alpha) || layer.alpha <= 0 ||
            !finiteFloat(layer.importance) || layer.importance < 0)
            throw std::runtime_error("AdaLoRA checkpoint: invalid layer metadata");
        total_importance += layer.importance;
        if (!finiteFloat(total_importance))
            throw std::runtime_error("AdaLoRA checkpoint: importance sum overflow");
        const auto b_count = checkedProduct(layer.in_dim, layer.max_rank);
        const auto a_count = checkedProduct(layer.out_dim, layer.max_rank);
        if (b_count > kMaxCheckpointBytes / sizeof(float) ||
            a_count > kMaxCheckpointBytes / sizeof(float))
            throw std::runtime_error("AdaLoRA checkpoint: invalid weight size");
        reader.require((static_cast<uint64_t>(b_count) + a_count) * sizeof(float));
        layer.B = reader.floats(b_count);
        layer.A = reader.floats(a_count);
        if (!validFloats(layer.B) || !validFloats(layer.A))
            throw std::runtime_error("AdaLoRA checkpoint: non-finite weights");
        replacement->layers_.emplace(name, std::move(layer));
        replacement->insertion_order_.push_back(std::move(name));
    }
    if (reader.remaining() != 0)
        throw std::runtime_error("AdaLoRA checkpoint: trailing data");
    if (readNative<uint64_t>(ifs) != reader.hash())
        throw std::runtime_error("AdaLoRA checkpoint: checksum mismatch");
    if (ifs.peek() != std::char_traits<char>::eof() || ifs.bad())
        throw std::runtime_error("AdaLoRA checkpoint: trailing data");
    impl_.swap(replacement);
    return fingerprint;
}

/**
 * @brief ─── isCacheValid ─────────────────────────────────────────────────────────────
 * @param[in] checkpoint_path Path to the checkpoint.
 * @param[in] current_fingerprint Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: ifs(), read(), std::memcmp(), data(), stored_fp(), empty().
 */

bool AdaLoRAAdapter::isCacheValid(const std::string& checkpoint_path,
                                  const std::string& current_fingerprint) {
    std::ifstream ifs(checkpoint_path, std::ios::binary);
    if (!ifs) {
      return false;
    }

    // Magic
    char magic_buf[8] = {};
    ifs.read(magic_buf, 8);
    if (!ifs || std::memcmp(magic_buf, kMagic.data(), 8) != 0) {
      return false;
    }

    // Version
    uint32_t version = 0;
    ifs.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (!ifs || version != kFormatVersion) {
      return false;
    }

    // Fingerprint
    char fp_buf[kFingerprintBytes + 1] = {};
    ifs.read(fp_buf, static_cast<std::streamsize>(kFingerprintBytes));
    if (!ifs) {
      return false;
    }

    const std::string stored_fp(fp_buf);
    if (stored_fp.empty() || current_fingerprint.empty()) {
      return false;
    }

    return stored_fp == current_fingerprint;
}

} // namespace training
} // namespace themis
