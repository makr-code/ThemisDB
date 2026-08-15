#pragma once

/**
 * @file lora_federation_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "distributed_knowledge/adapter_capability_announcement.h"
#include "governance/cross_border_transfer.h"
#include "governance/gdpr_subject_rights.h"
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
    double dp_epsilon  = 0.1;   ///< Privacy budget ε spent *per round* (lower = more private)
    double dp_delta    = 1e-5;  ///< Failure probability δ
    double dp_sensitivity = 1.0; ///< L2 sensitivity of gradient

    // Privacy budget cap (DK-6)
    size_t max_rounds  = 0;     ///< Maximum federation rounds (0 = unlimited)

    // Timeout (legacy chrono field kept for compatibility)
    std::chrono::minutes round_timeout{60}; ///< Max wait for all shards per round

    // DK-OR: millisecond-precision timeout for aggregation (used by triggerAggregation())
    size_t round_timeout_ms = 30000; ///< Aggregation timeout in ms (DK-OR-B-1); 0 = unlimited

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
     * @brief Manually trigger aggregation with an explicit timeout.
     *
     * Runs the aggregation asynchronously and throws `std::runtime_error` if it
     * does not complete within `timeout_ms` milliseconds.
     *
     * @param timeout_ms  Timeout in milliseconds.  Throws on expiry.
     * @return The aggregated global delta.
     * @throws std::runtime_error on timeout or when fewer than `min_participants`
     *         contributed.
     */
    virtual GlobalAdapterDelta triggerAggregation(size_t timeout_ms) = 0;

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
    ~LoRAFederationCoordinator() noexcept override;

    LoRAFederationCoordinator(const LoRAFederationCoordinator&)            = delete;
    LoRAFederationCoordinator& operator=(const LoRAFederationCoordinator&) = delete;
    LoRAFederationCoordinator(LoRAFederationCoordinator&&)                 = default;
    LoRAFederationCoordinator& operator=(LoRAFederationCoordinator&&)      noexcept;

    // ── ILoRAFederationCoordinator ───────────────────────────────────────────

    void submitGradient(const EncryptedGradient& gradient) override;
    GlobalAdapterDelta triggerAggregation() override;
    GlobalAdapterDelta triggerAggregation(size_t timeout_ms) override;
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

    // ── DK-6: Privacy budget observability ──────────────────────────────────

    /**
     * @brief Return total DP epsilon remaining in the configured budget.
     *
     * Returns `std::numeric_limits<double>::max()` when `max_rounds == 0`
     * (unlimited budget).  Otherwise: `max_rounds * dp_epsilon - total_epsilon_spent_`.
     */
    [[nodiscard]] double privacyBudgetRemaining() const;

    /**
     * @brief Return true when further federation rounds are permitted.
     *
     * False when `max_rounds > 0` and `current_round_ > max_rounds`.
     * `triggerAggregation()` throws `std::runtime_error` when this returns false.
     */
    [[nodiscard]] bool verifyPrivacyBudget() const;

    // ── DK-OR: Operational Resilience ────────────────────────────────────────

    /**
     * @brief GDPR erase: clear pending gradients and reset round state (DK-OR-H-1).
     *
     * Clears `pending_gradients_`, resets `current_round_` to 0, and increments
     * `erase_count_`.
     *
     * @param subject_id  Data subject identifier (for audit; may be empty).
     * @param regulation  Applicable regulation.
     */
    themis::governance::StoreErasureResult erase(
        const std::string& subject_id = "",
        themis::governance::Regulation regulation = themis::governance::Regulation::GDPR);

    /**
     * @brief Number of GDPR erase operations performed.
     */
    [[nodiscard]] size_t eraseCount() const;

    // ── DK-7: Admin, GDPR, and audit hooks ──────────────────────────────────

    /**
     * @brief Inject a GDPR cross-border transfer policy (DK-7 DI-setter).
     *
     * When set, `triggerAggregation()` calls `checkTransfer()` for every
     * participating shard's registered region before aggregation starts.
     * If any shard's region is PROHIBITED, a `std::runtime_error` is thrown
     * with the message "Cross-border transfer blocked: <region>".
     */
    void setCrossBorderPolicy(
        std::shared_ptr<themis::governance::CrossBorderTransferPolicy> policy);

    /**
     * @brief Register a shard-id → region-code mapping for GDPR checks.
     *
     * Used by `setCrossBorderPolicy()`.  Unknown shards default to "EU".
     * @param locations  Map of shard_id → ISO 3166-1 alpha-2 region code.
     */
    void setShardLocations(std::map<std::string, std::string> locations);

    /**
     * @brief Inject an audit record callback (DK-7 DI-setter).
     *
     * Called after every successful `triggerAggregation()` with a JSON
     * audit record containing `decision_type`, `round`, `participants`,
     * `epsilon_spent`, `algorithm`, and optional `sphincs_signature`.
     */
    void setAuditRecordCallback(
        std::function<void(const nlohmann::json&)> callback);

    /**
     * @brief Inject a SphincsPlus signing callback (DK-7 DI-setter).
     *
     * When set, the audit record is passed to this callback to produce a
     * post-quantum signature string that is stored in `audit_record["sphincs_signature"]`.
     */
    void setSigningCallback(
        std::function<std::string(const nlohmann::json&)> signing_fn);

    // ── FPD: Poisoning / Outlier Detection ────────────────────────────────────

    /**
     * @brief Callable type for gradient outlier/poisoning filters.
     *
     * A filter receives an `EncryptedGradient` and returns `true` when the
     * gradient is safe to include in the aggregation, or `false` to reject
     * it as a potential poisoning or outlier contribution.
     *
     * The built-in L2-norm filter (`makeL2NormOutlierFilter()`) rejects
     * gradients whose key-wise L2 norm deviates more than `z_threshold`
     * standard deviations from the mean of all submitted gradients.
     *
     * Custom filters (e.g., Krum, Bulyan) may be injected for testing or
     * production hardening without changing the coordinator's aggregation logic.
     */
    using GradientOutlierFilter =
        std::function<bool(const EncryptedGradient&,
                           const std::map<std::string, EncryptedGradient>&)>;

    /**
     * @brief Inject a gradient outlier / poisoning-detection filter (FPD).
     *
     * When set, `doAggregation()` calls the filter for every pending gradient
     * before aggregation.  Gradients for which the filter returns `false` are
     * excluded from the round and counted in `total_gradients_filtered_`.
     *
     * The filter is called under `mutex_`, so it must not call back into the
     * coordinator.
     *
     * @param filter  Callable `(const EncryptedGradient&,
     *                           const std::map<std::string, EncryptedGradient>&)
     *                          → bool`.  May be `nullptr` to disable filtering.
     */
    void setGradientOutlierFilter(GradientOutlierFilter filter);

    /**
     * @brief Return the number of gradients rejected by the outlier filter.
     *
     * Counter accumulates across all rounds.
     */
    [[nodiscard]] size_t filteredGradientsCount() const;

    /**
     * @brief Create a simple L2-norm-based outlier filter.
     *
     * Rejects any gradient whose L2 norm (computed as
     * `sqrt(Σ value²)` over all numeric keys in `data`) deviates more than
     * `z_threshold` standard deviations from the mean L2 norm of all
     * submitted gradients in the same round.
     *
     * **Edge case — fewer than 2 samples:** When the peer set contains fewer
     * than 2 gradients, no statistics can be computed and all gradients are
     * accepted.  This avoids false positives in minimal-participant rounds, but
     * means the filter provides no protection in single-participant rounds.
     * Configure `FederationConfig::min_participants >= 2` to avoid this.
     *
     * **Performance note:** The filter recalculates L2 norms for every gradient
     * in the peer set on each invocation.  For large rounds this is O(N) per
     * gradient call; the caller in `doAggregation()` supplies a pre-snapshotted
     * peer map so statistics remain consistent throughout filtering.
     *
     * This provides a lightweight poisoning defence without requiring the
     * full `ByzantineDetector` hierarchy (which depends on `GradientTensor`
     * and the LLM module).
     *
     * @param z_threshold  Rejection threshold in standard deviations (default 2.5).
     * @return A `GradientOutlierFilter` suitable for `setGradientOutlierFilter()`.
     */
    [[nodiscard]] static GradientOutlierFilter makeL2NormOutlierFilter(
        double z_threshold = 2.5);

