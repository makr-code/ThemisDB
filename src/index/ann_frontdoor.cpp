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
#include "utils/logger.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>

namespace themis {
namespace index {

// ============================================================================
// Lifecycle
// ============================================================================

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
        plan.strategy = AnnStrategy::DISTRIBUTED;
        plan.distributed = true;
        plan.reason = "shard-aware query routed to distributed ANN fan-out";
        return plan;
    }

    if (have_scope_backend) {
        if (effective_hot && vim_ && context.dataset_size <= config_.hnsw_max_elements) {
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
    result.is_distributed   = plan.distributed;

    switch (strategy) {

    // ------------------------------------------------------------------
    case AnnStrategy::DISTRIBUTED: {
        // Fan out to all registered shard backends, merge, and re-rank.
        std::vector<AnnSearchResult> merged;
        for (auto& [scope, backend] : backends_) {
            if (scope.empty()) continue;  // skip global backend
            auto partial = executeSearch(*backend, query_vector, dim, k);
            merged.insert(merged.end(), partial.begin(), partial.end());
        }
        // Also include global backend if present
        if (auto global = resolveBackend(""); global) {
            auto partial = executeSearch(*global, query_vector, dim, k);
            merged.insert(merged.end(), partial.begin(), partial.end());
        }
        // Sort merged by ascending distance and keep top-k
        std::sort(merged.begin(), merged.end(),
                  [](const AnnSearchResult& a, const AnnSearchResult& b) {
                      return a.distance < b.distance;
                  });
        if (static_cast<int>(merged.size()) > k) {
            merged.resize(static_cast<std::size_t>(k));
        }
        result.candidates = std::move(merged);
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
        }
        break;
    }

    } // end switch

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
        ss << " scope_kind=" << static_cast<int>(getScopeKind(context.scope_id));
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
