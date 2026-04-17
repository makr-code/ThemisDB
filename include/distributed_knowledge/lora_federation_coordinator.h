// Copyright 2026 ThemisDB — Licensed under MIT License
#pragma once

/**
 * @file lora_federation_coordinator.h
 * @brief Ebene B — Federated LoRA Gradient Aggregation (RAID-5-Kern)
 *
 * Implements RAID-5-style knowledge sharding: each shard exports a
 * Differential-Privacy-protected gradient contribution after its local
 * `IncrementalLoRATrainer` round.  A central (or rotating leader) coordinator
 * aggregates contributions via FedAvg / FedProx, applies DP noise, and
 * distributes the resulting global delta back to every shard.
 *
 * No shard receives raw training data from any other shard.  Only DP-noised
 * gradient aggregates are exchanged.
 *
 * ## Flow (every `federation_interval_hours` or on manual trigger)
 *
 * ```
 * Shard k: IncrementalLoRATrainer → exportGradient() → EncryptedGradient
 *     ↓  (N shards)
 * LoRAFederationCoordinator::submitGradient(shard_k, gradient)
 *     ↓  (after all expected shards submitted or timeout)
 * FederatedAggregator::aggregateUpdates([...], "FedAvg")
 *     ↓
 * DifferentialPrivacyManager::addDifferentialPrivacy(ε, δ)
 *     ↓
 * GlobalAdapterDelta  ─→  applyGlobalDelta(delta) on every shard
 * ```
 *
 * ## Design Constraints
 *  - `ILoRAFederationCoordinator` is the public interface; the default
 *    implementation (`LoRAFederationCoordinator`) uses the existing
 *    `FederatedImportCoordinator::FederatedAggregator` internally.
 *  - `submitGradient()` is idempotent per (shard_id, round): duplicate
 *    submissions from the same shard in the same round are ignored.
 *  - `triggerAggregation()` can be called manually (e.g. from tests or admin
 *    API) without waiting for the timer.
 *  - Thread-safe: all public methods are mutex-protected.
 *
 * @see include/importers/federated_learning.h  — FederatedAggregator + DP
 * @see include/training/incremental_lora_trainer.h  — gradient producer
 * @see include/rag/continuous_learning_orchestrator.h — FEDERATED_ROUND_START trigger
 *
 * Scientific references:
 *   McMahan, H.B. et al. (2017). Communication-Efficient Learning of Deep
 *   Networks from Decentralised Data. AISTATS 2017.
 *   Dwork, C. et al. (2014). The Algorithmic Foundations of Differential
 *   Privacy. Foundations and Trends in Theoretical Computer Science.
 */

