// Copyright 2026 ThemisDB — Licensed under MIT License
#pragma once

/**
 * @file federated_distillation_coordinator.h
 * @brief Federated Knowledge Distillation — teacher→student soft-label transfer
 *        with (ε, δ)-Differential Privacy.
 *
 * ## Motivation
 *
 * `LoRAFederationCoordinator` (Ebene B) federates LoRA *gradient* updates via
 * FedAvg/FedProx.  In scenarios where gradient sharing is too privacy-costly
 * or the teacher and student architectures differ, **Federated Distillation**
 * provides an alternative: the teacher shards share only *soft labels*
 * (probability vectors over a task-specific output vocabulary or class set),
 * and the student aggregates them.
 *
 * Soft-label exchange is complementary to gradient-based federation:
 *   • Soft labels convey less raw parameter information than gradients.
 *   • Calibrated Gaussian noise is applied to each label probability to
 *     enforce (ε, δ)-DP on the aggregated labels (same formula as Ebene B).
 *   • The student applies the aggregated labels in a cross-entropy distillation
 *     step, not a direct weight update.
 *
 * ## Flow (per distillation round)
 *
 * ```
 * Teacher shard k → computeSoftLabels(query_batch) → SoftLabelBatch
 *     ↓  (one per teacher shard)
 * FederatedDistillationCoordinator::submitLabels(shard_k, labels)
 *     ↓  (after min_teachers submitted or timeout)
 * aggregate() → arithmetic mean per label position
 *     ↓
 * addGaussianNoise(σ = sensitivity·√(2·ln(1.25/δ))/ε)
 *     ↓
 * AggregatedLabelBatch  ─→  student_callback_(batch)
 * ```
 *
 * ## Design Constraints
 *  - Raw training examples never leave a shard.
 *  - `submitLabels()` is idempotent per `(shard_id, round)`.
 *  - Thread-safe: all public methods are mutex-guarded.
 *  - PolicyGate DI-setter rejects rounds when governance policy denies transfer.
 *  - AuditCallback DI-setter emits an audit record after each round.
 *  - RollbackTrigger DI-setter is called when a round is abandoned (all rejected
 *    or policy-blocked) to let the student revert partial learning.
 *
 * Scientific references:
 *   Hinton, G. et al. (2015). "Distilling the Knowledge in a Neural Network."
 *   arXiv:1503.02531.
 *   Jeong, E. et al. (2018). "Communication-Efficient On-Device Machine Learning:
 *   Federated Distillation and Augmentation under Non-IID Private Data." NeurIPS FL Workshop.
 *   Dwork, C. et al. (2014). The Algorithmic Foundations of Differential Privacy.
 *
 * @see include/distributed_knowledge/lora_federation_coordinator.h — gradient federation
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

namespace themis::distributed_knowledge {

// ─────────────────────────────────────────────────────────────────────────────
// SoftLabelBatch — teacher output for one round
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Soft-label contribution from a single teacher shard.
 *
 * Each element of `labels` corresponds to one query/input; `labels[i]` is a
 * probability distribution over the output vocabulary (must sum to ≈1 after
 * normalisation, but the coordinator does not enforce this — it is the
 * teacher's responsibility).
 *
 * `labels` must be non-empty and all inner vectors must have the same length.
 */
struct SoftLabelBatch {
    /// Contributing shard identifier.
    std::string shard_id;
    /// Current distillation round.
    uint64_t    round{0};
    /// Soft labels: outer = queries, inner = probability over classes/tokens.
    std::vector<std::vector<double>> labels;
    /// Number of local training samples used to compute the labels.
    size_t      sample_count{0};

    [[nodiscard]] nlohmann::json toJson() const;
    [[nodiscard]] static SoftLabelBatch fromJson(const nlohmann::json& j);
};

// ─────────────────────────────────────────────────────────────────────────────
// AggregatedLabelBatch — coordinator output for one round
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief DP-noised aggregate of teacher soft labels for one distillation round.
 *
 * Distributed to the student shard after each round.
 */
struct AggregatedLabelBatch {
    /// Distillation round that produced this batch.
    uint64_t    round{0};
    /// Number of teacher shards that contributed.
    size_t      teachers{0};
    /// Aggregated labels: same shape as per-teacher `labels`.
    std::vector<std::vector<double>> labels;
    /// DP privacy budget spent for this round (ε_round).
    double      epsilon_spent{0.0};
    /// Monotonic version string, e.g. "distill-v7".
    std::string version;

    [[nodiscard]] nlohmann::json toJson() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// DistillationConfig — per-coordinator configuration
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Configuration for `FederatedDistillationCoordinator`.
 */
struct DistillationConfig {
    /// Minimum number of teacher shards required to trigger aggregation.
    size_t      min_teachers{1};
    /// ε parameter for the Gaussian DP noise applied to labels (must be > 0).
    double      dp_epsilon{1.0};
    /// δ parameter for the Gaussian DP noise (must be in (0, 1)).
    double      dp_delta{1e-5};
    /// Sensitivity of the soft-label mechanism (L2 bound on per-label influence).
    double      sensitivity{1.0};
    /// Maximum number of rounds before the privacy budget is exhausted.
    /// 0 = unlimited.
    size_t      max_rounds{0};

