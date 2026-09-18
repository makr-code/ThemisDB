#pragma once

/**
 * @file lora_federation_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
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

struct GlobalAdapterDelta {
    uint64_t       round = 0;               ///< Federated round that produced this delta
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

class ILoRAFederationCoordinator {
public:
    /**
     * @brief ILo RAFederation Coordinator.
     * @return Return value.
     */
    virtual ~ILoRAFederationCoordinator() = default;

    /**
     * @brief Submit Gradient.
     * @param[in] gradient Input parameter.
     */
    virtual void submitGradient(const EncryptedGradient& gradient) = 0;

    /**
     * @brief Trigger Aggregation.
     * @return Return value.
     */
    virtual GlobalAdapterDelta triggerAggregation() = 0;

    /**
     * @brief Trigger Aggregation.
     * @param[in] timeout_ms Input parameter.
     * @return Return value.
     */
    virtual GlobalAdapterDelta triggerAggregation(size_t timeout_ms) = 0;

    virtual void setGlobalDeltaCallback(
        std::function<void(const GlobalAdapterDelta&)> cb) = 0;

    [[nodiscard]] virtual uint64_t currentRound() const = 0;

    [[nodiscard]] virtual size_t submittedCount() const = 0;

    [[nodiscard]] virtual std::optional<GlobalAdapterDelta> lastDelta() const = 0;

    [[nodiscard]] virtual nlohmann::json getStats() const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// LoRAFederationCoordinator — default production implementation
// ─────────────────────────────────────────────────────────────────────────────

class LoRAFederationCoordinator : public ILoRAFederationCoordinator {
public:
    explicit LoRAFederationCoordinator(FederationConfig config = {});
    ~LoRAFederationCoordinator() noexcept override;

    LoRAFederationCoordinator(const LoRAFederationCoordinator&)            = delete;
    LoRAFederationCoordinator& operator=(const LoRAFederationCoordinator&) = delete;
    LoRAFederationCoordinator(LoRAFederationCoordinator&&) noexcept;
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

    /**
     * @brief ── Extra: manual round control ──────────────────────────────────────────
     */

    void advanceRound();

    /**
     * @brief Set Decision Record Processor.
     * @param[in] processor Input parameter.
     */
    void setDecisionRecordProcessor(
        std::shared_ptr<themis::llm::DecisionRecordYamlProcessor> processor);

    [[nodiscard]] const FederationConfig& config() const { return config_; }

    // ── DK-6: Privacy budget observability ──────────────────────────────────

    [[nodiscard]] double privacyBudgetRemaining() const;

    [[nodiscard]] bool verifyPrivacyBudget() const;

    // ── DK-OR: Operational Resilience ────────────────────────────────────────

    themis::governance::StoreErasureResult erase(
        const std::string& subject_id = "",
        themis::governance::Regulation regulation = themis::governance::Regulation::GDPR);

    [[nodiscard]] size_t eraseCount() const;

    /**
     * @brief ── DK-7: Admin, GDPR, and audit hooks ──────────────────────────────────
     * @param[in] policy Input parameter.
     */

    void setCrossBorderPolicy(
        std::shared_ptr<themis::governance::CrossBorderTransferPolicy> policy);

    void setShardLocations(std::map<std::string, std::string> locations);

    void setAuditRecordCallback(
        std::function<void(const nlohmann::json&)> callback);

    void setSigningCallback(
        std::function<std::string(const nlohmann::json&)> signing_fn);

    // ── FPD: Poisoning / Outlier Detection ────────────────────────────────────

    using GradientOutlierFilter =
        std::function<bool(const EncryptedGradient&,
                           const std::map<std::string, EncryptedGradient>&)>;

    /**
     * @brief Set Gradient Outlier Filter.
     * @param[in] filter Input parameter.
     */
    void setGradientOutlierFilter(GradientOutlierFilter filter);

    [[nodiscard]] size_t filteredGradientsCount() const;

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
    uint64_t total_gradients_filtered_{0};

    // FPD: injectable poisoning / outlier filter (optional)
    GradientOutlierFilter gradient_outlier_filter_;

    mutable std::mutex mutex_;

    // Internal helpers
    [[nodiscard]] GlobalAdapterDelta doAggregation();
    [[nodiscard]] nlohmann::json applyDifferentialPrivacy(
        const nlohmann::json& aggregated) const;
    [[nodiscard]] std::string nextDeltaVersion() const;
    /**
     * @brief Emit Federation Decision Record.
     * @param[in] delta Input parameter.
     * @param[in] outcome Input parameter.
     */
    void emitFederationDecisionRecord(const GlobalAdapterDelta& delta,
                                      const std::string& outcome) const;
};

} // namespace themis::distributed_knowledge
