/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ingestion_quality_judge.h                          ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-04-15 18:45:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     564                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • db7df90e31  2026-04-15  feat(ingestion): Google Benchmarks QJ01–QJ11 + SoC/OOP do... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB – Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ingestion_quality_judge.h                          ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-15                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file ingestion_quality_judge.h
 * @brief LLM-as-judge quality control with runtime re-ingestion.
 *
 * Analogous to the re-retrieval loop in the RAG module
 * (`rag/agentic_rag.h`), this subsystem evaluates the quality of an
 * extraction result after the `WorkflowEngine` has run and, if quality
 * falls below configurable thresholds, triggers targeted re-ingestion of
 * the affected document at runtime — without stopping the database.
 *
 * ## Scientific Foundation
 *
 * The design is grounded in three lines of peer-reviewed research:
 *
 * **[1] LLM-as-judge** — Zheng et al., NeurIPS 2023
 *    "Judging LLM-as-a-Judge with MT-Bench and Chatbot Arena"
 *    https://arxiv.org/abs/2306.05685
 *    Establishes that a capable language model can serve as a reliable
 *    automated evaluator, replacing costly human annotation for extraction
 *    quality.  `IngestionQualityJudge::evaluate()` implements the
 *    single-model variant of this protocol.
 *
 * **[2] RAGAS** — Es et al., EACL 2024
 *    "RAGAS: Automated Evaluation of Retrieval Augmented Generation"
 *    https://arxiv.org/abs/2309.15217
 *    The four quality dimensions mirror RAGAS metrics directly:
 *      - COMPLETENESS        ≈ Answer Relevance
 *      - GROUNDEDNESS        ≈ Faithfulness
 *      - ENTITY_COVERAGE     ≈ Context Recall
 *      - RELATION_COHERENCE  ≈ Context Precision
 *    Threshold defaults (0.70–0.80) are calibrated against RAGAS
 *    acceptance criteria reported for German legal corpora.
 *
 * **[3] CRAG — Corrective Retrieval Augmented Generation** — Yan et al., ICLR 2024
 *    https://arxiv.org/abs/2401.15884
 *    Motivates the re-ingestion feedback loop in `ReIngestionController`:
 *    evaluate quality → if below threshold, apply corrective action
 *    (re-run targeted steps) → retry up to `max_reingestion_attempts`.
 *    The "corrective" step config derived from `report.recommended_steps`
 *    is the ThemisDB analog of CRAG's knowledge refinement action.
 *
 * ## Architecture
 *
 * @code
 *   WorkflowEngine::run(manifest)
 *         │
 *         ▼ ExtractionContext
 *   IngestionQualityJudge::evaluate()
 *         │
 *         ├─► passed  →  persist via IngestionSinkBundle
 *         │
 *         └─► failed  →  ReIngestionController::reIngest()
 *                              │ (adjusts StepConfig, re-runs engine)
 *                              │
 *                              └─► loop (max_reingestion_attempts)
 * @endcode
 *
 * ## Quality Dimensions
 *
 * | Dimension           | What is measured                              | RAGAS equivalent     |
 * |---------------------|-----------------------------------------------|----------------------|
 * | COMPLETENESS        | All relevant information extracted?           | Answer Relevance     |
 * | GROUNDEDNESS        | Every extraction traceable to source text?    | Faithfulness         |
 * | ENTITY_COVERAGE     | Named entities (persons, orgs, laws) found?   | Context Recall       |
 * | RELATION_COHERENCE  | Extracted relations semantically valid?       | Context Precision    |
 *
 * The LLM is consulted for each enabled dimension via a structured
 * prompt that asks for a score in [0, 1] plus a brief rationale.
 * Scores below the per-dimension threshold are flagged and translated
 * into actionable re-ingestion hints (which steps to re-run, which
 * entities seem missing).
 *
 * ## SoC / OOP Design (SOLID Principles)
 *
 * This subsystem is designed according to the SOLID principles:
 *
 * - **Single Responsibility (SRP)**:
 *     `IngestionQualityJudge` evaluates quality only.
 *     `ReIngestionController` manages the retry loop only.
 *     `IIngestionQualityObserver` handles observability only.
 *
 * - **Open / Closed (OCP)**:
 *     New quality dimensions can be added to `IngestionQualityDimension`
 *     and `IngestionJudgeConfig` without changing the judge's evaluate()
 *     core logic.
 *
 * - **Liskov Substitution (LSP)**:
 *     Any `ITextGenerationBackend` implementation (real LLM, stub, mock)
 *     can be injected without changing the caller.
 *
 * - **Interface Segregation (ISP)**:
 *     `IIngestionQualityObserver` is kept narrow (3 callbacks).
 *     Callers that need only evaluated-event notification subclass
 *     with empty bodies for the unused callbacks.
 *
 * - **Dependency Inversion (DIP)**:
 *     No `llm/` headers are included here.
 *     The LLM backend is injected via `ITextGenerationBackend`.
 *     `WorkflowEngine` is injected into `ReIngestionController`
 *     rather than constructed internally.
 *
 * ## Thread-Safety
 *
 * `IngestionQualityJudge::evaluate()` is thread-safe — it holds no
 * mutable state and delegates generation to the injected backend.
 * `ReIngestionController::process()` is NOT thread-safe: call it from
 * a single ingestion worker thread per document.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "ingestion/extraction_context.h"
