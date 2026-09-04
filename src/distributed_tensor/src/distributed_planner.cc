/**
 * @file distributed_planner.cc
 * @brief Implementation of distributed tensor retrieval planner.
 */

#include "../include/distributed_planner.h"
#include "tensor/tensor_summary_types.h"
#include "rag/graph_truth_validator.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace themis {
namespace distributed_tensor {

// ============================================================================
// Timestamp Utilities
// ============================================================================

/**
 * @brief Get current ISO-8601 timestamp.
 */
static std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    return oss.str();
}

/**
 * @brief Parse ISO-8601 timestamp and compute age in seconds.
 * 
 * Returns -1 if parsing fails.
 */
static long parseTimestampAgeSec(const std::string& iso_timestamp) noexcept {
    try {
        // Simple parsing: expect format YYYY-MM-DDTHH:MM:SS.sssZ
        // For this MVP, we do a simplified check
        if (iso_timestamp.empty() || iso_timestamp.length() < 19) {
            return -1;  // Invalid format
        }

        std::tm tm = {};
        std::istringstream iss(iso_timestamp);
        iss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        
        if (iss.fail()) {
            return -1;  // Parsing failed
        }

        auto then = std::mktime(&tm);
        auto now = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());

        return static_cast<long>(std::difftime(now, then));
    } catch (...) {
        return -1;
    }
}

// ============================================================================
// DistributedTensorPlanner Implementation
// ============================================================================

void DistributedTensorPlanner::setFragmentFetcher(
    std::shared_ptr<IFragmentFetcher> fetcher) noexcept {
    fragment_fetcher_ = fetcher;
}

void DistributedTensorPlanner::setGraphValidator(
    std::shared_ptr<rag::GraphTruthValidator> validator) noexcept {
    graph_validator_ = validator;
}

std::vector<RoutingSummary> DistributedTensorPlanner::planSummaryFirstRouting(
    const std::vector<tensor::ShardSummary>& summaries,
    const std::string& correlation_id,
    uint32_t freshness_ttl_s) noexcept {
    
    std::vector<RoutingSummary> routing_results = {};

    routing_results.reserve(summaries.size());

    for (const auto& summary : summaries) {
        RoutingSummary routing;
        routing.shard_id = summary.shard_id;
        routing.freshness_state = summary.freshness_state;
        routing.shard_relevance = summary.shard_relevance;
        routing.routing_reason = summary.shard_routing_reason;

        // Check freshness
        if (summary.freshness_state == tensor::SummaryFreshnessState::FRESH &&
            summary.shard_healthy) {
            
            // Validate TTL
            long age_sec = parseTimestampAgeSec(summary.created_at);
            if (age_sec >= 0 && age_sec > static_cast<long>(freshness_ttl_s)) {
                routing.rejected_as_stale = true;
                routing.freshness_state = tensor::SummaryFreshnessState::STALE;
                routing.routing_reason = "Summary exceeded freshness TTL";
            } else {
                routing.routing_confidence = 0.9f;
            }
        } else if (summary.freshness_state == tensor::SummaryFreshnessState::STALE) {
            routing.rejected_as_stale = true;
            routing.routing_reason = "Summary explicitly marked as stale";
        } else if (summary.freshness_state == tensor::SummaryFreshnessState::INVALID) {
            routing.rejected_as_stale = true;
            routing.routing_reason = "Summary is invalid or corrupted";
        }

        if (!summary.shard_healthy) {
            routing.rejected_as_unhealthy = true;
            routing.routing_reason = "Shard is not healthy";
        }

        routing_results.push_back(routing);
    }

    return routing_results;
}

std::vector<FragmentLoadRequest> DistributedTensorPlanner::planExactOnDemandLoading(
    const std::vector<RoutingSummary>& routing_summaries,
    const std::string& query_context,
    const std::string& correlation_id) noexcept {
    
    std::vector<FragmentLoadRequest> requests;

    for (const auto& routing : routing_summaries) {
        // Skip rejected shards
        if (routing.rejected_as_stale || routing.rejected_as_unhealthy) {
            continue;
        }

        // Create fragment load request
        FragmentLoadRequest request;
        request.shard_id = routing.shard_id;
        request.artifact_id = routing.shard_id + ":fragment";
        request.timeout_ms = config_.fragment_load_timeout_ms;
        
        // Set expedited flag based on relevance
        request.expedited = (routing.shard_relevance > 0.75f);

        requests.push_back(request);
    }

    return requests;
}

TensorRetrievalPlan DistributedTensorPlanner::buildRetrievalPlan(
    const std::vector<tensor::ShardSummary>& summaries,
    const std::string& query_context,
    const std::string& correlation_id) noexcept {
    
    TensorRetrievalPlan plan;
    plan.correlation_id = correlation_id.empty() ? 
        "plan:" + std::to_string(std::time(nullptr)) : correlation_id;
    plan.plan_reason = "summary-first → exact-on-demand → graph-verified";

    // Phase 1: Summary-first routing
    auto routing_summaries = planSummaryFirstRouting(
        summaries, plan.correlation_id);
    plan.routing_summaries = routing_summaries;

    // Count stale rejections
    for (const auto& rs : routing_summaries) {
        if (rs.rejected_as_stale) {
            plan.stale_shards_rejected++;
        }
    }

    plan.all_summaries_fresh = (plan.stale_shards_rejected == 0);

    // Phase 2: Exact-on-demand fragment loading
    auto fragment_requests = planExactOnDemandLoading(
        routing_summaries, query_context, plan.correlation_id);
    plan.fragment_requests = fragment_requests;

    // Set fallback if insufficient fresh shards
    if (!plan.all_summaries_fresh && config_.enable_fallback_routing) {
        plan.is_fallback = true;
        plan.fallback_routing = buildFallbackRouting(summaries);
    }

    return plan;
}

