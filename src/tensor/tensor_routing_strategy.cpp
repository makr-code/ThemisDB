/**
 * @file tensor_routing_strategy.cpp
 * @brief Routing and prioritization strategy implementations.
 */

#include "tensor/tensor_routing_strategy.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace themis {
namespace tensor {

// ============================================================================
// SimilarityBasedPrioritization implementation
// ============================================================================

std::string SimilarityBasedPrioritization::name() const noexcept {
    return "SIMILARITY_BASED";
}

std::vector<float> SimilarityBasedPrioritization::prioritize(
    const std::vector<const BaseTensorSummary*>& summaries) const {

    std::vector<float> priorities;
    priorities.reserve(summaries.size());

    for (const auto* summary : summaries) {
        if (!summary) {
            priorities.push_back(0.0f);
            continue;
        }

        float score = (summary->similarity_score * similarity_weight) +
                     (summary->confidence * confidence_weight);
        priorities.push_back(std::clamp(score, 0.0f, 1.0f));
    }

    return priorities;
}

bool SimilarityBasedPrioritization::sort(
    std::vector<BaseTensorSummary>& summaries) const {

    std::sort(summaries.begin(), summaries.end(),
              [this](const BaseTensorSummary& a, const BaseTensorSummary& b) {
                  float score_a = (a.similarity_score * similarity_weight) +
                                 (a.confidence * confidence_weight);
                  float score_b = (b.similarity_score * similarity_weight) +
                                 (b.confidence * confidence_weight);
                  return score_a > score_b;
              });

    return true;
}

// ============================================================================
// RankBasedPrioritization implementation
// ============================================================================

std::string RankBasedPrioritization::name() const noexcept {
    return "RANK_BASED";
}

std::vector<float> RankBasedPrioritization::prioritize(
    const std::vector<const BaseTensorSummary*>& summaries) const {

    std::vector<float> priorities;
    priorities.reserve(summaries.size());

    for (const auto* summary : summaries) {
        if (!summary) {
            priorities.push_back(0.0f);
            continue;
        }

        // Compute freshness score (0.0 if very old, 1.0 if very recent)
        // TODO(tracked): Parse created_at and compute age-based freshness decay
        //   — see src/tensor/ROADMAP.md § "Routing Freshness Scoring"
        float freshness = 1.0f;
        
        float score = (summary->similarity_score * rank_weight) +
                     (freshness * freshness_weight);
        priorities.push_back(std::clamp(score, 0.0f, 1.0f));
    }

    return priorities;
}

bool RankBasedPrioritization::sort(
    std::vector<BaseTensorSummary>& summaries) const {

    std::sort(summaries.begin(), summaries.end(),
              [this](const BaseTensorSummary& a, const BaseTensorSummary& b) {
                  // TODO(tracked): Compute age-based freshness from timestamp
                  //   — see src/tensor/ROADMAP.md § "Routing Freshness Scoring"
                  float freshness_a = 1.0f;
                  float freshness_b = 1.0f;
                  float score_a = (a.similarity_score * rank_weight) +
                                 (freshness_a * freshness_weight);
                  float score_b = (b.similarity_score * rank_weight) +
                                 (freshness_b * freshness_weight);
                  return score_a > score_b;
              });

    return true;
}

// ============================================================================
// CostBasedPrioritization implementation
// ============================================================================

std::string CostBasedPrioritization::name() const noexcept {
    return "COST_BASED";
}

std::vector<float> CostBasedPrioritization::prioritize(
    const std::vector<const BaseTensorSummary*>& summaries) const {

    std::vector<float> priorities;
    priorities.reserve(summaries.size());

    for (const auto* summary : summaries) {
        if (!summary) {
            priorities.push_back(0.0f);
            continue;
        }

        // Efficiency is inverse of expected cost
        float efficiency = 1.0f / std::max(0.001f, summary->compression_info.compression_ratio);
        
        float score = (summary->similarity_score * quality_weight) +
                     (efficiency * efficiency_weight);
        priorities.push_back(std::clamp(score, 0.0f, 1.0f));
    }

    return priorities;
}

bool CostBasedPrioritization::sort(
    std::vector<BaseTensorSummary>& summaries) const {

    std::sort(summaries.begin(), summaries.end(),
              [this](const BaseTensorSummary& a, const BaseTensorSummary& b) {
                  float eff_a = 1.0f / std::max(0.001f, a.compression_info.compression_ratio);
                  float eff_b = 1.0f / std::max(0.001f, b.compression_info.compression_ratio);
                  float score_a = (a.similarity_score * quality_weight) + (eff_a * efficiency_weight);
                  float score_b = (b.similarity_score * quality_weight) + (eff_b * efficiency_weight);
                  return score_a > score_b;
              });

    return true;
}

// ============================================================================
// QualityBasedRouting implementation
// ============================================================================

std::string QualityBasedRouting::name() const noexcept {
    return "QUALITY_BASED";
}

RoutingDecision QualityBasedRouting::route(
    const std::vector<BaseTensorSummary>& summaries,
    std::size_t                           candidate_count,
    float                                 compression_ratio,
    const index::AnnQueryContext&         query_context) const {

    RoutingDecision decision;
    decision.confidence = 0.9f;

    // Check confidence and compression quality
    bool high_quality = true;
    float avg_confidence = 0.0f;

    for (const auto& summary : summaries) {
        avg_confidence += summary.confidence;
        if (summary.confidence < confidence_threshold) {
            high_quality = false;
        }
    }

    if (!summaries.empty()) {
        avg_confidence /= summaries.size();
    }

    if (high_quality && compression_ratio > compression_ratio_threshold) {
        decision.primary_target = "GRAPH_VALIDATION";
        decision.fallback_target = "ENRICHMENT";
        decision.reason = "High-confidence, well-compressed results";
        decision.reason_code = "QUALITY_HIGH";
    } else if (avg_confidence >= confidence_threshold * 0.7f) {
        decision.primary_target = "GRAPH_VALIDATION";
        decision.fallback_target = "FALLBACK";
        decision.reason = "Moderate-confidence results with enhanced validation";
        decision.reason_code = "QUALITY_MODERATE";
    } else {
        decision.primary_target = "FALLBACK";
        decision.fallback_target = "ENRICHMENT";
        decision.reason = "Low-confidence results requiring fallback";
        decision.reason_code = "QUALITY_LOW";
        decision.confidence = 0.5f;
    }

    decision.enable_caching = (avg_confidence >= confidence_threshold);
    decision.cache_ttl_seconds = decision.enable_caching ? 3600 : 0;
    decision.priority = static_cast<uint8_t>(avg_confidence * 100);

    return decision;
}

bool QualityBasedRouting::shouldRetryOnFailure(
    const std::string& reason,
    int                attempt_count,
    int                max_attempts) const noexcept {

    // Retry up to max_attempts if we haven't exhausted retries
    return attempt_count < max_attempts;
}

// ============================================================================
// ShardAwareRouting implementation
// ============================================================================

std::string ShardAwareRouting::name() const noexcept {
    return "SHARD_AWARE";
}

RoutingDecision ShardAwareRouting::route(
    const std::vector<BaseTensorSummary>& summaries,
    std::size_t                           candidate_count,
    float                                 compression_ratio,
    const index::AnnQueryContext&         query_context) const {

    RoutingDecision decision;
    decision.confidence = 0.85f;
    decision.primary_target = "GRAPH_VALIDATION";
    decision.fallback_target = "FALLBACK";
    decision.reason = "Shard-aware routing with health checks";
    decision.reason_code = "SHARD_SELECTED";

    // Collect shard hints for healthy, low-latency shards.
    // The summaries vector contains BaseTensorSummary objects; shard-specific
    // fields live in `ShardSummary`. Use a heuristic: treat IDs prefixed with
    // "shard:" as shard summaries and prefer them as hints.
    for (const auto& summary : summaries) {
        if (!summary.id.empty() && summary.id.rfind("shard:", 0) == 0) {
            decision.shard_hints.push_back(summary.id);
        }
    }

    if (decision.shard_hints.empty()) {
        decision.confidence = 0.5f;
        decision.fallback_target = "ENRICHMENT";
    }

    return decision;
}

bool ShardAwareRouting::shouldRetryOnFailure(
    const std::string& reason,
    int                attempt_count,
    int                max_attempts) const noexcept {

    // Retry with fallback shard selection if possible
    return attempt_count < max_attempts;
}

// ============================================================================
// AdaptiveRouting implementation
// ============================================================================

std::string AdaptiveRouting::name() const noexcept {
    return "ADAPTIVE";
}

RoutingDecision AdaptiveRouting::route(
    const std::vector<BaseTensorSummary>& summaries,
    std::size_t                           candidate_count,
    float                                 compression_ratio,
    const index::AnnQueryContext&         query_context) const {

    RoutingDecision decision;
    
    // For now, use a simple heuristic based on learned metrics
    // TODO(tracked): Implement adaptive learning with metrics tracking
    //   — see src/tensor/ROADMAP.md § "Adaptive Routing Learning"
    
    decision.primary_target = "GRAPH_VALIDATION";
    decision.fallback_target = "FALLBACK";
    decision.confidence = 0.8f;
    decision.reason = "Adaptive routing with performance learning";
    decision.reason_code = "ADAPTIVE_SELECTED";

    return decision;
}

bool AdaptiveRouting::shouldRetryOnFailure(
    const std::string& reason,
    int                attempt_count,
    int                max_attempts) const noexcept {

    return attempt_count < max_attempts;
}

void AdaptiveRouting::recordOutcome(
    const RoutingDecision& decision,
    bool                   success,
    float                  latency_ms) noexcept {

    auto& metrics = metrics_[decision.primary_target];
    
    if (metrics.observation_count == 0) {
        metrics.success_rate = success ? 1.0f : 0.0f;
        metrics.avg_latency_ms = latency_ms;
    } else {
        metrics.success_rate = (metrics.success_rate * metrics.observation_count + 
                               (success ? 1.0f : 0.0f)) / (metrics.observation_count + 1);
        metrics.avg_latency_ms = (metrics.avg_latency_ms * metrics.observation_count + 
                                 latency_ms) / (metrics.observation_count + 1);
    }
    
    metrics.observation_count++;
}

// ============================================================================
// RoutingFactory implementation
// ============================================================================

std::unique_ptr<IRoutingStrategy> RoutingFactory::createRouting(
    const std::string& strategy_name) {

    if (strategy_name == "QUALITY_BASED") {
        return std::make_unique<QualityBasedRouting>();
    } else if (strategy_name == "SHARD_AWARE") {
        return std::make_unique<ShardAwareRouting>();
    } else if (strategy_name == "ADAPTIVE") {
        return std::make_unique<AdaptiveRouting>();
    }

    return nullptr;
}

std::unique_ptr<IPrioritizationStrategy> RoutingFactory::createPrioritization(
    const std::string& strategy_name) {

    if (strategy_name == "SIMILARITY_BASED") {
        return std::make_unique<SimilarityBasedPrioritization>();
    } else if (strategy_name == "RANK_BASED") {
        return std::make_unique<RankBasedPrioritization>();
    } else if (strategy_name == "COST_BASED") {
        return std::make_unique<CostBasedPrioritization>();
    }

    return nullptr;
}

} // namespace tensor
} // namespace themis
