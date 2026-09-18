/**
 * @file ethics_selection_router.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ethics_ai/ethics_profile_registry.h"
#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <map>

namespace themis {
namespace plugins {
namespace ethics {

// ============================================================================
// LDM-1 — Layered Discourse Model mode selector
// ============================================================================

enum class DiscourseMode : uint8_t {
    SELECTION_ONLY = 0,

    LAYERED_FAST = 1,

    LAYERED_FULL = 2,
};

struct DiscourseOrchestratorPlan {
    DiscourseMode mode{DiscourseMode::SELECTION_ONLY};

    std::vector<std::string> ebene1_school_ids;

    double initial_weight{1.0};

    std::map<std::string, std::vector<std::string>> cluster_map;

    std::vector<std::string> tension_axes;

    bool empty() const noexcept { return ebene1_school_ids.empty(); }
};

struct RouterConfig {
    std::string taxonomy_yaml_path;

    size_t top_n{5};

    size_t stage2_top_k{10};

    double weight_semantic{0.40};
    double weight_precedent{0.40};
    double weight_taxonomy{0.20};

    std::string deployment_context;

    // =========================================================================
    // LDM-1 additions
    // =========================================================================

    DiscourseMode discourse_mode{DiscourseMode::SELECTION_ONLY};

    std::map<std::string, double> school_bias;

    std::string chain_visualizer_output_path;

    // =========================================================================
    // LDM hardening additions (Q4 2026)
    // =========================================================================

    double conflict_threshold_ratio{0.6};

    bool snapshot_profile_on_round_start{true};
};

struct RouterCandidate {
    std::string school_id;
    double taxonomy_score{0.0};  ///< 1.0 = direct class match, 0.5 = domain mapping
    double semantic_score{0.0};  ///< Term-overlap similarity in [0, 1]
    double precedent_dc{0.0};    ///< Historical DC score from precedent store
    double final_score{0.0};
};

struct RouterResult {
    std::vector<RouterCandidate> selected;
    size_t stage1_count{0};
    size_t stage2_count{0};
    size_t stage3_count{0};
};

class EthicsSelectionRouter {
public:
    using EmbeddingFn = std::function<std::vector<float>(const std::string& text)>;

    using PrecedentQueryFn = std::function<double(const std::string& dilemma_domain,
                                                   const std::string& school_id)>;

    EthicsSelectionRouter(IEthicsProfileRegistry* registry,
                          const RouterConfig& config = {});

    ~EthicsSelectionRouter();

    // Non-copyable, movable
    EthicsSelectionRouter(const EthicsSelectionRouter&)            = delete;
    EthicsSelectionRouter& operator=(const EthicsSelectionRouter&) = delete;
    EthicsSelectionRouter(EthicsSelectionRouter&&)                 noexcept = default;
    EthicsSelectionRouter& operator=(EthicsSelectionRouter&&)      noexcept = default;

    DiscourseOrchestratorPlan planDiscourse(
        const std::string& domain_context = {}) const;

    RouterResult route(
        const std::string& dilemma_text,
        const std::string& dilemma_domain,
        const std::vector<std::string>& dilemma_tags = {},
        bool regulatory_context = false) const;

    /**
     * @brief Record Decision Outcome.
     * @param[in] dilemma_type Input parameter.
     * @param[in] school_id Identifier of the school.
     * @param[in] dc_score Input parameter.
     */
    void recordDecisionOutcome(const std::string& dilemma_type,
                               const std::string& school_id,
                               double dc_score);

    /**
     * @brief Config.
     * @return Return value.
     */
    const RouterConfig& config() const;

    /**
     * @brief Set Embedding Fn.
     * @param[in] fn Input parameter.
     */
    void setEmbeddingFn(EmbeddingFn fn);

    /**
     * @brief Set Precedent Query Fn.
     * @param[in] fn Input parameter.
     */
    void setPrecedentQueryFn(PrecedentQueryFn fn);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
