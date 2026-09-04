#pragma once

/**
 * @file federated_distillation_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 96/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "distributed_knowledge/distributed_knowledge_api_contract.h"

namespace themis {
namespace distributed_knowledge {

// ─────────────────────────────────────────────────────────────────────────────
// SoftLabel — temperature-scaled probability distribution from teacher model
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A single teacher soft-label prediction for one query.
 *
 * `probabilities` is the softmax output of the teacher model after dividing
 * logits by `temperature`.  Higher temperature → softer distribution.
 *
 * Only `probabilities` is shared; the raw query text stays on the teacher node.
 */
struct SoftLabel {
    std::string              query_id;       ///< Opaque identifier (e.g. hash of query)
    std::vector<double>      probabilities;  ///< Temperature-scaled softmax distribution
    double                   temperature;    ///< Distillation temperature T (1.0 = argmax)
    std::string              teacher_id;     ///< Opaque teacher model identifier

    [[nodiscard]] nlohmann::json toJson() const {
        return {{"query_id",      query_id},
                {"probabilities", probabilities},
                {"temperature",   temperature},
                {"teacher_id",    teacher_id}};
    }

    [[nodiscard]] static SoftLabel fromJson(const nlohmann::json& j) {
        SoftLabel s;
        s.query_id     = j.value("query_id", "");
        s.probabilities = j.value("probabilities", std::vector<double>{});
        s.temperature  = j.value("temperature", 1.0);
        s.teacher_id   = j.value("teacher_id", "");
        return s;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// DistillationRound — coordinator-signed batch of soft labels for one round
// ─────────────────────────────────────────────────────────────────────────────

struct DistillationRound {
    uint64_t                 round = 0;          ///< Monotonic round counter
    std::string              teacher_id;     ///< Teacher model identifier
    std::vector<SoftLabel>   labels;         ///< DP-protected soft labels
    double                   epsilon_spent;  ///< DP budget spent this round
    size_t                   label_count;    ///< Number of soft labels
    bool                     dp_applied;     ///< Whether DP noise was applied

    [[nodiscard]] nlohmann::json toJson() const {
        nlohmann::json js_labels = nlohmann::json::array();
        for (const auto& l : labels) {
          js_labels.push_back(l.toJson());
        }
        return {{"round",         round},
                {"teacher_id",    teacher_id},
                {"labels",        js_labels},
                {"epsilon_spent", epsilon_spent},
                {"label_count",   label_count},
                {"dp_applied",    dp_applied}};
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// DistillationModelCard — governance snapshot per coordinator lifecycle
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Per-session governance snapshot produced by the coordinator.
 *
 * Captures the privacy, utility, and policy history up to the point of
 * generation.  Intended for audit logs, model registries, and risk reports
 * as required by issue #4743 Phase 4 ("Modellkarten + Risikoakte").
 *
 * Immutable once generated — call `generateModelCard()` to get a fresh
 * snapshot.  Serialisable to JSON via `toJson()`.
 */
struct DistillationModelCard {
    // Identity
    std::string coordinator_id;     ///< Coordinator instance identifier (optional)
    std::string teacher_id;         ///< Last teacher model identifier (or empty)

    // Privacy accounting
    uint64_t    rounds_completed;   ///< Total rounds broadcast
    double      total_epsilon;      ///< DP budget spent (sum over all rounds)
    double      dp_epsilon_per_round; ///< Configured ε per round
    double      dp_delta;           ///< Configured δ
    bool        dp_applied;         ///< Whether DP noise was applied in this session

    // Utility
    double      min_utility_reported; ///< Lowest utility reported by any student
    double      max_utility_reported; ///< Highest utility reported (1.0 = no reports)
    size_t      rollback_count;     ///< Number of rollbacks triggered

    // Policy
    size_t      policy_blocks;      ///< Broadcasts blocked by policy gate
    size_t      registered_students; ///< Number of registered student callbacks

    [[nodiscard]] nlohmann::json toJson() const {
        return {{"coordinator_id",      coordinator_id},
                {"teacher_id",         teacher_id},
                {"rounds_completed",   rounds_completed},
                {"total_epsilon",      total_epsilon},
                {"dp_epsilon_per_round", dp_epsilon_per_round},
                {"dp_delta",           dp_delta},
                {"dp_applied",         dp_applied},
                {"min_utility_reported", min_utility_reported},
                {"max_utility_reported", max_utility_reported},
                {"rollback_count",     rollback_count},
                {"policy_blocks",      policy_blocks},
                {"registered_students", registered_students}};
    }
};



struct DistillationConfig {
    // DP parameters (Gaussian mechanism)
    double dp_epsilon   = 0.5;    ///< DP ε per round (lower = more private)
    double dp_delta     = 1e-5;   ///< DP failure probability δ
    double dp_sensitivity = 0.1;  ///< L2 sensitivity of probability vectors

    // Distillation hyper-parameters
    double temperature  = 4.0;    ///< Default distillation temperature
    double alpha        = 0.5;    ///< Blend weight (0=hard labels, 1=soft labels only)

    // Budget control
    size_t max_rounds   = 0;      ///< 0 = unlimited

    // Utility gate — trigger rollback if student utility drops below this
    double min_utility_threshold = 0.90; ///< Fraction of teacher utility required

    // Require DP noise before broadcasting
    bool require_dp     = true;

    [[nodiscard]] bool isValid() const {
        return dp_epsilon > 0.0 && dp_epsilon <= 1.0 &&
               dp_delta   > 0.0 && dp_delta < 0.1 &&
               dp_sensitivity > 0.0 &&
               temperature >= 1.0 &&
               alpha >= 0.0 && alpha <= 1.0 &&
               min_utility_threshold > 0.0 && min_utility_threshold <= 1.0;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// IFederatedDistillationCoordinator — public interface
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Pure-virtual interface for the federated distillation coordinator.
 *
 * Injected by the cluster manager; tests inject a mock.
 */
class IFederatedDistillationCoordinator {
public:
    virtual ~IFederatedDistillationCoordinator() noexcept = default;

    /**
    * @brief Teacher submits soft labels for the current round.
    *
    * **Consistency Level: CAUSAL**
    *  - Operation preserves causal ordering by enforcing sequential submission
    *  - First submit after prior broadcast automatically advances current_round_
    *  - Subsequent submits within same round are idempotent (overwrite previous)
    *  - Rationale: Round number ensures students receive labels in order
    *
    * **Version Tracking:**
    *  - current_round_ implicitly versioned; automatically incremented on submit
    *  - Projection: if broadcast was recent, next submit increments round
    *
    * @param teacher_id  Teacher model identifier.
    * @param labels      Temperature-scaled predictions (one per shared query).
    * @throws std::runtime_error on budget exhaustion or policy rejection.
    * @throws std::invalid_argument if labels or teacher_id is empty.
    */
    virtual void submitSoftLabels(const std::string& teacher_id,
                                  std::vector<SoftLabel> labels) = 0;

    /**
    * @brief Broadcast DP-protected labels to all registered student callbacks.
    *
    * **Consistency Level: STRONG**
    *  - Privacy budget check (verifyPrivacyBudget) blocks if exhausted
    *  - Policy gate blocks if teacher/round violates policy
    *  - Operation atomic: either succeeds fully or throws (strong safety)
    *  - Rationale: Distribution must be consistent; partial broadcast unacceptable
    *
    * **Version Tracking:**
    *  - DistillationRound.round is causally ordered after submit
    *  - Round N+1 broadcast only after round N round is complete
    *
    * @return The `DistillationRound` that was broadcast.
    * @throws std::runtime_error when no labels have been submitted this round
    *         or policy gate rejects the broadcast.
    */
    virtual DistillationRound broadcastToStudents() = 0;

    /**
     * @brief Current federated round number (starts at 1 after first submit).
     */
    [[nodiscard]] virtual uint64_t currentRound() const = 0;

    /**
     * @brief Number of soft-label sets submitted for the current round.
     */
    [[nodiscard]] virtual size_t submittedCount() const = 0;

    /**
     * @brief Last broadcast round, if any.
     */
    [[nodiscard]] virtual std::optional<DistillationRound> lastRound() const = 0;

    /**
     * @brief Observability stats as JSON.
     */
    [[nodiscard]] virtual nlohmann::json getStats() const = 0;

    /**
     * @brief Register a student callback to receive broadcast rounds.
     *
     * Multiple students may register.  The coordinator calls every callback
     * (in registration order) during `broadcastToStudents()`.
     *
     * @param student_id  Opaque student identifier (used in audit records).
     * @param cb          Callback receiving the broadcast `DistillationRound`.
     */
    virtual void registerStudent(
        const std::string& student_id,
        std::function<void(const DistillationRound&)> cb) = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// FederatedDistillationCoordinator — production implementation
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Production implementation of the federated distillation coordinator.
 *
 * Thread-safe: all public methods are mutex-protected.
 *
 * ### DP Noise Injection (Gaussian mechanism)
 *
 * For each probability value `p_i` in the soft label:
 *   `p_noisy_i = p_i + Gaussian(0, σ²)`
 * where `σ = sensitivity * sqrt(2 * ln(1.25/δ)) / ε`.
 * The noisy distribution is then re-normalised to sum to 1.0 and clamped to
 * [0, 1] to maintain a valid probability simplex.
 *
 * ### Policy Gate
 *
 * An injectable callback of type `PolicyGate` is called before each broadcast.
 * Return `true` to allow, `false` to block.  When blocked, `broadcastToStudents()`
 * throws `std::runtime_error("Policy gate rejected distillation broadcast")`.
 */
class FederatedDistillationCoordinator : public IFederatedDistillationCoordinator {
public:
    /**
     * @brief Callback invoked before each broadcast.
     * @param round      Current round number.
     * @param teacher_id Teacher that submitted labels.
     * @return `true` if broadcast is permitted; `false` to block.
     */
    using PolicyGate =
        std::function<bool(uint64_t round, const std::string& teacher_id)>;

    explicit FederatedDistillationCoordinator(DistillationConfig cfg = {});
    ~FederatedDistillationCoordinator() noexcept override;

    FederatedDistillationCoordinator(const FederatedDistillationCoordinator&)            = delete;
    FederatedDistillationCoordinator& operator=(const FederatedDistillationCoordinator&) = delete;
    FederatedDistillationCoordinator(FederatedDistillationCoordinator&&)                 noexcept = default;
    FederatedDistillationCoordinator& operator=(FederatedDistillationCoordinator&&)      noexcept;

    // ── IFederatedDistillationCoordinator ─────────────────────────────────────

    void submitSoftLabels(const std::string& teacher_id,
                          std::vector<SoftLabel> labels) override;

    DistillationRound broadcastToStudents() override;

    [[nodiscard]] uint64_t                        currentRound()   const override;
    [[nodiscard]] size_t                          submittedCount() const override;
    [[nodiscard]] std::optional<DistillationRound> lastRound()     const override;
    [[nodiscard]] nlohmann::json                  getStats()       const override;

    void registerStudent(const std::string& student_id,
                         std::function<void(const DistillationRound&)> cb) override;

    // ── DI setters ─────────────────────────────────────────────────────────────

    /**
     * @brief Type alias for a pluggable Gaussian noise generator.
     *
     * @param labels  Label vector to modify in-place.
     * @param sigma   Pre-computed noise standard deviation from the Gaussian
     *                mechanism (derived from ε, δ, and sensitivity).
     */
    using NoiseGeneratorFn =
        std::function<void(std::vector<SoftLabel>& labels, double sigma)>;

    /**
     * @brief Inject a noise generator for DP noise application.
     *
     * When set, `applyDPNoise()` delegates to this function instead of the
     * built-in CPU `std::random_device`-seeded Gaussian path.  Pass an empty
     * (default-constructed) function to clear and revert to the CPU fallback.
     *
     * @par Production use
     * Inject a GPU-backed Gaussian noise kernel (CUDA cuRAND / HIP hipRAND)
     * for batch efficiency on large label distributions.  The CPU path is
     * retained as the documented fallback for builds without GPU support.
     *
     * @param fn  Callable that applies DP noise in-place; receives the label
     *            vector and the pre-computed sigma value.
     */
    void setNoiseGeneratorFn(NoiseGeneratorFn fn);

    /**
     * @brief Inject a policy gate callback.
     *
     * Called in `broadcastToStudents()`.  Pass `nullptr` to clear (allow all).
     */
    void setPolicyGate(PolicyGate gate);

    /**
     * @brief Apply a `DistillationBoundedPolicy` for runtime enforcement.
     *
     * Enforces hard caps on broadcast rounds and DP-epsilon budget
     * with FAIL_CLOSED semantics.  Any call to `broadcastToStudents()` that
     * would violate `policy.max_distillation_rounds` or
     * `policy.privacy_budget_hard_limit` throws `std::runtime_error`
     * immediately before executing the broadcast.
     *
     * ### Enforcement semantics
     * - `max_distillation_rounds != 0`: throws when `current_round_ >=
     *   policy.max_distillation_rounds` (round numbering starts at 0).
     * - `privacy_budget_hard_limit > 0.0`: throws when
     *   `total_epsilon_spent_ + per_round_epsilon >= policy.privacy_budget_hard_limit`.
     * - `policy_gate_enforcement == FAIL_CLOSED` (default): violations throw;
     *   no fallback or silent degradation is permitted.
     *
     * Pass a default-constructed (unconstrained) policy to clear enforcement.
     *
     * @param policy  Bounded policy to apply; replaces any previously set policy.
     *
     * @since Phase 2 hardening (Q4 2026)
     */
    void setBoundedPolicy(DistillationBoundedPolicy policy);

    /**
     * @brief Inject an audit callback.
     *
     * Called after every successful broadcast with a JSON audit record.
     */
    void setAuditCallback(std::function<void(const nlohmann::json&)> cb);

    /**
     * @brief Inject a rollback trigger.
     *
     * Called when `reportStudentUtility()` drops below
     * `DistillationConfig::min_utility_threshold`.
     *
     * @param cb  `void(uint64_t round, double utility)` — trigger rollback logic.
     */
    void setRollbackTrigger(std::function<void(uint64_t, double)> cb);

    // ── Utility reporting ──────────────────────────────────────────────────────

    /**
     * @brief Report student utility for the last broadcast round.
     *
     * If `utility` is below `config_.min_utility_threshold`, the registered
     * rollback trigger (if any) is called.
     *
     * @param student_id  Student reporting.
     * @param utility     Fraction of teacher utility achieved (0.0–1.0).
     */
    void reportStudentUtility(const std::string& student_id, double utility);

    // ── Budget observability ───────────────────────────────────────────────────

    /**
     * @brief Remaining DP epsilon budget.
     *
     * Returns `std::numeric_limits<double>::max()` when `max_rounds == 0`.
     */
    [[nodiscard]] double privacyBudgetRemaining() const;

    /**
     * @brief True when further rounds are permitted under the budget.
     */
    [[nodiscard]] bool verifyPrivacyBudget() const;

    /**
     * @brief Reset to round 0 and clear pending submissions (for testing).
     */
    void reset();

    /**
     * @brief Generate a governance model card snapshot.
     *
     * Captures privacy accounting, utility statistics, and policy history
     * at the point of invocation.  Thread-safe.
     *
     * @param coordinator_id  Optional identifier for this coordinator instance.
     * @return Immutable `DistillationModelCard` snapshot.
     */
    [[nodiscard]] DistillationModelCard generateModelCard(
        const std::string& coordinator_id = "") const;

    [[nodiscard]] const DistillationConfig& config() const { return config_; }

private:
    // ── Implementation ─────────────────────────────────────────────────────────

    /// Apply Gaussian DP noise and re-normalise each SoftLabel in-place.
    void applyDPNoise(std::vector<SoftLabel>& labels) const;

    DistillationConfig config_;

    mutable std::mutex                         mutex_;
    uint64_t                                   current_round_  = 0;
    double                                     total_epsilon_spent_ = 0.0;
    size_t                                     broadcast_count_ = 0;
    size_t                                     rollback_count_  = 0;
    size_t                                     policy_block_count_ = 0;
    double                                     min_utility_reported_ = 1.0;
    double                                     max_utility_reported_ = 1.0;
    bool                                       any_utility_reported_ = false;

    // Pending submission for current round
    std::string                                pending_teacher_id_;
    std::vector<SoftLabel>                     pending_labels_;
    bool                                       has_pending_ = false;

    std::optional<DistillationRound>           last_round_;

    // Registered students
    std::vector<std::pair<std::string,
        std::function<void(const DistillationRound&)>>>   students_;

    // Callbacks
    PolicyGate                                 policy_gate_;
    DistillationBoundedPolicy                  bounded_policy_;
    std::function<void(const nlohmann::json&)> audit_cb_;
    std::function<void(uint64_t, double)>      rollback_trigger_;
    NoiseGeneratorFn                           noise_generator_fn_;
};

} // namespace distributed_knowledge
} // namespace themis
