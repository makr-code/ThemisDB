/**
 * @file tensor_routing_strategy.h
 * @brief Routing and prioritization abstractions for the tensor mid-layer.
 * 
 * Defines routing decisions, prioritization strategies, and policies for
 * directing tensor-layer results to downstream graph validation or fallback paths.
 */

#pragma once

#include "index/ann_frontdoor.h"
#include "tensor/tensor_summary_types.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace tensor {

// ============================================================================
// RoutingDecision — decision output from routing strategies
// ============================================================================

/**
 * @brief Routing decision produced by a routing strategy.
 * 
 * Specifies where and how tensor-layer results should be forwarded:
 * - To graph validation for truth layer processing
 * - To fallback/degraded path if primary failed
 * - To caching/memoization for similar future queries
 */
struct RoutingDecision {
    /// Primary routing target (e.g., "GRAPH_VALIDATION", "FALLBACK", "CACHE").
    std::string primary_target;

    /// Fallback target if primary fails.
    std::string fallback_target;

    /// Confidence in this routing decision (0.0-1.0).
    float confidence = 1.0f;

    /// Cost estimate for primary path (relative units).
    float primary_cost = 0.0f;

    /// Cost estimate for fallback path (relative units).
    float fallback_cost = 0.0f;

    /// Expected latency for primary path (milliseconds).
    float expected_latency_ms = 0.0f;

    /// Human-readable explanation of the routing choice.
    std::string reason = {};

    /// Machine-readable routing reason code.
    std::string reason_code;

    /// Whether to cache results for future similar queries.
    bool enable_caching = true;

    /// TTL for cached results (seconds, 0 = no cache).
    uint32_t cache_ttl_seconds = 3600;

    /// Priority level for processing (higher = more urgent, 0-100).
    uint8_t priority = 50;

    /// Shard selection hints for federated queries (shard IDs).
    std::vector<std::string> shard_hints;
};

// ============================================================================
// PrioritizationStrategy — interface for candidate prioritization
// ============================================================================

/**
 * @brief Abstract interface for candidate prioritization strategies.
 * 
 * Implementations assign priority scores to candidates based on various
 * criteria (similarity, freshness, validation status, cost, etc.).
 */
class IPrioritizationStrategy {
public:
    virtual ~IPrioritizationStrategy() = default;

    /**
     * @brief Get the name of this prioritization strategy.
     * @return Human-readable strategy name.
     */
    [[nodiscard]] virtual std::string name() const noexcept = 0;

    /**
     * @brief Compute priority scores for candidates.
     * 
     * @param summaries   Vector of tensor summaries to prioritize.
     * @return Vector of priority scores (0.0-1.0), same length as summaries.
     */
    [[nodiscard]] virtual std::vector<float> prioritize(
        const std::vector<const BaseTensorSummary*>& summaries) const = 0;

    /**
     * @brief Sort summaries by priority (highest first).
     * 
     * @param summaries    Vector of summaries to sort (modified in-place).
     * @return true if sorting was successful.
     */
    virtual bool sort(std::vector<BaseTensorSummary>& summaries) const = 0;
};

// ============================================================================
// SimilarityBasedPrioritization — prioritization by similarity score
// ============================================================================

/**
 * @brief Prioritization based on tensor similarity scores.
 * 
 * Assigns higher priority to candidates with higher similarity scores,
 * optionally weighted by confidence.
 */
class SimilarityBasedPrioritization : public IPrioritizationStrategy {
public:
    /// Weight for similarity score (0.0-1.0).
    float similarity_weight = 0.7f;

    /// Weight for confidence score (0.0-1.0).
    float confidence_weight = 0.3f;

    std::string name() const noexcept override;

    std::vector<float> prioritize(
        const std::vector<const BaseTensorSummary*>& summaries) const override;

    bool sort(std::vector<BaseTensorSummary>& summaries) const override;
};

// ============================================================================
// RankeBasedPrioritization — prioritization by rank and freshness
// ============================================================================

/**
 * @brief Prioritization based on candidate rank and recency.
 * 
 * Combines ranking score with temporal freshness to prioritize
 * recently updated, highly-ranked candidates.
 */
class RankBasedPrioritization : public IPrioritizationStrategy {
public:
    /// Weight for rank score (0.0-1.0).
    float rank_weight = 0.6f;

    /// Weight for freshness/recency (0.0-1.0).
    float freshness_weight = 0.4f;

    /// Max age in hours for full freshness score.
    float max_age_hours = 24.0f;

    std::string name() const noexcept override;

    std::vector<float> prioritize(
        const std::vector<const BaseTensorSummary*>& summaries) const override;

    bool sort(std::vector<BaseTensorSummary>& summaries) const override;
};

// ============================================================================
// CostBasedPrioritization — prioritization considering cost/latency
// ============================================================================

/**
 * @brief Prioritization balancing quality and processing cost.
 * 
 * Favors candidates that achieve good scores with lower computational cost,
 * useful for latency-sensitive scenarios.
 */
class CostBasedPrioritization : public IPrioritizationStrategy {
public:
    /// Weight for quality/similarity (0.0-1.0).
    float quality_weight = 0.6f;

    /// Weight for cost efficiency (0.0-1.0).
    float efficiency_weight = 0.4f;

    std::string name() const noexcept override;

    std::vector<float> prioritize(
        const std::vector<const BaseTensorSummary*>& summaries) const override;

    bool sort(std::vector<BaseTensorSummary>& summaries) const override;
};

// ============================================================================
// RoutingStrategy — interface for routing decisions
// ============================================================================

