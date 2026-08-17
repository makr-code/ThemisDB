/**
 * @file rlaif_trainer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "rag/rag_judge.h"
#include "distributed_knowledge/cross_shard_feedback_sync.h"

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace themis::rag::training {

// ============================================================
// Enumerations
// ============================================================

/**
 * @brief Constitutional critique strategy applied during a training step.
 */
enum class ConstitutionalStrategy {
    HARMLESSNESS,   ///< Detect and revise harmful content.
    HELPFULNESS,    ///< Revise to maximise helpfulness.
    HONESTY,        ///< Detect hedging / uncertainty; revise to be accurate.
    FAIRNESS,       ///< Detect discriminatory language; revise to be neutral.
    COMBINED        ///< Apply all principles in sequence.
};

/**
 * @brief Outcome of a single constitutional critique-revision cycle.
 */
enum class RevisionOutcome {
    ACCEPTED,       ///< Revised response is better than the draft.
    REJECTED,       ///< Revision did not improve quality; draft retained.
    UNCHANGED       ///< No revision was needed (draft already passes).
};

// ============================================================
// Core data structures
// ============================================================

/**
 * @brief A single constitutional principle definition.
 */
struct AIPrinciple {
    std::string id;           ///< Unique identifier (e.g., "harmlessness-1")
    std::string description;  ///< Human-readable principle statement
    std::string critique_template; ///< Template for generating a critique
    std::string revision_template; ///< Template for generating a revision
    ConstitutionalStrategy strategy = ConstitutionalStrategy::COMBINED;
};

/**
 * @brief A (prompt, chosen, rejected) preference pair for reward-model training.
 *
 * Compatible with standard DPO / preference-optimisation data formats.
 */
struct PreferencePair {
    std::string prompt;         ///< The original user query / input
    std::string chosen;         ///< The preferred (better) response
    std::string rejected;       ///< The dispreferred (worse) response
    double preference_score = 0.0; ///< AI judge confidence ∈ [0, 1]
    std::string judge_rationale;   ///< Optional rationale from the judge
    std::vector<std::string> applied_principles; ///< Principles used
    std::chrono::system_clock::time_point created_at;
};

/**
 * @brief A constitutional AI self-critique of a candidate response.
 */
struct ConstitutionalCritique {
    std::string principle_id;   ///< Which principle was applied
    std::string critique_text;  ///< The AI critique of the response
    bool violation_detected = false; ///< true if principle was violated
    double severity = 0.0;      ///< Severity of violation ∈ [0, 1]
};

/**
 * @brief Result of one constitutional revision step.
 */
struct ConstitutionalRevision {
    std::string original_response;    ///< Input to this step
    std::string revised_response;     ///< Output of this step
    std::vector<ConstitutionalCritique> critiques; ///< Per-principle critiques
    RevisionOutcome outcome = RevisionOutcome::UNCHANGED;
    double quality_delta = 0.0; ///< Improvement in quality score
    int iteration = 0;          ///< Cycle index (0-based)
};

/**
 * @brief Result of a complete RLAIF training step.
 */
struct RLAIFTrainingStep {
    std::string query;
    std::vector<ConstitutionalRevision> revision_chain; ///< All iterations
    PreferencePair preference_pair;   ///< Final pair for training
    bool success = false;             ///< true if a usable pair was produced
    std::string error_message;        ///< Set when success == false
    std::chrono::milliseconds elapsed_ms{0};
};

/**
 * @brief Aggregated statistics over many training steps.
 */
struct RLAIFTrainerStats {
    size_t total_steps          = 0;
    size_t successful_steps     = 0;
    size_t failed_steps         = 0;
    size_t revisions_performed  = 0;
    size_t violations_detected  = 0;

    double avg_preference_score = 0.0;
    double avg_quality_delta    = 0.0;
    std::chrono::milliseconds avg_step_ms{0};

    /// Per-principle violation counts.
    std::vector<std::pair<std::string, size_t>> principle_violations;
};

// ============================================================
// Pluggable AI judge interface
// ============================================================

/**
 * @brief Interface for the AI preference judge.
 *
 * The judge is called when two candidate responses are available and must
 * decide which is preferred.  Implement this to integrate any strong LLM
 * (GPT-4, Claude, Gemini, local models, etc.).
 *
 * Contract:
 *  - Returns a score in [0, 1] for @p response_a relative to @p response_b.
 *    0.5 means no preference; >0.5 means @p response_a is preferred.
 *  - Must be exception-safe; any failure should return 0.5 (no preference).
 *  - Does NOT need to be thread-safe.
 */
