// Copyright 2026 ThemisDB — Licensed under MIT License
#pragma once

/**
 * @file cross_shard_feedback_sync.h
 * @brief Ebene D — Federated RLAIF (verteiltes DBA-Feedback)
 *
 * Propagates DBA / operator feedback across shard boundaries so that every
 * shard benefits from domain expertise captured on any other shard — without
 * leaking raw query text or PII.
 *
 * ## Flow
 *
 * ```
 * DBA on Shard 3: FeedbackCollector::recordFeedback(entry)
 *    ↓
 * CrossShardFeedbackSync::publishFeedback(entry)
 *    ↓  anonymise: replace query/response text with embeddings + type label
 *    ↓
 * FeedbackSummary  ─── GossipProtocol ("federated_feedback") ──→  all peers
 *    ↓  (each receiving shard)
 * CrossShardFeedbackSync::handleInboundSummary(summary)
 *    ↓
 * RLAIFTrainer::addPreferencePair(global_feedback_pair)
 *    ↓
 * next IncrementalLoRATrainer round incorporates global DBA knowledge
 * ```
 *
 * ## Privacy Contract
 *  - Raw query text and response text are NEVER included in `FeedbackSummary`.
 *  - Only `FeedbackType` label + a fixed-length reason embedding (provided by
 *    the caller) are propagated.
 *  - `shard_origin` is set to `"ANON"` in the outbound summary.
 *  - `ZeroTrustPolicyEnforcer` is expected to verify inbound gossip messages
 *    before `handleInboundSummary()` is called.
 *
 * ## Design Constraints
 *  - `CrossShardFeedbackSync` is a single-writer per shard; multiple readers
 *    may register via `setFeedbackCallback()`.
 *  - Deduplication is by `summary_id` to prevent re-processing gossip echoes.
 *  - Thread-safe: all public methods acquire `mutex_` internally.
 *
 * @see include/prompt_engineering/feedback_collector.h — local feedback source
 * @see include/rag/rlaif_trainer.h                     — RLAIF training consumer
 * @see include/sharding/gossip_protocol.h               — transport
 *
 * Scientific references:
 *   Bai, Y. et al. (2022). Constitutional AI: Harmlessness from AI Feedback.
 *     arXiv:2212.08073.
 *   Lee, H. et al. (2023). RLAIF: Scaling Reinforcement Learning from Human
 *     Feedback with AI Feedback. arXiv:2309.00267.
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

    ~CrossShardFeedbackSync();

    CrossShardFeedbackSync(const CrossShardFeedbackSync&)            = delete;
    CrossShardFeedbackSync& operator=(const CrossShardFeedbackSync&) = delete;
    CrossShardFeedbackSync(CrossShardFeedbackSync&&)                 noexcept;
    CrossShardFeedbackSync& operator=(CrossShardFeedbackSync&&)      noexcept;

    // ── Publishing ───────────────────────────────────────────────────────────

    /**
     * @brief Anonymise and broadcast a local feedback summary to all peers.
     *
     * Validates embedding dimension, stamps `shard_origin = "ANON"`, assigns
     * a fresh `summary_id`, and dispatches via the gossip function.
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
     * @param payload  JSON payload from the gossip message.
     */
    void handleInboundSummary(const nlohmann::json& payload);

    /**
     * @brief Register callback invoked for each new inbound feedback summary.
     * @param cb  Callback: `void(const FeedbackSummary&)`.
     */
    void setFeedbackCallback(FeedbackCallback cb);

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
     * @brief Return observability stats as JSON.
     */
    [[nodiscard]] nlohmann::json getStats() const;

private:
    FeedbackSyncConfig                    config_;
    std::string                           local_shard_id_;
    std::function<void(nlohmann::json)>   gossip_message_fn_;
    FeedbackCallback                      on_feedback_;

    // Decision traceability (optional, non-blocking)
    std::shared_ptr<themis::llm::DecisionRecordYamlProcessor> dr_processor_;

    // Deduplication
    std::unordered_set<std::string>       seen_ids_;

    // Counters
    size_t published_count_   = 0;
    size_t received_count_    = 0;
    size_t deduplicated_count_ = 0;

    mutable std::mutex mutex_;

    [[nodiscard]] static std::string generateSummaryId();
    void emitFeedbackDecisionRecord(const std::string& direction,
                                    const FeedbackSummary& summary) const;
};

} // namespace themis::distributed_knowledge
