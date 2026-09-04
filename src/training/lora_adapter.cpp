/**
 * @file lora_adapter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 96/100
 * @note Gap Summary: total=6; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=2, Debt=0, C=7, H=7, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "training/lora_adapter.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace training {

// ============================================================================
// Internal helpers
// ============================================================================
namespace detail {

/**
 * @brief Kaiming uniform initialisation for a weight matrix.
 *
 * Uses the fan-in heuristic:  limit = sqrt(6 / fan_in)
 * Values are drawn uniformly from [-limit, +limit].
 *
 * @param size  Total number of elements
 * @param fan_in  Input feature count (used to compute the uniform bound)
 * @param seed  PRNG seed (kept deterministic per-layer for reproducibility)
 */
static std::vector<float> kaimingUniform(size_t size, size_t fan_in, uint32_t seed) {
    std::vector<float> w(size);
    if (size == 0) {
      return w;
    }
    std::mt19937 gen(seed);
    const float limit = std::sqrt(6.0f / static_cast<float>(std::max<size_t>(1, fan_in)));
    std::uniform_real_distribution<float> dist(-limit, limit);
    for (auto& v : w) {
      v = dist(gen);
    }
    return w;
}

/**
 * @brief Compute a reproducible seed from a layer name.
 *
 * Uses a polynomial hash (FNV-1a-inspired) to derive a uint32_t seed
 * from the layer name string so that each layer gets distinct initial
 * B weights.
 */
static uint32_t seedFromName(const std::string& name) {
    uint32_t h = 2166136261u;  // FNV-1a 32-bit offset basis
    for (unsigned char c : name) {
        h ^= static_cast<uint32_t>(c);
        h *= 16777619u;         // FNV prime
    }
    return h;
}

/**
 * @brief Row-major matrix multiplication  C = A × B.
 *
 * A is (M × K), B is (K × N), C is (M × N).
 * All matrices are passed as flat row-major vectors.
 */
static std::vector<float> matmul(const std::vector<float>& A, size_t M, size_t K,
                                  const std::vector<float>& B, size_t N) {
    assert(A.size() == M * K);
    assert(B.size() == K * N);
    std::vector<float> C(M * N, 0.0f);
    for (size_t m = 0; m < M; ++m) {
        for (size_t k = 0; k < K; ++k) {
            const float a = A[m * K + k];
            if (a == 0.0f) continue;  // skip zero multiplication (common at init)
            for (size_t n = 0; n < N; ++n) {
                C[m * N + n] += a * B[k * N + n];
            }
        }
    }
    return C;
}

} // namespace detail

// ============================================================================
// Pimpl
// ============================================================================

/** @brief Pimpl. */
class LoRAAdapter::Impl {
public:
    explicit Impl(size_t default_rank, float default_alpha)
        : default_rank_(default_rank)
        , default_alpha_(default_alpha) {
        if (default_rank == 0)
            throw std::invalid_argument("LoRAAdapter: default_rank must be > 0");
        if (default_alpha <= 0.0f)
            throw std::invalid_argument("LoRAAdapter: default_alpha must be > 0");
    }

    // -------------------------------------------------------------------------
    // Layer management
    // -------------------------------------------------------------------------

