#pragma once

/**
 * @file cross_shard_feedback_sync.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Anonymised feedback summary propagated via gossip.
 *
 * Contains only:
 *  - `feedback_type_label` — string representation of `FeedbackType`
 *  - `reason_embedding`   — fixed-length float vector (no raw text)
 *  - `shard_origin`       — always "ANON" in outbound messages
 *  - `summary_id`         — UUID for deduplication
 *  - `round`              — RLAIF training round it targets
 */
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

/**
 * @brief Configuration for `CrossShardFeedbackSync`.
 */
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

/**
 * @brief Synchronises DBA feedback across shard boundaries for federated RLAIF.
 *
 * **Publishing** (local shard):
 *  1. Caller supplies a `FeedbackSummary` with `reason_embedding` (caller is
 *     responsible for embedding raw text externally; raw text must not appear
 *     in `FeedbackSummary`).
 *  2. `publishFeedback()` validates the embedding dimension, stamps
 *     `shard_origin = "ANON"`, generates a `summary_id`, serialises to JSON,
 *     and dispatches via `gossip_message_fn_`.
 *
 * **Receiving** (remote shard):
 *  1. Gossip dispatch calls `handleInboundSummary(json_payload)`.
 *  2. Deduplication by `summary_id` discards already-seen summaries.
 *  3. Registered `FeedbackCallback` is invoked with the deserialised summary.
 *  4. Callers typically forward the summary to `RLAIFTrainer::addPreferencePair`.
 *
 * Example:
 * @code
 *   CrossShardFeedbackSync sync(config, shard_id, [&](nlohmann::json payload) {
 *       gossip.broadcastCustom(payload);
 *   });
 *   sync.setFeedbackCallback([&](const FeedbackSummary& s) {
 *       rlaif_trainer.addPreferencePair(buildPairFromSummary(s));
 *   });
 *
 *   // On DBA feedback:
 *   FeedbackSummary fs;
 *   fs.feedback_type_label = "USER_NEGATIVE";
 *   fs.reason_embedding = embed(feedback_entry.query);
 *   sync.publishFeedback(fs);
 * @endcode
 */
class CrossShardFeedbackSync {
public:
    using FeedbackCallback = std::function<void(const FeedbackSummary&)>;

    /**
     * @brief Construct the sync component.
     *
     * @param config            Configuration.
     * @param local_shard_id    Shard identifier for outbound messages.
     * @param gossip_message_fn Callable that dispatches a JSON gossip payload.
     */
    CrossShardFeedbackSync(
        FeedbackSyncConfig              config,
        std::string                     local_shard_id,
        std::function<void(nlohmann::json)> gossip_message_fn);

    ~CrossShardFeedbackSync() noexcept;

    CrossShardFeedbackSync(const CrossShardFeedbackSync&)            = delete;
    CrossShardFeedbackSync& operator=(const CrossShardFeedbackSync&) = delete;
    CrossShardFeedbackSync(CrossShardFeedbackSync&&)                 noexcept = default;
    CrossShardFeedbackSync& operator=(CrossShardFeedbackSync&&)      noexcept;

    // ── Publishing ───────────────────────────────────────────────────────────

    /**
    * @brief Anonymise and broadcast a local feedback summary to all peers.
    *
    * Validates embedding dimension, stamps `shard_origin = "ANON"`, assigns
    * a fresh `summary_id`, and dispatches via the gossip function.
    *
    * **Consistency Level: EVENTUAL**
    *  - Publication is non-blocking and non-guaranteed (gossip assumptions)
    *  - Feedback may arrive at remote shards in any order or not at all
    *  - Deduplication via summary_id prevents double-counting on retransmission
    *  - Rationale: Gossip-level delivery acceptable for aggregated feedback
    *
    * **Replication Lag:**
    *  - Max acceptable lag: unbounded (gossip assumption)
    *  - Feedback may arrive minutes/hours late at distant shards
    *  - Policy: Late arrival handled gracefully via idempotent aggregation
    *  - Safety: Feedback aggregation is commutative over time
    *
    * @param summary  Feedback summary with `reason_embedding` already set.
    *                 `shard_origin` is overwritten with "ANON".
    * @throws std::invalid_argument if embedding dimension is wrong.
    */
    void publishFeedback(FeedbackSummary summary);

    // ── Receiving ────────────────────────────────────────────────────────────

