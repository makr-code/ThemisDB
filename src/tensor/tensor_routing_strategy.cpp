/**
 * @file tensor_routing_strategy.cpp
 * @brief Routing and prioritization strategy implementations.
 */

#include "tensor/tensor_routing_strategy.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <limits>
#include <sstream>
#include <unordered_map>

namespace themis {
namespace tensor {

// ============================================================================
// Helper: Parse ISO-8601 timestamp and compute freshness decay
// ============================================================================

/**
 * @brief Parse ISO-8601 timestamp string and return seconds since Unix epoch.
 * 
 * Supports formats: "2026-09-23T18:19:50Z" or "2026-09-23T18:19:50.728Z"
 * 
 * @param iso_timestamp ISO-8601 timestamp string
 * @return Time as seconds since Unix epoch, or 0 if parsing fails
 */
static double parseISO8601(const std::string& iso_timestamp) noexcept {
    if (iso_timestamp.empty()) {
        return 0.0;
    }
    
    std::tm tm = {};
    std::istringstream ss(iso_timestamp);
    
    // Try parsing with fractional seconds
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (ss.fail()) {
        return 0.0;  // Parsing failed
    }
    
    // Convert to time_t (seconds since epoch)
    time_t t = std::mktime(&tm);
    if (t == -1) {
        return 0.0;  // mktime failed
    }
    
    return static_cast<double>(t);
}

/**
 * @brief Compute freshness score based on age of tensor (0.0=stale, 1.0=fresh).
 * 
 * Freshness decay model:
 * - Younger than 1 hour: freshness = 1.0
 * - 1 to 24 hours: linear decay from 1.0 to 0.5
 * - 24+ hours: linear decay from 0.5 to 0.0 over next 24 hours
 * - Older than 48 hours: freshness = 0.0
 * 
 * @param created_at_timestamp Creation time in seconds since epoch
 * @return Freshness score in [0.0, 1.0]
 */
static float computeFreshnessDecay(double created_at_timestamp) noexcept {
    if (created_at_timestamp <= 0.0) {
        return 0.0f;  // Invalid timestamp
    }
    
    auto now = std::chrono::system_clock::now();
    auto now_timestamp = std::chrono::system_clock::to_time_t(now);
    double age_seconds = static_cast<double>(now_timestamp) - created_at_timestamp;
    
    if (age_seconds < 0.0) {
        return 0.0f;  // Future timestamp (invalid)
    }
    
    constexpr double ONE_HOUR_SECONDS = 3600.0;
    constexpr double ONE_DAY_SECONDS = 86400.0;
    constexpr double TWO_DAYS_SECONDS = 172800.0;
    
    if (age_seconds < ONE_HOUR_SECONDS) {
        // Very fresh: < 1 hour
        return 1.0f;
    } else if (age_seconds < ONE_DAY_SECONDS) {
        // Moderately fresh: 1-24 hours, linear decay from 1.0 to 0.5
        double fraction = (age_seconds - ONE_HOUR_SECONDS) / (ONE_DAY_SECONDS - ONE_HOUR_SECONDS);
        return 1.0f - (0.5f * static_cast<float>(fraction));
    } else if (age_seconds < TWO_DAYS_SECONDS) {
        // Becoming stale: 24-48 hours, linear decay from 0.5 to 0.0
        double fraction = (age_seconds - ONE_DAY_SECONDS) / (TWO_DAYS_SECONDS - ONE_DAY_SECONDS);
        return 0.5f * (1.0f - static_cast<float>(fraction));
    } else {
        // Very stale: > 48 hours
        return 0.0f;
    }
}

// ============================================================================
// SimilarityBasedPrioritization implementation
// ============================================================================

std::string SimilarityBasedPrioritization::name() const noexcept {
    return "SIMILARITY_BASED";
}

std::vector<float> SimilarityBasedPrioritization::prioritize(
    const std::vector<const BaseTensorSummary*>& summaries) const {

    std::vector<float> priorities = {};

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

    std::vector<float> priorities = {};

    priorities.reserve(summaries.size());

    for (const auto* summary : summaries) {
        if (!summary) {
            priorities.push_back(0.0f);
            continue;
        }

        // Compute freshness score (0.0 if very old, 1.0 if very recent)
        double created_at_timestamp = parseISO8601(summary->created_at);
        float freshness = computeFreshnessDecay(created_at_timestamp);
        
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
                  // Compute age-based freshness from timestamp
                  double created_at_a = parseISO8601(a.created_at);
                  double created_at_b = parseISO8601(b.created_at);
                  float freshness_a = computeFreshnessDecay(created_at_a);
                  float freshness_b = computeFreshnessDecay(created_at_b);
                  
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

    std::vector<float> priorities = {};

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

    (void)query_context;  // May be used for context-specific adaptations in future
    (void)candidate_count;

    RoutingDecision decision;
    
    // Implement adaptive learning based on observed performance metrics
    float best_success_rate = 0.0f;
    float best_avg_latency = std::numeric_limits<float>::max();
    std::string best_target = "GRAPH_VALIDATION";  // Default fallback
    
    // Analyze learned metrics from past routing decisions
    for (const auto& [target, metrics] : metrics_) {
        // Weighting formula: prefer targets with high success rate and low latency
        // Score = success_rate - (normalized_latency * 0.1)
        float normalized_latency = metrics.avg_latency_ms > 0.0f ? 
            std::min(1.0f, metrics.avg_latency_ms / 1000.0f) : 0.0f;
        float target_score = metrics.success_rate - (normalized_latency * 0.1f);
        
        if (target_score > best_success_rate - (best_avg_latency / 1000.0f) * 0.1f) {
            best_target = target;
            best_success_rate = metrics.success_rate;
            best_avg_latency = metrics.avg_latency_ms;
        }
    }
    
    // Compute confidence based on observed success rate and number of observations
    float confidence = best_success_rate;
    if (!metrics_.empty()) {
        auto it = metrics_.find(best_target);
        if (it != metrics_.end() && it->second.observation_count > 0) {
            // Confidence increases with more observations (up to a cap at 100 observations)
            int obs_count = std::min(100, it->second.observation_count);
            confidence = best_success_rate * (0.5f + 0.5f * obs_count / 100.0f);
        }
    }
    
    decision.primary_target = best_target;
    decision.fallback_target = (best_target == "GRAPH_VALIDATION") ? "FALLBACK" : "GRAPH_VALIDATION";
    decision.confidence = std::clamp(confidence, 0.5f, 0.95f);  // Keep in reasonable range
    decision.expected_latency_ms = best_avg_latency;
    decision.reason = "Adaptive routing based on " + std::to_string(
        metrics_.empty() ? 0 : metrics_[best_target].observation_count) + " learned observations";
    decision.reason_code = "ADAPTIVE_LEARNED";
    
    // Cache based on high confidence in learned routing
    decision.enable_caching = (confidence >= 0.8f);
    decision.cache_ttl_seconds = decision.enable_caching ? 1800 : 600;
    
    // Priority reflects compression efficiency + confidence
    decision.priority = static_cast<uint8_t>(std::clamp(
        confidence * 100.0f * std::max(0.1f, compression_ratio) / 2.0f, 
        10.0f, 100.0f));

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

/**
 * @brief Create Routing.
 * @param[in] strategy_name Name of the strategy.
 * @return Return value.
 * @details Implements createRouting without additional internal calls.
 */
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

/**
 * @brief Create Prioritization.
 * @param[in] strategy_name Name of the strategy.
 * @return Return value.
 * @details Implements createPrioritization without additional internal calls.
 */
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
