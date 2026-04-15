/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adversarial_tester.h                               ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 18:04:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     358                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8964e83dc5  2026-03-12  Add updater class integration and GitHub Actions for plug... ║
    • 67ce513a63  2026-03-12  fix(rag): Address adversarial tester review comments ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file adversarial_tester.h
 * @brief Adversarial Robustness Testing for RAG systems.
 *
 * Systematically tests RAG robustness against adversarial inputs, including
 * query perturbations, document poisoning, prompt injection, context overflow,
 * and sycophantic queries.
 *
 * Corresponds to roadmap item 10 "Adversarial Robustness Testing" (v1.18.0).
 *
 * ## Usage
 * ```cpp
 * #include "rag/adversarial_tester.h"
 *
 * using namespace themis::rag::adversarial;
 *
 * AdversarialTesterConfig cfg;
 * AdversarialTester tester(cfg);
 *
 * // Supply base queries and documents
 * tester.addBaseQuery("What is the capital of France?", "Paris");
 * tester.addBaseDocument({"doc1", "France is a country in Western Europe.", 1.0, {}});
 *
 * // Run full robustness suite
 * RAGJudge judge;
 * auto report = tester.testRobustness(judge);
 * std::cout << "Robustness score: " << report.robustness_score << "\n";
 * for (const auto& v : report.vulnerabilities) {
 *     std::cout << "  Vulnerability: " << v << "\n";
 * }
 * ```
 */

#pragma once

#include "rag/rag_judge.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace themis::rag::adversarial {

using judge::EvaluationInput;
using judge::EvaluationResult;
using judge::RAGJudge;
using judge::RetrievedDocument;

// ============================================================================
// Adversarial strategy types
// ============================================================================

/**
 * @brief Strategy used to generate adversarial queries.
 */
enum class AdversarialStrategy {
    SEMANTIC_PERTURBATION, ///< Rephrase without changing meaning
    LEXICAL_SUBSTITUTION,  ///< Swap synonyms or related words
    TYPO_INJECTION,        ///< Introduce common spelling mistakes
    NEGATION_FLIP,         ///< Add/remove negation to change semantics
    SYCOPHANCY,            ///< Frame as leading/biased question
};

/**
 * @brief Category of a detected adversarial vulnerability.
 */
enum class VulnerabilityCategory {
    QUERY_INSTABILITY,    ///< Score varies greatly for minor query changes
    DOCUMENT_POISONING,   ///< Poisoned documents affect faithfulness
    PROMPT_INJECTION,     ///< Injection payload detected in documents
    CONTEXT_OVERFLOW,     ///< Long contexts degrade answer quality
    SYCOPHANCY,           ///< System agrees with incorrect leading premise
};

// ============================================================================
// Data structures
// ============================================================================

/**
 * @brief A base query together with its expected (ground-truth) answer.
 */
struct BaseQuery {
    std::string query;          ///< Original query text
    std::string expected_answer; ///< Ground-truth answer (may be empty)
};

/**
 * @brief A single adversarial example – the original input, its perturbed
 *        variant, and the evaluation results for both.
 */
struct AdversarialExample {
    std::string           original_query;    ///< Unmodified query
    std::string           perturbed_query;   ///< Adversarially modified query
    AdversarialStrategy   strategy;          ///< Strategy that produced the perturbation
    EvaluationResult      original_result;   ///< Judge result for original
    EvaluationResult      perturbed_result;  ///< Judge result for perturbed variant
    double                score_delta;       ///< |original_score − perturbed_score|
};

/**
 * @brief Summary of a document-poisoning experiment.
 */
struct PoisoningResult {
    std::string original_doc_id;     ///< ID of the poisoned document
    std::string poison_payload;      ///< Injected content
    double      faithfulness_before; ///< Faithfulness before poisoning
    double      faithfulness_after;  ///< Faithfulness after poisoning
    bool        attack_succeeded;    ///< true if faithfulness dropped below threshold
};

/**
 * @brief Full robustness report returned by AdversarialTester::testRobustness().
 */
struct RobustnessReport {
    /// Aggregate score in [0, 1]. Higher is more robust.
    double robustness_score = 1.0;

    /// Human-readable description of each detected vulnerability.
    std::vector<std::string> vulnerabilities;

    /// All adversarial query examples that caused score instability
    /// (does not include sycophancy examples, which have their own field).
    std::vector<AdversarialExample> failing_examples;

    /// Per-document poisoning results (one entry per poisoned document).
    std::vector<PoisoningResult> poisoning_results;

    /// Number of prompt-injection payloads submitted during testing.
    /// These are attempts, not confirmed successful injections.
    size_t prompt_injection_attempts = 0;

    /// Whether context-overflow degraded quality beyond the threshold.
    bool context_overflow_detected = false;

    /// Whether sycophantic framing shifted scores beyond the threshold.
    bool sycophancy_detected = false;
};

// ============================================================================
// Configuration
// ============================================================================

/**
 * @brief Configuration for AdversarialTester.
 */
struct AdversarialTesterConfig {
    // ── Query perturbation ────────────────────────────────────────────────
    /// Score delta threshold above which a perturbation counts as a failure.
    double score_instability_threshold = 0.3;

    /// Number of perturbations to generate per base query and strategy.
    size_t perturbations_per_query = 3;

    // ── Document poisoning ────────────────────────────────────────────────
    /// Faithfulness drop threshold (absolute) to declare a poisoning attack
    /// successful.
    double poisoning_faithfulness_threshold = 0.2;

    // ── Context overflow ─────────────────────────────────────────────────
    /// Number of filler documents appended when testing context overflow.
    size_t context_overflow_padding_docs = 20;