class IAIJudge {
public:
    virtual ~IAIJudge() = default;

    /**
     * @brief Compare two responses and return preference for response_a.
     *
     * @param prompt      The original user prompt.
     * @param response_a  First candidate response.
     * @param response_b  Second candidate response.
     * @return Score in [0, 1]; >0.5 ⟹ prefer response_a.
     */
    [[nodiscard]] virtual double judge(const std::string& prompt,
                         const std::string& response_a,
                         const std::string& response_b) const = 0;

    /**
     * @brief Generate a critique of a response according to a principle.
     *
     * @param prompt      The original user prompt (context for the critique).
     * @param response    The response to critique.
     * @param principle   The constitutional principle to apply.
     * @return A critique text (may be empty if no violation detected).
     */
    [[nodiscard]] virtual std::string critique(
        const std::string& prompt,
        const std::string& response,
        const AIPrinciple& principle) const = 0;

    /**
     * @brief Generate a revised response that addresses the critique.
     *
     * @param prompt        The original user prompt (context for revision).
     * @param response      The original response.
     * @param critique_text The critique to address.
     * @param principle     The constitutional principle being applied.
     * @return A revised response (may equal the original if no change).
     */
    [[nodiscard]] virtual std::string revise(
        const std::string& prompt,
        const std::string& response,
        const std::string& critique_text,
        const AIPrinciple& principle) const = 0;

    /** @brief Human-readable name for logging. */
    [[nodiscard]] virtual std::string name() const = 0;
};

// ============================================================
// Heuristic default judge (no LLM runtime required)
// ============================================================

/**
 * @brief Heuristic IAIJudge that evaluates responses without an LLM.
 *
 * Uses response length, lexical diversity, and a keyword blacklist as
 * a lightweight quality proxy.  Designed for testing and environments
 * where no LLM runtime is available.
 *
 * Performance: O(|response_a| + |response_b|).
 */
class HeuristicAIJudge : public IAIJudge {
public:
    double judge(const std::string& prompt,
                 const std::string& response_a,
                 const std::string& response_b) const override;

    std::string critique(const std::string& prompt,
                         const std::string& response,
                         const AIPrinciple& principle) const override;

    std::string revise(const std::string& prompt,
                       const std::string& response,
                       const std::string& critique_text,
                       const AIPrinciple& principle) const override;

    std::string name() const override { return "HeuristicAIJudge"; }
};

// ============================================================
// Configuration
// ============================================================

/**
 * @brief Configuration for RLAIFTrainer.
 */
struct RLAIFConfig {
    // Constitutional AI settings
    int    max_revision_iterations = 3;    ///< Max critique-revision cycles
    double min_quality_threshold   = 0.6;  ///< Min score to accept a response
    double improvement_threshold   = 0.05; ///< Min δ to continue iterating

    // Preference pair settings
    double min_preference_score = 0.55; ///< Min judge score to create a pair
    bool   include_rationale    = true; ///< Include judge rationale in pair

    // Dataset settings
    size_t max_dataset_size = 10000; ///< Maximum stored preference pairs

    // Principles (populated by loadDefaultPrinciples() or manually)
    std::vector<AIPrinciple> principles;
};

// ============================================================
// RLAIFTrainer
// ============================================================

/**
 * @brief Constitutional AI / RLAIF training pipeline.
 *
 * Typical usage — generate a preference pair from a single query+response:
 * @code
 *   RLAIFConfig cfg;
 *   RLAIFTrainer trainer(cfg);
 *   trainer.loadDefaultPrinciples();
 *
 *   auto step = trainer.runTrainingStep(
 *       "Explain quantum entanglement.",
 *       draft_response);
 *
 *   if (step.success) {
 *       dataset.push_back(step.preference_pair);
 *   }
 * @endcode
 *
 * Batch mode:
 * @code
 *   trainer.addToQueue(query, draft);
 *   auto pairs = trainer.processBatch();
 * @endcode
 */
class RLAIFTrainer {
public:
    /**
     * @brief Construct with default config and heuristic judge.
     */
    RLAIFTrainer();

