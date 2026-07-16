/**
 * @file prompt_ab_experiment.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

#include "prompt_engineering/prompt_template_compiler.h"

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

// ══════════════════════════════════════════════════════════════════════════════
// IPromptABFramework — clean per-user deterministic variant-assignment interface
// ══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Opaque user identifier (any UTF-8 string; typically a session or
 *        tenant ID; never stored by the framework).
 */
using UserId       = std::string;

/**
 * @brief Opaque experiment key (e.g. `"rag-system-prompt-v2"`).
 */
using ExperimentKey = std::string;

/**
 * @brief Variant descriptor returned by `IPromptABFramework::assignVariant()`.
 *
 * `templateRef` is a non-owning pointer to the template associated with this
 * variant (may be `nullptr` when the experiment has no associated template).
 * `trafficWeight` is the configured fraction of traffic (0.0–1.0) assigned to
 * this variant.
 */
struct ABVariant {
    std::string           variantId;      ///< Stable identifier, e.g. `"control"` / `"treatment"`.
    const IPromptTemplate* templateRef;   ///< Non-owning; may be nullptr.
    double                trafficWeight;  ///< Fraction of traffic (0.0–1.0).
};

/**
 * @brief Descriptor used by `IPromptABFramework::listExperiments()`.
 */
struct ExperimentDescriptor {
    ExperimentKey         key;            ///< Unique experiment key.
    std::vector<ABVariant> variants;      ///< All variants for this experiment.
    bool                  active;         ///< Whether the experiment is currently running.
};

/**
 * @brief Abstract interface for deterministic per-user A/B variant assignment.
 *
 * Defined in `FUTURE_ENHANCEMENTS.md §Prompt A/B Experimentation Framework`.
 *
 * Contract
 * --------
 * - `assignVariant()` is a pure function: same `(UserId, ExperimentKey)` pair
 *   always returns the same `ABVariant` while the experiment is active.
 * - The implementation must not store raw user identifiers.
 * - `listExperiments()` returns a stable span; the span is invalidated if any
 *   experiment is added or removed.
 */
class IPromptABFramework {
public:
    virtual ~IPromptABFramework() = default;

    /**
     * @brief Deterministically assign a variant to the given user/experiment pair.
     *
     * @param user_id  Opaque user identifier.
     * @param key      Experiment key.
     * @return The assigned `ABVariant`.  If the experiment does not exist or is
     *         inactive, the "control" variant is returned.
     */
    virtual ABVariant assignVariant(const UserId&        user_id,
                                    const ExperimentKey& key) const = 0;

    /**
     * @brief Return all registered experiment descriptors.
     */
    [[nodiscard]] virtual std::vector<ExperimentDescriptor> listExperiments() const = 0;
};

// ── Concrete implementation ───────────────────────────────────────────────────

/**
 * @brief Simple deterministic `IPromptABFramework` backed by an in-memory
 *        experiment registry.
 *
 * Variant assignment uses FNV-1a-32 over `user_id + '\0' + key` modulo 100
 * to decide traffic split, matching the behaviour of `PromptABExperimentFramework`.
 *
 * Thread-safe: `assignVariant()` and `listExperiments()` are safe for
 * concurrent read access.  `registerExperiment()` and `deactivate()` are
 * serialised with a shared mutex.
 */
class SimplePromptABFramework final : public IPromptABFramework {
public:
    /**
     * @brief Register an experiment with the framework.
     *
     * @param descriptor  Experiment to register (must have a unique `key`).
     * @throws std::invalid_argument if an experiment with the same key already
     *         exists.
     */
    void registerExperiment(ExperimentDescriptor descriptor);

    /**
     * @brief Deactivate a running experiment (does not remove it from the list).
     *
     * @param key  Experiment key to deactivate.
     * @return `true` if the experiment was found and deactivated; `false` otherwise.
     */
    bool deactivate(const ExperimentKey& key);

    // IPromptABFramework interface
    ABVariant assignVariant(const UserId&        user_id,
                            const ExperimentKey& key) const override;

    std::vector<ExperimentDescriptor> listExperiments() const override;

private:
    /// FNV-1a-32 hash over `user_id + '\0' + key`.
    static uint32_t fnv1a32(const std::string& user_id,
                             const std::string& key) noexcept;

    mutable std::mutex                         mutex_;
    std::vector<ExperimentDescriptor>          experiments_;
};

} // namespace prompt_engineering
} // namespace themis
