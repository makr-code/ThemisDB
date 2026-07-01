/**
 * @file continuous_learning_orchestrator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ab_testing_framework.h"
#include "bayesian_optimizer.h"
#include "learning_metrics.h"
#include "training/lora_data_selection.h"

// Forward declarations for federation bridges (IMPL-A3)
namespace themis::training { class IncrementalLoRATrainer; }
namespace themis::distributed_knowledge { class ILoRAFederationCoordinator; }
namespace themis::performance::phase3 { class BaoOptimizer; }
namespace themis::performance { class WorkloadAdaptiveOptimizer; }
namespace themis::prompt_engineering { class FeedbackCollector; }

namespace themis::rag::learning {

/**
 * @brief Configuration for continuous learning
 */
struct ContinuousLearningConfig {
    // Trigger thresholds
    size_t min_feedback_samples = 100;
    double min_accuracy_drop    = 0.05; ///< 5% drop triggers retraining
    std::chrono::hours retraining_interval{24};

    // Learning rates
    double prompt_learning_rate    = 0.01;
    double retrieval_learning_rate = 0.1;
    double lora_learning_rate      = 3e-4;

    // A/B Testing
    bool enable_ab_testing       = true;
    double ab_test_traffic_split = 0.1; ///< 10% traffic for new models
    size_t min_ab_samples        = 1000;

    // Rollback safety
    double min_improvement_threshold = 0.02; ///< 2% minimum improvement
    bool enable_auto_rollback        = true;

    // Persistence
    std::string metrics_db_path     = "data/learning_metrics.db";
    std::string model_registry_path = "data/model_registry/";

    // Learning loop
    std::chrono::seconds learning_loop_interval{3600}; ///< Check every hour

    // ---- Automated Data Selection integration ----
    /// Configuration for the data selection pipeline executed before retraining.
    themis::training::LoRADataSelectionConfig data_selection_config;

    /// Adaptive self-improvement rules applied after each selection run.
    themis::training::SelfImprovementConfig   self_improvement_config;

    /// Path to LoRATrainerConfig.yaml for live-reload (empty = use defaults).
    std::string lora_trainer_config_path;

    /// Path to SelfImprovementModule.yaml for live-reload (empty = use defaults).
    std::string self_improvement_config_path;

    // ---- Production mode enforcement ----
    /// When true, loops require live signal providers (fallback is not allowed).
    /// In production mode, if a provider is unavailable, the loop is marked as failed.
    /// Set via THEMIS_PRODUCTION_MODE environment variable or explicitly.
    bool enforce_live_providers = false;

    /// When true, automatically detect production mode from THEMIS_PRODUCTION_MODE
    /// or THEMIS_ENVIRONMENT environment variables.
    bool auto_detect_production_mode = true;
};

/**
 * @brief Continuous Learning Orchestrator
 *
 * Main class that coordinates automatic learning across all RAG components.
 * Implements trigger-based retraining, prompt optimization, retrieval tuning,
 * and A/B testing with statistical validation.
 */
class ContinuousLearningOrchestrator {
  public:
    explicit ContinuousLearningOrchestrator(const ContinuousLearningConfig &config);
    ~ContinuousLearningOrchestrator();

    // Main API

    /**
     * @brief Start the background learning loop
     *
     * Launches a background thread that periodically checks for learning
     * opportunities and triggers optimization as needed.
     */
    void startLearningLoop();

    /**
     * @brief Stop the background learning loop
     */
    void stopLearningLoop();

    /**
     * @brief Trigger a learning iteration manually
     *
     * Forces an immediate check and execution of learning strategies.
     */
    void triggerLearningIteration();

    // Component registration
    // Note: These are registration stubs - actual implementation would require
    // the full component classes which may not be available yet

    /**
     * @brief Register a LoRA adapter for automatic retraining
     * @param adapter_id Unique identifier for the adapter
     * @param adapter_info Metadata about the adapter
     */
    void registerLoRAAdapter(const std::string &adapter_id, const std::string &adapter_info);

    /**
     * @brief Register retrieval system for parameter tuning
     * @param system_id Unique identifier
     */
    void registerRetrievalSystem(const std::string &system_id);

    /**
     * @brief Register prompt system for optimization
     * @param system_id Unique identifier
     */
    void registerPromptSystem(const std::string &system_id);

    /**
     * @brief Register knowledge gap detector for metrics
     * @param detector_id Unique identifier
     */
    void registerKnowledgeGapDetector(const std::string &detector_id);

    // Feedback logging

    /**
     * @brief Log a single RAG interaction
     * @param interaction Complete interaction record
     */
    void logInteraction(const Interaction &interaction);

