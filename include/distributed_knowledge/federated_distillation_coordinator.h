// SPDX-License-Identifier: MIT
// Copyright 2026 ThemisDB — Licensed under MIT License
#pragma once

/**
 * @file federated_distillation_coordinator.h
 * @brief Federated Distillation Coordinator — teacher-student knowledge transfer
 *        across institution/tenant boundaries without raw data exchange.
 *
 * ## Protocol
 *
 * ```
 * Teacher node
 *   └─ infer(query) → raw_logits
 *   └─ temperature_scale(raw_logits, T) → soft_label
 *   └─ submitSoftLabels(round, [{query_id, soft_label}])
 *           ↓
 * FederatedDistillationCoordinator
 *   └─ applyDPNoise(labels, ε, δ)          — Gaussian mechanism
 *   └─ checkPolicyGate()                   — cross-border / governance
 *   └─ broadcastSoftLabels(round, labels) →
 *           ↓
 * Student node(s)
 *   └─ receiveSoftLabels(labels) → distillation_loss += KL(student || teacher)
 *   └─ train() → LoRA adapter update
 * ```
 *
 * ## Design Constraints
 *   - Raw training samples / cleartext query text never leave the teacher node.
 *   - Only temperature-scaled logit distributions (soft labels) are shared.
 *   - Gaussian DP noise is applied *before* broadcasting; ε is tracked per round.
 *   - A configurable `PolicyGate` callback can block distribution for any shard.
 *   - Rollback is triggered automatically when utility falls below threshold.
 *
 * ## Scientific references
 *   Hinton, G., Vinyals, O., Dean, J. (2015). "Distilling the Knowledge in a
 *   Neural Network." NIPS Workshop.
 *   Anil, R., et al. (2018). "Large-Scale Distributed Neural Network Training
 *   Through Online Distillation." ICLR 2018.
 *   Dwork, C., et al. (2014). "The Algorithmic Foundations of Differential
 *   Privacy." Foundations and Trends in Theoretical Computer Science.
 *
 * @see include/distributed_knowledge/lora_federation_coordinator.h — gradient path
 * @see include/training/adapter_serving.h — rollback/deploy lifecycle
 * @see include/training/incremental_lora_trainer.h — student trainer interface
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
    uint64_t                 round;          ///< Monotonic round counter
    std::string              teacher_id;     ///< Teacher model identifier
    std::vector<SoftLabel>   labels;         ///< DP-protected soft labels
    double                   epsilon_spent;  ///< DP budget spent this round
    size_t                   label_count;    ///< Number of soft labels
    bool                     dp_applied;     ///< Whether DP noise was applied

    [[nodiscard]] nlohmann::json toJson() const {
        nlohmann::json js_labels = nlohmann::json::array();
        for (const auto& l : labels) js_labels.push_back(l.toJson());
        return {{"round",         round},
                {"teacher_id",    teacher_id},
                {"labels",        js_labels},
                {"epsilon_spent", epsilon_spent},
                {"label_count",   label_count},
                {"dp_applied",    dp_applied}};
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// DistillationConfig
// ─────────────────────────────────────────────────────────────────────────────

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
    virtual ~IFederatedDistillationCoordinator() = default;

    /**
     * @brief Teacher submits soft labels for the current round.
     *
     * @param teacher_id  Teacher model identifier.
     * @param labels      Temperature-scaled predictions (one per shared query).
     * @throws std::runtime_error on budget exhaustion or policy rejection.
     */
    virtual void submitSoftLabels(const std::string& teacher_id,
                                  std::vector<SoftLabel> labels) = 0;

    /**
     * @brief Broadcast DP-protected labels to all registered student callbacks.
     *
     * @return The `DistillationRound` that was broadcast.
     * @throws std::runtime_error when no labels have been submitted this round.
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
    ~FederatedDistillationCoordinator() override;

    FederatedDistillationCoordinator(const FederatedDistillationCoordinator&)            = delete;
    FederatedDistillationCoordinator& operator=(const FederatedDistillationCoordinator&) = delete;

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
     * @brief Inject a policy gate callback.
     *
     * Called in `broadcastToStudents()`.  Pass `nullptr` to clear (allow all).
     */
    void setPolicyGate(PolicyGate gate);

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
    std::function<void(const nlohmann::json&)> audit_cb_;
    std::function<void(uint64_t, double)>      rollback_trigger_;
};

} // namespace distributed_knowledge
} // namespace themis