    /**
     * @brief Construct with custom config and optional judge.
     * @param config  Training configuration.
     * @param judge   Optional AI judge; if nullptr, uses HeuristicAIJudge.
     * @throws std::invalid_argument on invalid config parameters.
     */
    explicit RLAIFTrainer(const RLAIFConfig&       config,
                          std::shared_ptr<IAIJudge> judge = nullptr);

    ~RLAIFTrainer();

    // RLAIFTrainer is move-only (PIMPL with unique_ptr).
    RLAIFTrainer(RLAIFTrainer&&) noexcept            noexcept = default;
    RLAIFTrainer& operator=(RLAIFTrainer&&) noexcept noexcept = default;

    RLAIFTrainer(const RLAIFTrainer&)            = delete;
    RLAIFTrainer& operator=(const RLAIFTrainer&) = delete;

    // ═══════════════════════════════════════════════════════════
    // Principle management
    // ═══════════════════════════════════════════════════════════

    /**
     * @brief Load a default set of constitutional principles.
     *
     * Loads principles based on:
     *  - Harmlessness (Bai et al., 2022)
     *  - Helpfulness (InstructGPT-style)
     *  - Honesty / calibration
     *  - Fairness / non-discrimination
     */
    void loadDefaultPrinciples();

    /** @brief Add a custom principle. */
    void addPrinciple(const AIPrinciple& principle);

    /** @brief Remove a principle by ID. */
    void removePrinciple(const std::string& principle_id);

    /** @brief Return all currently registered principles. */
    const std::vector<AIPrinciple>& getPrinciples() const;

    // ═══════════════════════════════════════════════════════════
    // Core training API
    // ═══════════════════════════════════════════════════════════

    /**
     * @brief Run one constitutional AI + preference generation step.
     *
     * Steps performed:
     *  1. Apply critique-revision cycles to @p draft_response.
     *  2. After all iterations, call the judge on (draft, final_revision).
     *  3. If judge prefers the revision, create a (query, revision, draft)
     *     preference pair; otherwise (query, draft, revision).
     *  4. Store the pair in the internal dataset when
     *     preference_score >= config_.min_preference_score.
     *
     * @param query          The user query that generated @p draft_response.
     * @param draft_response The initial (potentially low-quality) response.
     * @return Training step result including the preference pair.
     */
    RLAIFTrainingStep runTrainingStep(const std::string& query,
                                      const std::string& draft_response);

    /**
     * @brief Generate a constitutional critique for a single principle.
     *
     * @param response   Response to critique.
     * @param principle  Principle to apply.
     * @return Critique result including violation flag and severity.
     */
    ConstitutionalCritique generateCritique(const std::string& response,
                                            const AIPrinciple& principle) const;

    /**
     * @brief Generate a constitutional revision given a critique.
     *
     * @param response       Original response.
     * @param critique_text  Critique text to address.
     * @param principle      Principle being applied.
     * @return Revised response text.
     */
    std::string generateRevision(const std::string& response,
                                  const std::string& critique_text,
                                  const AIPrinciple& principle) const;

    /**
     * @brief Create a preference pair from two candidate responses.
     *
     * Calls the judge to determine which response is preferred, then
     * constructs a PreferencePair accordingly.
     *
     * @param query       Original user query.
     * @param response_a  First candidate response.
     * @param response_b  Second candidate response.
     * @return Preference pair (chosen is the higher-scored response).
     */
    PreferencePair createPreferencePair(const std::string& query,
                                        const std::string& response_a,
                                        const std::string& response_b) const;

    // ═══════════════════════════════════════════════════════════
    // Batch operations
    // ═══════════════════════════════════════════════════════════

    /**
     * @brief Add a (query, draft) pair to the processing queue.
     */
    void addToQueue(const std::string& query,
                    const std::string& draft_response);

    /**
     * @brief Process all queued (query, draft) pairs and return new pairs.
     *
     * Clears the internal queue after processing.
     *
     * @return Training step results for each queued pair.
     */
    std::vector<RLAIFTrainingStep> processBatch();

    // ═══════════════════════════════════════════════════════════
    // Dataset access
    // ═══════════════════════════════════════════════════════════

    /**
     * @brief Return all accumulated preference pairs.
     */
    const std::vector<PreferencePair>& getDataset() const;

