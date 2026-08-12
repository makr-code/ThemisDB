/**
 * @file federated_search.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "search/hybrid_search.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {

/**
 * @brief Federated search across isolated per-tenant HybridSearch indexes.
 *
 * FederatedSearch allows a single query to be executed concurrently across
 * multiple tenant-isolated `HybridSearch` instances and returns a merged,
 * globally ranked result set.
 *
 * ### Design
 *
 * Each tenant is represented by a named `HybridSearch` instance (or null for
 * an unknown tenant).  Results from each tenant are labelled with the
 * `tenant_id` and optionally weighted before cross-tenant Reciprocal Rank
 * Fusion (RRF) produces the final ranked list.
 *
 * ### Privacy isolation
 *
 * Results from each tenant index are completely isolated: a result from
 * tenant A cannot be boosted or suppressed by data from tenant B.  The
 * `Result::tenant_id` field makes the provenance of every result explicit.
 *
 * ### Tenant weights
 *
 * Tenants can be assigned a `double` weight in [0, 1] via `setTenantWeight()`.
 * A weight of 0 excludes the tenant from the merged result.  The default
 * weight for unregistered tenants is 1.0.
 *
 * ### Typical usage
 * ```cpp
 * FederatedSearch::Config cfg;
 * cfg.k = 20;
 * FederatedSearch fs(cfg);
 *
 * fs.registerTenant("tenant_A", &hs_a);
 * fs.registerTenant("tenant_B", &hs_b);
 * fs.setTenantWeight("tenant_A", 0.8);
 *
 * auto results = fs.search("machine learning");
 * for (const auto& r : results) {
 *     std::cout << r.tenant_id << "/" << r.document_id << " " << r.score << "\n";
 * }
 * ```
 *
 * @note Thread Safety: A single FederatedSearch instance is NOT thread-safe.
 *   Calls to `registerTenant()`, `setTenantWeight()`, and `search()` must not
 *   occur concurrently.
 * @note Exception Safety: `search()` never throws; all per-tenant exceptions
 *   are caught internally.  The constructor throws `std::invalid_argument`
 *   on invalid config values.
 *
 * @since v2.4.0 (Phase 5 — Federated Search)
 */
class FederatedSearch {
public:
    // -----------------------------------------------------------------------
    // Types
    // -----------------------------------------------------------------------

    /**
     * @brief Engine configuration.
     */
    struct Config {
        /// Final merged result count across all tenants.
        size_t k = 10;

        /// RRF smoothing constant for cross-tenant result fusion.
        double rrf_k = 60.0;

        /// When true, tenants with null HybridSearch pointers are silently
        /// skipped.  When false, a null pointer causes an error log entry.
        bool skip_null_tenants = true;
    };

    /**
     * @brief A single federated search result enriched with tenant metadata.
     */
    struct Result {
        std::string document_id;  ///< Primary key within the tenant index
        std::string tenant_id;    ///< Originating tenant identifier
        double score = 0.0;       ///< Fused relevance score
        double bm25_score = 0.0;  ///< Raw BM25 score from the tenant index
        double vector_score = 0.0; ///< Raw vector score from the tenant index
    };

    /**
     * @brief Per-tenant diagnostics returned alongside search results.
     */
    struct TenantStats {
        std::string tenant_id;    ///< Tenant identifier
        size_t results_count = 0; ///< Results returned by this tenant
        bool skipped = false;     ///< True when tenant was skipped (null/weight=0)
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @brief Construct a FederatedSearch engine.
     *
     * @throws std::invalid_argument on invalid config values.
     */
    explicit FederatedSearch();
    /**
     * @brief Construct a FederatedSearch engine.
     *
     * @param config  Engine configuration.
     * @throws std::invalid_argument on invalid config values.
     */
    explicit FederatedSearch(const Config& config);

    // -----------------------------------------------------------------------
    // Tenant management
    // -----------------------------------------------------------------------

    /**
     * @brief Register a tenant HybridSearch instance.
     *
     * Replaces any previously registered instance for the same tenant ID.
     * The pointer is non-owning; the caller retains ownership.
     *
     * @param tenant_id     Unique tenant identifier.
     * @param hybrid_search Pointer to the tenant's HybridSearch engine.
     *                      May be null; the tenant will be skipped during search.
     */
    void registerTenant(const std::string& tenant_id,
                        HybridSearch* hybrid_search);

    /**
     * @brief Remove a previously registered tenant.
     *
     * No-op when the tenant ID is not registered.
     *
     * @param tenant_id  Tenant to remove.
     */
    void removeTenant(const std::string& tenant_id);

    /**
     * @brief Set the contribution weight for a tenant.
     *
     * Weights are multiplied into the tenant's per-document RRF contribution
     * before cross-tenant fusion.  A weight of 0.0 effectively excludes the
     * tenant.  Weights are clamped to [0.0, 1.0].
     *
     * @param tenant_id  Target tenant.
     * @param weight     Contribution weight in [0.0, 1.0].
     */
    void setTenantWeight(const std::string& tenant_id, double weight);

    /**
     * @brief Return the weight associated with a tenant (default 1.0).
     */
    double getTenantWeight(const std::string& tenant_id) const;

    /**
     * @brief Return the number of registered tenants.
     */
    size_t tenantCount() const { return tenants_.size(); }

    // -----------------------------------------------------------------------
    // Search
    // -----------------------------------------------------------------------

    /**
     * @brief Execute a federated search across all registered tenants.
     *
     * Queries each non-skipped tenant `HybridSearch` instance in sequence,
     * applies per-tenant weights, merges results via Reciprocal Rank Fusion,
     * and returns the top-k globally ranked results.
     *
     * Per-tenant diagnostics are written to @p tenant_stats when non-null.
     *
     * Never throws; all per-tenant exceptions are caught internally.
     *
     * @param query         Full-text query string.
     * @param vector_query  Optional semantic embedding vector.
     * @param tenant_stats  Optional: per-tenant diagnostics output.
     * @return Top-k merged results sorted by score descending.
     */
    std::vector<Result> search(
        const std::string& query,
        const std::vector<float>& vector_query = {},
        std::vector<TenantStats>* tenant_stats = nullptr
    );

    /**
     * @brief Merge per-tenant result lists via weighted RRF (public for tests).
     *
     * @param tenant_results  Map of tenant_id to result list.
     * @return Merged results sorted by score descending, capped at Config::k.
     */
    std::vector<Result> mergeTenantResults(
        const std::unordered_map<std::string,
                                  std::vector<HybridSearch::Result>>& tenant_results
    ) const;

    // -----------------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------------

    const Config& getConfig() const { return config_; }
    void setConfig(const Config& config);

private:
    Config config_;
    std::unordered_map<std::string, HybridSearch*> tenants_;
    std::unordered_map<std::string, double> tenant_weights_;
};

} // namespace themis