#include "distributed_knowledge/adapter_capability_announcement.h"
#include "llm/decision_record_yaml_processor.h"

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis::distributed_knowledge {

// ─────────────────────────────────────────────────────────────────────────────
// EncryptedGradient — opaque per-shard gradient contribution
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Opaque LoRA gradient contribution exported by one shard.
 *
 * The `data` blob is treated as opaque by the coordinator; only the
 * `FederatedAggregator` performs numeric aggregation.  This models the
 * "encrypted gradient" pattern from the FedAvg literature where raw weight
 * updates travel as structured JSON that cannot be linked back to individual
 * training samples.
 */
struct EncryptedGradient {
    std::string shard_id;         ///< Contributing shard
    uint64_t    round;            ///< Federated round number
    size_t      sample_count;     ///< Number of local samples (used for FedAvg weighting)
    nlohmann::json data;          ///< Opaque gradient payload (key→float delta map)

    // Serialisation
    [[nodiscard]] nlohmann::json toJson() const {
        return {{"shard_id", shard_id},
                {"round",    round},
                {"sample_count", sample_count},
                {"data",     data}};
    }

    [[nodiscard]] static EncryptedGradient fromJson(const nlohmann::json& j) {
        return {j.value("shard_id", ""),
                j.value<uint64_t>("round", 0),
                j.value<size_t>("sample_count", 0),
                j.value("data", nlohmann::json::object())};
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// GlobalAdapterDelta — aggregated, DP-protected update for all shards
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Aggregated LoRA weight delta produced by the coordinator.
 *
 * Distributed to every participating shard at the end of each federated round.
 * Each shard calls `IncrementalLoRATrainer::applyGlobalDelta()` with this
 * payload to incorporate global knowledge without full retraining.
 */
struct GlobalAdapterDelta {
    uint64_t       round;               ///< Federated round that produced this delta
    std::string    version;             ///< Monotonic version string, e.g. "global-v42"
    size_t         participants;        ///< Number of shards that contributed
    std::string    algorithm;           ///< Aggregation algorithm used ("FedAvg" etc.)
    double         epsilon_spent;       ///< DP privacy budget spent this round
    nlohmann::json delta;               ///< Aggregated weight delta (key→float map)

    [[nodiscard]] nlohmann::json toJson() const {
        return {{"round",          round},
                {"version",        version},
                {"participants",   participants},
                {"algorithm",      algorithm},
                {"epsilon_spent",  epsilon_spent},
                {"delta",          delta}};
    }

    [[nodiscard]] static GlobalAdapterDelta fromJson(const nlohmann::json& j) {
        GlobalAdapterDelta g;
        g.round        = j.value<uint64_t>("round", 0);
        g.version      = j.value("version", "");
        g.participants = j.value<size_t>("participants", 0);
        g.algorithm    = j.value("algorithm", "FedAvg");
        g.epsilon_spent = j.value("epsilon_spent", 0.0);
        g.delta        = j.value("delta", nlohmann::json::object());
        return g;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// FederationConfig
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Configuration for a `LoRAFederationCoordinator` instance.
 */
struct FederationConfig {
    // Participation
    size_t min_participants      = 2;       ///< Minimum shards required for aggregation
    size_t max_participants      = 64;      ///< Maximum shards tracked per round
    std::chrono::hours federation_interval{24}; ///< Auto-trigger interval

    // Aggregation
    std::string aggregation_algorithm = "FedAvg"; ///< "FedAvg" | "FedProx" | "median"
    bool weight_by_sample_count = true;    ///< Weight gradient by shard sample count

    // Differential Privacy
    double dp_epsilon  = 0.1;   ///< Privacy budget ε (lower = more private)
    double dp_delta    = 1e-5;  ///< Failure probability δ
    double dp_sensitivity = 1.0; ///< L2 sensitivity of gradient

    // Timeout
    std::chrono::minutes round_timeout{60}; ///< Max wait for all shards per round

    // Validate
    [[nodiscard]] bool isValid() const {
        return min_participants >= 2 &&
               max_participants >= min_participants &&
               dp_epsilon > 0.0 && dp_epsilon <= 1.0 &&
               dp_delta   > 0.0 && dp_delta < 0.1 &&
               dp_sensitivity > 0.0;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// ILoRAFederationCoordinator — public interface
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Interface for coordinating federated LoRA gradient aggregation.
 *
 * All shards interact exclusively through this interface; the concrete
 * implementation is injected by the cluster manager.
 */
class ILoRAFederationCoordinator {
public:
    virtual ~ILoRAFederationCoordinator() = default;

    /**
     * @brief Submit a local gradient contribution for the current round.
     *
     * Idempotent per (shard_id, round): a duplicate submission is silently
     * ignored.  Triggers aggregation automatically once `min_participants`
     * shards have submitted or `round_timeout` elapses.
     *
     * @param gradient  Gradient exported by the local `IncrementalLoRATrainer`.
     */
    virtual void submitGradient(const EncryptedGradient& gradient) = 0;

    /**
     * @brief Manually trigger aggregation regardless of participant count.
     *
     * Uses whatever gradients have been submitted so far.  Throws if fewer
     * than `min_participants` contributed.
     *
     * @return The aggregated global delta.
     */
    virtual GlobalAdapterDelta triggerAggregation() = 0;

    /**
     * @brief Register a callback invoked when a new global delta is ready.
     *
     * Shards register this to receive and apply the aggregated delta.
     * @param cb  Callback: `void(const GlobalAdapterDelta&)`.
     */
    virtual void setGlobalDeltaCallback(
        std::function<void(const GlobalAdapterDelta&)> cb) = 0;

    /**
     * @brief Return the current federated round number.
     */
    [[nodiscard]] virtual uint64_t currentRound() const = 0;

    /**
     * @brief Return number of gradients received for the current round.
     */
    [[nodiscard]] virtual size_t submittedCount() const = 0;

    /**
     * @brief Return the last successfully produced global delta, if any.
     */
    [[nodiscard]] virtual std::optional<GlobalAdapterDelta> lastDelta() const = 0;

    /**
     * @brief Return aggregate statistics as JSON (for observability).
     */
    [[nodiscard]] virtual nlohmann::json getStats() const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// LoRAFederationCoordinator — default production implementation
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Production implementation of `ILoRAFederationCoordinator`.
 *
 * Delegates gradient aggregation to the existing
 * `FederatedImportCoordinator::FederatedAggregator` and applies calibrated
 * Gaussian noise via `DifferentialPrivacyManager`.
 *
 * Thread safety: all public methods acquire `mutex_` before accessing state.
 */
class LoRAFederationCoordinator : public ILoRAFederationCoordinator {
public:
    /**
     * @brief Construct with configuration.
     * @param config  Federation configuration.
     */
    explicit LoRAFederationCoordinator(FederationConfig config = {});
    ~LoRAFederationCoordinator() override;

    LoRAFederationCoordinator(const LoRAFederationCoordinator&)            = delete;
    LoRAFederationCoordinator& operator=(const LoRAFederationCoordinator&) = delete;
    LoRAFederationCoordinator(LoRAFederationCoordinator&&)                 noexcept;
    LoRAFederationCoordinator& operator=(LoRAFederationCoordinator&&)      noexcept;

    // ── ILoRAFederationCoordinator ───────────────────────────────────────────

    void submitGradient(const EncryptedGradient& gradient) override;
    GlobalAdapterDelta triggerAggregation() override;
    void setGlobalDeltaCallback(
        std::function<void(const GlobalAdapterDelta&)> cb) override;

    [[nodiscard]] uint64_t currentRound()    const override;
    [[nodiscard]] size_t   submittedCount()  const override;
    [[nodiscard]] std::optional<GlobalAdapterDelta> lastDelta() const override;
    [[nodiscard]] nlohmann::json getStats()  const override;

    // ── Extra: manual round control ──────────────────────────────────────────

    /**
     * @brief Advance to the next round, discarding any pending submissions.
     *
     * Intended for testing and administrative resets.
     */
    void advanceRound();

    /**
     * @brief Inject a `DecisionRecordYamlProcessor` for async YAML traceability.
     *
     * When set, every successful federated aggregation round emits a
     * `FEDERATED_ROUND` decision record that is written asynchronously to
     * `logs/decisions/YYYY-MM-DD/<ts>_FEDERATED_ROUND_<id>.yaml`.
     *
     * This is intentionally decoupled from the core aggregation path: the
     * processor runs on its own background thread and the coordinator does not
     * wait for the write to complete.
     *
     * @param processor  Shared processor instance (may be nullptr to disable).
     */
    void setDecisionRecordProcessor(
        std::shared_ptr<themis::llm::DecisionRecordYamlProcessor> processor);

    /**
     * @brief Return the current configuration.
     */
    [[nodiscard]] const FederationConfig& config() const { return config_; }

private:
    FederationConfig                         config_;
    uint64_t                                 current_round_{1};
    std::map<std::string, EncryptedGradient> pending_gradients_; // shard_id → gradient
    std::optional<GlobalAdapterDelta>        last_delta_;
    std::function<void(const GlobalAdapterDelta&)> delta_callback_;

    // Decision traceability (optional, non-blocking)
    std::shared_ptr<themis::llm::DecisionRecordYamlProcessor> dr_processor_;

    // Statistics
    uint64_t total_rounds_completed_{0};
    uint64_t total_gradients_processed_{0};
    double   total_epsilon_spent_{0.0};

    mutable std::mutex mutex_;

    // Internal helpers
    [[nodiscard]] GlobalAdapterDelta doAggregation();
    [[nodiscard]] nlohmann::json applyDifferentialPrivacy(
        const nlohmann::json& aggregated) const;
    [[nodiscard]] std::string nextDeltaVersion() const;
    /// Emit a FEDERATED_ROUND DecisionRecord (non-blocking, caller holds mutex_).
    void emitFederationDecisionRecord(const GlobalAdapterDelta& delta,
                                      const std::string& outcome) const;
};

} // namespace themis::distributed_knowledge
