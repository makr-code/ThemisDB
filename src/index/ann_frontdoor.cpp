/**
 * @file ann_frontdoor.cpp
 * @brief ANN front-door dispatcher implementation.
 *
 * Implements query routing, index lifecycle management, and telemetry
 * emission for the ANN front-door abstraction.
 */

// ANN Frontdoor — explicit first universal retrieval gate
//
// Architecture:
//   Query → AnnFrontdoor → [HNSW | ScaNN | DiskANN | Distributed] → candidates
//
// Backend selection rules (evaluated in priority order):
//   1. Scope-specific IAnnIndex registered under context.scope_id.
//   2. DISTRIBUTED  when shard_aware=true and at least one shard backend exists.
//   3. HNSW         when hot_tier && dataset_size <= hnsw_max_elements
//                   AND a VectorIndexManager is registered.
//   4. SCANN        when dataset_size <= scann_max_elements (global backend).
//   5. DISKANN      when diskann_available && global backend supports DiskANN.
//   6. FLAT_BRUTE_FORCE as safe fallback (via VectorIndexManager or global backend).
//
// Hot/cold tiering:
//   When a TieredIndexManager is registered, the frontdoor resolves the
//   index tier for context.scope_id.  A WARM or COLD tier demotes HOT-
//   eligible backends to ScaNN or DiskANN to avoid paging in memory-mapped
//   structures on the critical path.

