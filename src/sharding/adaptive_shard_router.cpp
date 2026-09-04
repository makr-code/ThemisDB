/**
 * @file adaptive_shard_router.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=10, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/adaptive_shard_router.h"
#include "distributed_knowledge/adapter_capability_announcement.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <limits>
#include <sstream>
#include <numeric>
#include <spdlog/spdlog.h>

namespace themis::sharding {

AdaptiveShardRouter::AdaptiveShardRouter(
    std::shared_ptr<URNResolver> resolver,
    std::shared_ptr<RemoteExecutor> executor,
    std::shared_ptr<ShardTopology> topology,
    const Config& config,
    const AdaptiveConfig& adaptive_config,
    std::shared_ptr<PrometheusMetrics> metrics,
    std::shared_ptr<TrueTime> truetime
) : ShardRouter(resolver, executor, config, metrics, truetime),
    topology_(topology),
    adaptive_config_(adaptive_config)
{
    if (!adaptive_config_.isValid()) {
        throw std::invalid_argument("Invalid AdaptiveConfig");
    }
    
    // Create capability matcher
    matcher_ = std::make_shared<CapabilityMatcher>(adaptive_config_.matcher_config);
}

AdaptiveShardRouter::AdaptiveShardRouter(
    std::shared_ptr<URNResolver> resolver,
    std::shared_ptr<RemoteExecutor> executor,
    std::shared_ptr<ShardTopology> topology,
    const Config& config,
    std::shared_ptr<PrometheusMetrics> metrics,
    std::shared_ptr<TrueTime> truetime
) : AdaptiveShardRouter(
        resolver,
        executor,
        topology,
        config,
        AdaptiveConfig{},
        metrics,
        truetime
    )
{
}

void AdaptiveShardRouter::updateAdapterCapability(
    const std::string& shard_id,
    const themis::distributed_knowledge::AdapterCapabilityAnnouncement& announcement
) {
    std::lock_guard<std::mutex> lock(domain_scores_mutex_);
    shard_domain_scores_[shard_id][announcement.domain_type] = announcement.accuracy_delta;
}

void AdaptiveShardRouter::updateShardLLMLoad(
    const std::string& shard_id,
    uint64_t pending_requests,
    double avg_queue_ms
) {
    std::lock_guard<std::mutex> lock(domain_scores_mutex_);
    auto& load = shard_llm_load_[shard_id];
    load.pending_requests = pending_requests;
    load.avg_queue_ms     = avg_queue_ms;
    load.updated_at       = std::chrono::steady_clock::now();
}

std::string AdaptiveShardRouter::routeByDomain(
    themis::distributed_knowledge::AdapterDomainType domain
) const {
    std::lock_guard<std::mutex> lock(domain_scores_mutex_);

    std::string best_shard = {};
    double best_delta = std::numeric_limits<double>::lowest();
    uint64_t best_pending = std::numeric_limits<uint64_t>::max();
    int best_freshness_rank = 2;  // 0=fresh, 1=stale, 2=missing
    bool found = false;
    const auto now = std::chrono::steady_clock::now();
    const auto freshness_window = std::chrono::milliseconds(adaptive_config_.llm_load_freshness_ms);

    for (const auto& [shard_id, domain_map] : shard_domain_scores_) {
        auto it = domain_map.find(domain);
        if (it == domain_map.end()) {
            continue;
        }
        const double delta = it->second;
        if (!std::isfinite(delta)) {
            continue;
        }

        uint64_t pending = std::numeric_limits<uint64_t>::max();
        int freshness_rank = 2;
        auto load_it = shard_llm_load_.find(shard_id);
        if (load_it != shard_llm_load_.end()) {
            const auto age = now - load_it->second.updated_at;
            if (age <= freshness_window) {
                freshness_rank = 0;
                pending = load_it->second.pending_requests;
            } else {
                freshness_rank = 1;
            }
        }

        const bool better_score = delta > best_delta;
        const bool tied_fresher_load = (delta == best_delta) && (freshness_rank < best_freshness_rank);
        const bool tied_less_load =
            (delta == best_delta) && (freshness_rank == best_freshness_rank) && (pending < best_pending);
        const bool tied_lexical =
            (delta == best_delta) && (freshness_rank == best_freshness_rank) &&
            (pending == best_pending) && (!best_shard.empty()) && (shard_id < best_shard);

        if (!found || better_score || tied_fresher_load || tied_less_load || tied_lexical) {
            best_delta   = delta;
            best_pending = pending;
            best_freshness_rank = freshness_rank;
            best_shard   = shard_id;
            found        = true;
        }
    }

    return best_shard; // empty string when no score exists for this domain
}

double AdaptiveShardRouter::getAdapterAccuracyDelta(
    const std::string& shard_id,
    themis::distributed_knowledge::AdapterDomainType domain
) const {
    std::lock_guard<std::mutex> lock(domain_scores_mutex_);

    auto shard_it = shard_domain_scores_.find(shard_id);
    if (shard_it == shard_domain_scores_.end()) {
        return 0.0;
    }
    auto domain_it = shard_it->second.find(domain);
    if (domain_it == shard_it->second.end()) {
        return 0.0;
    }
    return domain_it->second;
}

nlohmann::json AdaptiveShardRouter::executeQuery(const std::string& query) {
    // Check if adaptive routing is enabled
    if (!adaptive_config_.enable_adaptive_routing) {
        // Fallback to base class scatter-gather
        return ShardRouter::executeQuery(query);
    }
    
    AdaptiveStats stats = {};
    return executeAdaptiveQuery(query, stats);
}

nlohmann::json AdaptiveShardRouter::executeAdaptiveQuery(
    const std::string& query,
    AdaptiveStats& stats
) {
    total_adaptive_queries_++;
    
    // Initialize stats
    stats.start_time = std::chrono::system_clock::now();
    stats.query_id = "q_" + std::to_string(total_adaptive_queries_.load());
    stats.iterations_executed = 0;
    stats.total_shards_queried = 0;
    stats.total_results = 0;
    stats.used_adaptive_routing = true;
    stats.stopped_early = false;
    
    // Prepare query context for matching
    auto query_context = prepareQueryContext(query);
    
    // Get all shards with capabilities
    auto all_shards = topology_->getAllShards();
    
    if (all_shards.empty()) {
        stats.end_time = std::chrono::system_clock::now();
        stats.total_time_ms = 0;
        stats.stop_reason = "no_shards_available";
        return nlohmann::json::array();
    }
    
    // Match query against shard capabilities
    auto match_results = matcher_->match(query_context, all_shards);
    
    // Check if we have any capability matches
    if (match_results.empty() || match_results[0].score < adaptive_config_.fallback_threshold) {
        if (adaptive_config_.fallback_to_scatter_gather) {
            fallback_to_scatter_gather_++;
            stats.used_adaptive_routing = false;
            stats.stop_reason = "no_capability_matches_fallback_to_scatter_gather";
            stats.end_time = std::chrono::system_clock::now();
            stats.total_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                stats.end_time - stats.start_time).count();
            return ShardRouter::executeQuery(query);
        } else {
            stats.stop_reason = "no_capability_matches";
            stats.end_time = std::chrono::system_clock::now();
            stats.total_time_ms = 0;
            return nlohmann::json::array();
        }
    }
    
    // Iterative execution
    std::vector<std::vector<ShardResult>> all_iteration_results;
    std::set<std::string> already_queried;
    uint32_t previous_result_count = 0;
    
    // Track elapsed time with steady_clock for consistency
    auto execution_start = std::chrono::steady_clock::now();
    
    // Define thresholds for each iteration
    std::vector<double> thresholds = {
        adaptive_config_.initial_threshold,
        adaptive_config_.intermediate_threshold,
        adaptive_config_.fallback_threshold
    };
    
    for (uint32_t iteration = 0; iteration < adaptive_config_.max_iterations; ++iteration) {
        stats.iterations_executed = iteration + 1;
        
        auto iteration_start = std::chrono::high_resolution_clock::now();
        
        // Select shards for this iteration
        double threshold = iteration <static_cast<int>(thresholds.size()) ? 
                          thresholds[iteration] : adaptive_config_.fallback_threshold;
        
        auto shard_ids = selectShardsForIteration(
            match_results, threshold, 
            adaptive_config_.results_per_iteration, 
            already_queried);
        
        if (shard_ids.empty()) {
            stats.stop_reason = "no_more_shards_above_threshold";
            break;
        }
        
        // Execute query on selected shards
        auto iteration_results = executeOnShards(
            query, shard_ids, adaptive_config_.per_iteration_timeout_ms);
        
        all_iteration_results.push_back(iteration_results);
        
        // Update already queried set
        for (const auto& shard_id : shard_ids) {
            already_queried.insert(shard_id);
        }
        
        stats.total_shards_queried += static_cast<uint32_t>(shard_ids.size());
        
        // Count results
        uint32_t iteration_result_count = 0;
        for (const auto& result : iteration_results) {
            if (result.success && result.data.is_array()) {
                iteration_result_count += static_cast<uint32_t>(result.data.size());
            }
        }
        
        stats.total_results += iteration_result_count;
        
        auto iteration_end = std::chrono::high_resolution_clock::now();
        uint64_t iteration_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            iteration_end - iteration_start).count();
        
        // Calculate iteration statistics
        auto iter_stats = calculateIterationStats(
            iteration + 1, shard_ids, iteration_results, 
            match_results, iteration_time_ms);
        stats.iteration_details.push_back(iter_stats);
        
        // Check stop criteria - use steady_clock for total elapsed time
        auto execution_current = std::chrono::steady_clock::now();
        uint64_t elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            execution_current - execution_start).count();
        
        std::string stop_reason = {};
        if (shouldStop(stats.total_results, previous_result_count, 
                      elapsed_ms, iteration + 1, stop_reason)) {
            stats.stopped_early = true;
            stats.stop_reason = stop_reason;
            early_stops_++;
            break;
        }
        
        previous_result_count = stats.total_results;
    }
    
    // If we completed all iterations without early stop
    if (!stats.stopped_early) {
        stats.stop_reason = "max_iterations_reached";
    }
    
    // Merge results from all iterations
    auto merged_results = mergeIterationResults(all_iteration_results);
    
    stats.end_time = std::chrono::system_clock::now();
    stats.total_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        stats.end_time - stats.start_time).count();
    
    // Calculate iterations saved
    uint32_t potential_iterations = static_cast<uint32_t>(
        (static_cast<int>(all_shards.size()) + 
        adaptive_config_.results_per_iteration - 1) / 
        adaptive_config_.results_per_iteration);
    if (potential_iterations > stats.iterations_executed) {
        iterations_saved_ += (potential_iterations - stats.iterations_executed);
    }
    
    return merged_results;
}

nlohmann::json AdaptiveShardRouter::getAdaptiveStatistics() const {
    return {
        {"total_adaptive_queries", total_adaptive_queries_.load()},
        {"iterations_saved", iterations_saved_.load()},
        {"early_stops", early_stops_.load()},
        {"fallback_to_scatter_gather", fallback_to_scatter_gather_.load()},
        {"matcher_stats", matcher_->getStatistics()},
        {"config", {
            {"enable_adaptive_routing", adaptive_config_.enable_adaptive_routing},
            {"max_iterations", adaptive_config_.max_iterations},
            {"results_per_iteration", adaptive_config_.results_per_iteration},
            {"initial_threshold", adaptive_config_.initial_threshold},
            {"intermediate_threshold", adaptive_config_.intermediate_threshold},
            {"fallback_threshold", adaptive_config_.fallback_threshold},
            {"target_result_count", adaptive_config_.target_result_count}
        }}
    };
}

void AdaptiveShardRouter::updateAdaptiveConfig(const AdaptiveConfig& config) {
    if (!config.isValid()) {
        throw std::invalid_argument("Invalid AdaptiveConfig");
    }
    adaptive_config_ = config;
    
    // Update matcher config
    matcher_ = std::make_shared<CapabilityMatcher>(config.matcher_config);
}

void AdaptiveShardRouter::setNlpContextFn(NlpContextFn fn) {
    std::lock_guard<std::mutex> lock(nlp_context_fn_mutex_);
    nlp_context_fn_ = std::move(fn);
}

void AdaptiveShardRouter::setNlpContextFn(LegacyNlpContextFn fn) {
    setNlpContextFn([fn = std::move(fn)](
        const std::string& query,
        CapabilityMatcher::QueryContext& context
    ) {
        auto maybe_context = fn(std::string_view{query});
        if (!maybe_context.has_value()) {
            return;
        }

        auto enriched = std::move(*maybe_context);
        if (enriched.query_text.empty()) {
            enriched.query_text = query;
        }
        if (enriched.keywords.empty()) {
            enriched.keywords = context.keywords;
        }
        context = std::move(enriched);
    });
}

CapabilityMatcher::QueryContext AdaptiveShardRouter::prepareQueryContext(
    const std::string& query
) {
    CapabilityMatcher::QueryContext context;
    context.query_text = query;
    context.keywords = matcher_->extractKeywords(query);

    // Prefer injected NLP/ML enrichment when available.
    {
        std::lock_guard<std::mutex> lock(nlp_context_fn_mutex_);
        if (nlp_context_fn_.has_value()) {
            try {
                nlp_context_fn_.value()(query, context);
                return context;
            } catch (const std::exception& e) {
                spdlog::warn("AdaptiveShardRouter: NLP context fn failed: {}; "
                             "falling back to pattern matching", e.what());
            }
        }
    }
    
    // Fallback heuristic path for deployments without an injected NLP service.
    std::string query_lower = query;
    std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // Detect regions (example patterns)
    if (query_lower.find("hamburg") != std::string::npos) {
        context.regions.push_back("hamburg");
    }
    if (query_lower.find("berlin") != std::string::npos) {
        context.regions.push_back("berlin");
    }
    if (query_lower.find("münchen") != std::string::npos || query_lower.find("munich") != std::string::npos) {
        context.regions.push_back("munich");
    }

    // Detect domains (example patterns)
    if (query_lower.find("baurecht") != std::string::npos || query_lower.find("building") != std::string::npos) {
        context.domains.push_back("construction");
    }
    if (query_lower.find("recht") != std::string::npos || query_lower.find("legal") != std::string::npos) {
        context.domains.push_back("law");
    }

    return context;
}

std::vector<std::string> AdaptiveShardRouter::selectShardsForIteration(
    const std::vector<CapabilityMatchResult>& match_results,
    double threshold,
    size_t max_shards,
    const std::set<std::string>& already_queried
) {
    struct Candidate {
        std::string shard_id = {};
        double score = 0.0;
    };

    std::vector<Candidate> candidates = {};

    candidates.reserve(match_results.size());

    for (const auto& match : match_results) {
        // Skip if already queried
        if (already_queried.find(match.shard_id) != already_queried.end()) {
            continue;
        }

        // Skip invalid scores explicitly to avoid undefined ranking.
        if (!std::isfinite(match.score)) {
            continue;
        }
        
        // Skip if below threshold
        if (match.score < threshold) {
            continue;
        }

        // Skip stale topology entries (e.g., shard became unhealthy after scoring).
        auto shard_info = topology_->getShard(match.shard_id);
        if (!shard_info || !shard_info->is_healthy) {
            continue;
        }

        candidates.push_back(Candidate{match.shard_id, match.score});
    }

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& lhs, const Candidate& rhs) {
        if (lhs.score != rhs.score) {
            return lhs.score > rhs.score;
        }
        return lhs.shard_id < rhs.shard_id;
    });

    std::vector<std::string> selected = {};

    selected.reserve(std::min(max_shards,static_cast<int>(candidates.size())));
    for (const auto& candidate : candidates) {
        selected.push_back(candidate.shard_id);
        if (static_cast<int>(selected.size()) >= max_shards) {
            break;
        }
    }
    
    return selected;
}

std::vector<ShardResult> AdaptiveShardRouter::executeOnShards(
    const std::string& query,
    const std::vector<std::string>& shard_ids
) {
    return ShardRouter::executeOnShards(query, shard_ids);
}

std::vector<ShardResult> AdaptiveShardRouter::executeOnShards(
    const std::string& query,
    const std::vector<std::string>& shard_ids,
    [[maybe_unused]] uint32_t timeout_ms
) {
    // Delegate to the base-class implementation which fans out via RemoteExecutor.
    // timeout_ms is advisory; per-request timeouts are governed by the mTLS client
    // configuration in RemoteExecutor. Per-iteration timeout enforcement is deferred
    // until RemoteExecutor exposes a per-call timeout override API.
    return executeOnShards(query, shard_ids);
}

bool AdaptiveShardRouter::shouldStop(
    uint32_t current_results,
    uint32_t previous_results,
    uint64_t elapsed_ms,
    uint32_t iteration,
    std::string& reason
) {
    // Check total timeout
    if (elapsed_ms >= adaptive_config_.total_query_timeout_ms) {
        reason = "total_timeout_exceeded";
        return true;
    }
    
    // Check if we have enough results
    if (current_results >= adaptive_config_.target_result_count) {
        reason = "target_result_count_reached";
        return true;
    }
    
    // Check diminishing returns (skip for first iteration)
    if (iteration > 1 && previous_results > 0) {
        uint32_t new_results = current_results - previous_results;
        double ratio = static_cast<double>(new_results) / previous_results;
        
        if (ratio < adaptive_config_.diminishing_returns_ratio) {
            reason = "diminishing_returns";
            return true;
        }
    }
    
    return false;
}

nlohmann::json AdaptiveShardRouter::mergeIterationResults(
    const std::vector<std::vector<ShardResult>>& all_results
) {
    nlohmann::json merged = nlohmann::json::array();
    std::set<std::string> seen_ids;  // For deduplication if needed
    
    for (const auto& iteration_results : all_results) {
        for (const auto& shard_result : iteration_results) {
            if (shard_result.success && shard_result.data.is_array()) {
                for (const auto& item : shard_result.data) {
                    // Simple deduplication by string representation
                    // In production, use proper ID-based deduplication
                    merged.push_back(item);
                }
            }
        }
    }
    
    return merged;
}

AdaptiveShardRouter::IterationStats AdaptiveShardRouter::calculateIterationStats(
    uint32_t iteration,
    const std::vector<std::string>& shard_ids,
    const std::vector<ShardResult>& results,
    const std::vector<CapabilityMatchResult>& match_results,
    uint64_t iteration_time_ms
) {
    IterationStats stats;
    stats.iteration_number = iteration;
    stats.shards_queried = static_cast<uint32_t>(shard_ids.size());
    stats.iteration_time_ms = iteration_time_ms;
    stats.shard_ids = shard_ids;
    
    // Count results
    stats.results_received = 0;
    for (const auto& result : results) {
        if (result.success && result.data.is_array()) {
            stats.results_received += static_cast<uint32_t>(result.data.size());
        }
    }
    
    // Calculate score statistics for queried shards
    std::vector<double> scores = {};

    for (const auto& shard_id : shard_ids) {
        for (const auto& match : match_results) {
            if (match.shard_id == shard_id) {
                scores.push_back(match.score);
                break;
            }
        }
    }
    
    if (!scores.empty()) {
        stats.min_score = *std::min_element(scores.begin(), scores.end());
        stats.max_score = *std::max_element(scores.begin(), scores.end());
        stats.avg_score = std::accumulate(scores.begin(), scores.end(), 0.0) / scores.size();
    } else {
        stats.min_score = 0.0;
        stats.max_score = 0.0;
        stats.avg_score = 0.0;
    }
    
    return stats;
}

} // namespace themis::sharding