    void addLayer(const std::string& layer_name,
                  size_t in_dim, size_t out_dim,
                  size_t rank, float alpha) {
        if (layer_name.empty())
            throw std::invalid_argument("LoRAAdapter::addLayer: layer_name must not be empty");
        if (in_dim == 0 || out_dim == 0)
            throw std::invalid_argument("LoRAAdapter::addLayer: in_dim and out_dim must be > 0");
        if (layers_.count(layer_name))
            throw std::invalid_argument("LoRAAdapter::addLayer: layer '" + layer_name + "' already exists");

        const size_t r = (rank  > 0)    ? rank  : default_rank_;
        const float  a = (alpha > 0.0f) ? alpha : default_alpha_;

        if (r > in_dim || r > out_dim) {
            // Clamp rank to min(in_dim, out_dim) to guarantee valid dimensions
            // while still providing a functional adapter.
            throw std::invalid_argument(
                "LoRAAdapter::addLayer: rank (" + std::to_string(r) +
                ") must be <= min(in_dim, out_dim) = " +
                std::to_string(std::min(in_dim, out_dim)));
        }

        LoRAWeightEntry entry;
        entry.layer_name = layer_name;
        entry.in_dim     = in_dim;
        entry.out_dim    = out_dim;
        entry.rank       = r;
        entry.alpha      = a;

        // B: Kaiming-uniform (in_dim × rank) — non-zero, seed from name
        entry.B = detail::kaimingUniform(in_dim * r, in_dim, detail::seedFromName(layer_name));
        // A: zero-initialised (rank × out_dim) — ensures initial output = 0
        entry.A.assign(r * out_dim, 0.0f);

        layers_.emplace(layer_name, std::move(entry));
    }

    bool removeLayer(const std::string& layer_name) {
        return layers_.erase(layer_name) > 0;
    }

    bool hasLayer(const std::string& layer_name) const {
        return layers_.count(layer_name) > 0;
    }

    std::vector<std::string> layerNames() const {
        std::vector<std::string> names = {};

        names.reserve(layers_.size());
        for (const auto& kv : layers_) {
          names.push_back(kv.first);
        }
        return names;
    }

    size_t layerCount() const { return layers_.size(); }

    size_t totalParameterCount() const {
        size_t total = 0;
        for (const auto& kv : layers_) {
            const auto& e = kv.second;
            total += e.B.size() + e.A.size();  // in_dim*rank + rank*out_dim
        }
        return total;
    }

    // -------------------------------------------------------------------------
    // Weight access
    // -------------------------------------------------------------------------

    const LoRAWeightEntry& getWeights(const std::string& layer_name) const {
        auto it = layers_.find(layer_name);
        if (it == layers_.end())
            throw std::out_of_range("LoRAAdapter::getWeights: unknown layer '" + layer_name + "'");
        return it->second;
    }

    void setWeights(const std::string& layer_name,
                    const std::vector<float>& B,
                    const std::vector<float>& A) {
        auto it = layers_.find(layer_name);
        if (it == layers_.end())
            throw std::out_of_range("LoRAAdapter::setWeights: unknown layer '" + layer_name + "'");

        LoRAWeightEntry& e = it->second;
        const size_t expected_B = e.in_dim  * e.rank;
        const size_t expected_A = e.rank    * e.out_dim;

        if (static_cast<int>(B.size()) != expected_B) {
            std::ostringstream oss = {};
            oss << "LoRAAdapter::setWeights: B size mismatch for layer '" << layer_name
                << "' (expected " << expected_B << ", got " << B.size() << ")";
            throw std::invalid_argument(oss.str());
        }
        if (static_cast<int>(A.size()) != expected_A) {
            std::ostringstream oss = {};
            oss << "LoRAAdapter::setWeights: A size mismatch for layer '" << layer_name
                << "' (expected " << expected_A << ", got " << A.size() << ")";
            throw std::invalid_argument(oss.str());
        }

        e.B = B;
        e.A = A;
    }

    // -------------------------------------------------------------------------
    // Weight update
    // -------------------------------------------------------------------------

