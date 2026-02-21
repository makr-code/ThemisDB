/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rabitq.cpp                                         ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     252                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "performance/rabitq.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <queue>
#include <stdexcept>

namespace themis {
namespace performance {

RaBitQEncoder::RaBitQEncoder(size_t dimension) 
    : dimension_(dimension), 
      mean_(dimension, 0.0f),
      scale_(dimension, 1.0f),
      thresholds_(dimension) {
    // Initialize default thresholds for 2-bit quantization
    for (size_t i = 0; i < dimension; i++) {
        thresholds_[i] = {-1.0f, 0.0f, 1.0f}; // 4 bins: <-1, [-1,0), [0,1), >=1
    }
}

void RaBitQEncoder::train(const std::vector<std::vector<float>>& training_data) {
    if (training_data.empty()) return;
    
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
    if (normalized < thresh[0]) return 0;
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
    std::vector<std::vector<float>> subvectors(num_subvectors_);
    for (size_t i = 0; i < num_subvectors_; i++) {
        size_t start = i * subvector_dimension_;
        size_t end = start + subvector_dimension_;
        subvectors[i].assign(vec.begin() + start, vec.begin() + end);
    }
    return subvectors;
}

void ProductQuantizer::train(const std::vector<std::vector<float>>& training_data) {
    // Simplified: would do k-means clustering per subvector
    codebooks_.resize(num_subvectors_);
}

std::vector<uint8_t> ProductQuantizer::encode(const std::vector<float>& vec) const {
    auto subvectors = split_vector(vec);
    std::vector<uint8_t> codes(num_subvectors_);
    // Would assign nearest centroid index per subvector
    return codes;
}

} // namespace performance
} // namespace themis