    /**
     * @brief Log multiple interactions in batch
     * @param interactions Vector of interactions
     */
    void logInteractionBatch(const std::vector<Interaction> &interactions);

    // Metrics & monitoring

    /**
     * @brief Get current learning statistics
     */
    LearningStats getStats() const;

    /**
     * @brief Get performance history over time
     * @param lookback_period How far back to look
     * @return Vector of performance snapshots
     */
    std::vector<PerformanceSnapshot> getPerformanceHistory(std::chrono::hours lookback_period) const;

    /**
     * @brief Check if system is improving over time
     */
    bool isSystemImproving() const;

    // ---- Data selection ----

    /**
     * @brief Run the automated data selection pipeline for a specific adapter.
     *
     * Loads (or reloads) configuration from `lora_trainer_config_path` if set,
     * executes all five data-selection stages on the provided candidate samples,
     * applies adaptive self-improvement rules to the config for the next run,
     * and returns the selection result including the JSONL audit entry.
     *
     * This is called automatically during `runLoRARetraining()` and can also be
     * invoked directly for manual testing or scheduled jobs.
     *
     * @param adapter_id        Adapter being retrained (used for logging).
     * @param candidate_samples Raw samples to run the pipeline on.
     * @param current_metrics   Monitoring metrics for adaptive threshold adjustment.
     * @return Selection result (selected samples + audit provenance).
     */
    themis::training::DataSelectionResult runDataSelectionForAdapter(
        const std::string& adapter_id,
        const std::vector<themis::training::DataSample>& candidate_samples,
        const themis::training::DataSelectionMetrics& current_metrics = {});

    /**
     * @brief Get the data selection configuration currently in use.
     */
    const themis::training::LoRADataSelectionConfig& getDataSelectionConfig() const;

    /**
     * @brief Update the data selection configuration (live reload).
     */
    void setDataSelectionConfig(const themis::training::LoRADataSelectionConfig& cfg);

    // ---- Adaptive retrieval ----

    /**
     * @brief Get the current optimized retrieval parameters.
     *
     * Returns the retrieval parameters most recently updated by the adaptive
     * optimization loop.  Callers should apply these parameters when issuing
     * retrieval requests so that the system benefits from online learning.
     *
     * The parameters are updated every time `triggerLearningIteration()` runs
     * and sufficient feedback (both user signals and evaluation confidence
     * scores) has been collected.  Until the first optimization cycle the
     * method returns the default values from `RetrievalParams`.
     *
     * @return Current optimized retrieval parameters (thread-safe read).
     */
    RetrievalParams getOptimizedRetrievalParams() const;

    // ---- Loop orchestration (IMPL-A2) ----------------------------------------

    /**
     * @brief Typed outcome from a single query execution, used by Loop 1.
     *
     * Carries the raw signal that `triggerLoop1QueryExecution()` passes to the
     * BaoOptimizer hint-update path.
     */
    struct QueryExecutionOutcome {
        std::string query_id;          ///< Stable query fingerprint / request-id
        double      latency_ms{0.0};   ///< Observed end-to-end latency in ms
        std::string explain_plan_json; ///< JSON-serialised EXPLAIN / BaoOptimizer plan
        bool        used_index{true};  ///< Whether an index was selected for execution
    };

    /**
     * @brief Named learning loops as defined in THEMISDB_LORA_RESEARCH_PAPER.md §5.
     *
     * - LOOP_1_HNSW_QUERY   : Daily, fully autonomous — HNSW/BaoOptimizer retraining.
     * - LOOP_2_WORKLOAD     : Weekly, fully autonomous — workload-profile adaptation.
     * - LOOP_3_SCHEMA_INDEX : Weekly, advisory-only  — schema / index suggestions.
     * - LOOP_4_RLAIF        : Monthly, semi-autonomous — preference-pair RLAIF.
     * - IDLE                : No loop currently active.
     */
    enum class LoopPhase {
        IDLE                = 0,
        LOOP_1_HNSW_QUERY   = 1,
        LOOP_2_WORKLOAD     = 2,
        LOOP_3_SCHEMA_INDEX = 3,
        LOOP_4_RLAIF        = 4,
    };

    /**
     * @brief Result produced by a loop execution.
     */
    struct LoopResult {
        LoopPhase   phase            = LoopPhase::IDLE;
        bool        success          = false;
        bool        guardrail_passed = false; ///< False → adapter commit blocked.
        std::string adapter_version;          ///< Newly registered adapter version (if any).
        double      metric_delta     = 0.0;   ///< Δ(primary_metric) for this round.
        double      signal_value     = 0.0;   ///< Last observed loop signal value.
        std::string signal_source;            ///< live|fallback_missing|fallback_error|fallback_invalid.
    };