    WeightUpdateResult applyUpdate(const std::string& layer_name,
                                   const std::vector<float>& delta_B,
                                   const std::vector<float>& delta_A) {
        auto it = layers_.find(layer_name);
        if (it == layers_.end())
            throw std::out_of_range("LoRAAdapter::applyUpdate: unknown layer '" + layer_name + "'");

        LoRAWeightEntry& e = it->second;
        if (static_cast<int>(delta_B.size()) != e.B.size()) {
            std::ostringstream oss = {};
            oss << "LoRAAdapter::applyUpdate: delta_B size mismatch for layer '" << layer_name
                << "' (expected " << e.B.size() << ", got " << delta_B.size() << ")";
            throw std::invalid_argument(oss.str());
        }
        if (static_cast<int>(delta_A.size()) != e.A.size()) {
            std::ostringstream oss = {};
            oss << "LoRAAdapter::applyUpdate: delta_A size mismatch for layer '" << layer_name
                << "' (expected " << e.A.size() << ", got " << delta_A.size() << ")";
            throw std::invalid_argument(oss.str());
        }

        // B_new = B + delta_B
        for (size_t i = 0; i < e.B.size(); ++i) {
          e.B[i] += delta_B[i];
        }
        // A_new = A + delta_A
        for (size_t i = 0; i < e.A.size(); ++i) {
          e.A[i] += delta_A[i];
        }

        WeightUpdateResult result;
        result.success        = true;
        result.layers_updated = 1;
        return result;
    }

    WeightUpdateResult applyBatchUpdate(const WeightUpdateBatch& batch) {
        if (static_cast<int>(batch.layer_names.size()) != batch.delta_B.size() ||
            batch.layer_names.size() != batch.delta_A.size()) {
            throw std::invalid_argument(
                "LoRAAdapter::applyBatchUpdate: batch vectors must have the same length");
        }

        WeightUpdateResult result;
        result.success = true;

        for (size_t i = 0; i < batch.layer_names.size(); ++i) {
            const std::string& name = batch.layer_names[i];
            auto it = layers_.find(name);
            if (it == layers_.end()) {
                // Unknown layer – skip silently, count it
                ++result.layers_skipped;
                continue;
            }

            LoRAWeightEntry& e = it->second;

            // Size validation per entry – on mismatch skip and count as skipped
            if (batch.delta_B[i].size() != e.B.size() ||
                batch.delta_A[i].size() != e.A.size()) {
                ++result.layers_skipped;
                if (result.error_message.empty()) {
                    result.error_message =
                        "size mismatch for layer '" + name + "' (and possibly others)";
                }
                continue;
            }

            for (size_t j = 0; j < e.B.size(); ++j) {
              e.B[j] += batch.delta_B[i][j];
            }
            for (size_t j = 0; j < e.A.size(); ++j) {
              e.A[j] += batch.delta_A[i][j];
            }
            ++result.layers_updated;
        }

        return result;
    }

    // -------------------------------------------------------------------------
    // Forward pass  (real numeric computation, no simulation)
    // -------------------------------------------------------------------------

    std::vector<float> forward(const std::string& layer_name,
                               const std::vector<float>& input,
                               size_t batch_size) const {
        auto it = layers_.find(layer_name);
        if (it == layers_.end())
            throw std::out_of_range("LoRAAdapter::forward: unknown layer '" + layer_name + "'");

        const LoRAWeightEntry& e = it->second;

        if (static_cast<int>(input.size()) != batch_size * e.in_dim) {
            std::ostringstream oss = {};
            oss << "LoRAAdapter::forward: input size mismatch for layer '" << layer_name
                << "' (expected " << (batch_size * e.in_dim)
                << " = batch_size(" << batch_size << ") × in_dim(" << e.in_dim
                << "), got " << input.size() << ")";
            throw std::invalid_argument(oss.str());
        }

        // Step 1: hidden = input @ B  →  (batch_size × in_dim) @ (in_dim × rank)
        //                                = (batch_size × rank)
        std::vector<float> hidden = detail::matmul(input,  batch_size, e.in_dim,
                                                    e.B,    e.rank);

        // Step 2: output = hidden @ A  →  (batch_size × rank) @ (rank × out_dim)
        //                                 = (batch_size × out_dim)
        std::vector<float> output = detail::matmul(hidden, batch_size, e.rank,
                                                    e.A,    e.out_dim);

        // Step 3: scale by alpha / rank
        const float scaling = e.alpha / static_cast<float>(e.rank);
        for (auto& v : output) {
          v *= scaling;
        }

        return output;
    }

    // -------------------------------------------------------------------------
    // Serialisation
    // -------------------------------------------------------------------------

