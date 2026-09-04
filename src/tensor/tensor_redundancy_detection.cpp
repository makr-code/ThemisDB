/**
 * @file tensor_redundancy_detection.cpp
 * @brief Redundancy detection and deduplication implementations.
 */

#include "tensor/tensor_redundancy_detection.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <unordered_map>
#include <string>

namespace themis {
namespace tensor {

// ============================================================================
// Helper functions
// ============================================================================

static float computeCosineSimilarity(
    const std::vector<float>& a,
    const std::vector<float>& b) {

    if (a.empty() || b.empty()) {
      return 0.0f;
    }

    float dot_product = 0.0f;
    float norm_a = 0.0f;
    float norm_b = 0.0f;

    size_t min_len = std::min(a.size(), b.size());
    for (size_t i = 0; i < min_len; ++i) {
        dot_product += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }

    norm_a = std::sqrt(norm_a);
    norm_b = std::sqrt(norm_b);

    if (norm_a < 1e-9f || norm_b < 1e-9f) {
      return 0.0f;
    }

    return dot_product / (norm_a * norm_b);
}

// ============================================================================
// SimilarityBasedDetector implementation
// ============================================================================

std::string SimilarityBasedDetector::name() const noexcept {
    return "SIMILARITY_BASED";
}

RedundancyMetrics SimilarityBasedDetector::detect(
    const std::vector<const BaseTensorSummary*>& summaries,
    float                                        threshold) const {

    RedundancyMetrics metrics;
    metrics.total_candidates = summaries.size();
    metrics.similarity_threshold = threshold;

    if (summaries.size() < 2) {
        metrics.redundant_count = 0;
        metrics.unique_count = summaries.size();
        metrics.redundancy_ratio = 0.0f;
        return metrics;
    }

    int redundant = 0;
    for (size_t i = 0; i < summaries.size(); ++i) {
        for (size_t j = i + 1; j < summaries.size(); ++j) {
            if (summaries[i] && summaries[j]) {
                float sim_diff = std::abs(summaries[i]->similarity_score - 
                                         summaries[j]->similarity_score);
                if (sim_diff < threshold) {
                    redundant++;
                }
            }
        }
    }

    metrics.redundant_count = redundant;
    metrics.unique_count = metrics.total_candidates - redundant;
    metrics.redundancy_ratio = metrics.total_candidates > 0 ?
        static_cast<float>(redundant) / metrics.total_candidates : 0.0f;
    metrics.explanation = "Detected " + std::to_string(redundant) + 
                         " redundant candidates by similarity";

    return metrics;
}

std::vector<std::size_t> SimilarityBasedDetector::deduplicate(
    std::vector<BaseTensorSummary>& summaries,
    float                           threshold) {

    std::vector<std::size_t> removed_indices;

    for (size_t i = 0; i < summaries.size(); ++i) {
        for (size_t j = i + 1; j < summaries.size(); ++j) {
            float sim_diff = std::abs(summaries[i].similarity_score - 
                                     summaries[j].similarity_score);
            if (sim_diff < threshold) {
                // Keep higher score, mark lower for removal
                if (summaries[i].similarity_score < summaries[j].similarity_score) {
                    removed_indices.push_back(i);
                } else {
                    removed_indices.push_back(j);
                }
            }
        }
    }

    // Remove duplicates from removed_indices and sort in reverse
    std::sort(removed_indices.begin(), removed_indices.end());
    removed_indices.erase(std::unique(removed_indices.begin(), removed_indices.end()),
                         removed_indices.end());
    std::reverse(removed_indices.begin(), removed_indices.end());

    // Remove items at marked indices
    for (size_t idx : removed_indices) {
        if (static_cast<int>(summaries.size()) > idx) {
            summaries.erase(summaries.begin() + static_cast<ptrdiff_t>(idx));
        }
    }

    return removed_indices;
}

bool SimilarityBasedDetector::areRedundant(
    const BaseTensorSummary& a,
    const BaseTensorSummary& b,
    float                    threshold) const noexcept {

    float sim_diff = std::abs(a.similarity_score - b.similarity_score);
    return sim_diff < threshold;
}

// ============================================================================
// ContentHashDetector implementation
// ============================================================================

std::string ContentHashDetector::name() const noexcept {
    return "CONTENT_HASH";
}

RedundancyMetrics ContentHashDetector::detect(
    const std::vector<const BaseTensorSummary*>& summaries,
    float                                        threshold) const {

    RedundancyMetrics metrics;
    metrics.total_candidates = summaries.size();
    metrics.similarity_threshold = threshold;

    std::unordered_map<std::string, int> hash_counts = {};

    for (const auto* summary : summaries) {
        if (summary) {
            std::string hash = hashSummary(*summary);
            hash_counts[hash]++;
        }
    }

    int redundant = 0;
    for (const auto& [hash, count] : hash_counts) {
        if (count > 1) {
            redundant += count - 1;
        }
    }

    metrics.redundant_count = redundant;
    metrics.unique_count = metrics.total_candidates - redundant;
    metrics.redundancy_ratio = metrics.total_candidates > 0 ?
        static_cast<float>(redundant) / metrics.total_candidates : 0.0f;
    metrics.explanation = "Detected " + std::to_string(redundant) + 
                         " exact/near-duplicate candidates by hash";

    return metrics;
}

std::vector<std::size_t> ContentHashDetector::deduplicate(
    std::vector<BaseTensorSummary>& summaries,
    float                           threshold) {

    std::vector<std::size_t> removed_indices;
    std::unordered_set<std::string> seen_hashes;

    for (size_t i = 0; i < summaries.size(); ++i) {
        std::string hash = hashSummary(summaries[i]);
        if (seen_hashes.count(hash) > 0) {
            removed_indices.push_back(i);
        } else {
            seen_hashes.insert(hash);
        }
    }

    // Remove in reverse order
    std::sort(removed_indices.rbegin(), removed_indices.rend());
    for (size_t idx : removed_indices) {
        if (static_cast<int>(summaries.size()) > idx) {
            summaries.erase(summaries.begin() + static_cast<ptrdiff_t>(idx));
        }
    }

    return removed_indices;
}

bool ContentHashDetector::areRedundant(
    const BaseTensorSummary& a,
    const BaseTensorSummary& b,
    float                    threshold) const noexcept {

    return hashSummary(a) == hashSummary(b);
}

std::string ContentHashDetector::hashSummary(const BaseTensorSummary& s) const noexcept {
    // Simple hash combining key fields
    // In production, use proper cryptographic hashing
    std::string combined = s.id + "|" + s.domain + "|" + s.compression_strategy;
    
    // FNV-1a hash (simple 64-bit)
    uint64_t hash = 14695981039346656037;
    for (char c : combined) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211;
    }
    