    /**
     * @brief Clear the accumulated preference pairs and statistics.
     */
    void clearDataset();

    /** @brief Return the number of accumulated preference pairs. */
    size_t datasetSize() const;

    // ═══════════════════════════════════════════════════════════
    // Statistics & monitoring
    // ═══════════════════════════════════════════════════════════

    /** @brief Return aggregated training statistics. */
    RLAIFTrainerStats getStats() const;

    /** @brief Reset statistics counters (does not clear the dataset). */
    void resetStats();

    /**
     * @brief Set a callback invoked after each completed training step.
     *
     * The callback is called with a const reference; it must not throw.
     */
    void setStepCallback(
        std::function<void(const RLAIFTrainingStep&)> callback);

    // ═══════════════════════════════════════════════════════════
    // Configuration
    // ═══════════════════════════════════════════════════════════

    /** @brief Return current configuration. */
    const RLAIFConfig& getConfig() const;

    /**
     * @brief Replace configuration.
     * @throws std::invalid_argument on invalid parameters.
     */
    void setConfig(const RLAIFConfig& config);

    /** @brief Replace the AI judge (nullptr ⟹ heuristic fallback). */
    void setJudge(std::shared_ptr<IAIJudge> judge);

    /** @brief Return the active judge name. */
    std::string judgeName() const;

    /**
     * @brief Validate a RLAIFConfig.
     * @throws std::invalid_argument describing the first violation found.
     */
    static void validateConfig(const RLAIFConfig& config);

    // ═══════════════════════════════════════════════════════════
    // DK-5: Cross-shard RLAIF feedback integration
    // ═══════════════════════════════════════════════════════════

    /**
     * @brief Aggregated stats for cross-shard preference pairs.
     *
     * Counters are updated by `addCrossShardSummary()`.
     */
    struct CrossShardStats {
        size_t received_summaries = 0; ///< Total inbound summaries accepted
        size_t applied_pairs      = 0; ///< Preference pairs added to dataset
        size_t skipped_summaries  = 0; ///< Summaries skipped (embedding lookup failure)
    };

    /**
     * @brief Ingest a cross-shard feedback summary as a synthetic preference pair.
     *
     * The caller is responsible for constructing the `PreferencePair` (e.g.
     * via nearest-neighbour lookup on `summary.reason_embedding`).  This
     * method is a type-safe wrapper that appends the pair to the dataset and
     * increments the cross-shard counters.
     *
     * @param summary        Anonymised feedback summary from a remote shard.
     * @param synthetic_pair Pre-constructed preference pair for the summary.
     */
    void addCrossShardSummary(
        const distributed_knowledge::FeedbackSummary& summary,
        const PreferencePair& synthetic_pair);

    /**
     * @brief Return counters for cross-shard preference pair ingestion.
     */
    [[nodiscard]] CrossShardStats getCrossShardStats() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    // Internal helpers
    ConstitutionalRevision applyRevisionCycle(
        const std::string& response,
        int                iteration) const;

    double scoreResponse(const std::string& response) const;
};

// ============================================================
// Factory
// ============================================================

/**
 * @brief Factory helpers for common RLAIFTrainer configurations.
 */
class RLAIFTrainerFactory {
public:
    /**
     * @brief Create a standard trainer with default principles and heuristic
     *        judge.  Suitable for testing and offline dataset generation.
     */
    static RLAIFTrainer createDefault();

    /**
     * @brief Create a strict trainer: 5 revision iterations, high quality
     *        threshold.
     * @param judge Optional AI judge; nullptr uses heuristic.
     */
    static RLAIFTrainer createStrict(
        std::shared_ptr<IAIJudge> judge = nullptr);

    /**
     * @brief Create a fast trainer: 1 revision iteration, lower threshold.
     *        Optimised for throughput over quality.
     * @param judge Optional AI judge; nullptr uses heuristic.
     */
    static RLAIFTrainer createFast(
        std::shared_ptr<IAIJudge> judge = nullptr);

    /**
     * @brief Create a trainer with a custom AI judge.
     * @param judge   AI judge implementation.
     * @param config  Training configuration.
     */
    static RLAIFTrainer createWithJudge(
        std::shared_ptr<IAIJudge> judge,
        const RLAIFConfig&        config = {});
};

} // namespace themis::rag::training
