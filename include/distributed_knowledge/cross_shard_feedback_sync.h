#pragma once

/**
 * @file cross_shard_feedback_sync.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <string>
#include <vector>
#include <unordered_set>
#include <functional>
#include <memory>
#include <mutex>
#include <chrono>
#include <optional>
#include <nlohmann/json.hpp>
#include "governance/gdpr_subject_rights.h"
#include "llm/decision_record_yaml_processor.h"

namespace themis::distributed_knowledge {

// ─────────────────────────────────────────────────────────────────────────────
// FeedbackSummary — privacy-safe cross-shard feedback unit
// ─────────────────────────────────────────────────────────────────────────────

struct FeedbackSummary {
    std::string          summary_id;            ///< Unique identifier for dedup
    std::string          feedback_type_label;   ///< e.g. "USER_NEGATIVE", "SECURITY_ISSUE"
    std::vector<float>   reason_embedding;      ///< Semantic reason (no raw text)
    std::string          shard_origin = "ANON"; ///< Always anonymised in outbound
    uint64_t             rlaif_round  = 0;      ///< Target RLAIF training round
    std::chrono::system_clock::time_point created_at;

    [[nodiscard]] nlohmann::json toJson() const {
        using nlohmann::json;
        json j = {{"summary_id",          summary_id},
                  {"feedback_type_label", feedback_type_label},
                  {"shard_origin",        shard_origin},
                  {"rlaif_round",         rlaif_round},
                  {"created_at_ms",
                   std::chrono::duration_cast<std::chrono::milliseconds>(
                       created_at.time_since_epoch()).count()}};
        j["reason_embedding"] = reason_embedding;
        return j;
    }

    [[nodiscard]] static FeedbackSummary fromJson(const nlohmann::json& j) {
        FeedbackSummary s;
        s.summary_id          = j.value("summary_id", "");
        s.feedback_type_label = j.value("feedback_type_label", "");
        s.shard_origin        = j.value("shard_origin", "ANON");
        s.rlaif_round         = j.value<uint64_t>("rlaif_round", 0);
        if (j.contains("reason_embedding") && j["reason_embedding"].is_array()) {
            s.reason_embedding = j["reason_embedding"].get<std::vector<float>>();
        }
        const auto ms = j.value<int64_t>("created_at_ms", 0);
        s.created_at = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(ms));
        return s;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// FeedbackSyncConfig
// ─────────────────────────────────────────────────────────────────────────────

struct FeedbackSyncConfig {
    size_t max_embedding_dim = 384;       ///< Fixed embedding dimension (validated on publish)
    size_t dedup_cache_size  = 10000;     ///< Max summary_ids retained for dedup
    bool   validate_embedding_dim = true; ///< Reject summaries with wrong dim

    [[nodiscard]] bool isValid() const {
        return max_embedding_dim > 0 && dedup_cache_size > 0;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// CrossShardFeedbackSync
// ─────────────────────────────────────────────────────────────────────────────

class CrossShardFeedbackSync {
public:
    using FeedbackCallback = std::function<void(const FeedbackSummary&)>;

    CrossShardFeedbackSync(
        FeedbackSyncConfig              config,
        std::string                     local_shard_id,
        std::function<void(nlohmann::json)> gossip_message_fn);

    ~CrossShardFeedbackSync() noexcept;

    CrossShardFeedbackSync(const CrossShardFeedbackSync&)            = delete;
    CrossShardFeedbackSync& operator=(const CrossShardFeedbackSync&) = delete;
    CrossShardFeedbackSync(CrossShardFeedbackSync&&)                 noexcept = default;
    CrossShardFeedbackSync& operator=(CrossShardFeedbackSync&&)      noexcept;

    /**
     * @brief ── Publishing ───────────────────────────────────────────────────────────
     * @param[in] summary Input parameter.
     */

    void publishFeedback(FeedbackSummary summary);

    /**
     * @brief ── Receiving ────────────────────────────────────────────────────────────
     * @param[in] payload Input parameter.
     */

    void handleInboundSummary(const nlohmann::json& payload);

    /**
     * @brief Set Feedback Callback.
     * @param[in] cb Input parameter.
     */
    void setFeedbackCallback(FeedbackCallback cb);

    using InboundPolicyCheck = std::function<bool(const FeedbackSummary&)>;

    /**
     * @brief Set Inbound Policy Check.
     * @param[in] check Input parameter.
     */
    void setInboundPolicyCheck(InboundPolicyCheck check);

    /**
     * @brief Set Decision Record Processor.
     * @param[in] processor Input parameter.
     */
    void setDecisionRecordProcessor(
        std::shared_ptr<themis::llm::DecisionRecordYamlProcessor> processor);

    // ── DK-OR: Operational Resilience ────────────────────────────────────────

    using ZeroTrustEnforcer = std::function<bool(const FeedbackSummary&)>;

    /**
     * @brief Set Zero Trust Enforcer.
     * @param[in] enforcer Input parameter.
     */
    void setZeroTrustEnforcer(ZeroTrustEnforcer enforcer);

    [[nodiscard]] size_t getSkippedPublishCount() const;

    themis::governance::StoreErasureResult erase(
        const std::string& subject_id = "",
        themis::governance::Regulation regulation = themis::governance::Regulation::GDPR);

    [[nodiscard]] size_t eraseCount() const;

    // ── Observability ────────────────────────────────────────────────────────

    [[nodiscard]] size_t publishedCount() const;

    [[nodiscard]] size_t receivedCount() const;

    [[nodiscard]] size_t deduplicatedCount() const;

    [[nodiscard]] size_t rejectedByPolicyCount() const;

    [[nodiscard]] nlohmann::json getStats() const;

private:
    FeedbackSyncConfig                    config_;
    std::string                           local_shard_id_;
    std::function<void(nlohmann::json)>   gossip_message_fn_;
    FeedbackCallback                      on_feedback_;
    InboundPolicyCheck                    policy_check_;
    ZeroTrustEnforcer                     zero_trust_enforcer_; ///< DK-OR-S

    // Decision traceability (optional, non-blocking)
    std::shared_ptr<themis::llm::DecisionRecordYamlProcessor> dr_processor_;

    // Deduplication
    std::unordered_set<std::string>       seen_ids_;

    // Counters
    size_t published_count_       = 0;
    size_t received_count_        = 0;
    size_t deduplicated_count_    = 0;
    size_t rejected_by_policy_    = 0;
    size_t skipped_publish_count_ = 0;  ///< DK-OR-B-2: gossip backpressure skips
    size_t erase_count_           = 0;  ///< DK-OR-H-3: GDPR erase ops

    mutable std::mutex mutex_;

    [[nodiscard]] static std::string generateSummaryId();
    /**
     * @brief Emit Feedback Decision Record.
     * @param[in] direction Input parameter.
     * @param[in] summary Input parameter.
     */
    void emitFeedbackDecisionRecord(const std::string& direction,
                                    const FeedbackSummary& summary) const;
};

} // namespace themis::distributed_knowledge