/**
 * @brief Abstract interface for routing strategy implementations.
 * 
 * Decides where tensor-layer results should be forwarded (graph validation,
 * fallback, cache, etc.) based on summaries and query context.
 */
class IRoutingStrategy {
public:
    virtual ~IRoutingStrategy() = default;

    /**
     * @brief Get the name of this routing strategy.
     * @return Human-readable strategy name.
     */
    [[nodiscard]] virtual std::string name() const noexcept = 0;

    /**
     * @brief Make a routing decision for tensor-layer results.
     * 
     * @param summaries          Tensor summaries produced by compression.
     * @param candidate_count    Total candidates before compression.
     * @param compression_ratio  Achieved compression ratio.
     * @param query_context      Original ANN query context.
     * @return Routing decision specifying where to forward results.
     */
    [[nodiscard]] virtual RoutingDecision route(
        const std::vector<BaseTensorSummary>& summaries,
        std::size_t                           candidate_count,
        float                                 compression_ratio,
        const index::AnnQueryContext&         query_context) const = 0;

    /**
     * @brief Evaluate whether to retry on routing failure.
     * 
     * @param reason             Failure reason from primary path.
     * @param attempt_count      Number of retry attempts so far.
     * @param max_attempts       Maximum allowed retry attempts.
     * @return true if should retry with fallback strategy.
     */
    [[nodiscard]] virtual bool shouldRetryOnFailure(
        const std::string& reason,
        int                attempt_count,
        int                max_attempts) const noexcept = 0;
};

// ============================================================================
// QualityBasedRouting — route based on summary quality metrics
// ============================================================================

/**
 * @brief Routing strategy based on summary quality and confidence.
 * 
 * Routes high-confidence results directly to graph validation,
 * lower-confidence results to fallback or enrichment paths.
 */
class QualityBasedRouting : public IRoutingStrategy {
public:
    /// Confidence threshold for direct graph validation (0.0-1.0).
    float confidence_threshold = 0.7f;

    /// Compression ratio threshold for routing to enhanced validation.
    float compression_ratio_threshold = 2.0f;

    std::string name() const noexcept override;

    RoutingDecision route(
        const std::vector<BaseTensorSummary>& summaries,
        std::size_t                           candidate_count,
        float                                 compression_ratio,
        const index::AnnQueryContext&         query_context) const override;

    bool shouldRetryOnFailure(
        const std::string& reason,
        int                attempt_count,
        int                max_attempts) const noexcept override;
};

// ============================================================================
// ShardAwareRouting — route considering shard health and availability
// ============================================================================

/**
 * @brief Routing strategy aware of shard-level health and latency.
 * 
 * Directs queries toward healthy, low-latency shards and provides
 * fallback hints for unavailable shards.
 */
class ShardAwareRouting : public IRoutingStrategy {
public:
    /// Latency threshold for shard selection (milliseconds).
    float latency_threshold_ms = 100.0f;

    /// Minimum shard health score for inclusion (0.0-1.0).
    float health_threshold = 0.8f;

    std::string name() const noexcept override;

    RoutingDecision route(
        const std::vector<BaseTensorSummary>& summaries,
        std::size_t                           candidate_count,
        float                                 compression_ratio,
        const index::AnnQueryContext&         query_context) const override;

    bool shouldRetryOnFailure(
        const std::string& reason,
        int                attempt_count,
        int                max_attempts) const noexcept override;
};

// ============================================================================
// AdaptiveRouting — adaptive routing based on runtime performance
// ============================================================================

/**
 * @brief Routing strategy that adapts based on observed performance metrics.
 * 
 * Learns from past routing decisions and their outcomes to optimize
 * future routing in similar query scenarios.
 */
class AdaptiveRouting : public IRoutingStrategy {
public:
    std::string name() const noexcept override;

    RoutingDecision route(
        const std::vector<BaseTensorSummary>& summaries,
        std::size_t                           candidate_count,
        float                                 compression_ratio,
        const index::AnnQueryContext&         query_context) const override;

    bool shouldRetryOnFailure(
        const std::string& reason,
        int                attempt_count,
        int                max_attempts) const noexcept override;

    /**
     * @brief Record the outcome of a routing decision for learning.
     * 
     * @param decision      The routing decision made.
     * @param success       Whether the downstream processing succeeded.
     * @param latency_ms    Observed latency in milliseconds.
     */
    void recordOutcome(
        const RoutingDecision& decision,
        bool                   success,
        float                  latency_ms) noexcept;

private:
    struct RouteMetrics {
        float success_rate = 0.0f;
        float avg_latency_ms = 0.0f;
        int observation_count = 0;
    };

    mutable std::unordered_map<std::string, RouteMetrics> metrics_;
};

// ============================================================================
// RoutingFactory — factory for creating routing strategies
// ============================================================================

/**
 * @brief Factory for creating routing and prioritization strategy instances.
 */
class RoutingFactory {
public:
    /**
     * @brief Create a routing strategy by name.
     * 
     * @param strategy_name Strategy name (e.g., "QUALITY_BASED", "SHARD_AWARE").
     * @return Pointer to strategy, or nullptr if name not recognized.
     */
    [[nodiscard]] static std::unique_ptr<IRoutingStrategy> createRouting(
        const std::string& strategy_name);

    /**
     * @brief Create a prioritization strategy by name.
     * 
     * @param strategy_name Strategy name (e.g., "SIMILARITY_BASED", "RANK_BASED").
     * @return Pointer to strategy, or nullptr if name not recognized.
     */
    [[nodiscard]] static std::unique_ptr<IPrioritizationStrategy> createPrioritization(
        const std::string& strategy_name);
};

} // namespace tensor
} // namespace themis