    /**
    * @brief Handle an inbound gossip payload containing a `FeedbackSummary`.
    *
    * Call this from the gossip dispatch path when
    * `message_type == "federated_feedback"`.  Deduplicates and invokes the
    * registered callback if the summary is new.
    *
    * **Consistency Level: EVENTUAL**
    *  - No ordering guarantees; feedback may arrive out-of-order
    *  - Deduplication by summary_id prevents double-counting
    *  - ZeroTrust check (DK-OR-S) provides safety gate but not consistency
    *  - Rationale: Idempotent feedback aggregation tolerates out-of-order arrival
    *
    * **Version Tracking:**
    *  - FeedbackSummary.created_at provides temporal context (informational)
    *  - No ordering constraint enforced; late feedback accepted
    *
    * @param payload  JSON payload from the gossip message.
    * @throws std::runtime_error when ZeroTrust enforcer rejects the summary
    *         (high-risk context).
    */
    void handleInboundSummary(const nlohmann::json& payload);

    /**
     * @brief Register callback invoked for each new inbound feedback summary.
     * @param cb  Callback: `void(const FeedbackSummary&)`.
     */
    void setFeedbackCallback(FeedbackCallback cb);

    /**
     * @brief Inbound policy check type (DK-5 ZeroTrust integration).
     *
     * Returns `true` when the summary is acceptable and should be processed,
     * `false` to silently drop it.  Mirrors the ZeroTrust "never trust,
     * always verify" pattern without coupling to the full
     * `ZeroTrustPolicyEnforcer` API.
     */
    using InboundPolicyCheck = std::function<bool(const FeedbackSummary&)>;

    /**
     * @brief Inject an inbound policy check (DK-5 DI-setter).
     *
     * When set, `handleInboundSummary()` calls the check before invoking the
     * feedback callback.  Summaries that fail the check are dropped silently
     * and `rejectedByPolicyCount()` is incremented.
     *
     * Typical use: wrap `ZeroTrustPolicyEnforcer::verify()` in a lambda.
     */
    void setInboundPolicyCheck(InboundPolicyCheck check);

    /**
     * @brief Inject a `DecisionRecordYamlProcessor` for async YAML traceability.
     *
     * When set, every call to `publishFeedback()` and every processed inbound
     * summary emits a `FEDERATED_FEEDBACK` decision record written
     * asynchronously by the processor's background thread.
     *
     * @param processor  Shared processor instance (may be nullptr to disable).
     */
    void setDecisionRecordProcessor(
        std::shared_ptr<themis::llm::DecisionRecordYamlProcessor> processor);

    // ── DK-OR: Operational Resilience ────────────────────────────────────────

    /**
     * @brief ZeroTrust enforcer type (DK-OR-S).
     *
     * Returns `true` if the inbound summary is trusted (should be processed),
     * `false` if it is high-risk (should throw).
     */
    using ZeroTrustEnforcer = std::function<bool(const FeedbackSummary&)>;

    /**
     * @brief Inject a ZeroTrust risk check for inbound summaries (DK-OR-S-1/2).
     *
     * When set, `handleInboundSummary()` calls the enforcer before dedup/callback.
     * If the enforcer returns `false` (risk=HIGH), throws
     * `std::runtime_error("inbound feedback rejected: high-risk context")`.
     *
     * @param enforcer  Check function; nullptr to disable.
     */
    void setZeroTrustEnforcer(ZeroTrustEnforcer enforcer);

    /**
     * @brief Number of publish calls silently skipped due to gossip backpressure
     *        (DK-OR-B-2).
     */
    [[nodiscard]] size_t getSkippedPublishCount() const;

    /**
     * @brief GDPR erase: clear the dedup cache (DK-OR-H-3).
     *
     * Clears `seen_summary_ids_` dedup set and increments `erase_count_`.
     */
    themis::governance::StoreErasureResult erase(
        const std::string& subject_id = "",
        themis::governance::Regulation regulation = themis::governance::Regulation::GDPR);

    [[nodiscard]] size_t eraseCount() const;

    // ── Observability ────────────────────────────────────────────────────────

    /**
     * @brief Number of summaries published by this shard.
     */
    [[nodiscard]] size_t publishedCount() const;

    /**
     * @brief Number of inbound summaries processed (after dedup).
     */
    [[nodiscard]] size_t receivedCount() const;

    /**
     * @brief Number of summaries dropped due to deduplication.
     */
    [[nodiscard]] size_t deduplicatedCount() const;

    /**
     * @brief Number of summaries rejected by the inbound policy check.
     */
    [[nodiscard]] size_t rejectedByPolicyCount() const;

    /**
     * @brief Return observability stats as JSON.
     */
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
    void emitFeedbackDecisionRecord(const std::string& direction,
                                    const FeedbackSummary& summary) const;
};

} // namespace themis::distributed_knowledge