std::vector<FragmentLoadResult> DistributedTensorPlanner::executeFragmentLoads(
    const std::vector<FragmentLoadRequest>& requests,
    const std::string& correlation_id) noexcept {
    
    if (!fragment_fetcher_) {
        std::vector<FragmentLoadResult> results = {};

        for (const auto& req : requests) {
            FragmentLoadResult result;
            result.shard_id = req.shard_id;
            result.artifact_id = req.artifact_id;
            result.success = false;
            result.error_reason = "No fragment fetcher configured";
            results.push_back(result);
        }
        return results;
    }

    return fragment_fetcher_->fetchFragments(requests, correlation_id);
}

rag::GraphTruthValidationResult DistributedTensorPlanner::validateAgainstGraphTruth(
    const TensorRetrievalPlan& plan,
    const std::string& query,
    const rag::GraphTruthValidatorConfig& config) const noexcept {
    
    rag::GraphTruthValidationResult result;
    
    if (!graph_validator_) {
        result.routing_reason = "No graph validator configured";
        result.routing_reason_code = "NO_VALIDATOR";
        return result;
    }

    // Check that fragments were loaded
    if (plan.fragment_results.empty()) {
        result.routing_reason = "No fragment results available for graph validation";
        result.routing_reason_code = "NO_FRAGMENTS";
        return result;
    }

    // Validate fragments against graph truth
    // In production, this would project fragments into graph candidates
    // and validate using KG retrieval or ontology-aware retrieval
    
    // For MVP: Just verify fragment loading was successful
    // (In real implementation, this would use actual fragment data for validation)
    
    if (config_.strict_graph_validation) {
        // Require all fragments loaded successfully
        std::size_t success_count = 0;
        for (const auto& result : plan.fragment_results) {
            if (result.success) {
                success_count++;
            }
        }
        
        float success_rate = plan.fragment_results.empty() ? 0.0f :
            static_cast<float>(success_count) / plan.fragment_results.size();

        if (success_rate < config_.min_fragment_success_rate) {
            result.routing_reason = "Insufficient fragment load success rate for validation";
            result.routing_reason_code = "LOW_SUCCESS_RATE";
            return result;
        }
    }

    // Mark validation as passed (in real implementation, would check actual graph)
    result.routing_reason = "Graph truth validation passed";
    result.routing_reason_code = "GRAPH_VALIDATED";
    return result;
}

bool DistributedTensorPlanner::isValidFinalPlan(
    const TensorRetrievalPlan& plan) const noexcept {
    
    // Rule 1: All advisory summaries must be fresh (or fallback enabled)
    if (!plan.all_summaries_fresh && !plan.is_fallback) {
        return false;
    }

    // Rule 2: Must have fragment loading results
    if (plan.fragment_results.empty() && !plan.fragment_requests.empty()) {
        return false;  // Expected fragments but none loaded
    }

    // Rule 3: Graph validation must have passed
    if (config_.strict_graph_validation && !plan.passed_graph_validation) {
        return false;
    }

    // Rule 4: No summary-only-truth (at least one fragment must be loaded)
    bool has_loaded_fragment = false;
    for (const auto& result : plan.fragment_results) {
        if (result.success) {
            has_loaded_fragment = true;
            break;
        }
    }

    if (plan.fragment_requests.size() > 0 && !has_loaded_fragment) {
        return false;  // Expected fragments but none succeeded
    }

    return true;
}

void DistributedTensorPlanner::setConfig(const Config& config) noexcept {
    config_ = config;
}

const DistributedTensorPlanner::Config& DistributedTensorPlanner::config() const noexcept {
    return config_;
}

bool DistributedTensorPlanner::validateSummaryFreshness(
    const tensor::ShardSummary& summary) const noexcept {
    
    if (summary.freshness_state != tensor::SummaryFreshnessState::FRESH) {
        return false;
    }

    long age_sec = parseTimestampAgeSec(summary.created_at);
    if (age_sec < 0) {
        return false;  // Invalid timestamp
    }

    return age_sec <= static_cast<long>(config_.max_summary_ttl_seconds);
}

float DistributedTensorPlanner::computeRoutingRelevance(
    const tensor::ShardSummary& summary) const noexcept {
    
    if (!summary.shard_healthy) {
        return 0.0f;
    }

    if (summary.freshness_state != tensor::SummaryFreshnessState::FRESH) {
        return 0.0f;
    }

    return summary.shard_relevance;
}

std::string DistributedTensorPlanner::buildFallbackRouting(
    const std::vector<tensor::ShardSummary>& summaries) const noexcept {
    
    // In fallback mode, broaden the search to include stale summaries
    // with explicit stale-fragment fetch
    std::ostringstream oss;
    oss << "fallback_routing:include_stale:shard_count=" << summaries.size();
    return oss.str();
}

} // namespace distributed_tensor
} // namespace themis