private:
    FederationConfig                         config_;
    uint64_t                                 current_round_{1};
    std::map<std::string, EncryptedGradient> pending_gradients_; // shard_id → gradient
    std::optional<GlobalAdapterDelta>        last_delta_;
    std::function<void(const GlobalAdapterDelta&)> delta_callback_;

    // Decision traceability (optional, non-blocking)
    std::shared_ptr<themis::llm::DecisionRecordYamlProcessor> dr_processor_;

    // DK-OR: GDPR erase count
    size_t erase_count_{0};

    // DK-7: GDPR cross-border policy + shard location map
    std::shared_ptr<themis::governance::CrossBorderTransferPolicy> cross_border_policy_;
    std::map<std::string, std::string>               shard_locations_; // shard_id → region

    // DK-7: Audit and signing callbacks
    std::function<void(const nlohmann::json&)>       audit_record_callback_;
    std::function<std::string(const nlohmann::json&)> signing_callback_;

    // Statistics
    uint64_t total_rounds_completed_{0};
    uint64_t total_gradients_processed_{0};
    double   total_epsilon_spent_{0.0};
    /// Number of gradients rejected by the outlier/poisoning filter (FPD).
    uint64_t total_gradients_filtered_{0};

    // FPD: injectable poisoning / outlier filter (optional)
    GradientOutlierFilter gradient_outlier_filter_;

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