    [[nodiscard]] bool isValid() const {
        return min_teachers >= 1 &&
               dp_epsilon   >  0.0 &&
               dp_delta     >  0.0 && dp_delta < 1.0 &&
               sensitivity  >  0.0;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// FederatedDistillationCoordinator
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Coordinates federated knowledge distillation from N teacher shards
 *        to a single student shard.
 *
 * ### Usage
 * @code
 * DistillationConfig cfg;
 * cfg.min_teachers = 3;
 * cfg.dp_epsilon   = 2.0;
 * FederatedDistillationCoordinator coord(cfg);
 *
 * // Install student callback:
 * coord.setStudentCallback([](const AggregatedLabelBatch& b) {
 *     student.applyDistillationBatch(b.labels);
 * });
 *
 * // Each teacher shard submits its soft labels:
 * coord.submitLabels({shard_id, round, labels, sample_count});
 *
 * // After min_teachers have submitted, aggregation fires automatically.
 * // Or trigger manually:
 * coord.triggerAggregation();
 * @endcode
 */
class FederatedDistillationCoordinator {
public:
    explicit FederatedDistillationCoordinator(DistillationConfig config = {});
    ~FederatedDistillationCoordinator() = default;

    FederatedDistillationCoordinator(const FederatedDistillationCoordinator&)            = delete;
    FederatedDistillationCoordinator& operator=(const FederatedDistillationCoordinator&) = delete;
    FederatedDistillationCoordinator(FederatedDistillationCoordinator&&)                 noexcept = default;
    FederatedDistillationCoordinator& operator=(FederatedDistillationCoordinator&&)      noexcept = default;

    // ── Core API ─────────────────────────────────────────────────────────────

    /**
     * @brief Submit soft labels from a teacher shard.
     *
     * Idempotent per `(shard_id, round)`.  Triggers aggregation automatically
     * when `submittedCount() >= config.min_teachers`.
     *
     * @param batch  Soft-label batch from the teacher shard.
     * @throws std::invalid_argument  on malformed batch (empty labels, mismatched sizes).
     * @throws std::runtime_error     when privacy budget is exhausted.
     */
    void submitLabels(const SoftLabelBatch& batch);

    /**
     * @brief Manually trigger aggregation of all currently submitted labels.
     *
     * @return  The aggregated label batch.
     * @throws std::runtime_error  when fewer than `min_teachers` have submitted,
     *                             when the policy gate rejects the round, or
     *                             when all labels are filtered out.
     */
    AggregatedLabelBatch triggerAggregation();

    // ── Accessors ─────────────────────────────────────────────────────────────

    [[nodiscard]] uint64_t currentRound()    const;
    [[nodiscard]] size_t   submittedCount()  const;
    [[nodiscard]] size_t   completedRounds() const;
    [[nodiscard]] double   privacyBudgetRemaining() const;
    [[nodiscard]] bool     verifyPrivacyBudget() const;
    [[nodiscard]] std::optional<AggregatedLabelBatch> lastBatch() const;
    [[nodiscard]] const DistillationConfig& config() const { return config_; }

    // ── DI-setters ────────────────────────────────────────────────────────────

    /**
     * @brief Inject the student callback.
     *
     * Called after every successful aggregation with the `AggregatedLabelBatch`.
     */
    void setStudentCallback(std::function<void(const AggregatedLabelBatch&)> cb);

    /**
     * @brief Inject a policy gate (FDF DI-setter).
     *
     * Invoked before aggregation.  If the gate returns `false`, aggregation is
     * aborted for the current round and the rollback trigger (if set) is called.
     * Signature: `bool gate(round, submittedCount)`.
     */
    void setPolicyGate(std::function<bool(uint64_t round, size_t teachers)> gate);

    /**
     * @brief Inject an audit callback (FDF DI-setter).
     *
     * Called after every successful aggregation with a JSON audit record
     * containing `decision_type`, `round`, `teachers`, `epsilon_spent`.
     */
    void setAuditCallback(std::function<void(const nlohmann::json&)> cb);

    /**
     * @brief Inject a rollback trigger (FDF DI-setter).
     *
     * Called when a round is abandoned (policy-blocked or all labels filtered).
     * Signature: `void trigger(round, reason_message)`.
     */
    void setRollbackTrigger(std::function<void(uint64_t round, const std::string& reason)> trigger);

private:
    DistillationConfig                              config_;
    uint64_t                                        current_round_{1};
    std::map<std::string, SoftLabelBatch>           pending_labels_; ///< shard_id → labels
    std::optional<AggregatedLabelBatch>             last_batch_;
    size_t                                          completed_rounds_{0};
    double                                          total_epsilon_spent_{0.0};

    // DI callbacks
    std::function<void(const AggregatedLabelBatch&)>                 student_callback_;
    std::function<bool(uint64_t, size_t)>                            policy_gate_;
    std::function<void(const nlohmann::json&)>                       audit_callback_;
    std::function<void(uint64_t, const std::string&)>                rollback_trigger_;

    mutable std::mutex mutex_;

    // Internal helpers
    [[nodiscard]] AggregatedLabelBatch doAggregation();
    [[nodiscard]] std::vector<std::vector<double>> applyGaussianNoise(
        std::vector<std::vector<double>> labels) const;
    [[nodiscard]] std::string nextBatchVersion() const;
};

} // namespace themis::distributed_knowledge
