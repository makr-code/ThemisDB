#pragma once

/**
 * @file model_router.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/llm_plugin_interface.h"

#include <regex>
#include <shared_mutex>
#include <string>
#include <vector>

namespace themis {
namespace llm {

struct RoutingRule {
    /**
     * @brief Routing Rule.
     * @return Return value.
     */
    virtual ~RoutingRule() = default;
    std::string id;

    int priority = 0;

    std::string target_model_id;

    std::vector<std::string> prompt_patterns;

    std::vector<std::string> metadata_tags;

    enum class MatchMode {
        ANY, ///< Rule matches when at least one pattern or tag matches.
        ALL  ///< Rule matches only when every pattern and every tag matches.
    };
    MatchMode match_mode = MatchMode::ANY;

    std::string description;
};

struct RoutingResult {
    std::string model_id;
    std::string rule_id;
    bool matched = false;
};

class ModelRouter {
public:
    ModelRouter() = default;

    /**
     * @brief Add Rule.
     * @param[in] rule Input parameter.
     */
    void addRule(const RoutingRule& rule);

    /**
     * @brief Remove Rule.
     * @param[in] rule_id Identifier of the rule.
     * @return True when the operation succeeds.
     */
    bool removeRule(const std::string& rule_id);

    /**
     * @brief Get Rules.
     * @return Return value.
     */
    std::vector<RoutingRule> getRules() const;

    /**
     * @brief Clear Rules.
     */
    void clearRules();

    /**
     * @brief Route.
     * @param[in] prompt Input parameter.
     * @param[in] metadata Input parameter.
     * @return Return value.
     */
    RoutingResult route(const std::string& prompt,
                        const nlohmann::json& metadata) const;

    /**
     * @brief Rule Count.
     * @return Return value.
     */
    size_t ruleCount() const;

private:
    struct CompiledRule {
        RoutingRule             rule;
        std::vector<std::regex> compiled_patterns;
    };

    /**
     * @brief Compile Patterns.
     * @param[in] rule Input parameter.
     * @return Return value.
     */
    static std::vector<std::regex> compilePatterns(const RoutingRule& rule);

    /**
     * @brief Evaluate.
     * @param[in] cr Input parameter.
     * @param[in] prompt Input parameter.
     * @param[in] tags Input parameter.
     * @return True when the operation succeeds.
     */
    static bool evaluate(const CompiledRule& cr,
                         const std::string& prompt,
                         const std::vector<std::string>& tags);

    mutable std::mutex mutex_;
    std::vector<CompiledRule> rules_;
};

} // namespace llm
} // namespace themis