    /**
     * @brief Return the currently active loop phase (IDLE when no loop runs).
     */
    [[nodiscard]] LoopPhase currentLoop() const;

    /**
     * @brief Explicitly trigger a named learning loop.
     *
     * Runs the loop's phase-specific adaptation routine and guardrail check.
     * The registered completion handler (if any) is invoked
     * synchronously before returning.
     *
     * @param phase  Loop to trigger.  Passing IDLE is a no-op.
     * @return LoopResult describing the outcome.
     */
    LoopResult triggerLoop(LoopPhase phase);

    // ── Named typed trigger methods (IMPL-A2 Phase 2) ──────────────────────

    /**
     * @brief Trigger Loop 1 — per-query BaoOptimizer feedback (target: ≤ 10 ms).
     *
     * Stores @p outcome for the JSON context serialiser, then delegates to
     * `triggerLoop(LOOP_1_HNSW_QUERY)`.  Returns a cooldown-blocked result if
     * called within the configured cooldown window.
     *
     * @param outcome  Typed query execution result from the query executor.
     * @return LoopResult; `success == false` and `adapter_version == "cooldown"` when
     *         the call is rejected due to the per-resource cooldown guard.
     */
    LoopResult triggerLoop1QueryExecution(const QueryExecutionOutcome& outcome);

    /**
     * @brief Trigger Loop 2 — WorkloadAdaptiveOptimizer + HNSW (60 s interval).
     *
     * Delegates to `triggerLoop(LOOP_2_WORKLOAD)` after the cooldown guard passes.
     */
    LoopResult triggerLoop2WorkloadAdaptation();

    /**
     * @brief Trigger Loop 3 — IndexSuggestionEngine advisory cycle.
     *
     * Advisory-only; always passes the guardrail.  Delegates to
     * `triggerLoop(LOOP_3_SCHEMA_INDEX)` after the cooldown guard passes.
     */
    LoopResult triggerLoop3IndexLifecycle();

    /**
     * @brief Trigger Loop 4 — RLAIF preference-learning cycle.
     *
     * Delegates to `triggerLoop(LOOP_4_RLAIF)`.  On success + guardrail pass,
     * `FEDERATED_ROUND_START` is fired automatically.
     */
    LoopResult triggerLoop4AdapterImprovement();

    // ── Cooldown guard (RQ10) ───────────────────────────────────────────────

    /**
     * @brief Set the per-loop cooldown window.
     *
     * Any `triggerLoopN*()` call issued within @p cooldown of the previous
     * invocation for the same loop is rejected (returns cooldown-blocked result).
     * Default: 10 s.
     *
     * @param cooldown  Minimum interval between successive triggers of the same loop.
     */
    void setOptimizationCooldown(std::chrono::seconds cooldown);

    // ── JSON context serialiser ─────────────────────────────────────────────

    /**
     * @brief Serialise the latest Loop 1–3 outcome signals to a JSON context block.
     *
     * Returns a compact JSON string of at most 2 000 tokens (approx. 8 000 chars)
     * suitable for injection into an LLM prompt or decision-record context.
     * Fields include: loop_id, phase, latency_ms (Loop 1), metric_delta, adapter_version,
     * timestamp_iso.
     *
     * @return JSON string; empty object `{}` when no loops have been triggered yet.
     */
    [[nodiscard]] std::string serializeLoopContext() const;

    /**
     * @brief Register a completion handler for a specific loop phase.
     *
     * The handler is called (synchronously) at the end of every `triggerLoop()`
     * invocation for the specified phase.  Only one handler per phase is
     * supported; a second call overwrites the previous one.
     *
     * @param phase    Loop phase to register for.
     * @param handler  Callable accepting (LoopPhase, LoopResult).
     */
    void registerLoopCompletionHandler(
        LoopPhase phase,
        std::function<void(LoopPhase, const LoopResult&)> handler);

    // ── IMPL-A3: Federation bridges ──────────────────────────────────────────

    /**
     * @brief Internal trigger events used to decouple inter-loop signals.
     *
     * - `FEDERATED_ROUND_START`: Fired automatically after a successful Loop-4
     *   (`LOOP_4_RLAIF`) run in which `guardrail_passed == true`.  Triggers
     *   `exportGradient()` on the injected trainer and `submitGradient()` on the
     *   injected `ILoRAFederationCoordinator`.
     */
    enum class TriggerEvent {
        FEDERATED_ROUND_START = 0,
    };

