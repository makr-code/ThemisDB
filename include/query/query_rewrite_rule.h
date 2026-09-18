/**
 * @file query_rewrite_rule.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis {
namespace query {

struct RewriteContext {
    std::unordered_map<std::string, size_t> collection_row_counts;

    size_t or_to_in_threshold = 3;

    bool enable_constant_folding = true;
};

struct RewriteStats {
    size_t rules_applied = 0;

    std::vector<std::string> applied_rule_names;

    size_t total_transformations = 0;
};

class IQueryRewriteRule {
public:
    /**
     * @brief IQuery Rewrite Rule.
     * @return Return value.
     */
    virtual ~IQueryRewriteRule() = default;

    [[nodiscard]] virtual std::string name() const = 0;

    [[nodiscard]] virtual bool applies(const nlohmann::json& plan, const RewriteContext& ctx) const = 0;

    [[nodiscard]] virtual size_t apply(nlohmann::json& plan, const RewriteContext& ctx) const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Concrete rules
// ─────────────────────────────────────────────────────────────────────────────

class PredicatePushdownRule : public IQueryRewriteRule {
public:
    ~PredicatePushdownRule() override = default;
    std::string name() const override { return "PredicatePushdown"; }
    bool applies(const nlohmann::json& plan, const RewriteContext& ctx) const override;
    size_t apply(nlohmann::json& plan, const RewriteContext& ctx) const override;
};

class ProjectionPushdownRule : public IQueryRewriteRule {
public:
    ~ProjectionPushdownRule() override = default;
    std::string name() const override { return "ProjectionPushdown"; }
    bool applies(const nlohmann::json& plan, const RewriteContext& ctx) const override;
    size_t apply(nlohmann::json& plan, const RewriteContext& ctx) const override;
};

class OrToInRewriteRule : public IQueryRewriteRule {
public:
    ~OrToInRewriteRule() override = default;
    std::string name() const override { return "OrToIn"; }
    bool applies(const nlohmann::json& plan, const RewriteContext& ctx) const override;
    size_t apply(nlohmann::json& plan, const RewriteContext& ctx) const override;
};

class ConstantFoldingRule : public IQueryRewriteRule {
public:
    ~ConstantFoldingRule() override = default;
    std::string name() const override { return "ConstantFolding"; }
    bool applies(const nlohmann::json& plan, const RewriteContext& ctx) const override;
    size_t apply(nlohmann::json& plan, const RewriteContext& ctx) const override;
};

class CommonSubexpressionRule : public IQueryRewriteRule {
public:
    ~CommonSubexpressionRule() override = default;
    std::string name() const override { return "CommonSubexpressionElimination"; }
    bool applies(const nlohmann::json& plan, const RewriteContext& ctx) const override;
    size_t apply(nlohmann::json& plan, const RewriteContext& ctx) const override;
};

// ─────────────────────────────────────────────────────────────────────────────
// Pipeline
// ─────────────────────────────────────────────────────────────────────────────

class QueryRewritePipeline {
public:
    static constexpr size_t kDefaultMaxIterations = 5;

    explicit QueryRewritePipeline(size_t max_iterations = kDefaultMaxIterations);

    /**
     * @brief Add Rule.
     * @param[in] rule Input parameter.
     */
    void addRule(std::shared_ptr<IQueryRewriteRule> rule);

    /**
     * @brief Clear Rules.
     */
    void clearRules();

    /**
     * @brief Rule Count.
     * @return Return value.
     */
    size_t ruleCount() const;

    /**
     * @brief Run.
     * @param[in,out] plan Input/output parameter.
     * @param[in] ctx Input parameter.
     * @return Return value.
     */
    RewriteStats run(nlohmann::json& plan, const RewriteContext& ctx) const;

    /**
     * @brief Create Default.
     * @return Return value.
     */
    static QueryRewritePipeline createDefault();

private:
    std::vector<std::shared_ptr<IQueryRewriteRule>> rules_;
    size_t max_iterations_{kDefaultMaxIterations};
};

} // namespace query
} // namespace themis
