// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/// @file shard_summary_coordinator.cc
/// @brief Phase C: shard summary refresh, summary-first routing with
///        escalation, and exact-on-demand tensor fetch implementation.

#include "shard_summary_coordinator.h"

#include <chrono>
#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

namespace themis {
namespace distributed_tensor {

// ============================================================================
// Internal helpers
// ============================================================================

namespace {

/// Return current epoch-milliseconds via steady/system clock.
[[nodiscard]] int64_t wallClockMs() noexcept {
    using namespace std::chrono;
    return duration_cast<milliseconds>(
               system_clock::now().time_since_epoch())
        .count();
}

} // anonymous namespace

// ============================================================================
// ShardFreshnessRecord
// ============================================================================

bool ShardFreshnessRecord::isExpired(int64_t now_ms) const noexcept {
    if (last_refresh_ms == 0) {
        // Never refreshed → treat as expired.
        return true;
    }
    const int64_t ref = (now_ms > 0) ? now_ms : wallClockMs();
    const int64_t elapsed_ms = ref - last_refresh_ms;
    return elapsed_ms > static_cast<int64_t>(ttl_seconds) * 1000;
}

void ShardFreshnessRecord::markRefreshed(int64_t refresh_time_ms) noexcept {
    last_refresh_ms = (refresh_time_ms > 0) ? refresh_time_ms : wallClockMs();
    freshness_state = tensor::SummaryFreshnessState::FRESH;
    last_refresh_error.clear();
    ++refresh_generation;
}

// ============================================================================
// ShardSummaryCoordinator — construction
// ============================================================================

ShardSummaryCoordinator::ShardSummaryCoordinator(
    std::shared_ptr<IShardFetcher> fetcher,
    ManifestStore* manifest_store,
    Config config) noexcept
    : fetcher_(std::move(fetcher)),
      manifest_store_(manifest_store),
      config_(config) {
    config_.validateAndClamp();
}

// ============================================================================
// Config validation and Byzantine safety (SG-DT-01)
// ============================================================================

void ShardSummaryCoordinator::Config::validateAndClamp() noexcept {
    // Enforce minimum quorum ratio for Byzantine Fault Tolerance.
    // SG-DT-01 requires majority (>= 50%) participation.
    if (freshness_quorum_ratio < 0.5f) {
        // Log diagnostic: unsafe quorum ratio being clamped
        spdlog::warn("ShardSummaryCoordinator::Config::validateAndClamp: "
                     "freshness_quorum_ratio {} is below minimum 0.5; clamping to 0.5",
                     freshness_quorum_ratio);
        freshness_quorum_ratio = 0.5f;
    } else if (freshness_quorum_ratio > 1.0f) {
        // Also clamp upper bound to valid fraction
        spdlog::warn("ShardSummaryCoordinator::Config::validateAndClamp: "
                     "freshness_quorum_ratio {} exceeds maximum 1.0; clamping to 1.0",
                     freshness_quorum_ratio);
        freshness_quorum_ratio = 1.0f;
    }
}

// ============================================================================
// Shard registration
// ============================================================================

void ShardSummaryCoordinator::registerShard(const std::string& shard_id,
                                             uint32_t ttl_seconds) noexcept {
    const uint32_t ttl =
        (ttl_seconds > 0) ? ttl_seconds : config_.default_ttl_seconds;

    std::lock_guard<std::mutex> lk(records_mutex_);
    if (records_.count(shard_id) == 0) {
        ShardFreshnessRecord rec;
        rec.shard_id = shard_id;
        rec.ttl_seconds = ttl;
        records_.emplace(shard_id, std::move(rec));
    }
}

void ShardSummaryCoordinator::unregisterShard(
    const std::string& shard_id) noexcept {
    std::lock_guard<std::mutex> lk(records_mutex_);
    records_.erase(shard_id);
}

// ============================================================================
// Summary refresh
// ============================================================================

ShardSummaryRefreshResult ShardSummaryCoordinator::refreshShard(
    const std::string& shard_id,
    tensor::ShardSummary& summary,
    int64_t now_ms) noexcept {
    ShardSummaryRefreshResult result;
    result.shard_id = shard_id;

    const int64_t ts = resolveNow(now_ms);

    {
        std::lock_guard<std::mutex> lk(records_mutex_);
        auto it = records_.find(shard_id);
        if (it == records_.end()) {
            // Auto-register with default TTL.
            spdlog::debug("ShardSummaryCoordinator::refreshShard: auto-registering shard_id={} "
                         "with default_ttl_seconds={}",
                         shard_id, config_.default_ttl_seconds);
            ShardFreshnessRecord rec;
            rec.shard_id = shard_id;
            rec.ttl_seconds = config_.default_ttl_seconds;
            records_.emplace(shard_id, rec);
            it = records_.find(shard_id);
        }

        it->second.markRefreshed(ts);
        result.generation = it->second.refresh_generation;
        result.freshness_state = tensor::SummaryFreshnessState::FRESH;
        
        spdlog::debug("ShardSummaryCoordinator::refreshShard: refreshed shard_id={} "
                     "at_timestamp_ms={} generation={}",
                     shard_id, ts, result.generation);
    }

    // Update the advisory summary's freshness fields.
    summary.freshness_state = tensor::SummaryFreshnessState::FRESH;
    summary.markAsFresh(/*update_timestamp=*/true);

    result.success = true;
    result.refreshed_at_ms = ts;
    stat_refreshes_.fetch_add(1, std::memory_order_relaxed);
    return result;
}

std::vector<ShardSummaryRefreshResult> ShardSummaryCoordinator::refreshAll(
    std::unordered_map<std::string, tensor::ShardSummary>& summaries,
    int64_t now_ms) noexcept {
    std::vector<ShardSummaryRefreshResult> results = {};

    results.reserve(summaries.size());

    const int64_t ts = resolveNow(now_ms);
    for (auto& [shard_id, summary] : summaries) {
        results.push_back(refreshShard(shard_id, summary, ts));
    }
    return results;
}

// ============================================================================
// Freshness queries
// ============================================================================

std::optional<ShardFreshnessRecord> ShardSummaryCoordinator::getFreshnessRecord(
    const std::string& shard_id) const noexcept {
    std::lock_guard<std::mutex> lk(records_mutex_);
    auto it = records_.find(shard_id);
    if (it == records_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool ShardSummaryCoordinator::isFresh(const std::string& shard_id,
                                       int64_t now_ms) const noexcept {
    std::lock_guard<std::mutex> lk(records_mutex_);
    auto it = records_.find(shard_id);
    if (it == records_.end()) {
        return false;
    }
    const auto& rec = it->second;
    if (rec.freshness_state != tensor::SummaryFreshnessState::FRESH) {
        return false;
    }
    return !rec.isExpired(resolveNow(now_ms));
}

FreshnessConsensusResult ShardSummaryCoordinator::checkFreshnessConsensus(
    const std::vector<std::string>& shard_ids,
    int64_t now_ms) const noexcept {
    FreshnessConsensusResult res;
    res.total_shards = shard_ids.size();
    res.quorum_ratio = config_.freshness_quorum_ratio;

    if (shard_ids.empty()) {
        res.quorum_met = false;
        return res;
    }

    const int64_t ts = resolveNow(now_ms);

    std::lock_guard<std::mutex> lk(records_mutex_);
    for (const auto& sid : shard_ids) {
        auto it = records_.find(sid);
        if (it == records_.end()) {
            ++res.stale_shards;
            continue;
        }
        const auto& rec = it->second;
        if (rec.freshness_state == tensor::SummaryFreshnessState::INVALID) {
            ++res.invalid_shards;
        } else if (rec.freshness_state == tensor::SummaryFreshnessState::FRESH &&
                   !rec.isExpired(ts)) {
            ++res.fresh_shards;
        } else {
            ++res.stale_shards;
        }
    }

    const float ratio =
        static_cast<float>(res.fresh_shards) /
        static_cast<float>(res.total_shards);
    res.quorum_met = (ratio >= config_.freshness_quorum_ratio);
    return res;
}

// ============================================================================
// Summary-first routing with escalation
// ============================================================================

std::vector<RoutingDecision> ShardSummaryCoordinator::routeSummaryFirst(
    const std::vector<tensor::ShardSummary>& summaries,
    AccuracyMode mode,
    int64_t now_ms) const noexcept {
    std::vector<RoutingDecision> decisions = {};

    decisions.reserve(summaries.size());

    const int64_t ts = resolveNow(now_ms);
    uint32_t escalation_count = 0;

    for (const auto& s : summaries) {
        stat_routing_decisions_.fetch_add(1, std::memory_order_relaxed);

        RoutingDecision d;
        d.shard_id = s.shard_id;
        d.advisory_score = s.shard_relevance;

        // Resolve actual freshness. Explicit summary state (STALE/INVALID)
        // takes precedence so a bad advisory summary is not silently
        // reclassified by an unrefreshed coordinator record.
        tensor::SummaryFreshnessState effective_state = s.freshness_state;
        if (effective_state == tensor::SummaryFreshnessState::FRESH) {
            std::lock_guard<std::mutex> lk(records_mutex_);
            auto it = records_.find(s.shard_id);
            if (it != records_.end()) {
                const auto& rec = it->second;
                if (rec.freshness_state == tensor::SummaryFreshnessState::INVALID) {
                    effective_state = tensor::SummaryFreshnessState::INVALID;
                } else if (rec.isExpired(ts)) {
                    effective_state = tensor::SummaryFreshnessState::STALE;
                } else {
                    effective_state = rec.freshness_state;
                }
            }
        }
        d.summary_freshness = effective_state;

        if (effective_state == tensor::SummaryFreshnessState::INVALID) {
            if (config_.skip_invalid_shards) {
                d.include_shard = false;
                d.escalate_to_exact = false;
                d.reason = "shard_summary_invalid_skipped";
                spdlog::debug("ShardSummaryCoordinator::routeSummaryFirst: shard_id={} "
                             "INVALID state, skipping (config.skip_invalid_shards=true)",
                             d.shard_id);
                decisions.push_back(std::move(d));
                continue;
            }
            // Include with escalation even for INVALID when configured.
            d.include_shard = true;
            d.escalate_to_exact = true;
            d.reason = "shard_summary_invalid_escalate";
            stat_escalations_.fetch_add(1, std::memory_order_relaxed);
            ++escalation_count;
            spdlog::warn("ShardSummaryCoordinator::routeSummaryFirst: shard_id={} "
                        "INVALID state, escalating to exact fetch (config.skip_invalid_shards=false)",
                        d.shard_id);
            decisions.push_back(std::move(d));
            continue;
        }

        if (effective_state == tensor::SummaryFreshnessState::STALE) {
            if (config_.escalate_stale_shards) {
                d.include_shard = true;
                d.escalate_to_exact = true;
                d.reason = "shard_summary_stale_escalate_to_exact";
                stat_escalations_.fetch_add(1, std::memory_order_relaxed);
                ++escalation_count;
                spdlog::info("ShardSummaryCoordinator::routeSummaryFirst: shard_id={} "
                            "STALE, escalating to exact fetch", d.shard_id);
            } else {
                d.include_shard = false;
                d.escalate_to_exact = false;
                d.reason = "shard_summary_stale_skipped";
                spdlog::debug("ShardSummaryCoordinator::routeSummaryFirst: shard_id={} "
                             "STALE, skipping (config.escalate_stale_shards=false)", d.shard_id);
            }
            decisions.push_back(std::move(d));
            continue;
        }

        // Summary is FRESH.
        d.include_shard = true;
        if (mode == AccuracyMode::EXACT) {
            d.escalate_to_exact = true;
            d.reason = "accuracy_mode_exact_forced";
            stat_escalations_.fetch_add(1, std::memory_order_relaxed);
            ++escalation_count;
            spdlog::debug("ShardSummaryCoordinator::routeSummaryFirst: shard_id={} "
                         "FRESH summary, forced to exact fetch (accuracy_mode=EXACT)",
                         d.shard_id);
        } else {
            d.escalate_to_exact = false;
            d.reason = "shard_summary_fresh_advisory";
            spdlog::debug("ShardSummaryCoordinator::routeSummaryFirst: shard_id={} "
                         "using FRESH advisory summary (advisory_score={})",
                         d.shard_id, d.advisory_score);
        }
        decisions.push_back(std::move(d));
    }
    
    if (escalation_count > 0) {
        spdlog::info("ShardSummaryCoordinator::routeSummaryFirst: "
                    "processed {} summaries with {} escalations to exact fetch",
                    summaries.size(), escalation_count);
    }
    return decisions;
}

// ============================================================================
// Exact-on-demand fetch
// ============================================================================

ExactFetchResult ShardSummaryCoordinator::fetchExact(
    const ExactFetchRequest& request) const noexcept {
    stat_exact_fetches_.fetch_add(1, std::memory_order_relaxed);

    ExactFetchResult result;
    result.shard_id = request.shard_id;
    result.artifact_id = request.artifact_id;
    result.fetched_at_ms = resolveNow(0);

    if (!fetcher_) {
        result.success = false;
        result.error_reason = "no_fetcher_configured";
        stat_refresh_failures_.fetch_add(1, std::memory_order_relaxed);
        spdlog::error("ShardSummaryCoordinator::fetchExact: no fetcher configured for "
                     "artifact_id={} shard_id={}",
                     request.artifact_id, request.shard_id);
        return result;
    }

    spdlog::debug("ShardSummaryCoordinator::fetchExact: starting exact fetch for "
                 "artifact_id={} shard_id={} with timeout_ms={}",
                 request.artifact_id, request.shard_id, request.timeout_ms);

    const auto start = std::chrono::steady_clock::now();
    result = fetcher_->fetch(request);
    const auto end = std::chrono::steady_clock::now();

    // Overwrite latency with measured value regardless of fetcher's reporting.
    const float measured_ms = static_cast<float>(
        std::chrono::duration_cast<std::chrono::microseconds>(end - start)
            .count()) /
        1000.0f;
    result.fetch_latency_ms = measured_ms;

    if (result.success) {
        stat_exact_fetch_successes_.fetch_add(1, std::memory_order_relaxed);
        spdlog::debug("ShardSummaryCoordinator::fetchExact: success for "
                     "artifact_id={} shard_id={} latency_ms={:.2f}",
                     request.artifact_id, request.shard_id, measured_ms);
    } else {
        stat_refresh_failures_.fetch_add(1, std::memory_order_relaxed);
        spdlog::warn("ShardSummaryCoordinator::fetchExact: failure for "
                    "artifact_id={} shard_id={} reason={} latency_ms={:.2f}",
                    request.artifact_id, request.shard_id, result.error_reason, measured_ms);
    }
    return result;
}

std::vector<ExactFetchResult> ShardSummaryCoordinator::fetchEscalated(
    const std::vector<RoutingDecision>& decisions,
    const std::string& artifact_id,
    const std::string& correlation_id) const noexcept {
    std::vector<ExactFetchResult> results;

    for (const auto& d : decisions) {
        if (!d.escalate_to_exact) {
            continue;
        }
        ExactFetchRequest req;
        req.shard_id = d.shard_id;
        req.artifact_id = artifact_id;
        req.timeout_ms = config_.exact_fetch_timeout_ms;
        req.correlation_id = correlation_id;
        results.push_back(fetchExact(req));
    }
    return results;
}

// ============================================================================
// Configuration and stats
// ============================================================================

void ShardSummaryCoordinator::setConfig(const Config& config) noexcept {
    Config validated_config = config;
    validated_config.validateAndClamp();
    config_ = validated_config;
}

ShardSummaryCoordinator::Config ShardSummaryCoordinator::config() const noexcept {
    return config_;
}

ShardSummaryCoordinator::Stats ShardSummaryCoordinator::stats() const noexcept {
    Stats s;
    s.total_refreshes = stat_refreshes_.load(std::memory_order_relaxed);
    s.total_refresh_failures = stat_refresh_failures_.load(std::memory_order_relaxed);
    s.total_routing_decisions = stat_routing_decisions_.load(std::memory_order_relaxed);
    s.total_escalations = stat_escalations_.load(std::memory_order_relaxed);
    s.total_exact_fetches = stat_exact_fetches_.load(std::memory_order_relaxed);
    s.total_exact_fetch_successes = stat_exact_fetch_successes_.load(std::memory_order_relaxed);
    return s;
}

// ============================================================================
// Internal
// ============================================================================

/*static*/ int64_t ShardSummaryCoordinator::resolveNow(int64_t hint_ms) noexcept {
    return (hint_ms > 0) ? hint_ms : wallClockMs();
}

} // namespace distributed_tensor
} // namespace themis
