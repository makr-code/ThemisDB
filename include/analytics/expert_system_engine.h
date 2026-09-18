/**
 * @file expert_system_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ExpertSystemEngine — ThemisDB analytics expert system.
 *
 * Provides forward chaining (to fixpoint) and backward chaining (DLS)
 * over a KnowledgeBase of Horn clause rules.  An optional ML scorer
 * (ModelServingEngine* or injection function) gates rule firing on
 * confidence thresholds.
 *
 * Thread-safety: read-only methods (explain, factCount, ruleCount, queryGoal)
 * acquire a std::shared_lock so multiple readers may run concurrently.
 * Write methods (assertFact, retractFact, setMLScorer, forwardChain, …) acquire
 * a std::unique_lock.  forwardChain() releases the lock before invoking the
 * optional ML scorer callback to prevent re-entrancy deadlocks (items #41–45).
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "analytics/knowledge_base.h"
#include "analytics/model_serving.h"

namespace themisdb {
namespace analytics {

// ──────────────────────────────────────────────────────────────────────────────
// ProofStep — one step in a forward/backward proof trace
// ──────────────────────────────────────────────────────────────────────────────
struct ProofStep {
    std::string       rule_id;
    std::vector<Fact> matched_facts;  ///< Facts that satisfied the rule conditions
    Fact              derived_fact;   ///< Fact that was derived by this step
};

// ──────────────────────────────────────────────────────────────────────────────
// GoalResult — result of queryGoal()
// ──────────────────────────────────────────────────────────────────────────────
struct GoalResult {
    bool                    success    = false;
    std::vector<ProofStep>  proof_trace;
    int                     depth_used = 0;
};

// ──────────────────────────────────────────────────────────────────────────────
// ExpertSystemEngineConfig (hoisted outside class to allow default-argument use)
// ──────────────────────────────────────────────────────────────────────────────
struct ExpertSystemEngineConfig {
    int  max_forward_chain_cycles = 100; ///< Prevents infinite loops in FC
    int  max_backward_chain_depth = 10;  ///< DLS depth limit for BC
};

// ──────────────────────────────────────────────────────────────────────────────
// ExpertSystemEngine
// ──────────────────────────────────────────────────────────────────────────────
class ExpertSystemEngine {
public:
    using Config   = ExpertSystemEngineConfig;
    using ScorerFn = std::function<double(const HornClause&, const std::vector<Fact>&)>;

    explicit ExpertSystemEngine(Config cfg = Config{});
    ~ExpertSystemEngine() = default;

    ExpertSystemEngine(const ExpertSystemEngine&)            = delete;
    ExpertSystemEngine& operator=(const ExpertSystemEngine&) = delete;

    /**
     * @brief ── KnowledgeBase access ──────────────────────────────────────────────────
     * @param[in] kb Input parameter.
     */

    void setKnowledgeBase(std::shared_ptr<KnowledgeBase> kb);

    [[nodiscard]] KnowledgeBase& knowledgeBase();

    // ── Working Memory ────────────────────────────────────────────────────────

    [[nodiscard]] std::string assertFact(const std::string& subject,
                                          const std::string& predicate,
                                          const std::string& object);

    /**
     * @brief Retract Fact.
     * @param[in] fact_id Identifier of the fact.
     * @return True when the operation succeeds.
     */
    bool retractFact(const std::string& fact_id);

    // ── Inference ─────────────────────────────────────────────────────────────

    [[nodiscard]] int forwardChain(int max_cycles = 100);

    [[nodiscard]] GoalResult queryGoal(const TriplePattern& goal);

    // ── Explanation ───────────────────────────────────────────────────────────

    [[nodiscard]] std::string explain(const std::string& fact_id) const;

    // ── ML Scorer ─────────────────────────────────────────────────────────────

    void setMLScorer(ModelServingEngine* scorer,
                     const std::string& model_name   = "expert_scorer",
                     const std::string& model_version = "v1");

    /**
     * @brief Set MLScorer Fn.
     * @param[in] fn Input parameter.
     */
    void setMLScorerFn(ScorerFn fn);

    // ── State queries ─────────────────────────────────────────────────────────

    [[nodiscard]] std::size_t factCount() const;
    [[nodiscard]] std::size_t ruleCount() const;

private:
    using Bindings = std::unordered_map<std::string, std::string>;

    [[nodiscard]] std::optional<Bindings> matchConditions(
        const HornClause&          rule,
        const std::vector<Fact>&   all_facts) const;

    [[nodiscard]] bool matchConditionsRec(
        const std::vector<TriplePattern>& conditions,
        std::size_t                        cond_idx,
        const std::vector<Fact>&           all_facts,
        Bindings&                          bindings) const;

    [[nodiscard]] std::vector<Bindings> matchAllConditions(
        const HornClause&        rule,
        const std::vector<Fact>& all_facts) const;

    /**
     * @brief Match All Bindings Rec.
     * @param[in] conditions Input parameter.
     * @param[in] cond_idx Input parameter.
     * @param[in] all_facts Input parameter.
     * @param[in,out] current Input/output parameter.
     * @param[in,out] results Input/output parameter.
     */
    void matchAllBindingsRec(
        const std::vector<TriplePattern>& conditions,
        std::size_t                        cond_idx,
        const std::vector<Fact>&           all_facts,
        Bindings&                          current,
        std::vector<Bindings>&             results) const;

    [[nodiscard]] static std::string applyBinding(const std::string& elem,
                                                   const Bindings&    b);

    [[nodiscard]] bool factExists(const std::string& s,
                                   const std::string& p,
                                   const std::string& o) const;

    [[nodiscard]] static double mlConfidenceNoLock(
        ModelServingEngine*       scorer,
        const ScorerFn&           scorer_fn,
        const std::string&        model_name,
        const std::string&        model_ver,
        const HornClause&         rule,
        const std::vector<Fact>&  matched);

    [[nodiscard]] bool backwardChainDLS(
        const TriplePattern&    goal,
        std::vector<ProofStep>& trace,
        int                     depth,
        int                     max_depth) const;

    [[nodiscard]] bool tripleInWM(const std::string& s,
                                   const std::string& p,
                                   const std::string& o) const;

    Config                         cfg_;
    std::shared_ptr<KnowledgeBase> kb_;
    mutable std::shared_mutex      mutex_;  ///< shared for reads, unique for writes

    // fact_id → proof steps that derived it (for explain())
    std::unordered_map<std::string, std::vector<ProofStep>> decision_log_;

    ModelServingEngine* ml_scorer_         = nullptr;
    std::string         ml_model_name_;
    std::string         ml_model_version_;
    ScorerFn            ml_scorer_fn_;     // optional function override
};

} // namespace analytics
} // namespace themisdb
