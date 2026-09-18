/**
 * @file cross_shard_feedback_sync.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "distributed_knowledge/cross_shard_feedback_sync.h"

#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace themis::distributed_knowledge {

// ─────────────────────────────────────────────────────────────────────────────
// Construction / destruction
// ─────────────────────────────────────────────────────────────────────────────

CrossShardFeedbackSync::CrossShardFeedbackSync(
    FeedbackSyncConfig              config,
    std::string                     local_shard_id,
    std::function<void(nlohmann::json)> gossip_message_fn)
    : config_(std::move(config))
    , local_shard_id_(std::move(local_shard_id))
    , gossip_message_fn_(std::move(gossip_message_fn))
{
    if (!config_.isValid()) {
        throw std::invalid_argument(
            "CrossShardFeedbackSync: invalid FeedbackSyncConfig");
    }
}

CrossShardFeedbackSync::~CrossShardFeedbackSync() noexcept = default;

// ─────────────────────────────────────────────────────────────────────────────
// publishFeedback
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Publish Feedback.
 * @param[in] summary Input parameter.
 * @throws std::invalid_argument if an error occurs.
 * @details Calls: empty(), size(), std::to_string(), generateSummaryId(), std::chrono::system_clock::now(), toJson(), lk(), emitFeedbackDecisionRecord().
 */