    std::vector<LoRAWeightEntry> exportWeights() const {
        std::vector<LoRAWeightEntry> entries = {};

        entries.reserve(layers_.size());
        for (const auto& kv : layers_) {
          entries.push_back(kv.second);
        }
        return entries;
    }

    void importWeights(const std::vector<LoRAWeightEntry>& entries) {
        for (const LoRAWeightEntry& e : entries) {
            if (e.in_dim == 0 || e.out_dim == 0 || e.rank == 0)
                throw std::invalid_argument(
                    "LoRAAdapter::importWeights: entry '" + e.layer_name +
                    "' has zero dimension or rank");

            const size_t expected_B = e.in_dim  * e.rank;
            const size_t expected_A = e.rank    * e.out_dim;

            if (static_cast<int>(e.B.size()) != expected_B || e.A.size() != expected_A) {
                std::ostringstream oss = {};
                oss << "LoRAAdapter::importWeights: size mismatch for entry '" << e.layer_name
                    << "': B expected " << expected_B << " (got " << e.B.size()
                    << "), A expected " << expected_A << " (got " << e.A.size() << ")";
                throw std::invalid_argument(oss.str());
            }

            layers_[e.layer_name] = e;
        }
    }

private:
    size_t default_rank_;
    float  default_alpha_;
    std::unordered_map<std::string, LoRAWeightEntry> layers_;
};

// ============================================================================
// LoRAAdapter public API delegation
// ============================================================================

LoRAAdapter::LoRAAdapter(size_t default_rank, float default_alpha)
    : impl_(std::make_unique<Impl>(default_rank, default_alpha)) {}

LoRAAdapter::~LoRAAdapter() = default;

LoRAAdapter::LoRAAdapter(LoRAAdapter&&) noexcept = default;

LoRAAdapter& LoRAAdapter::operator=(LoRAAdapter&&) noexcept = default;

void LoRAAdapter::addLayer(const std::string& layer_name,
                            size_t in_dim, size_t out_dim,
                            size_t rank, float alpha) {
    impl_->addLayer(layer_name, in_dim, out_dim, rank, alpha);
}

bool LoRAAdapter::removeLayer(const std::string& layer_name) {
    return impl_->removeLayer(layer_name);
}

bool LoRAAdapter::hasLayer(const std::string& layer_name) const {
    return impl_->hasLayer(layer_name);
}

std::vector<std::string> LoRAAdapter::layerNames() const {
    return impl_->layerNames();
}

size_t LoRAAdapter::layerCount() const {
    return impl_->layerCount();
}

size_t LoRAAdapter::totalParameterCount() const {
    return impl_->totalParameterCount();
}

const LoRAWeightEntry& LoRAAdapter::getWeights(const std::string& layer_name) const {
    return impl_->getWeights(layer_name);
}

void LoRAAdapter::setWeights(const std::string& layer_name,
                              const std::vector<float>& B,
                              const std::vector<float>& A) {
    impl_->setWeights(layer_name, B, A);
}

WeightUpdateResult LoRAAdapter::applyUpdate(const std::string& layer_name,
                                             const std::vector<float>& delta_B,
                                             const std::vector<float>& delta_A) {
    return impl_->applyUpdate(layer_name, delta_B, delta_A);
}

WeightUpdateResult LoRAAdapter::applyBatchUpdate(const WeightUpdateBatch& batch) {
    return impl_->applyBatchUpdate(batch);
}

std::vector<float> LoRAAdapter::forward(const std::string& layer_name,
                                         const std::vector<float>& input,
                                         size_t batch_size) const {
    return impl_->forward(layer_name, input, batch_size);
}

std::vector<LoRAWeightEntry> LoRAAdapter::exportWeights() const {
    return impl_->exportWeights();
}

void LoRAAdapter::importWeights(const std::vector<LoRAWeightEntry>& entries) {
    impl_->importWeights(entries);
}

} // namespace training
} // namespace themis
