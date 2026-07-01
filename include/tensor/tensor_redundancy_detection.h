/**
 * @file tensor_redundancy_detection.h
 * @brief Redundancy detection and deduplication abstractions.
 * 
 * Defines interfaces and algorithms for detecting and removing duplicate or
 * redundant candidates in tensor-layer processing.
 */

#pragma once

#include "tensor/tensor_summary_types.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace themis {
namespace tensor {

// ============================================================================
// RedundancyMetrics — metrics for redundancy detection
// ============================================================================

/**
 * @brief Metrics about detected redundancy in a candidate set.
 */
struct RedundancyMetrics {
    /// Total candidates in input.
    std::size_t total_candidates = 0;

    /// Redundant/duplicate candidates detected.
    std::size_t redundant_count = 0;

    /// Unique candidates after deduplication.
    std::size_t unique_count = 0;

    /// Redundancy ratio: redundant_count / total_candidates.
    float redundancy_ratio = 0.0f;

    /// Similarity threshold used for detection.
    float similarity_threshold = 0.0f;

    /// Human-readable explanation of detected redundancies.
    std::string explanation;
};

// ============================================================================
// IRedundancyDetector — interface for redundancy detection
// ============================================================================

/**
 * @brief Abstract interface for redundancy detection strategies.
 * 
 * Implementations identify and remove duplicate candidates based on
 * various similarity or hash-based criteria.
 */
class IRedundancyDetector {
public:
    virtual ~IRedundancyDetector() = default;

    /**
     * @brief Get the name of this detector strategy.
     * @return Human-readable strategy name.
     */
    [[nodiscard]] virtual std::string name() const noexcept = 0;

    /**
     * @brief Detect redundant candidates in a summary list.
     * 
     * @param summaries    Vector of tensor summaries to analyze.
     * @param threshold    Similarity threshold for redundancy (0.0-1.0).
     * @return Redundancy metrics and indices of redundant items.
     */
    [[nodiscard]] virtual RedundancyMetrics detect(
        const std::vector<const BaseTensorSummary*>& summaries,
        float                                        threshold) const = 0;

    /**
     * @brief Remove redundant candidates, keeping the highest-ranked one.
     * 
     * @param summaries    Vector of summaries (modified in-place).
     * @param threshold    Similarity threshold for considering items redundant.
     * @return Indices of removed redundant items.
     */
    virtual std::vector<std::size_t> deduplicate(
        std::vector<BaseTensorSummary>& summaries,
        float                           threshold) = 0;

    /**
     * @brief Check if two summaries are redundant.
     * 
     * @param a           First summary.
     * @param b           Second summary.
     * @param threshold   Similarity threshold.
     * @return true if a and b are considered redundant.
     */
    [[nodiscard]] virtual bool areRedundant(
        const BaseTensorSummary& a,
        const BaseTensorSummary& b,
        float                    threshold) const noexcept = 0;
};

// ============================================================================
// SimilarityBasedDetector — detect redundancy using similarity scores
// ============================================================================

/**
 * @brief Redundancy detection based on summary similarity scores.
 * 
 * Considers candidates redundant if their similarity scores are too close,
 * indicating they represent the same or very similar information.
 */
class SimilarityBasedDetector : public IRedundancyDetector {
public:
    std::string name() const noexcept override;

    RedundancyMetrics detect(
        const std::vector<const BaseTensorSummary*>& summaries,
        float                                        threshold) const override;

    std::vector<std::size_t> deduplicate(
        std::vector<BaseTensorSummary>& summaries,
        float                           threshold) override;

    bool areRedundant(
        const BaseTensorSummary& a,
        const BaseTensorSummary& b,
        float                    threshold) const noexcept override;
};

// ============================================================================
// ContentHashDetector — detect redundancy using content hashing
// ============================================================================

/**
 * @brief Redundancy detection based on content hashing.
 * 
 * Computes cryptographic or LSH hashes of candidate content and
 * identifies exact or near-duplicate items.
 */
class ContentHashDetector : public IRedundancyDetector {
public:
    /// Hash function to use (e.g., "SHA256", "XXHASH64", "BLAKE3").
    std::string hash_function = "XXHASH64";

    /// Whether to use approximate hashing for near-duplicates.
    bool approximate_matching = false;

    std::string name() const noexcept override;

    RedundancyMetrics detect(
        const std::vector<const BaseTensorSummary*>& summaries,
        float                                        threshold) const override;

