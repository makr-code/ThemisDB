/**
 * @file federated_search.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "search/federated_search.h"
#include "utils/logger.h"
#include <algorithm>
#include <stdexcept>

namespace themis {

// ============================================================================
// Construction
// ============================================================================

FederatedSearch::FederatedSearch()
    : FederatedSearch(Config{}) {}

FederatedSearch::FederatedSearch(const Config& config) : config_(config) {
    if (config_.k == 0) {
        throw std::invalid_argument("FederatedSearch: k must be > 0");
    }
    if (config_.rrf_k <= 0.0) {
        throw std::invalid_argument("FederatedSearch: rrf_k must be > 0");
    }
}

// ============================================================================
// Tenant management
// ============================================================================

void FederatedSearch::registerTenant(const std::string& tenant_id,
                                     HybridSearch* hybrid_search) {
    tenants_[tenant_id] = hybrid_search;
}

void FederatedSearch::removeTenant(const std::string& tenant_id) {
    tenants_.erase(tenant_id);
    tenant_weights_.erase(tenant_id);
}

void FederatedSearch::setTenantWeight(const std::string& tenant_id,
                                      double weight) {
    // Clamp to [0, 1]
    tenant_weights_[tenant_id] = std::max(0.0, std::min(1.0, weight));
}

double FederatedSearch::getTenantWeight(const std::string& tenant_id) const {
    auto it = tenant_weights_.find(tenant_id);
    return (it != tenant_weights_.end()) ? it->second : 1.0;
}

// ============================================================================
// setConfig
// ============================================================================

void FederatedSearch::setConfig(const Config& config) {
    if (config.k == 0) {
        throw std::invalid_argument("FederatedSearch: k must be > 0");
    }
    if (config.rrf_k <= 0.0) {
        throw std::invalid_argument("FederatedSearch: rrf_k must be > 0");
    }
    config_ = config;
}

// ============================================================================
// search
// ============================================================================

std::vector<FederatedSearch::Result> FederatedSearch::search(
    const std::string& query,
    [[maybe_unused]] const std::vector<float>& vector_query,
    std::vector<TenantStats>* tenant_stats) {

    std::unordered_map<std::string, std::vector<HybridSearch::Result>>
        tenant_results;

    for (auto& [tid, hs] : tenants_) {
        TenantStats stats;
        stats.tenant_id = tid;

        const double weight = getTenantWeight(tid);
        if (weight == 0.0) {
            stats.skipped = true;
            if (tenant_stats) tenant_stats->push_back(stats);
            continue;
        }

        if (!hs) {
            if (!config_.skip_null_tenants) {
                THEMIS_ERROR("FederatedSearch: tenant '{}' has null HybridSearch",
                             tid);
            }
            stats.skipped = true;
            if (tenant_stats) tenant_stats->push_back(stats);
            continue;
        }

        try {
            // Set vector_query on a per-tenant search call if non-empty
            auto results = hs->search(query);
            stats.results_count = results.size();
            tenant_results[tid] = std::move(results);
        } catch (const std::exception& e) {
            THEMIS_ERROR("FederatedSearch: tenant '{}' search failed: {}",
                         tid, e.what());
            stats.skipped = true;
        } catch (...) {
            THEMIS_ERROR("FederatedSearch: tenant '{}' search failed with unknown error",
                         tid);
            stats.skipped = true;
        }
        if (tenant_stats) tenant_stats->push_back(stats);
    }

    return mergeTenantResults(tenant_results);
}

// ============================================================================
// mergeTenantResults
// ============================================================================

std::vector<FederatedSearch::Result> FederatedSearch::mergeTenantResults(
    const std::unordered_map<std::string,
                              std::vector<HybridSearch::Result>>& tenant_results)
    const {

    // Accumulate RRF scores per (tenant, doc) pair
    // Score formula: score += weight_t / (rrf_k + rank)
    struct Accumulator {
        double score = 0.0;
        double bm25_score = 0.0;
        double vector_score = 0.0;
        std::string tenant_id;
        std::string document_id;
    };

    std::unordered_map<std::string, Accumulator> accum; // key = "tenant_id\ndoc_id"

    for (const auto& [tid, results] : tenant_results) {
        const double weight = getTenantWeight(tid);
        for (size_t rank = 0; rank < results.size(); ++rank) {
            const auto& r = results[rank];
            if (r.document_id.empty()) continue;

            const std::string key = tid + '\n' + r.document_id;
            auto& acc = accum[key];
            acc.tenant_id   = tid;
            acc.document_id = r.document_id;
            acc.bm25_score  = r.bm25_score;
            acc.vector_score = r.vector_score;
            acc.score += weight / (config_.rrf_k + static_cast<double>(rank + 1));
        }
    }

    // Build result list and sort by score descending
    std::vector<Result> merged;
    merged.reserve(accum.size());
    for (const auto& [key, acc] : accum) {
        Result res;
        res.document_id  = acc.document_id;
        res.tenant_id    = acc.tenant_id;
        res.score        = acc.score;
        res.bm25_score   = acc.bm25_score;
        res.vector_score = acc.vector_score;
        merged.push_back(std::move(res));
    }

    std::sort(merged.begin(), merged.end(),
              [](const Result& a, const Result& b) {
                  return a.score > b.score;
              });

    if (merged.size() > config_.k) {
        merged.resize(config_.k);
    }
    return merged;
}

} // namespace themis