    return std::to_string(hash);
}

// ============================================================================
// EmbeddingBasedDetector implementation
// ============================================================================

std::string EmbeddingBasedDetector::name() const noexcept {
    return "EMBEDDING_BASED";
}

RedundancyMetrics EmbeddingBasedDetector::detect(
    const std::vector<const BaseTensorSummary*>& summaries,
    float                                        threshold) const {

    RedundancyMetrics metrics;
    metrics.total_candidates = summaries.size();
    metrics.similarity_threshold = threshold;

    int redundant = 0;
    for (size_t i = 0; i < summaries.size(); ++i) {
        for (size_t j = i + 1; j < summaries.size(); ++j) {
            if (summaries[i] && summaries[j]) {
                // Try to get embeddings; different summary types have different fields
                // For now, just check if they're AdapterSummary or similar
                if (!summaries[i]->domain.empty() && !summaries[j]->domain.empty() &&
                    summaries[i]->domain == summaries[j]->domain) {
                    // Could check embeddings if available
                    redundant++;
                }
            }
        }
    }

    metrics.redundant_count = redundant;
    metrics.unique_count = metrics.total_candidates - redundant;
    metrics.redundancy_ratio = metrics.total_candidates > 0 ?
        static_cast<float>(redundant) / metrics.total_candidates : 0.0f;

    return metrics;
}

std::vector<std::size_t> EmbeddingBasedDetector::deduplicate(
    std::vector<BaseTensorSummary>& summaries,
    float                           threshold) {

    std::vector<std::size_t> removed_indices;

    for (size_t i = 0; i < summaries.size(); ++i) {
        for (size_t j = i + 1; j < summaries.size(); ++j) {
            if (summaries[i].domain == summaries[j].domain) {
                removed_indices.push_back(j);
                break;  // Remove only one per iteration to avoid index issues
            }
        }
    }

    return removed_indices;
}

bool EmbeddingBasedDetector::areRedundant(
    const BaseTensorSummary& a,
    const BaseTensorSummary& b,
    float                    threshold) const noexcept {

    // Check if same domain (simplified check)
    return a.domain == b.domain;
}

float EmbeddingBasedDetector::cosineSimilarity(
    const std::vector<float>& a,
    const std::vector<float>& b) const noexcept {

    return computeCosineSimilarity(a, b);
}

// ============================================================================
// MetadataBasedDetector implementation
// ============================================================================

std::string MetadataBasedDetector::name() const noexcept {
    return "METADATA_BASED";
}

RedundancyMetrics MetadataBasedDetector::detect(
    const std::vector<const BaseTensorSummary*>& summaries,
    float                                        threshold) const {

    RedundancyMetrics metrics;
    metrics.total_candidates = summaries.size();
    metrics.similarity_threshold = threshold;

    // Check for identical metadata patterns
    int redundant = 0;
    for (size_t i = 0; i < summaries.size(); ++i) {
        for (size_t j = i + 1; j < summaries.size(); ++j) {
            if (summaries[i] && summaries[j]) {
                if (summaries[i]->domain == summaries[j]->domain &&
                    summaries[i]->tenant_id == summaries[j]->tenant_id) {
                    redundant++;
                }
            }
        }
    }

    metrics.redundant_count = redundant;
    metrics.unique_count = metrics.total_candidates - redundant;
    metrics.redundancy_ratio = metrics.total_candidates > 0 ?
        static_cast<float>(redundant) / metrics.total_candidates : 0.0f;

    return metrics;
}

std::vector<std::size_t> MetadataBasedDetector::deduplicate(
    std::vector<BaseTensorSummary>& summaries,
    float                           threshold) {

    std::vector<std::size_t> removed_indices;

    for (size_t i = 0; i < summaries.size(); ++i) {
        for (size_t j = i + 1; j < summaries.size(); ++j) {
            if (summaries[i].domain == summaries[j].domain &&
                summaries[i].tenant_id == summaries[j].tenant_id) {
                removed_indices.push_back(j);
            }
        }
    }

    std::sort(removed_indices.rbegin(), removed_indices.rend());
    for (size_t idx : removed_indices) {
        if (static_cast<int>(summaries.size()) > idx) {
            summaries.erase(summaries.begin() + static_cast<ptrdiff_t>(idx));
        }
    }

    return removed_indices;
}

bool MetadataBasedDetector::areRedundant(
    const BaseTensorSummary& a,
    const BaseTensorSummary& b,
    float                    threshold) const noexcept {

    return a.domain == b.domain && a.tenant_id == b.tenant_id;
}

// ============================================================================
// CompositeDetector implementation
// ============================================================================

void CompositeDetector::addDetector(
    std::unique_ptr<IRedundancyDetector> detector,
    float                                weight) {

    detectors_.push_back({std::move(detector), weight});
}

std::string CompositeDetector::name() const noexcept {
    return "COMPOSITE";
}

RedundancyMetrics CompositeDetector::detect(
    const std::vector<const BaseTensorSummary*>& summaries,
    float                                        threshold) const {

    RedundancyMetrics combined;
    combined.total_candidates = summaries.size();
    combined.similarity_threshold = threshold;

    if (detectors_.empty()) {
        combined.unique_count = summaries.size();
        combined.redundancy_ratio = 0.0f;
        return combined;
    }

    // Combine results from all detectors with weighted voting
    float total_redundancy = 0.0f;
    float total_weight = 0.0f;

    for (const auto& entry : detectors_) {
        auto metrics = entry.detector->detect(summaries, threshold);
        total_redundancy += metrics.redundancy_ratio * entry.weight;
        total_weight += entry.weight;
    }

    if (total_weight > 0.0f) {
        combined.redundancy_ratio = total_redundancy / total_weight;
    }

    combined.redundant_count = static_cast<std::size_t>(
        combined.redundancy_ratio * combined.total_candidates);
    combined.unique_count = combined.total_candidates - combined.redundant_count;

    return combined;
}

std::vector<std::size_t> CompositeDetector::deduplicate(
    std::vector<BaseTensorSummary>& summaries,
    float                           threshold) {

    std::vector<std::size_t> all_removed;

    for (const auto& entry : detectors_) {
        auto removed = entry.detector->deduplicate(summaries, threshold);
        all_removed.insert(all_removed.end(), removed.begin(), removed.end());
    }

    return all_removed;
}

bool CompositeDetector::areRedundant(
    const BaseTensorSummary& a,
    const BaseTensorSummary& b,
    float                    threshold) const noexcept {

    // All detectors must agree
    for (const auto& entry : detectors_) {
        if (!entry.detector->areRedundant(a, b, threshold)) {
            return false;
        }
    }

    return !detectors_.empty();
}

// ============================================================================
// RedundancyFactory implementation
// ============================================================================

std::unique_ptr<IRedundancyDetector> RedundancyFactory::create(
    const std::string& strategy_name) {

    if (strategy_name == "SIMILARITY_BASED") {
        return std::make_unique<SimilarityBasedDetector>();
    } else if (strategy_name == "CONTENT_HASH") {
        return std::make_unique<ContentHashDetector>();
    } else if (strategy_name == "EMBEDDING_BASED") {
        return std::make_unique<EmbeddingBasedDetector>();
    } else if (strategy_name == "METADATA_BASED") {
        return std::make_unique<MetadataBasedDetector>();
    }

    return nullptr;
}

std::unique_ptr<CompositeDetector> RedundancyFactory::createDefaultComposite() {
    auto composite = std::make_unique<CompositeDetector>();
    
    composite->addDetector(std::make_unique<SimilarityBasedDetector>(), 0.4f);
    composite->addDetector(std::make_unique<ContentHashDetector>(), 0.35f);
    composite->addDetector(std::make_unique<MetadataBasedDetector>(), 0.25f);
    
    return composite;
}

} // namespace tensor
} // namespace themis