    std::vector<std::size_t> deduplicate(
        std::vector<BaseTensorSummary>& summaries,
        float                           threshold) override;

    bool areRedundant(
        const BaseTensorSummary& a,
        const BaseTensorSummary& b,
        float                    threshold) const noexcept override;

private:
    /// Compute hash of a summary's content.
    [[nodiscard]] std::string hashSummary(const BaseTensorSummary& s) const noexcept;
};

// ============================================================================
// EmbeddingBasedDetector — detect redundancy using embedding similarity
// ============================================================================

/**
 * @brief Redundancy detection based on embedding similarity.
 * 
 * Uses embedding vectors (e.g., from adapters or chunks) to compute
 * cosine similarity and detect near-duplicate content.
 */
class EmbeddingBasedDetector : public IRedundancyDetector {
public:
    std::string name() const noexcept override;

    RedundancyMetrics detect(
        const std::vector<const BaseTensorSummary*>& summaries,
        float                                        threshold) const override;

    std::vector<std::size_t> deduplicate(
        std::vector<BaseTensorSummary>& summaries,
        float                           threshold) override;

    bool areRedundant(
        const BaseTensorSummary& a,
        const BaseTensorSummary& b,
        float                    threshold) const noexcept override;

private:
    /// Compute cosine similarity between two embedding vectors.
    [[nodiscard]] float cosineSimilarity(
        const std::vector<float>& a,
        const std::vector<float>& b) const noexcept;
};

// ============================================================================
// MetadataBasedDetector — detect redundancy using metadata patterns
// ============================================================================

/**
 * @brief Redundancy detection based on metadata patterns.
 * 
 * Identifies redundant candidates by comparing metadata fields
 * (e.g., source, domain, entity type) and structural patterns.
 */
class MetadataBasedDetector : public IRedundancyDetector {
public:
    std::string name() const noexcept override;

    RedundancyMetrics detect(
        const std::vector<const BaseTensorSummary*>& summaries,
        float                                        threshold) const override;

    std::vector<std::size_t> deduplicate(
        std::vector<BaseTensorSummary>& summaries,
        float                           threshold) override;

    bool areRedundant(
        const BaseTensorSummary& a,
        const BaseTensorSummary& b,
        float                    threshold) const noexcept override;
};

// ============================================================================
// CompositeDetector — combines multiple detection strategies
// ============================================================================

/**
 * @brief Composite redundancy detector using multiple strategies.
 * 
 * Combines results from multiple detection strategies with configurable
 * voting or weighting to improve redundancy detection accuracy.
 */
class CompositeDetector : public IRedundancyDetector {
public:
    /**
     * @brief Add a detection strategy to the composite detector.
     * 
     * @param detector Strategy implementation to add.
     * @param weight   Weight for this strategy in composite voting (0.0-1.0).
     */
    void addDetector(
        std::unique_ptr<IRedundancyDetector> detector,
        float                                weight = 1.0f);

    std::string name() const noexcept override;

    RedundancyMetrics detect(
        const std::vector<const BaseTensorSummary*>& summaries,
        float                                        threshold) const override;

    std::vector<std::size_t> deduplicate(
        std::vector<BaseTensorSummary>& summaries,
        float                           threshold) override;

    bool areRedundant(
        const BaseTensorSummary& a,
        const BaseTensorSummary& b,
        float                    threshold) const noexcept override;

private:
    struct DetectorEntry {
        std::unique_ptr<IRedundancyDetector> detector;
        float weight = 1.0f;
    };

    std::vector<DetectorEntry> detectors_;
};

// ============================================================================
// RedundancyFactory — factory for creating detectors
// ============================================================================

/**
 * @brief Factory for creating redundancy detector instances.
 */
class RedundancyFactory {
public:
    /**
     * @brief Create a redundancy detector by name.
     * 
     * @param strategy_name Strategy name (e.g., "SIMILARITY_BASED", "CONTENT_HASH").
     * @return Pointer to detector, or nullptr if name not recognized.
     */
    [[nodiscard]] static std::unique_ptr<IRedundancyDetector> create(
        const std::string& strategy_name);

    /**
     * @brief Create a composite detector with recommended default strategies.
     * 
     * @return CompositeDetector with built-in strategies and weights.
     */
    [[nodiscard]] static std::unique_ptr<CompositeDetector> createDefaultComposite();
};

} // namespace tensor
} // namespace themis
