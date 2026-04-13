/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_ab_experiment.h                             ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-13 04:18:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     437                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3e5560456  2026-03-23  feat(prompt_engineering): A/B Experiment Framework — Prom... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file prompt_ab_experiment.h
 * @brief Public A/B experiment framework for prompt template variants (Phase 5 / v1.9.0).
 *
 * Provides deterministic per-request traffic splitting and automated winner
 * promotion for prompt template A/B experiments.
 *
 * Key types
 * ---------
 *
 * - **`ExperimentVariant`** — `CONTROL` or `TREATMENT`.
 * - **`ExperimentContext`** — carries `request_id` and `experiment_id` into
 *   the render path; `PromptABExperimentFramework::assignVariant()` returns
 *   the appropriate variant.
 * - **`PromptExperiment`** — experiment descriptor: template ID, control and
 *   treatment version IDs, `split_pct` (0–100 % of traffic to TREATMENT),
 *   `min_samples`, `confidence_level`, timestamps.  `toJson()` / `fromJson()`.
 * - **`ExperimentOutcome`** — a single scored observation: experiment_id,
 *   variant, score, request_id, timestamp.  `toJson()`.
 * - **`ExperimentStatus`** — `RUNNING`, `WINNER_CONTROL`, `WINNER_TREATMENT`,
 *   `INCONCLUSIVE`, `COMPLETED`.
 * - **`PromptABExperimentFramework`** — orchestrates experiments:
 *     * `create()` / `stop()` / `getExperiment()` / `listExperiments()`
 *     * `assignVariant(context)` — deterministic via MurmurHash3 seed 0x9747b28c
 *     * `recordOutcome(experiment_id, variant, score, request_id)` — appends
 *       score; triggers significance check once `min_samples` per variant are
 *       reached.
 *     * `checkSignificance(experiment_id)` — Welch t-test; when both variants
 *       have ≥ `min_samples` scores and p < `(1 − confidence_level)`, sets
 *       status to `WINNER_CONTROL` or `WINNER_TREATMENT`.
 *     * `promoteWinner(experiment_id)` — returns the winning version ID.
 *     * `getStatus(experiment_id)` / `getSummary(experiment_id)` (`toJson()`).
 *
 * Variant assignment
 * ------------------
 *
 * Given `request_id`, variant = TREATMENT iff
 * `murmur3_32(request_id, 0x9747b28c) % 100 < split_pct`.
 * The 32-bit MurmurHash3 body is inlined; no external hash library is required.
 *
 * Scientific grounding
 * --------------------
 *
 * - Box & Tiao (1973) "Bayesian Inference in Statistical Analysis" (Welch t-test).
 * - Kohavi, Longbotham et al. (2009) "Controlled experiments on the web"
 *   [Knowl. Inf. Syst. 18(1):5–36, DOI:10.1007/s10115-008-0194-x].
 * - Kohavi & Thomke (2017) "The surprising power of online experiments"
 *   [HBR, Sept.–Oct. 2017].
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace themis {
namespace prompt_engineering {

// ============================================================================
// ExperimentVariant
// ============================================================================

/** @brief The two sides of an A/B experiment. */
enum class ExperimentVariant {
    CONTROL,    ///< Current (baseline) template version.
    TREATMENT   ///< Candidate (new) template version.
};

/** @brief Convert ExperimentVariant to string ("control" / "treatment"). */
std::string variantToString(ExperimentVariant v);

/** @brief Parse ExperimentVariant from string; returns nullopt on unknown. */
std::optional<ExperimentVariant> stringToVariant(const std::string& s);

// ============================================================================
// ExperimentContext
// ============================================================================

/**
 * @brief Context passed into the render path to select the active variant.
 *
 * Both fields are required; `experiment_id` identifies the experiment;
 * `request_id` is hashed to produce a deterministic, per-user assignment.
 */
struct ExperimentContext {
    std::string experiment_id; ///< Which experiment to route through.
    std::string request_id;    ///< Stable user / session / request identifier.
};

// ============================================================================
// ExperimentStatus
// ============================================================================

/** @brief Lifecycle state of a `PromptExperiment`. */
enum class ExperimentStatus {
    RUNNING,           ///< Still collecting observations.
    WINNER_CONTROL,    ///< Control won at configured significance level.
    WINNER_TREATMENT,  ///< Treatment won at configured significance level.
    INCONCLUSIVE,      ///< Stopped without reaching significance.
    COMPLETED          ///< Manually stopped; winner already promoted.
};

/** @brief Convert ExperimentStatus to string. */
std::string statusToString(ExperimentStatus s);

// ============================================================================
// PromptExperiment
// ============================================================================

/**
 * @brief Descriptor for a single A/B experiment.
 *
 * An experiment compares `control_version_id` (current production version)
 * against `treatment_version_id` (candidate) for the template `template_id`.
 */
struct PromptExperiment {
    std::string experiment_id;       ///< Unique ID (UUID-style, assigned by framework).
    std::string template_id;         ///< Template being tested.
    std::string control_version_id;  ///< Version ID of the control variant.
    std::string treatment_version_id;///< Version ID of the treatment variant.

    /// Percentage of traffic (0–100) routed to TREATMENT.
    /// Default: 50 (50/50 split).
    int split_pct = 50;

    /// Minimum observations per variant before significance check begins.
    std::size_t min_samples = 200;

    /// Required significance level (e.g. 0.95 → p < 0.05).
    double confidence_level = 0.95;

    std::chrono::system_clock::time_point created_at; ///< When the experiment was created.
    std::chrono::system_clock::time_point stopped_at; ///< When stopped (zero = still running).

    ExperimentStatus status = ExperimentStatus::RUNNING;

    /** @brief Serialise to JSON. */
    nlohmann::json toJson() const;

    /** @brief Deserialise from JSON. */
    static PromptExperiment fromJson(const nlohmann::json& j);
};

// ============================================================================
// ExperimentOutcome
// ============================================================================

/** @brief A single scored observation for one variant of an experiment. */
struct ExperimentOutcome {
    std::string experiment_id; ///< Parent experiment ID.
    ExperimentVariant variant;  ///< Which side of the experiment.
    double score = 0.0;         ///< Quality score (0.0–1.0).
    std::string request_id;    ///< Originating request ID.
    std::chrono::system_clock::time_point timestamp;

    /** @brief Serialise to JSON. */
    nlohmann::json toJson() const;
};

// ============================================================================
// ExperimentSummary
// ============================================================================

/** @brief Snapshot of current experiment metrics for reporting. */
struct ExperimentSummary {
    PromptExperiment experiment;      ///< Experiment descriptor (current state).
    std::size_t control_samples   = 0;
    std::size_t treatment_samples = 0;
    double mean_control_score     = 0.0;
    double mean_treatment_score   = 0.0;
    double delta_pct              = 0.0; ///< (treatment-control)/control × 100.
    double p_value                = 1.0; ///< Welch t-test p-value.
    bool   significant            = false;
    std::string winner_version_id; ///< Non-empty once a winner is determined.

    /** @brief Serialise to JSON. */
    nlohmann::json toJson() const;
};

// ============================================================================
// PromptABExperimentFramework
// ============================================================================

/**
 * @brief Public A/B experiment framework for prompt template variants.
 *
 * Usage
 * -----
 * @code
 * PromptABExperimentFramework fw;
 *
 * // Register an experiment.
 * PromptExperiment exp;
 * exp.template_id           = "contract_summary";
 * exp.control_version_id    = "v1.0";
 * exp.treatment_version_id  = "v1.1";
 * exp.split_pct             = 10;  // 10 % → treatment
 * exp.min_samples           = 200;
 * auto id = fw.create(exp);
 *
 * // On each request, assign the variant deterministically.
 * ExperimentContext ctx{ id, user_id };
 * auto variant = fw.assignVariant(ctx);   // CONTROL or TREATMENT
 * std::string version_id = (variant == ExperimentVariant::TREATMENT)
 *     ? exp.treatment_version_id : exp.control_version_id;
 *
 * // Record the quality outcome.
 * fw.recordOutcome(id, variant, quality_score, user_id);
 *
 * // Inspect.
 * auto summary = fw.getSummary(id);
 * if (summary.significant) {
 *     std::string winner = fw.promoteWinner(id);
 * }
 * @endcode
 */
class PromptABExperimentFramework {
public:
    /// Callback fired when an experiment auto-promotes a winner.
    using WinnerCallback = std::function<void(const std::string& experiment_id,
                                               ExperimentVariant winner,
                                               const std::string& winning_version_id)>;

    PromptABExperimentFramework() = default;

    // -------------------------------------------------------------------------
    // Experiment lifecycle
    // -------------------------------------------------------------------------

    /**
     * @brief Register a new A/B experiment.
     *
     * If `exp.experiment_id` is empty, a unique ID is generated.
     *
     * @param exp Experiment descriptor (split_pct, min_samples, etc.).
     * @return The assigned `experiment_id`.
     */
    std::string create(PromptExperiment exp);

    /**
     * @brief Stop a running experiment without promoting a winner.
     *
     * Sets status to `INCONCLUSIVE` if no winner has been declared yet.
     *
     * @param experiment_id Experiment to stop.
     * @return `true` if the experiment existed and was stopped.
     */
    bool stop(const std::string& experiment_id);

    /**
     * @brief Get the experiment descriptor.
     * @return Experiment, or `nullopt` if not found.
     */
    std::optional<PromptExperiment> getExperiment(
        const std::string& experiment_id) const;

    /**
     * @brief Return all registered experiments (running and completed).
     */
    std::vector<PromptExperiment> listExperiments() const;

    // -------------------------------------------------------------------------
    // Variant assignment
    // -------------------------------------------------------------------------

    /**
     * @brief Deterministically assign CONTROL or TREATMENT for a request.
     *
     * Uses MurmurHash3-32 of `context.request_id` (seed `0x9747b28c`).
     * Returns `CONTROL` if the experiment is not found or no longer running.
     *
     * Assignment rule:
     *   murmur3_32(request_id) % 100 < split_pct  →  TREATMENT
     *   else                                        →  CONTROL
     *
     * @param context ExperimentContext with experiment_id and request_id.
     * @return Assigned variant.
     */
    ExperimentVariant assignVariant(const ExperimentContext& context) const;

    // -------------------------------------------------------------------------
    // Outcome recording & significance
    // -------------------------------------------------------------------------

    /**
     * @brief Record a quality outcome for a variant.
     *
     * Appends `score` to the variant's score vector.  Automatically calls
     * `checkSignificance()` once both variants have ≥ `min_samples`
     * observations.
     *
     * @param experiment_id Experiment identifier.
     * @param variant       Which side produced this outcome.
     * @param score         Quality score in [0.0, 1.0].
     * @param request_id    Originating request ID (for audit).
     * @return `false` if the experiment is not found or not `RUNNING`.
     */
    bool recordOutcome(
        const std::string& experiment_id,
        ExperimentVariant  variant,
        double             score,
        const std::string& request_id = "");

    /**
     * @brief Explicitly run the significance test for an experiment.
     *
     * Welch two-sample t-test; auto-updates status to WINNER_CONTROL or
     * WINNER_TREATMENT when significant and fires `winner_callback_` if set.
     *
     * @param experiment_id Experiment to test.
     * @return `true` if a winner was determined.
     */
    bool checkSignificance(const std::string& experiment_id);

    // -------------------------------------------------------------------------
    // Results
    // -------------------------------------------------------------------------

    /**
     * @brief Promote the winning variant and return its version ID.
     *
     * May only be called once the experiment status is WINNER_CONTROL or
     * WINNER_TREATMENT.  Sets status to COMPLETED.
     *
     * @param experiment_id Experiment to finalise.
     * @return Winning version ID, or empty string on error.
     */
    std::string promoteWinner(const std::string& experiment_id);

    /**
     * @brief Get the current status of an experiment.
     */
    ExperimentStatus getStatus(const std::string& experiment_id) const;

    /**
     * @brief Build a full summary for an experiment.
     * @return Summary, or an empty optional if not found.
     */
    std::optional<ExperimentSummary> getSummary(
        const std::string& experiment_id) const;

    /**
     * @brief Get all recorded outcomes for an experiment.
     * @param experiment_id Experiment to query.
     * @return Vector of ExperimentOutcome (may be empty).
     */
    std::vector<ExperimentOutcome> getOutcomes(
        const std::string& experiment_id) const;

    // -------------------------------------------------------------------------
    // Callbacks
    // -------------------------------------------------------------------------

    /**
     * @brief Attach a callback invoked when a winner is auto-promoted.
     * @param cb Callback; pass empty `std::function` to disable.
     */
    void setWinnerCallback(WinnerCallback cb);

private:
    mutable std::mutex mutex_;

    // experiment_id → descriptor
    std::unordered_map<std::string, PromptExperiment> experiments_;

    // experiment_id → (control_scores, treatment_scores)
    struct ScoreStore {
        std::vector<double> control;
        std::vector<double> treatment;
        std::vector<ExperimentOutcome> outcomes;
    };
    std::unordered_map<std::string, ScoreStore> scores_;

    WinnerCallback winner_callback_;

    // -------------------------------------------------------------------------
    // Internals
    // -------------------------------------------------------------------------

    /** @brief Generate a unique experiment ID. */
    static std::string generateId();

    /**
     * @brief 32-bit MurmurHash3 of an ASCII string.
     * Seed: 0x9747b28c (chosen to produce a uniform bucket distribution).
     */
    static std::uint32_t murmur3_32(const std::string& key) noexcept;

    /**
     * @brief Welch two-sample t-test; returns p-value in [0, 1].
     * Returns 1.0 when either sample is too small (< 2 elements).
     */
    static double welchPValue(const std::vector<double>& a,
                               const std::vector<double>& b) noexcept;

    /**
     * @brief Two-tailed p-value from t-statistic and degrees of freedom.
     * Uses a rational approximation to the Student-t CDF.
     */
    static double tDistCdf(double t, double df) noexcept;

    /** @brief checkSignificance() body called while holding mutex_. */
    bool checkSignificanceLocked(const std::string& experiment_id);
};

} // namespace prompt_engineering
} // namespace themis
