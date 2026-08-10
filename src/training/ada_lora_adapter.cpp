/**
 * @file ada_lora_adapter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=6, H=14, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "training/ada_lora_adapter.h"

#include <algorithm>
#include <cmath>
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
    std::mt19937 gen(seed);
    std::uniform_real_distribution<float> dist(-bound, bound);
    std::vector<float> w(rows * cols);
    for (auto& v : w) v = dist(gen);
    return w;
}

std::vector<float> zeros(size_t count) {
    return std::vector<float>(count, 0.0f);
}

uint32_t stableSeedFromName(const std::string& name) {
    uint32_t h = 2166136261u;
    for (unsigned char c : name) {
        h ^= static_cast<uint32_t>(c);
        h *= 16777619u;
    }
    return h;
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

    explicit Impl(size_t default_rank, float default_alpha, size_t rank_budget)
        : default_rank_(default_rank > 0 ? default_rank : 4)
        , default_alpha_(default_alpha > 0.0f ? default_alpha : 8.0f)
        , rank_budget_(rank_budget > 0 ? rank_budget : 64)
    {}

    // -------------------------------------------------------------------------
    // Layer management
    // -------------------------------------------------------------------------

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
        // deterministic FNV-1a seed from layer name for reproducible init
        uint32_t seed = stableSeedFromName(name);
        lay.B = kaimingUniform(in_dim, r, seed);
        lay.A = zeros(r * out_dim);

        layers_[name]       = std::move(lay);
        insertion_order_.push_back(name);
    }

    bool removeLayer(const std::string& name) {
        auto it = layers_.find(name);
        if (it == layers_.end()) return false;
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

    // Importance ≈ mean_i( ||B[:,i]||_F^2 * ||A[i,:]||_F^2 ) for i in active_rank.
    // This is a rank-component outer-product approximation to the nuclear-norm
    // importance used in the original AdaLoRA paper.
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

    void updateAllImportances() {
        for (auto& [name, _] : layers_) {
            updateImportance(name);
        }
    }

    // -------------------------------------------------------------------------
    // Rank reallocation
    // -------------------------------------------------------------------------

    ReallocResult reallocateRanks(size_t total_budget) {
        if (total_budget == 0)
            throw std::invalid_argument("total_budget must be > 0");
        if (layers_.empty()) return {};
        if (total_budget < layers_.size()) {
            throw std::invalid_argument(
                "total_budget must be >= number of layers to keep minimum rank 1 per layer");
        }

        // Compute total importance
        float total_importance = 0.0f;
        for (const auto& [_, lay] : layers_)
            total_importance += lay.importance;

        ReallocResult result;

        if (total_importance <= 0.0f) {
            // No importance data yet; deterministic floor+remainder distribution
            // in insertion order while respecting [1, max_rank] per layer bounds.
            const size_t n = insertion_order_.size();
            std::vector<size_t> allocs(n, 1);
            size_t remaining = total_budget - n;
            while (remaining > 0) {
                bool assigned = false;
                for (size_t i = 0; i < n && remaining > 0; ++i) {
                    auto& lay = layers_.at(insertion_order_[i]);
                    if (allocs[i] < lay.max_rank) {
                        ++allocs[i];
                        --remaining;
                        assigned = true;
                    }
                }
                if (!assigned) {
                    break; // all layers reached max_rank
                }
            }

            for (size_t i = 0; i < n; ++i) {
                auto& lay = layers_.at(insertion_order_[i]);
                const size_t new_rank = allocs[i];
                if (new_rank < lay.active_rank) ++result.layers_pruned;
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
            if (new_rank < lay.active_rank) ++result.layers_pruned;
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
        std::vector<AdaLoRALayerStats> stats;
        stats.reserve(insertion_order_.size());
        for (const auto& name : insertion_order_) {
            const Layer& lay = layers_.at(name);
            AdaLoRALayerStats s;
            s.layer_name   = lay.layer_name;
            s.max_rank     = lay.max_rank;
            s.active_rank  = lay.active_rank;
            s.importance   = lay.importance;
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

    size_t rankBudget() const { return rank_budget_; }
    void setRankBudget(size_t b) { rank_budget_ = b; }

private:
    size_t default_rank_;
    float  default_alpha_;
    size_t rank_budget_;

    std::unordered_map<std::string, Layer> layers_;
    std::vector<std::string> insertion_order_;

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

void AdaLoRAAdapter::addLayer(const std::string& layer_name,
                              size_t in_dim, size_t out_dim,
                              size_t max_rank, float alpha) {
    impl_->addLayer(layer_name, in_dim, out_dim, max_rank, alpha);
}

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

void AdaLoRAAdapter::updateImportance(const std::string& layer_name) {
    impl_->updateImportance(layer_name);
}

void AdaLoRAAdapter::updateAllImportances() {
    impl_->updateAllImportances();
}

ReallocResult AdaLoRAAdapter::reallocateRanks(size_t total_budget) {
    return impl_->reallocateRanks(total_budget);
}

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

size_t AdaLoRAAdapter::rankBudget() const {
    return impl_->rankBudget();
}

void AdaLoRAAdapter::setRankBudget(size_t budget) {
    impl_->setRankBudget(budget);
}

} // namespace training
} // namespace themis
