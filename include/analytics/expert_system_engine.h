/**
 * @file expert_system_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

    // ── KnowledgeBase access ──────────────────────────────────────────────────

    /**
     * Replace the underlying KnowledgeBase.
     * Must not be called concurrently with any other method.
     */
    void setKnowledgeBase(std::shared_ptr<KnowledgeBase> kb);

    /**
     * Direct access to the KnowledgeBase (for programmatic rule loading, etc.).
     */
    [[nodiscard]] KnowledgeBase& knowledgeBase();

    // ── Working Memory ────────────────────────────────────────────────────────

    /**
     * Assert a fact into the working memory.
     * @return Assigned fact id.
     */
    [[nodiscard]] std::string assertFact(const std::string& subject,
                                          const std::string& predicate,
                                          const std::string& object);

    /**
     * Retract a fact by id.
     * @return true iff found.
     */
    bool retractFact(const std::string& fact_id);

    // ── Inference ─────────────────────────────────────────────────────────────

    /**
     * Forward chaining to fixpoint (Rete-like: repeat until no new facts).
     *
     * @param max_cycles Upper bound on iterations (prevents infinite loops).
     * @return Total number of rule firings across all cycles.
     */
    [[nodiscard]] int forwardChain(int max_cycles = 100);

    /**
     * Backward chaining: prove whether the given goal triple is satisfiable.
     *
     * Uses depth-limited search (DLS) up to Config::max_backward_chain_depth.
     *
     * @param goal  TriplePattern to prove (may contain variable elements).
     * @return      GoalResult with success flag and proof trace.
     */
    [[nodiscard]] GoalResult queryGoal(const TriplePattern& goal);

    // ── Explanation ───────────────────────────────────────────────────────────

    /**
     * Return a JSON proof trace for the given derived fact_id.
     *
     * Format: JSON array of ProofStep objects.
     * Returns "[]" if the fact_id was not derived by forwardChain().
     */
    [[nodiscard]] std::string explain(const std::string& fact_id) const;

    // ── ML Scorer ─────────────────────────────────────────────────────────────

    /**
     * Inject a ModelServingEngine as ML scorer.
     * When set, rule firing is gated on the scorer's confidence prediction.
     *
     * @param scorer       Pointer to a live ModelServingEngine (not owned).
     * @param model_name   Name of the registered model to call.
     * @param model_version Version string.
     */
    void setMLScorer(ModelServingEngine* scorer,
                     const std::string& model_name   = "expert_scorer",
                     const std::string& model_version = "v1");

    /**
     * Inject a custom confidence function (for testing without a full
     * ModelServingEngine).  Overrides any scorer set via setMLScorer().
     *
     * Signature: double(const HornClause&, const vector<Fact>& matched)
     * Return value must be in [0.0, 1.0].
     */
    void setMLScorerFn(ScorerFn fn);

    // ── State queries ─────────────────────────────────────────────────────────

    [[nodiscard]] std::size_t factCount() const;
    [[nodiscard]] std::size_t ruleCount() const;

private:
    using Bindings = std::unordered_map<std::string, std::string>;

    /**
     * Try to match all conditions of a rule against the current WM.
     * @return Bindings (variable→literal) if all conditions match, nullopt otherwise.
     */
    [[nodiscard]] std::optional<Bindings> matchConditions(
        const HornClause&          rule,
        const std::vector<Fact>&   all_facts) const;

    /**
     * Recursively match conditions starting at index cond_idx with existing bindings.
     */
    [[nodiscard]] bool matchConditionsRec(
        const std::vector<TriplePattern>& conditions,
        std::size_t                        cond_idx,
        const std::vector<Fact>&           all_facts,
        Bindings&                          bindings) const;

    /**
     * Collect ALL possible complete binding sets for a rule's conditions against
     * the current working memory. Each element of the returned vector represents
     * one fully-grounded variable assignment.
     */
    [[nodiscard]] std::vector<Bindings> matchAllConditions(
        const HornClause&        rule,
        const std::vector<Fact>& all_facts) const;

    /** Recursive helper for matchAllConditions. */
    void matchAllBindingsRec(
        const std::vector<TriplePattern>& conditions,
        std::size_t                        cond_idx,
        const std::vector<Fact>&           all_facts,
        Bindings&                          current,
        std::vector<Bindings>&             results) const;

    /** Apply bindings to a pattern element. */
    [[nodiscard]] static std::string applyBinding(const std::string& elem,
                                                   const Bindings&    b);

    /** Check if the triple (s, p, o) already exists in WM. */
    [[nodiscard]] bool factExists(const std::string& s,
                                   const std::string& p,
                                   const std::string& o) const;

    /** Return ML confidence for a rule + matched facts (1.0 if no scorer set).
     *  Must NOT be called while mutex_ is held (scorer may call back into the
     *  engine; see lock-under-callback fix in forwardChain). */
    [[nodiscard]] static double mlConfidenceNoLock(
        ModelServingEngine*       scorer,
        const ScorerFn&           scorer_fn,
        const std::string&        model_name,
        const std::string&        model_ver,
        const HornClause&         rule,
        const std::vector<Fact>&  matched);

    /** Backward chaining DLS. Appends steps to trace. */
    [[nodiscard]] bool backwardChainDLS(
        const TriplePattern&    goal,
        std::vector<ProofStep>& trace,
        int                     depth,
        int                     max_depth) const;

    /** Check whether a concrete triple (s,p,o) is already in WM (no variables). */
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