#include "ingestion/inference_backend.h"
#include "ingestion/ingestion_step.h"
#include "ingestion/workflow_engine.h"

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace ingestion {

// ============================================================================
// IngestionQualityDimension
// ============================================================================

/**
 * @brief Quality evaluation dimensions for ingestion results.
 */
enum class IngestionQualityDimension {
    COMPLETENESS,        ///< All relevant information extracted from source text
    GROUNDEDNESS,        ///< Every extracted fact traceable to the source
    ENTITY_COVERAGE,     ///< Named entities (persons, orgs, laws, norms) identified
    RELATION_COHERENCE,  ///< Extracted relations are semantically consistent
    OVERALL              ///< Weighted combination of all active dimensions
};

// ============================================================================
// IngestionJudgeConfig
// ============================================================================

/**
 * @brief Configuration for IngestionQualityJudge.
 */
struct IngestionJudgeConfig {
    // ---- Per-dimension thresholds (0.0–1.0) ----
    /// Minimum completeness score before re-ingestion is triggered.
    double completeness_threshold     = 0.75;
    /// Minimum groundedness score.
    double groundedness_threshold     = 0.80;
    /// Minimum entity-coverage score.
    double entity_coverage_threshold  = 0.70;
    /// Minimum relation-coherence score.
    double relation_coherence_threshold = 0.70;
    /// Minimum overall (weighted) score.
    double overall_threshold          = 0.75;

    // ---- Dimension weights for overall score ----
    double completeness_weight        = 0.30;
    double groundedness_weight        = 0.35;
    double entity_coverage_weight     = 0.20;
    double relation_coherence_weight  = 0.15;

    // ---- LLM generation parameters ----
    int    max_tokens   = 512;   ///< Token budget per dimension prompt
    double temperature  = 0.05;  ///< Low temperature for deterministic scoring
    /// Optional LoRA adapter (empty = none; e.g. "legal-judge-lora-v1")
    std::string lora_adapter;

    // ---- Re-ingestion control ----
    /// Maximum re-ingestion passes per document (0 = evaluate only, no re-ingestion)
    int max_reingestion_attempts = 3;
    /// Workflow profile used for targeted re-ingestion passes.
    /// Empty = use same profile as the original ingestion.
    std::string reingestion_profile;

    // ---- Enable/disable dimensions ----
    bool evaluate_completeness         = true;
    bool evaluate_groundedness         = true;
    bool evaluate_entity_coverage      = true;
    bool evaluate_relation_coherence   = true;

    // ---- Minimum context requirements for evaluation ----
    /// Skip evaluation when raw_text is shorter than this (bytes).
    size_t min_text_bytes_for_eval = 100;
    /// Skip evaluation when fewer than this many entities were extracted.
    size_t min_entities_for_eval = 1;
};

// ============================================================================
// IngestionQualityReport
// ============================================================================

/**
 * @brief Result of one quality-judge evaluation pass.
 */
struct IngestionQualityReport {
    // ---- Identity ----
    std::string doc_id;        ///< manifest.source_uri or generated ID
    int         attempt = 0;   ///< 0 = initial ingestion, 1+ = re-ingestion pass

    // ---- Per-dimension scores (0.0–1.0; -1.0 = not evaluated) ----
    double completeness_score         = -1.0;
    double groundedness_score         = -1.0;
    double entity_coverage_score      = -1.0;
    double relation_coherence_score   = -1.0;
    double overall_score              = -1.0;

    // ---- Pass / fail ----
    /// True when overall_score ≥ IngestionJudgeConfig::overall_threshold
    /// AND all enabled dimensions meet their individual thresholds.
    bool passed = false;

    // ---- Actionable diagnostics ----
    /// Entity labels the LLM believes were missed in the extraction.
    std::vector<std::string> missing_entities;
    /// Claims in the extraction that could not be grounded in source text.
    std::vector<std::string> ungrounded_claims;
    /// Human-readable improvement suggestions from the LLM.
    std::vector<std::string> improvement_hints;
    /// Step plugin names recommended for re-execution (e.g. "builtin.ner_de").
    std::vector<std::string> recommended_steps;

    // ---- LLM rationale (short, for observability / audit) ----
    std::string completeness_rationale;
    std::string groundedness_rationale;
    std::string entity_coverage_rationale;
    std::string relation_coherence_rationale;

    // ---- Metadata ----
    std::chrono::milliseconds evaluation_time{0};
    std::string judge_backend;   ///< ITextGenerationBackend::describe()
};

// ============================================================================
// IIngestionQualityObserver
// ============================================================================

/**
 * @brief Observer hook for ingestion quality events.
 *
 * Implementations MUST be noexcept — all exceptions are swallowed by the
 * judge to protect the ingestion pipeline.
 */
class IIngestionQualityObserver {
public:
    virtual ~IIngestionQualityObserver() = default;

    /**
     * @brief Called after every quality evaluation (pass or fail).
     *
     * @param doc_id  Source URI or generated document identifier.
     * @param report  Full quality report for this pass.
     */
    virtual void onQualityEvaluated(const std::string&            doc_id,
                                    const IngestionQualityReport& report) noexcept = 0;

    /**
     * @brief Called just before a re-ingestion pass is launched.
     *
     * @param doc_id   Document being re-ingested.
     * @param attempt  1-based attempt counter.
     * @param reasons  Which dimension thresholds were not met.
     */
    virtual void onReIngestionTriggered(
        const std::string&              doc_id,
        int                             attempt,
        const std::vector<std::string>& reasons) noexcept = 0;

    /**
     * @brief Called after a re-ingestion pass finishes.
     *
     * @param doc_id   Document identifier.
     * @param attempt  1-based attempt counter.
     * @param improved True when the overall_score improved compared to the
     *                 previous pass; false when score stayed the same or fell.
     */
    virtual void onReIngestionComplete(const std::string& doc_id,
                                       int                attempt,
                                       bool               improved) noexcept = 0;
};

// ============================================================================
// IngestionQualityJudge
// ============================================================================

/**
 * @brief LLM-as-judge evaluator for ingestion extraction results.
 *
 * Builds structured prompts for each quality dimension, calls the injected
 * `ITextGenerationBackend`, parses the numeric score from the response, and
 * aggregates into an `IngestionQualityReport`.
 *
 * The judge is **stateless** between calls — `evaluate()` is thread-safe.
 *
 * Usage:
 * @code
 *   auto backend = std::make_shared<LlmIngestionBridge>(plugin_mgr);
 *   IngestionQualityJudge judge(backend);
 *
 *   auto report = judge.evaluate(ctx);
 *   if (!report.passed) {
 *       // re-ingest with report.recommended_steps as hints
 *   }
 * @endcode
 */
class IngestionQualityJudge {
public:
    /**
     * @brief Construct with an LLM backend and optional configuration.
     *
     * @param backend  Text generation backend (must not be null).
     * @param config   Judge configuration; defaults apply when omitted.
     */
    explicit IngestionQualityJudge(
        std::shared_ptr<ITextGenerationBackend> backend,
        IngestionJudgeConfig                    config = {});

    ~IngestionQualityJudge();

    // ---- Evaluation -------------------------------------------------------

    /**
     * @brief Evaluate the quality of an extraction result.
     *
     * Uses `ctx.raw_text` as the reference source for groundedness and
     * completeness checks.  If the backend is unavailable or the context
     * is too sparse (< `min_text_bytes_for_eval`), the report is returned
     * with all scores set to -1.0 and `passed = true` (fail-open).
     *
     * @param ctx  Completed extraction context from the workflow engine.
     * @return     Quality report with per-dimension scores and recommendations.
     */
    IngestionQualityReport evaluate(const ::themis::ingestion::ExtractionContext& ctx) const;

    /**
     * @brief Evaluate with an explicit source text.
     *
     * Use this when the canonical source text differs from `ctx.raw_text`
     * (e.g. when comparing against a pre-parsed reference copy).
     *
     * @param ctx         Extraction context.
     * @param source_text Reference text for groundedness/completeness checks.
     * @return            Quality report.
     */
    IngestionQualityReport evaluate(const ::themis::ingestion::ExtractionContext& ctx,
                                    const std::string&       source_text) const;

    // ---- Configuration ----------------------------------------------------

    const IngestionJudgeConfig& config() const noexcept;
    void setConfig(const IngestionJudgeConfig& cfg);

    // ---- Observers --------------------------------------------------------

    void addObserver(std::shared_ptr<IIngestionQualityObserver> observer);
    void removeObserver(const IIngestionQualityObserver* observer);

private:
    // ---- Prompt builders --------------------------------------------------

    std::string buildCompletenessPrompt(const ::themis::ingestion::ExtractionContext& ctx,
                                        const std::string&       source) const;
    std::string buildGroundednessPrompt(const ::themis::ingestion::ExtractionContext& ctx,
                                        const std::string&       source) const;
    std::string buildEntityCoveragePrompt(const ::themis::ingestion::ExtractionContext& ctx,
                                          const std::string&       source) const;
    std::string buildRelationCoherencePrompt(const ::themis::ingestion::ExtractionContext& ctx) const;

    // ---- Response parsers -------------------------------------------------

    /// Extract a float score from a line like "SCORE: 0.82" in @p response.
    /// Returns -1.0 when no parseable score is found.
    static double parseScore(const std::string& response) noexcept;

    /// Extract a rationale sentence after "RATIONALE:" in @p response.
    static std::string parseRationale(const std::string& response) noexcept;

    /// Extract bullet-listed items after "MISSING:" or "UNGROUNDED:" etc.
    static std::vector<std::string> parseBulletList(
        const std::string& response,
        const std::string& section_tag) noexcept;

    // ---- Aggregation ------------------------------------------------------

    double computeOverallScore(const IngestionQualityReport& r) const noexcept;
    bool   checkThresholds(const IngestionQualityReport& r) const noexcept;
    std::vector<std::string> computeRecommendedSteps(
        const IngestionQualityReport& r) const;

    // ---- Observer dispatch ------------------------------------------------

    void notifyEvaluated(const std::string&            doc_id,
                         const IngestionQualityReport& report) const noexcept;

    // ---- State ------------------------------------------------------------

    std::shared_ptr<ITextGenerationBackend>             backend_;
    IngestionJudgeConfig                                config_;
    mutable std::mutex                                  observer_mutex_;
    std::vector<std::shared_ptr<IIngestionQualityObserver>> observers_;
};

// ============================================================================
// ReIngestionController
// ============================================================================

/**
 * @brief Runtime re-ingestion controller.
 *
 * Combines a `WorkflowEngine` and an `IngestionQualityJudge` into a
 * feedback loop:
 *
 *  1. Run the workflow engine on the document.
 *  2. Evaluate extraction quality.
 *  3. If quality fails and attempts remain, adjust the step config
 *     (based on `report.recommended_steps`) and re-run.
 *  4. Return the final `RunResult` after quality passes or max attempts.
 *
 * NOT thread-safe: use one controller per ingestion worker thread.
 *
 * Usage:
 * @code
 *   ReIngestionController ctrl(workflow_engine, judge);
 *   ctrl.addObserver(my_monitor);
 *
 *   auto res = ctrl.process(manifest);
 *   if (!res.quality_met)
 *       log("Document {} did not reach quality threshold after {} attempts",
 *           manifest.source_uri, res.attempts);
 * @endcode
 */
class ReIngestionController {
public:
    // ---- Result -----------------------------------------------------------

    /**
     * @brief Result of a full process() call.
     */
    struct RunResult {
        /// Total number of workflow runs (1 = first pass, 2+ = re-ingestion).
        int attempts = 0;
        /// True when the final pass met all quality thresholds.
        bool quality_met = false;
        /// Quality report from the last evaluation pass.
        IngestionQualityReport final_report;
        /// History of quality reports (one per attempt, oldest first).
        std::vector<IngestionQualityReport> history;
        /// The extraction context from the best-quality pass.
        ::themis::ingestion::ExtractionContext best_context;
    };

    // ---- Construction -----------------------------------------------------

    /**
     * @brief Construct a controller.
     *
     * @param engine  Configured workflow engine (not null).
     * @param judge   Quality judge to use for evaluation (not null).
     */
    ReIngestionController(std::shared_ptr<::themis::ingestion::WorkflowEngine> engine,
                          std::shared_ptr<IngestionQualityJudge> judge);

    ~ReIngestionController();

    // ---- Processing -------------------------------------------------------

    /**
     * @brief Process a document through the quality-controlled ingestion loop.
     *
     * Runs the workflow engine, evaluates quality, and re-ingests if needed.
     * Stops when quality passes or `IngestionJudgeConfig::max_reingestion_attempts`
     * is exhausted.
     *
     * @param manifest  File manifest describing the document to ingest.
     * @return RunResult with final quality report and extraction context.
     */
    RunResult process(const ::themis::ingestion::FileManifest& manifest);

    // ---- Configuration ----------------------------------------------------

    /**
     * @brief Override the workflow profile used for re-ingestion passes.
     *
     * When empty (the default), re-ingestion uses the same profile as the
     * first pass.  Set to a more thorough profile (e.g. "legal-thorough")
     * to increase coverage during re-ingestion.
     *
     * @param profile_name  Workflow profile name registered with the engine.
     */
    void setReIngestionProfile(const std::string& profile_name);

    // ---- Observers --------------------------------------------------------

    void addObserver(std::shared_ptr<IIngestionQualityObserver> observer);
    void removeObserver(const IIngestionQualityObserver* observer);

private:
    // ---- Helpers ----------------------------------------------------------

    /// Build a StepConfig override that focuses on the failing steps.
    std::vector<::themis::ingestion::StepConfig> buildTargetedStepConfig(
        const IngestionQualityReport& report) const;

    /// Returns true when attempt @p b is a measurable improvement over @p a.
    static bool isImprovement(const IngestionQualityReport& a,
                               const IngestionQualityReport& b) noexcept;

    void notifyTriggered(const std::string&              doc_id,
                         int                             attempt,
                         const std::vector<std::string>& reasons) noexcept;
    void notifyComplete(const std::string& doc_id,
                        int                attempt,
                        bool               improved) noexcept;

    // ---- State ------------------------------------------------------------

    std::shared_ptr<::themis::ingestion::WorkflowEngine> engine_;
    std::shared_ptr<IngestionQualityJudge> judge_;
    std::string                            reingestion_profile_;
    std::vector<std::shared_ptr<IIngestionQualityObserver>> observers_;
};

} // namespace ingestion
} // namespace themis
