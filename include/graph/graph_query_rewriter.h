/**
 * @file graph_query_rewriter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.9
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <memory>
#include <functional>
#include <unordered_map>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace graph {

struct GraphRewriteStats {
    size_t rules_applied = 0;

    std::vector<std::string> applied_rule_names;

    size_t total_transformations = 0;
};

class GraphQueryRewriter {
public:
    // ─── Rewrite rule enumeration ────────────────────────────────────────────

    enum class RewriteRule {
        PREDICATE_PUSHDOWN,   ///< Push filters into traversal prune conditions
        COMMON_SUBEXPRESSION, ///< Eliminate repeated identical sub-traversals
        JOIN_REORDERING,      ///< Reorder join operands by estimated selectivity
        MATERIALIZED_VIEW,    ///< Tag sub-graphs for materialisation
        QUERY_DECOMPOSITION,  ///< Decompose multi-start traversals for parallelism
        PRUNE_EARLY           ///< Alias – early branch pruning (= PREDICATE_PUSHDOWN)
    };

    // ─── Configuration ───────────────────────────────────────────────────────

    struct RewriteConfig {
        std::set<RewriteRule> enabled_rules;

        bool aggressive_optimization = false;

        double rewrite_time_limit_ms = 100.0;
    };

    // ─── Result type ─────────────────────────────────────────────────────────

    struct RewriteResult {
        nlohmann::json plan;    ///< Rewritten plan (may equal original if no rule fired)
        GraphRewriteStats stats; ///< Statistics about applied rules
    };

    // ─── Construction ────────────────────────────────────────────────────────

    explicit GraphQueryRewriter(const std::optional<RewriteConfig>& config = std::nullopt);

    /**
     * @brief ─── Core API ────────────────────────────────────────────────────────────
     * @param[in] plan Input parameter.
     * @return Return value.
     */

    RewriteResult rewrite(const nlohmann::json& plan) const;

    /**
     * @brief Explain Rewrites.
     * @param[in] original Input parameter.
     * @param[in] rewritten Input parameter.
     * @return Return value.
     */
    std::string explainRewrites(const nlohmann::json& original,
                                const nlohmann::json& rewritten) const;

    /**
     * @brief Estimate Speedup.
     * @param[in] original Input parameter.
     * @param[in] rewritten Input parameter.
     * @return Return value.
     */
    double estimateSpeedup(const nlohmann::json& original,
                           const nlohmann::json& rewritten) const;

    void addCustomRule(std::string_view name,
                       std::function<size_t(nlohmann::json&)> rule);

    /**
     * @brief Clear Custom Rules.
     */
    void clearCustomRules();

    const RewriteConfig& config() const { return config_; }

    // ─── Static helpers ──────────────────────────────────────────────────────

    static nlohmann::json makeTraversalPlan(
        std::string_view graph_id,
        std::string_view start_vertex,
        std::string_view direction = "OUTBOUND",
        int min_depth = 1,
        int max_depth = 1,
        nlohmann::json vertex_filters = nlohmann::json::array());

    /**
     * @brief Make Filter Scan Plan.
     * @param[in] filter Input parameter.
     * @param[in] child Input parameter.
     * @return Return value.
     */
    static nlohmann::json makeFilterScanPlan(nlohmann::json filter,
                                             nlohmann::json child);

    static nlohmann::json makeJoinPlan(nlohmann::json left,
                                       nlohmann::json right,
                                       std::string_view join_key = "vertex_id");

    static nlohmann::json makeMultiTraversalPlan(
        std::string_view graph_id,
        const std::vector<std::string>& start_vertices,
        std::string_view direction = "OUTBOUND",
        int max_depth = 1,
        nlohmann::json vertex_filters = nlohmann::json::array());

    /**
     * @brief Estimate Cardinality.
     * @param[in] node Input parameter.
     * @return Return value.
     */
    static double estimateCardinality(const nlohmann::json& node);

private:
    RewriteConfig config_;

    struct CustomRule {
        std::string name;
        std::function<size_t(nlohmann::json&)> fn;
    };
    std::vector<CustomRule> custom_rules_;

    // Maximum fixed-point iterations to avoid infinite loops.
    static constexpr size_t kMaxIterations = 5;

    /**
     * @brief ─── Individual rule implementations ─────────────────────────────────
     * @param[in,out] plan Input/output parameter.
     * @return Return value.
     */

    static size_t applyPredicatePushdown(nlohmann::json& plan);
    /**
     * @brief Apply Common Subexpression Elimination.
     * @param[in,out] plan Input/output parameter.
     * @return Return value.
     */
    static size_t applyCommonSubexpressionElimination(nlohmann::json& plan);
    /**
     * @brief Apply Join Reordering.
     * @param[in,out] plan Input/output parameter.
     * @return Return value.
     */
    static size_t applyJoinReordering(nlohmann::json& plan);
    /**
     * @brief Apply Materialized View.
     * @param[in,out] plan Input/output parameter.
     * @param[in] aggressive Input parameter.
     * @return Return value.
     */
    static size_t applyMaterializedView(nlohmann::json& plan, bool aggressive);
    /**
     * @brief Apply Query Decomposition.
     * @param[in,out] plan Input/output parameter.
     * @return Return value.
     */
    static size_t applyQueryDecomposition(nlohmann::json& plan);

    /**
     * @brief Is Enabled.
     * @param[in] rule Input parameter.
     * @return True when the operation succeeds.
     */
    bool isEnabled(RewriteRule rule) const;
};

} // namespace graph
} // namespace themis
