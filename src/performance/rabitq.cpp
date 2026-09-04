/**
 * @file rabitq.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "performance/rabitq.h"
#include "performance/phase2_feature_flags.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <queue>
#include <random>
#include <stdexcept>

namespace themis {
namespace performance {

// Hardware validation for RaBitQ
static bool is_rabitq_hardware_supported() {
    return Phase2FeatureFlags::instance().rabitq_hardware_supported();
}

/// Validate dimension and hardware before any allocation; returns dimension on success.
static size_t validate_rabitq_dimension([[maybe_unused]] size_t dimension) {
    if (dimension == 0) {
        throw std::runtime_error("RaBitQ: dimension must be positive");
    }
    if (dimension > (1ULL << 20)) {  // 1M dimensions max
        throw std::runtime_error("RaBitQ: dimension exceeds maximum (1M)");
    }
    if (!is_rabitq_hardware_supported()) {
        throw std::runtime_error(
            "RaBitQ: Hardware does not support SIMD operations (SSE2/AVX2/NEON) required for quantization. "
            "Use standard floating-point vectors instead."
        );
    }
    return dimension;
}

RaBitQEncoder::RaBitQEncoder(size_t dimension)
    : dimension_(validate_rabitq_dimension(dimension)),
      mean_(dimension_, 0.0f),
      scale_(dimension_, 1.0f),
      thresholds_(dimension_) {

    // Initialize default thresholds for 2-bit quantization
    for (size_t i = 0; i < dimension_; i++) {
        thresholds_[i] = {-1.0f, 0.0f, 1.0f}; // 4 bins: <-1, [-1,0), [0,1), >=1
    }
}

void RaBitQEncoder::train(const std::vector<std::vector<float>>& training_data) {
    if (training_data.empty()) {
      return;
    }
    
    // Validate training data consistency
    for (const auto& vec : training_data) {
        if (vec.size() != dimension_) {
            throw std::runtime_error(
                "RaBitQ: Training data vector dimension mismatch"
            );
        }
    }
    
    // Compute per-dimension statistics
    for (size_t d = 0; d < dimension_; d++) {
        // Compute mean
        float sum = 0.0f;
        for (const auto& vec : training_data) {
            sum += vec[d];
        }
        mean_[d] = sum / training_data.size();
        
        // Compute standard deviation
        float sq_sum = 0.0f;
        for (const auto& vec : training_data) {
            float diff = vec[d] - mean_[d];
            sq_sum += diff * diff;
        }
        float stddev = std::sqrt(sq_sum / training_data.size());
        scale_[d] = stddev > 0.0f ? stddev : 1.0f;
        
        // Set thresholds at -1σ, 0, +1σ (4 equal probability bins)
        thresholds_[d][0] = -1.0f;
        thresholds_[d][1] = 0.0f;
        thresholds_[d][2] = 1.0f;
    }
}

RaBitQVector RaBitQEncoder::encode(const std::vector<float>& vec) const {
    // Validate input dimension
    if (vec.size() != dimension_) {
        throw std::runtime_error(
            "RaBitQ: Vector dimension mismatch (expected " + std::to_string(dimension_) + 
            ", got " + std::to_string(vec.size()) + ")"
        );
    }
    
    RaBitQVector result(vec.size());
    
    for (size_t i = 0; i < vec.size(); i++) {
        uint8_t quantized = quantize_value(vec[i], i);
        result.set(i, quantized);
    }
    
    return result;
}

std::vector<float> RaBitQEncoder::decode(const RaBitQVector& quantized) const {
    std::vector<float> result(quantized.dimension());
    
    for (size_t i = 0; i < quantized.dimension(); i++) {
        result[i] = dequantize_value(quantized.get(i), i);
    }
    
    return result;
}

float RaBitQEncoder::compute_distance(const RaBitQVector& a, const RaBitQVector& b) const {
    // Approximate L2 distance using quantized values
    float dist = 0.0f;
    for (size_t i = 0; i < a.dimension(); i++) {
        float val_a = dequantize_value(a.get(i), i);
        float val_b = dequantize_value(b.get(i), i);
        float diff = val_a - val_b;
        dist += diff * diff;
    }
    return std::sqrt(dist);
}

float RaBitQEncoder::asymmetric_distance(const std::vector<float>& query, const RaBitQVector& db_vector) const {
    // Query is full precision, database vector is quantized
    float dist = 0.0f;
    for (size_t i = 0; i < query.size(); i++) {
        float db_val = dequantize_value(db_vector.get(i), i);
        float diff = query[i] - db_val;
        dist += diff * diff;
    }
    return std::sqrt(dist);
}

uint8_t RaBitQEncoder::quantize_value(float value, size_t dim) const {
    // Normalize
    float normalized = (value - mean_[dim]) / scale_[dim];
    
    // Quantize to 2 bits (4 levels)
    const auto& thresh = thresholds_[dim];
    if (normalized < thresh[0]) {
      return 0;
    }
    else if (normalized < thresh[1]) return 1;
    else if (normalized < thresh[2]) return 2;
    else return 3;
}

float RaBitQEncoder::dequantize_value(uint8_t quantized, size_t dim) const {
    // Map 2-bit value back to float (use bin centers)
    float normalized;
    const auto& thresh = thresholds_[dim];
    
    switch (quantized) {
        case 0: normalized = thresh[0] - 0.5f; break;
        case 1: normalized = (thresh[0] + thresh[1]) / 2.0f; break;
        case 2: normalized = (thresh[1] + thresh[2]) / 2.0f; break;
        case 3: normalized = thresh[2] + 0.5f; break;
        default: normalized = 0.0f;
    }
    
    // Denormalize
    return normalized * scale_[dim] + mean_[dim];
}

RaBitQIndex::RaBitQIndex(size_t dimension, size_t max_capacity)
    : dimension_(dimension) {
    encoder_ = std::make_unique<RaBitQEncoder>(dimension);
    ids_.reserve(max_capacity);
    vectors_.reserve(max_capacity);
}

void RaBitQIndex::train(const std::vector<std::vector<float>>& training_vectors) {
    encoder_->train(training_vectors);
}

void RaBitQIndex::add(uint64_t id, const std::vector<float>& vector) {
    ids_.push_back(id);
    vectors_.push_back(encoder_->encode(vector));
}

std::vector<RaBitQIndex::SearchResult> RaBitQIndex::search(const std::vector<float>& query, int k) const {
    return linear_scan(query, k);
}

std::vector<RaBitQIndex::SearchResult> RaBitQIndex::linear_scan(const std::vector<float>& query, int k) const {
    if (vectors_.empty()) {
        return {};
    }
    
    // Use min-heap to keep only top-k results (more efficient than sorting all)
    auto cmp = [](const SearchResult& a, const SearchResult& b) {
        return a.distance < b.distance;  // Max heap (largest distance on top)
    };
    std::priority_queue<SearchResult, std::vector<SearchResult>, decltype(cmp)> heap(cmp);
    
    // Compute distances to all vectors
    for (size_t i = 0; i < vectors_.size(); i++) {
        float dist = encoder_->asymmetric_distance(query, vectors_[i]);
        
        if (heap.size() < static_cast<size_t>(k)) {
            heap.push({ids_[i], dist});
        } else if (dist < heap.top().distance) {
            heap.pop();
            heap.push({ids_[i], dist});
        }
    }
    
    // Extract results and sort by distance
    std::vector<SearchResult> results;
    results.reserve(heap.size());
    while (!heap.empty()) {
        results.push_back(heap.top());
        heap.pop();
    }
    
    // Reverse to get ascending order
    std::reverse(results.begin(), results.end());
    return results;
}

RaBitQIndex::MemoryStats RaBitQIndex::get_memory_stats() const {
    MemoryStats stats;
    stats.uncompressed_bytes = vectors_.size() * dimension_ * sizeof(float);
    stats.compressed_bytes = 0;
    for (const auto& vec : vectors_) {
        stats.compressed_bytes += vec.compressed_size();
    }
    stats.compression_ratio = static_cast<double>(stats.uncompressed_bytes) / 
                             std::max(stats.compressed_bytes, size_t(1));
    return stats;
}

ProductQuantizer::ProductQuantizer(size_t dimension, size_t num_subvectors)
    : dimension_(dimension), 
      num_subvectors_(num_subvectors),
      subvector_dimension_(dimension / num_subvectors) {
    // Validate that dimension is evenly divisible by num_subvectors
    if (dimension % num_subvectors != 0) {
        throw std::invalid_argument(
            "Dimension must be evenly divisible by num_subvectors"
        );
    }
}

std::vector<std::vector<float>> ProductQuantizer::split_vector(const std::vector<float>& vec) const {
    if (vec.size() != dimension_) {
        throw std::invalid_argument("Vector dimension mismatch in ProductQuantizer::split_vector");
    }

    std::vector<std::vector<float>> subvectors(num_subvectors_);
    for (size_t i = 0; i < num_subvectors_; i++) {
        size_t start = i * subvector_dimension_;
        size_t end = start + subvector_dimension_;
        subvectors[i].assign(vec.begin() + start, vec.begin() + end);
    }
    return subvectors;
}

void ProductQuantizer::train(const std::vector<std::vector<float>>& training_data) {
    if (training_data.empty()) {
        return;
    }

    // Avoid overfitting tiny training sets by capping centroids with a
    // sample-dependent bound instead of blindly using up to n centroids.
    const size_t k_upper = std::min(static_cast<size_t>(256), training_data.size());
    const size_t suggested_k = static_cast<size_t>(
        std::max(1.0, std::sqrt(static_cast<double>(training_data.size())) * 2.0));
    const size_t adaptive_k = std::max(static_cast<size_t>(1), std::min(k_upper, suggested_k));
    const size_t k = (num_subvectors_ == 1 && k_upper >= 2) ? static_cast<size_t>(2) : adaptive_k;

    codebooks_.resize(num_subvectors_);

    for (size_t sq = 0; sq < num_subvectors_; ++sq) {
        const size_t start_dim = sq * subvector_dimension_;

        // --- Extract subvectors for this subquantizer ---
        std::vector<std::vector<float>> subvec_data;
        subvec_data.reserve(training_data.size());
        for (const auto& vec : training_data) {
            subvec_data.emplace_back(vec.begin() + start_dim,
                                     vec.begin() + start_dim + subvector_dimension_);
        }

        const size_t n = subvec_data.size();

        // --- k-means++ initialisation ---
        std::mt19937 rng(static_cast<uint32_t>(sq * 1337u + 42u));
        std::uniform_int_distribution<uint64_t> uniform(0, n - 1);

        std::vector<std::vector<float>> centroids;
        centroids.reserve(k);
        centroids.push_back(subvec_data[static_cast<size_t>(uniform(rng))]);

        for (size_t ci = 1; ci < k; ++ci) {
            // For each sample compute D^2 distance to the nearest existing centroid.
            std::vector<float> d2(n);
            for (size_t i = 0; i < n; ++i) {
                float min_d2 = std::numeric_limits<float>::max();
                for (const auto& c : centroids) {
                    float d = 0.0f;
                    for (size_t dim = 0; dim < subvector_dimension_; ++dim) {
                        float diff = subvec_data[i][dim] - c[dim];
                        d += diff * diff;
                    }
                    min_d2 = std::min(min_d2, d);
                }
                d2[i] = min_d2;
            }
            std::discrete_distribution<uint64_t> weighted(d2.begin(), d2.end());
            centroids.push_back(subvec_data[static_cast<size_t>(weighted(rng))]);
        }

        // --- k-means Lloyd iterations (max 25) ---
        std::vector<size_t> assignments(n, 0);
        constexpr int MAX_ITER = 25;

        for (int iter = 0; iter < MAX_ITER; ++iter) {
            // Assignment step
            for (size_t i = 0; i < n; ++i) {
                float min_dist = std::numeric_limits<float>::max();
                size_t best = 0;
                for (size_t ci = 0; ci < k; ++ci) {
                    float dist = 0.0f;
                    for (size_t dim = 0; dim < subvector_dimension_; ++dim) {
                        float diff = subvec_data[i][dim] - centroids[ci][dim];
                        dist += diff * diff;
                    }
                    if (dist < min_dist) {
                        min_dist = dist;
                        best = ci;
                    }
                }
                assignments[i] = best;
            }

            // Update step: compute new centroid means
            std::vector<std::vector<float>> new_centroids(
                k, std::vector<float>(subvector_dimension_, 0.0f));
            std::vector<size_t> counts(k, 0);

            for (size_t i = 0; i < n; ++i) {
                size_t cluster = assignments[i];
                ++counts[cluster];
                for (size_t dim = 0; dim < subvector_dimension_; ++dim) {
                    new_centroids[cluster][dim] += subvec_data[i][dim];
                }
            }

            float max_shift = 0.0f;
            for (size_t ci = 0; ci < k; ++ci) {
                if (counts[ci] > 0) {
                    float shift = 0.0f;
                    for (size_t dim = 0; dim < subvector_dimension_; ++dim) {
                        new_centroids[ci][dim] /= static_cast<float>(counts[ci]);
                        float d = new_centroids[ci][dim] - centroids[ci][dim];
                        shift += d * d;
                    }
                    max_shift = std::max(max_shift, shift);
                } else {
                    // Empty cluster: reinitialize to a random sample
                    new_centroids[ci] = subvec_data[static_cast<size_t>(uniform(rng))];
                }
            }

            centroids = std::move(new_centroids);

            // Convergence: centroid shift < 1e-6 (sum of squared component deltas)
            if (max_shift < 1e-6f) {
                break;
            }
        }

        codebooks_[sq] = std::move(centroids);
    }
}

std::vector<uint8_t> ProductQuantizer::encode(const std::vector<float>& vec) const {
    if (vec.size() != dimension_) {
        throw std::invalid_argument("Vector dimension mismatch in ProductQuantizer::encode");
    }

    // Preserve deterministic no-op behavior before train() populated codebooks.
    if (codebooks_.size() != num_subvectors_) {
        return std::vector<uint8_t>(num_subvectors_, 0);
    }

    auto subvectors = split_vector(vec);
    std::vector<uint8_t> codes(num_subvectors_);

    for (size_t sq = 0; sq < num_subvectors_; ++sq) {
        const auto& subvec = subvectors[sq];
        const auto& codebook = codebooks_[sq];

        if (codebook.empty()) {
            codes[sq] = 0;
            continue;
        }

        float min_dist = std::numeric_limits<float>::max();
        uint8_t best = 0;

        for (size_t ci = 0; ci < codebook.size(); ++ci) {
            float dist = 0.0f;
            for (size_t dim = 0; dim < subvector_dimension_; ++dim) {
                float diff = subvec[dim] - codebook[ci][dim];
                dist += diff * diff;
            }
            if (dist < min_dist) {
                min_dist = dist;
                best = static_cast<uint8_t>(ci);
            }
        }

        codes[sq] = best;
    }

    return codes;
}

} // namespace performance
} // namespace themis