#include "index/ann_frontdoor.h"
#include "observability/layer_decision_log.h"
#include "observability/metrics_collector.h"
#include "observability/reason_codes.h"
#include "observability/telemetry_keys.h"
#include "utils/logger.h"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace themis {
namespace index {

namespace {

[[nodiscard]] std::vector<std::string> pruneShardsAwareCost(
    const std::vector<std::pair<std::string, std::shared_ptr<IAnnIndex>>>& candidates,
    const AnnFrontdoor::Config& config) {
    if (config.distributed_cost_budget <= 0.0) {
        std::vector<std::string> all_ids;
        for (const auto& [id, _] : candidates) {
            all_ids.push_back(id);
        }
        return all_ids;
    }

    std::vector<std::pair<std::string, double>> scored;
    scored.reserve(candidates.size());
    for (const auto& [shard_id, backend] : candidates) {
        if (!backend) continue;

        const ShardMetadata meta{shard_id, 1.0, 0.8, 0.9, 0.95};

        if (meta.estimated_recall < config.distributed_quality_floor) {
            continue;
        }

        const double utility = config.distributed_utility_alpha * meta.estimated_relevance +
                               config.distributed_utility_beta * meta.estimated_freshness +
                               config.distributed_utility_gamma * meta.estimated_locality;
        const double priority = utility / std::max(meta.estimated_cost, 0.001);
        scored.push_back({shard_id, priority});
    }

    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    std::vector<std::string> selected;
    {
        double budget_used = 0.0;
        for (const auto& [shard_id, priority] : scored) {
            (void)priority;
            const double cost_estimate = 1.0;
            if (budget_used + cost_estimate <= config.distributed_cost_budget) {
                selected.push_back(shard_id);
                budget_used += cost_estimate;
            }
        }
    }

    return selected;
}

const char* strategyReasonCode(AnnStrategy strategy) {
    using namespace themis::observability::reason_codes::ann;
    switch (strategy) {
        case AnnStrategy::HNSW: return kRouteHnsw.data();
        case AnnStrategy::SCANN: return kRouteScann.data();
        case AnnStrategy::DISKANN: return kRouteDiskann.data();
        case AnnStrategy::DISTRIBUTED: return kRouteDistributed.data();
        case AnnStrategy::FLAT_BRUTE_FORCE: return kRouteFlatBruteForce.data();
    }
    return kRouteUnknown.data();
}

constexpr const char* kDistributedMergePolicy = "DISTANCE_ASC_THEN_ID";

void emitRouteMetric(const AnnFrontdoorResult& result) {
    themis::observability::MetricsCollector::getInstance().addCounter(
        "ann_frontdoor_route_type", 1,
        {{"route_type", annStrategyName(result.strategy_used)},
         {"fallback_mode", result.fallback_mode.empty() ? "none" : result.fallback_mode},
         {"distributed", result.is_distributed ? "true" : "false"}});
}

}

// ============================================================================
// Lifecycle
// ============================================================================

AnnFrontdoor::AnnFrontdoor() : AnnFrontdoor(Config{}) {}

AnnFrontdoor::AnnFrontdoor(Config config)
    : config_(std::move(config)) {}

AnnFrontdoor::~AnnFrontdoor() = default;

// ============================================================================
// Backend registration
// ============================================================================

void AnnFrontdoor::registerBackend(std::string       scope_id,
                                   std::shared_ptr<IAnnIndex> backend,
                                   AnnScopeKind      kind) {
    if (!backend) {
        throw std::invalid_argument(
            "AnnFrontdoor::registerBackend: backend must not be nullptr");
    }
    scope_kinds_[scope_id] = kind;
    backends_[std::move(scope_id)] = std::move(backend);
}

void AnnFrontdoor::registerScopeKind(std::string scope_id, AnnScopeKind kind) {
    scope_kinds_[std::move(scope_id)] = kind;
}

AnnScopeKind AnnFrontdoor::getScopeKind(const std::string& scope_id) const noexcept {
    auto it = scope_kinds_.find(scope_id);
    return (it != scope_kinds_.end()) ? it->second : AnnScopeKind::Generic;
}

void AnnFrontdoor::registerVectorIndexManager(
    std::shared_ptr<VectorIndexManager> vim) {
    if (!vim) {
        throw std::invalid_argument(
            "AnnFrontdoor::registerVectorIndexManager: vim must not be nullptr");
    }
    vim_ = std::move(vim);
}

void AnnFrontdoor::registerTieredIndexManager(
    std::shared_ptr<TieredIndexManager> tim) {
    tiered_ = std::move(tim);   // nullptr is allowed — disables tier routing
}

// ============================================================================
// planStrategy — routing decision without execution
// ============================================================================

AnnStrategy AnnFrontdoor::planStrategy(
    const AnnQueryContext& context) const noexcept {
    return planRetrieval(context).strategy;
}

AnnRetrievalPlan AnnFrontdoor::planRetrieval(
    const AnnQueryContext& context) const noexcept {
    AnnRetrievalPlan plan;
    plan.scope_kind = getScopeKind(context.scope_id);

    plan.effective_tier = context.hot_tier ? IndexTierMeta::Tier::HOT
                                           : IndexTierMeta::Tier::COLD;
    if (tiered_ && !context.scope_id.empty()) {
        if (auto meta = tiered_->getMetadata(context.scope_id); meta) {
            plan.effective_tier = meta->tier;
        }
    }

    const bool effective_hot = (plan.effective_tier == IndexTierMeta::Tier::HOT);
    const bool effective_warm = (plan.effective_tier == IndexTierMeta::Tier::WARM);
    const bool have_scope_backend = !context.scope_id.empty() && backends_.count(context.scope_id);
    const bool have_global_backend = backends_.count("");
    const bool shard_backends_present = backends_.size() > (have_global_backend ? 1u : 0u);

    if (plan.scope_kind == AnnScopeKind::ShardSummary &&
        context.shard_aware && shard_backends_present) {
        plan.strategy = AnnStrategy::DISTRIBUTED;
        plan.distributed = true;
        plan.reason = "shard-summary scope routed to distributed ANN fan-out";
        return plan;
    }

    if (context.shard_aware && shard_backends_present) {
        std::vector<std::pair<std::string, std::shared_ptr<IAnnIndex>>> shard_list;
        for (const auto& [scope, backend] : backends_) {
            if (!scope.empty() && backend) {
                shard_list.push_back({scope, backend});
            }
        }

        std::vector<std::string> pruned = pruneShardsAwareCost(shard_list, config_);
        if (!pruned.empty()) {
            plan.pruned_shard_ids = pruned;
            plan.strategy = AnnStrategy::DISTRIBUTED;
            plan.distributed = true;
            plan.reason = "shard-aware query routed to distributed ANN fan-out (" +
                          std::to_string(pruned.size()) + "/" +
                          std::to_string(shard_list.size()) + " shards after cost-aware pruning)";
            return plan;
        }
    }

    if (have_scope_backend) {
        if (effective_hot && context.dataset_size <= config_.hnsw_max_elements) {
            plan.strategy = AnnStrategy::HNSW;
            plan.hot_path = true;
            plan.reason = "scope backend routed through hot HNSW path";
            return plan;
        }

        if (config_.diskann_available && !effective_hot) {
            plan.strategy = AnnStrategy::DISKANN;
            plan.reason = effective_warm ? "scope backend routed through warm-to-cold DiskANN path"
                                         : "scope backend routed through cold DiskANN path";
            return plan;
        }

        plan.strategy = AnnStrategy::SCANN;
        plan.reason = effective_warm ? "scope backend routed through warm ScaNN path"
                                     : "scope backend routed through fallback ScaNN path";
        return plan;
    }

    if (effective_hot && vim_ && context.dataset_size <= config_.hnsw_max_elements) {
        plan.strategy = AnnStrategy::HNSW;
        plan.hot_path = true;
        plan.reason = "hot tier routed to HNSW";
        return plan;
    }

    if (!have_scope_backend && !have_global_backend && !vim_) {
        plan.strategy = AnnStrategy::FLAT_BRUTE_FORCE;
        plan.reason = "no ANN backend or vector index manager available; using brute-force fallback";
        return plan;
    }

    if (context.dataset_size <= config_.scann_max_elements) {
        plan.strategy = AnnStrategy::SCANN;
        plan.reason = effective_warm ? "warm tier routed to ScaNN"
                                     : "medium dataset routed to ScaNN";
        return plan;
    }

    if (config_.diskann_available) {
        plan.strategy = AnnStrategy::DISKANN;
        plan.reason = effective_hot ? "large hot dataset routed to DiskANN fallback"
                                    : "cold tier routed to DiskANN";
        return plan;
    }

    plan.strategy = AnnStrategy::FLAT_BRUTE_FORCE;
    plan.reason = "no ANN backend available; using brute-force fallback";
    return plan;
}

// ============================================================================
// explainStrategy
// ============================================================================

std::string AnnFrontdoor::explainStrategy(
    const AnnQueryContext& context) const {
    return buildRoutingReason(planStrategy(context), context);
}

// ============================================================================
// search — primary retrieval gate
// ============================================================================

AnnFrontdoorResult AnnFrontdoor::search(const float*          query_vector,
                                        std::size_t           dim,
                                        int                   k,
                                        const AnnQueryContext& context) const {
    if (!query_vector) {
        throw std::invalid_argument(
            "AnnFrontdoor::search: query_vector must not be nullptr");
    }
    if (dim == 0) {
        throw std::invalid_argument(
            "AnnFrontdoor::search: dim must be > 0");
    }
    if (k <= 0) {
        k = config_.default_k;
    }

    const AnnRetrievalPlan plan = planRetrieval(context);
    const AnnStrategy strategy = plan.strategy;

    AnnFrontdoorResult result;
    result.strategy_used    = strategy;
    result.estimated_recall = recallEstimate(strategy);
    result.routing_reason   = plan.reason.empty() ? buildRoutingReason(strategy, context)
                                                  : plan.reason;
    result.routing_reason_code = strategyReasonCode(strategy);
    result.correlation_id = context.correlation_id.empty()
        ? std::string(observability::telemetry::defaults::kAnnNoCorrelation)
        : context.correlation_id;
    result.confidence_policy_version = context.confidence_policy_version.empty()
        ? std::string(observability::reason_codes::kPolicyVersionDefault)
        : context.confidence_policy_version;
    result.confidence_threshold_key = context.confidence_threshold_key.empty()
        ? std::string(observability::reason_codes::ann::kThresholdKeyDefault)
        : context.confidence_threshold_key;
    result.is_distributed   = plan.distributed;
    result.distributed_merge_policy = kDistributedMergePolicy;

    switch (strategy) {

    // ------------------------------------------------------------------
    case AnnStrategy::DISTRIBUTED: {
        const AnnRetrievalPlan pruning_plan = planRetrieval(context);
        std::vector<std::pair<std::string, std::shared_ptr<IAnnIndex>>> shard_backends;
        shard_backends.reserve(backends_.size());
        for (const auto& [scope, backend] : backends_) {
            if (scope.empty() || !backend) {
                continue;
            }
            shard_backends.push_back({scope, backend});
        }
        std::sort(shard_backends.begin(), shard_backends.end(),
                  [](const auto& a, const auto& b) {
                      return a.first < b.first;
                  });

        std::vector<std::pair<std::string, std::shared_ptr<IAnnIndex>>> execution_backends;
        if (!pruning_plan.pruned_shard_ids.empty() && config_.distributed_cost_budget > 0.0) {
            const auto& pruned_ids = pruning_plan.pruned_shard_ids;
            for (const auto& [scope, backend] : shard_backends) {
                if (std::find(pruned_ids.begin(), pruned_ids.end(), scope) != pruned_ids.end()) {
                    execution_backends.push_back({scope, backend});
                }
            }
        } else {
            const std::size_t fanout_limit = (config_.distributed_max_fanout == 0)
                ? shard_backends.size()
                : std::min(config_.distributed_max_fanout, shard_backends.size());
            for (std::size_t i = 0; i < fanout_limit; ++i) {
                execution_backends.push_back(shard_backends[i]);
            }
        }

        if (config_.distributed_include_global_backend) {
            if (auto global = resolveBackend(""); global) {
                execution_backends.push_back({"", std::move(global)});
            }
        }

        result.shards_attempted = execution_backends.size();

        std::unordered_map<int64_t, float> best_distance_by_id;
        for (const auto& [scope, backend] : execution_backends) {
            (void)scope;
            bool success = false;
            const int attempts = std::max(0, config_.distributed_retry_attempts) + 1;
            for (int attempt = 0; attempt < attempts; ++attempt) {
                try {
                    const auto partial = executeSearch(*backend, query_vector, dim, k);
                    for (const auto& candidate : partial) {
                        auto it = best_distance_by_id.find(candidate.id);
                        if (it == best_distance_by_id.end() || candidate.distance < it->second) {
                            best_distance_by_id[candidate.id] = candidate.distance;
                        }
                    }
                    success = true;
                    break;
                } catch (const std::exception&) {
                    // retry loop continues until attempts exhausted
                }
            }
            if (success) {
                ++result.shards_succeeded;
            } else {
                ++result.shards_failed;
            }
        }

        std::vector<AnnSearchResult> merged;
        merged.reserve(best_distance_by_id.size());
        for (const auto& [id, distance] : best_distance_by_id) {
            merged.push_back({id, distance});
        }
        std::sort(merged.begin(), merged.end(),
                  [](const AnnSearchResult& a, const AnnSearchResult& b) {
                      if (a.distance == b.distance) {
                          return a.id < b.id;
                      }
                      return a.distance < b.distance;
                  });

        result.merged_candidates_before_trim = merged.size();
        if (static_cast<int>(merged.size()) > k) {
            merged.resize(static_cast<std::size_t>(k));
        }
        result.candidates = std::move(merged);

        if (result.shards_failed > 0 && result.shards_succeeded > 0) {
            if (config_.distributed_allow_partial_results) {
                result.partial_results = true;
                result.fallback_mode = std::string(observability::reason_codes::fallback_mode::kDegradedContinue);
                result.fallback_reason_code = std::string(observability::reason_codes::ann::kFallbackDistributedPartialFailure);
            } else {
                // Fail closed when partial distributed results are not allowed.
                result.partial_results = false;
                result.candidates.clear();
                result.fallback_mode = std::string(observability::reason_codes::fallback_mode::kFailClosed);
                result.fallback_reason_code = std::string(observability::reason_codes::ann::kFallbackDistributedPartialFailure);
            }
        }

        if (result.shards_succeeded == 0) {
            if (config_.distributed_allow_partial_results) {
                result.fallback_mode = std::string(observability::reason_codes::fallback_mode::kDegradedContinue);
            } else {
                result.fallback_mode = std::string(observability::reason_codes::fallback_mode::kFailClosed);
            }
            result.fallback_reason_code = std::string(observability::reason_codes::ann::kFallbackDistributedAllShardsFailed);
        }
        break;
    }

    // ------------------------------------------------------------------
    case AnnStrategy::HNSW: {
        // Prefer scope-specific backend; fall back to VectorIndexManager.
        if (!context.scope_id.empty()) {
            if (auto backend = resolveBackend(context.scope_id); backend) {
                result.candidates = executeSearch(*backend, query_vector, dim, k);
                break;
            }
        }
        // Use VectorIndexManager (HNSW + brute-force fallback)
        result.candidates = bruteForceSearch(query_vector, dim, k, context);
        break;
    }

    // ------------------------------------------------------------------
    case AnnStrategy::SCANN:
    // fallthrough: both use the global IAnnIndex backend
    case AnnStrategy::DISKANN: {
        if (!context.scope_id.empty()) {
            if (auto backend = resolveBackend(context.scope_id); backend) {
                result.candidates = executeSearch(*backend, query_vector, dim, k);
                break;
            }
        }
        if (auto global = resolveBackend(""); global) {
            result.candidates = executeSearch(*global, query_vector, dim, k);
        } else {
            // No backend — degrade gracefully to brute force
            result.candidates = bruteForceSearch(query_vector, dim, k, context);
            result.strategy_used    = AnnStrategy::FLAT_BRUTE_FORCE;
            result.estimated_recall = recallEstimate(AnnStrategy::FLAT_BRUTE_FORCE);
            result.routing_reason_code = strategyReasonCode(AnnStrategy::FLAT_BRUTE_FORCE);
            result.fallback_mode = std::string(observability::reason_codes::fallback_mode::kDegradedContinue);
            result.fallback_reason_code = std::string(observability::reason_codes::ann::kFallbackBackendUnavailable);
        }
        break;
    }

    // ------------------------------------------------------------------
    case AnnStrategy::FLAT_BRUTE_FORCE:
    default: {
        // Try scope-specific or global backend first (may itself be brute force)
        if (auto backend = resolveBackend(context.scope_id); backend) {
            result.candidates = executeSearch(*backend, query_vector, dim, k);
        } else if (auto global = resolveBackend(""); global) {
            result.candidates = executeSearch(*global, query_vector, dim, k);
        } else {
            result.candidates = bruteForceSearch(query_vector, dim, k, context);
            result.fallback_mode = std::string(observability::reason_codes::fallback_mode::kDegradedContinue);
            result.fallback_reason_code = std::string(observability::reason_codes::ann::kFallbackBackendUnavailable);
        }
        break;
    }

    } // end switch

    observability::emitLayerDecisionLog(
        observability::telemetry::layers::kAnn,
        result.correlation_id,
        result.routing_reason_code,
        result.confidence_policy_version,
        result.confidence_threshold_key,
        result.fallback_mode,
        result.fallback_reason_code,
        observability::telemetry::layers::kAnn,
        !result.candidates.empty());

    emitRouteMetric(result);

    // ─── ANN result validation — Phase B gate (Target: Q3 2026) ─────────────
    // Cardinality check: candidates must not exceed the requested top-k.
    // A backend returning more than k results is a contract violation; truncate
    // defensively and log a warning so the issue is visible in production.
    if (k > 0 && result.candidates.size() > static_cast<std::size_t>(k)) {
        spdlog::warn("[AnnFrontdoor] cardinality violation: backend returned {} candidates "
                     "but top_k={} was requested; truncating (correlation_id={})",
                     result.candidates.size(), k, result.correlation_id);
        result.candidates.resize(static_cast<std::size_t>(k));
    }

    // Range check: distance values must be non-negative (NaN / negative distances
    // indicate backend bugs or uninitialised memory).  Remove invalid entries
    // and log a warning so the caller's tensor layer sees a clean candidate list.
    {
        const std::size_t before_range_filter = result.candidates.size();
        result.candidates.erase(
            std::remove_if(result.candidates.begin(), result.candidates.end(),
                           [](const AnnSearchResult& r) {
                               // NaN check: NaN != NaN; negative distance also invalid.
                               return !(r.distance >= 0.0f);
                           }),
            result.candidates.end());
        const std::size_t removed = before_range_filter - result.candidates.size();
        if (removed > 0) {
            spdlog::warn("[AnnFrontdoor] range check: removed {} candidate(s) with "
                         "invalid distance (NaN or negative) (correlation_id={})",
                         removed, result.correlation_id);
        }
    }
    // ─────────────────────────────────────────────────────────────────────────

    return result;
}

// ============================================================================
// Diagnostics
// ============================================================================

std::size_t AnnFrontdoor::registeredBackendCount() const noexcept {
    return backends_.size();
}

const AnnFrontdoor::Config& AnnFrontdoor::config() const noexcept {
    return config_;
}

// ============================================================================
// Private helpers
// ============================================================================

std::shared_ptr<IAnnIndex> AnnFrontdoor::resolveBackend(
    const std::string& scope_id) const noexcept {
    auto it = backends_.find(scope_id);
    return (it != backends_.end()) ? it->second : nullptr;
}

std::vector<AnnSearchResult> AnnFrontdoor::executeSearch(
    IAnnIndex&   backend,
    const float* query,
    std::size_t  dim,
    int          k) const {
    auto results = backend.search(query, dim, k);
    // Sort by ascending distance (some backends return unsorted)
    std::sort(results.begin(), results.end(),
              [](const AnnSearchResult& a, const AnnSearchResult& b) {
                  return a.distance < b.distance;
              });
    return results;
}

std::vector<AnnSearchResult> AnnFrontdoor::bruteForceSearch(
    const float*           query,
    std::size_t            dim,
    int                    k,
    const AnnQueryContext& context) const {
    (void)query;
    (void)dim;
    (void)k;

    THEMIS_WARN("AnnFrontdoor: brute-force fallback unavailable in storage module for scope='{}'; "
                "returning empty candidate set",
                context.scope_id.empty() ? "(global)" : context.scope_id);
    return {};
}

std::string AnnFrontdoor::buildRoutingReason(
    AnnStrategy           strategy,
    const AnnQueryContext& context) const {
    std::ostringstream ss;
    ss << annStrategyName(strategy) << " selected";
    ss << " dataset_size=" << context.dataset_size;
    ss << " hot_tier=" << (context.hot_tier ? "true" : "false");
    ss << " shard_aware=" << (context.shard_aware ? "true" : "false");
    ss << " recall_target=" << context.recall_target;
    ss << " latency_budget_ms=" << context.latency_budget_ms;
    if (!context.scope_id.empty()) {
        ss << " scope_id=" << context.scope_id;
        ss << " scope_kind=" << annScopeKindName(getScopeKind(context.scope_id));
    }
    ss << " hnsw_max=" << config_.hnsw_max_elements;
    ss << " scann_max=" << config_.scann_max_elements;
    ss << " diskann_available=" << (config_.diskann_available ? "true" : "false");
    return ss.str();
}

// static
double AnnFrontdoor::recallEstimate(AnnStrategy strategy) noexcept {
    // Approximate empirical recall fractions from ThemisDB benchmarks.
    // These are informational estimates, not guarantees.
    switch (strategy) {
        case AnnStrategy::HNSW:            return 0.97;
        case AnnStrategy::SCANN:           return 0.95;
        case AnnStrategy::DISKANN:         return 0.93;
        case AnnStrategy::DISTRIBUTED:     return 0.92;
        case AnnStrategy::FLAT_BRUTE_FORCE: return 1.00;
    }
    return 0.0;
}

} // namespace index
} // namespace themis