    /**
     * @brief Inject an `ILoRAFederationCoordinator` for federated LoRA aggregation.
     *
     * When set, a successful Loop-4 completion with `guardrail_passed == true`
     * automatically calls `coordinator->submitGradient()` with the gradient
     * exported from the injected trainer.
     *
     * Pass `nullptr` to detach the coordinator.
     *
     * @param coordinator  Shared federation coordinator instance.
     */
    void setFederationCoordinator(
        std::shared_ptr<themis::distributed_knowledge::ILoRAFederationCoordinator>
            coordinator);

    /**
     * @brief Inject an `IncrementalLoRATrainer` for federated gradient export.
     *
     * The orchestrator calls `trainer->exportGradient()` when the
     * `FEDERATED_ROUND_START` event fires.  The trainer must remain valid for
     * the lifetime of this orchestrator (non-owning pointer).
     *
     * Pass `nullptr` to detach.
     *
     * @param trainer  Pointer to the local shard's trainer.
     */
    void setTrainerForFederation(themis::training::IncrementalLoRATrainer* trainer);

    // ── Signal-source injection APIs (production-wired) ─────────────────────

    /**
     * @brief Inject a BaoOptimizer miss-rate provider for Loop 1 guardrail checks.
     *
     * The callback should return the current HNSW query miss-rate in [0.0, 1.0].
     * Loop 1's guardrail passes when the returned value is below 0.05.
     * Without a provider the guardrail falls back to the accuracy-proxy heuristic.
     *
     * Roadmap ref: src/rag/ROADMAP.md §Phase 8 Loop 1
     */
    void setHnswMissRateProvider(std::function<double()> provider);

    /**
     * @brief Inject a WorkloadAdaptiveOptimizer drift provider for Loop 2.
     *
     * The callback should return the current workload profile drift in [0.0, 1.0].
     * Loop 2's guardrail passes when the returned value is below 0.1.
     * Without a provider the guardrail falls back to the accuracy-proxy heuristic.
     *
     * Roadmap ref: src/rag/ROADMAP.md §Phase 8 Loop 2
     */
    void setWorkloadDriftProvider(std::function<double()> provider);

    /**
     * @brief Inject a FeedbackCollector new-entry-count provider for Loop 4.
     *
     * The callback should return the number of new feedback entries since the
     * last Loop-4 run.  Loop 4 only commits a new adapter when the count
     * reaches ≥ 100.  Without a provider the guardrail falls back to the
     * accuracy-proxy heuristic.
     *
     * Roadmap ref: src/rag/ROADMAP.md §Phase 8 Loop 4
     */
    void setFeedbackEntryCountProvider(std::function<size_t()> provider);

    /**
     * @brief Wire production signal providers from live subsystem instances.
     *
     * This bootstrap helper binds Loop-1/2/4 signals to:
     * - BaoOptimizer::getMissRate()
     * - WorkloadAdaptiveOptimizer::getProfileDrift()
     * - FeedbackCollector::newEntryCount()
     *
     * Internally weak references are used, so providers fail closed (with
     * warning + fallback) if a dependency is released during runtime.
     */
    void wireLiveSignalProviders(
        std::shared_ptr<themis::performance::phase3::BaoOptimizer> bao_optimizer,
        std::shared_ptr<themis::performance::WorkloadAdaptiveOptimizer> workload_optimizer,
        std::shared_ptr<themis::prompt_engineering::FeedbackCollector> feedback_collector);

    // ── Provider health monitoring (production mode support) ──────────────────
    /**
     * @brief Get provider health status for all 4 loops.
     *
     * Returns JSON with provider availability, failure counts, and last failure timestamps.
     * Useful for monitoring and debugging production issues.
     *
     * Example output:
     * {
     *   "loop_1_hnsw": {"available": true, "failures": 0, "last_failure": ""},
     *   "loop_2_workload": {"available": true, "failures": 0, "last_failure": ""},
     *   "loop_4_rlaif": {"available": true, "failures": 0, "last_failure": ""}
     * }
     */
    std::string getProviderHealthMetrics() const;

    // Persistence
    void saveMetrics();
    void loadMetrics();
    void saveModelCheckpoint(const std::string &model_id);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    // Learning strategies
    void runPromptOptimization();
    void runRetrievalOptimization();
    void runLoRARetraining();

    // A/B Testing
    void deployABTest(const std::string &model_id);
    void promoteOrRollback(const ABTestResult &result);

    // Background thread
    void learningLoopThread();

    // IMPL-A3: Federation event handler
    void handleFederatedRoundStart();

    // IMPL-A2: Cooldown helper — returns true if the named loop is still within
    // its cooldown window.  Updates last-trigger timestamp when allowed.
    bool checkAndUpdateCooldown(LoopPhase phase);
};

} // namespace themis::rag::learning