    /// Minimum overall-score drop to declare context-overflow degradation.
    double context_overflow_score_threshold = 0.15;

    // ── Sycophancy ────────────────────────────────────────────────────────
    /// Score delta above which a sycophantic framing counts as a failure.
    double sycophancy_score_threshold = 0.25;

    // ── Strategies to test ────────────────────────────────────────────────
    /// Adversarial strategies included in the full test run.
    std::vector<AdversarialStrategy> enabled_strategies = {
        AdversarialStrategy::SEMANTIC_PERTURBATION,
        AdversarialStrategy::LEXICAL_SUBSTITUTION,
        AdversarialStrategy::TYPO_INJECTION,
        AdversarialStrategy::NEGATION_FLIP,
        AdversarialStrategy::SYCOPHANCY,
    };
};

// ============================================================================
// AdversarialTester
// ============================================================================

/**
 * @brief Systematically tests a RAGJudge against adversarial inputs.
 *
 * The tester is populated with base queries and documents, then
 * AdversarialTester::testRobustness() orchestrates five attack categories:
 *
 *  1. **Query perturbations** – generates variants with
 *     AdversarialStrategy and measures score stability.
 *  2. **Document poisoning** – injects misleading content into retrieved
 *     documents and measures faithfulness degradation.
 *  3. **Prompt injection** – places known injection payloads inside
 *     documents and tests whether they affect evaluation scores.
 *  4. **Context overflow** – pads retrieved documents to exceed typical
 *     context budgets and checks for quality degradation.
 *  5. **Sycophancy** – applies biased/presupposed query framing and
 *     measures score shift.
 *
 * Thread-safety: the tester is **not** thread-safe.  Create one instance
 * per thread or add external synchronisation.
 */
class AdversarialTester {
public:
    /**
     * @brief Construct with optional configuration.
     */
    explicit AdversarialTester(const AdversarialTesterConfig& config = AdversarialTesterConfig{});

    ~AdversarialTester();

    // Non-copyable, movable
    AdversarialTester(const AdversarialTester&)            = delete;
    AdversarialTester& operator=(const AdversarialTester&) = delete;
    AdversarialTester(AdversarialTester&&) noexcept;
    AdversarialTester& operator=(AdversarialTester&&) noexcept;

    // ── Population ────────────────────────────────────────────────────────

    /**
     * @brief Add a base query (and optional expected answer) to the test set.
     */
    void addBaseQuery(const std::string& query,
                      const std::string& expected_answer = {});

    /**
     * @brief Add a base document available to the judge during evaluation.
     */
    void addBaseDocument(const RetrievedDocument& document);

    /**
     * @brief Replace the entire base-query set.
     */
    void setBaseQueries(const std::vector<BaseQuery>& queries);

    /**
     * @brief Replace the entire base-document set.
     */
    void setBaseDocuments(const std::vector<RetrievedDocument>& documents);

    // ── Generation helpers (public so callers can inspect generated inputs) ──

    /**
     * @brief Generate perturbed query variants for a single query.
     * @param query     Original query text.
     * @param strategy  Perturbation strategy to apply.
     * @param count     Number of variants to produce.
     * @return          Vector of perturbed query strings.
     */
    std::vector<std::string> generatePerturbedQueries(
        const std::string&  query,
        AdversarialStrategy strategy,
        size_t              count) const;

    /**
     * @brief Create a poisoned copy of @p documents by injecting
     *        misleading content into one or more documents.
     * @param documents Original document set.
     * @return          Documents with injected poison payload.
     */
    std::vector<RetrievedDocument> generatePoisonedDocuments(
        const std::vector<RetrievedDocument>& documents) const;

    /**
     * @brief Generate a sycophantic rephrasing of @p query that presupposes
     *        an incorrect or biased premise.
     */
    std::string generateSycophancyQuery(const std::string& query) const;

    // ── Testing ───────────────────────────────────────────────────────────

    /**
     * @brief Run the full adversarial robustness suite.
     *
     * Executes all four attack categories and returns a consolidated
     * RobustnessReport.  Requires at least one base query and one base
     * document to have been added.
     *
     * @param judge  RAGJudge instance used for all evaluations.
     * @return       Consolidated robustness report.
     */
    RobustnessReport testRobustness(RAGJudge& judge);

    /**
     * @brief Test only query perturbation stability.
     */
    void testQueryPerturbations(RAGJudge& judge, RobustnessReport& report);

    /**
     * @brief Test only document-poisoning robustness.
     */
    void testDocumentPoisoning(RAGJudge& judge, RobustnessReport& report);

    /**
     * @brief Test only prompt-injection resilience.
     */
    void testPromptInjection(RAGJudge& judge, RobustnessReport& report);

    /**
     * @brief Test only context-overflow degradation.
     */
    void testContextOverflow(RAGJudge& judge, RobustnessReport& report);

    /**
     * @brief Test only sycophancy susceptibility.
     */
    void testSycophancy(RAGJudge& judge, RobustnessReport& report);

    // ── Helpers ───────────────────────────────────────────────────────────

    /**
     * @brief Return true if the attack on @p original succeeded: i.e. the
     *        adversarial answer differs substantially from the baseline.
     *
     * Compares @p original_answer with @p adversarial_answer using a simple
     * word-overlap divergence metric.
     */
    bool isSuccessfulAttack(const std::string& original_answer,
                            const std::string& adversarial_answer) const;

    /**
     * @brief Return the current configuration.
     */
    AdversarialTesterConfig getConfig() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace themis::rag::adversarial