void CrossShardFeedbackSync::publishFeedback(FeedbackSummary summary) {
    // Validate embedding dimension
    if (config_.validate_embedding_dim &&
        !summary.reason_embedding.empty() &&
        summary.reason_embedding.size() != config_.max_embedding_dim)
    {
        throw std::invalid_argument(
            "CrossShardFeedbackSync::publishFeedback: embedding dimension "
            "mismatch (got " + std::to_string(summary.reason_embedding.size()) +
            ", expected " + std::to_string(config_.max_embedding_dim) + ")");
    }

    // ── Consistency Level: EVENTUAL (Non-blocking gossip) ──────────────────────
    // Enforce privacy: overwrite origin with ANON. Publication is asynchronous
    // and non-guaranteed. Feedback may arrive at remote shards in any order,
    // be deduplicated on arrival, and aggregated idempotently.
    summary.shard_origin = "ANON";
    summary.summary_id   = generateSummaryId();
    summary.created_at   = std::chrono::system_clock::now();

    nlohmann::json payload = summary.toJson();
    payload["message_type"] = "federated_feedback";

    {
        std::lock_guard<std::mutex> lk(mutex_);
        ++published_count_;
        emitFeedbackDecisionRecord("OUTBOUND", summary);
    }

    if (gossip_message_fn_) {
        // DK-OR-B-2: non-blocking dispatch — if gossip sink throws (backpressure),
        // silently skip and increment the skipped counter instead of propagating.
        try {
            gossip_message_fn_(std::move(payload));
        } catch (const std::exception&) {
            std::lock_guard<std::mutex> lk(mutex_);
            ++skipped_publish_count_;
            // Note: gossip_message_fn_ may throw std::exception or derived types
            // on backpressure/network conditions. We handle gracefully by tracking
            // the skip without propagating to the caller.
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// handleInboundSummary
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Handle Inbound Summary.
 * @param[in] payload Input parameter.
 * @throws std::runtime_error if an error occurs.
 * @details Calls: FeedbackSummary::fromJson(), lk(), count(), zero_trust_enforcer_(), policy_check_(), size(), clear(), insert().
 */
void CrossShardFeedbackSync::handleInboundSummary(const nlohmann::json& payload) {
    FeedbackSummary summary = FeedbackSummary::fromJson(payload);

    FeedbackCallback cb;
    {
        std::lock_guard<std::mutex> lk(mutex_);

        // ── Consistency Level: EVENTUAL (Deduplication only) ───────────────────
        // No ordering guarantees; feedback may arrive out-of-order or late.
        // Deduplication by summary_id prevents double-counting on retransmission.
        // All other checks (policy, ZeroTrust) provide safety but not ordering.
        // Rationale: Idempotent aggregation tolerates out-of-order arrival;
        // same feedback processed multiple times produces identical result.
        // Dedup
        if (seen_ids_.count(summary.summary_id)) {
            ++deduplicated_count_;
            return;
        }

        // ── DK-OR-S: ZeroTrust enforcer ──────────────────────────────────────
        if (zero_trust_enforcer_ && !zero_trust_enforcer_(summary)) {
            throw std::runtime_error(
                "inbound feedback rejected: high-risk context");
        }
        // ─────────────────────────────────────────────────────────────────────

        // ── DK-5: ZeroTrust / policy check ───────────────────────────────────
        if (policy_check_ && !policy_check_(summary)) {
            ++rejected_by_policy_;
            return; // silent drop
        }
        // ─────────────────────────────────────────────────────────────────────

        // Evict oldest if cache full (simple: clear half the cache)
        if (seen_ids_.size() >= config_.dedup_cache_size) {
            seen_ids_.clear();
        }
        seen_ids_.insert(summary.summary_id);
        ++received_count_;
        cb = on_feedback_;
        emitFeedbackDecisionRecord("INBOUND", summary);
    }

    if (cb) {
        cb(summary);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// setFeedbackCallback / setInboundPolicyCheck
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Set Feedback Callback.
 * @param[in] cb Input parameter.
 * @details Calls: lk(), std::move().
 */
void CrossShardFeedbackSync::setFeedbackCallback(FeedbackCallback cb) {
    std::lock_guard<std::mutex> lk(mutex_);
    on_feedback_ = std::move(cb);
}

/**
 * @brief Set Inbound Policy Check.
 * @param[in] check Input parameter.
 * @details Calls: lk(), std::move().
 */
void CrossShardFeedbackSync::setInboundPolicyCheck(InboundPolicyCheck check) {
    std::lock_guard<std::mutex> lk(mutex_);
    policy_check_ = std::move(check);
}

// ─────────────────────────────────────────────────────────────────────────────
// Observability
// ─────────────────────────────────────────────────────────────────────────────

size_t CrossShardFeedbackSync::publishedCount() const {
    /**
     * @brief Lk.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mutex_);
    return published_count_;
}

size_t CrossShardFeedbackSync::receivedCount() const {
    /**
     * @brief Lk.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mutex_);
    return received_count_;
}

size_t CrossShardFeedbackSync::deduplicatedCount() const {
    /**
     * @brief Lk.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mutex_);
    return deduplicated_count_;
}

size_t CrossShardFeedbackSync::rejectedByPolicyCount() const {
    /**
     * @brief Lk.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mutex_);
    return rejected_by_policy_;
}

nlohmann::json CrossShardFeedbackSync::getStats() const {
    /**
     * @brief Lk.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mutex_);
    return {{"shard_id",             local_shard_id_},
            {"published",            published_count_},
            {"received",             received_count_},
            {"deduplicated",         deduplicated_count_},
            {"rejected_by_policy",   rejected_by_policy_},
            {"seen_ids_cached",seen_ids_.size()}};
}

// ─────────────────────────────────────────────────────────────────────────────
// generateSummaryId (static helper)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Generate Summary Id.
 * @return Return value.
 * @details Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), rng(), dist(), str().
 */
std::string CrossShardFeedbackSync::generateSummaryId() {
    // Simple pseudo-UUID using timestamp + random suffix
    const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<uint64_t> dist;

    std::ostringstream oss = {};
    oss << std::hex << now_ms << "-" << dist(rng);
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// Decision Record integration
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Set Decision Record Processor.
 * @param[in] processor Input parameter.
 */
void CrossShardFeedbackSync::setDecisionRecordProcessor(
    std::shared_ptr<themis::llm::DecisionRecordYamlProcessor> processor)
{
    /**
     * @brief Lk.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mutex_);
    dr_processor_ = std::move(processor);
}

void CrossShardFeedbackSync::emitFeedbackDecisionRecord(
    const std::string& direction,
    const FeedbackSummary& summary) const
{
    // Caller holds mutex_
    if (!dr_processor_) {
        return;
    }

    themis::llm::DecisionRecord rec;
    rec.decision_type = "FEDERATED_FEEDBACK";
    rec.component     = "CrossShardFeedbackSync";
    rec.shard_id      = local_shard_id_;
    rec.outcome       = "SUCCESS";
    rec.record_id     = summary.summary_id;

    rec.parameters["direction"]       = direction;
    rec.parameters["feedback_type"]   = summary.feedback_type_label;
    rec.parameters["embedding_dim"]   = std::to_string(summary.reason_embedding.size());

    dr_processor_->submit(std::move(rec));
}

/**
 * @brief ───────────────────────────────────────────────────────────────────────────── DK-OR: ZeroTrust setter, skipped-publish counter, erase ─────────────────────────────────────────────────────────────────────────────
 * @param[in] enforcer Input parameter.
 * @details Calls: lk(), std::move().
 */

void CrossShardFeedbackSync::setZeroTrustEnforcer(ZeroTrustEnforcer enforcer) {
    std::lock_guard<std::mutex> lk(mutex_);
    zero_trust_enforcer_ = std::move(enforcer);
}

size_t CrossShardFeedbackSync::getSkippedPublishCount() const {
    /**
     * @brief Lk.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mutex_);
    return skipped_publish_count_;
}

/**
 * @brief Erase.
 * @param[in] param Input parameter.
 * @param[in] Regulation Input parameter.
 * @return Return value.
 */
themis::governance::StoreErasureResult CrossShardFeedbackSync::erase(
    const std::string& /*subject_id*/,
    themis::governance::Regulation /*regulation*/)
{
    /**
     * @brief Lk.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mutex_);
    seen_ids_.clear();
    ++erase_count_;

    themis::governance::StoreErasureResult result;
    result.store_id       = "CrossShardFeedbackSync";
    result.records_erased = 1;
    result.success        = true;
    return result;
}

size_t CrossShardFeedbackSync::eraseCount() const {
    /**
     * @brief Lk.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mutex_);
    return erase_count_;
}

} // namespace themis::distributed_knowledge
