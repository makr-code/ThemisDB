/**
 * @file query_federation_timeout.cpp
 * @brief Implementation of timeout and retry infrastructure for federated queries
 * @version 0.0.1
 * @date 2026-08-05
 * @copyright Apache-2.0, (c) 2026 ThemisDB Contributors
 */

#include "query/query_federation_timeout.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <spdlog/spdlog.h>

namespace themis::query {

// ============================================================================
// TimeoutPolicy Implementation
// ============================================================================

TimeoutPolicy TimeoutPolicy::Builder::build() const {
    if (per_shard_timeout_.count() <= 0) {
        throw std::invalid_argument("per_shard_timeout must be positive");
    }
    if (overall_timeout_.count() <= 0) {
        throw std::invalid_argument("overall_timeout must be positive");
    }
    if (max_retries_ < 0) {
        throw std::invalid_argument("max_retries must be non-negative");
    }
    if (backoff_multiplier_ <= 1.0) {
        throw std::invalid_argument("backoff_multiplier must be > 1.0");
    }
    if (jitter_fraction_ < 0.0 || jitter_fraction_ > 1.0) {
        throw std::invalid_argument("jitter_fraction must be in [0, 1]");
    }

    return TimeoutPolicy(
        per_shard_timeout_,
        overall_timeout_,
        max_retries_,
        initial_backoff_ms_,
        max_backoff_ms_,
        backoff_multiplier_,
        jitter_fraction_);
}

TimeoutPolicy::TimeoutPolicy(
    std::chrono::milliseconds per_shard_timeout,
    std::chrono::milliseconds overall_timeout,
    int max_retries,
    int initial_backoff_ms,
    int max_backoff_ms,
    double backoff_multiplier,
    double jitter_fraction)
    : per_shard_timeout_(per_shard_timeout),
      overall_timeout_(overall_timeout),
      max_retries_(max_retries),
      initial_backoff_ms_(initial_backoff_ms),
      max_backoff_ms_(max_backoff_ms),
      backoff_multiplier_(backoff_multiplier),
      jitter_fraction_(jitter_fraction) {
    spdlog::debug(
        "TimeoutPolicy constructed: per_shard={}ms, overall={}ms, max_retries={}",
        per_shard_timeout_.count(),
        overall_timeout_.count(),
        max_retries_);
}

std::chrono::milliseconds TimeoutPolicy::calculateBackoff(int attempt) const {
    if (attempt < 0) {
        return std::chrono::milliseconds(0);
    }

    // Calculate base backoff: initial_backoff * (multiplier ^ attempt)
    double base = static_cast<double>(initial_backoff_ms_) *
                  std::pow(backoff_multiplier_, static_cast<double>(attempt));
    
    // Cap at max_backoff
    base = std::min(base, static_cast<double>(max_backoff_ms_));

    // Add jitter
    std::uniform_real_distribution<double> dist(
        1.0 - jitter_fraction_, 1.0 + jitter_fraction_);
    double jittered = base * dist(rng_);

    return std::chrono::milliseconds(static_cast<int64_t>(jittered));
}

bool TimeoutPolicy::shouldRetry(
    std::chrono::milliseconds elapsed,
    int attempt) const {
    // Check if attempt limit exceeded
    if (attempt >= max_retries_) {
        spdlog::debug("shouldRetry: max retries ({}) exceeded", max_retries_);
        return false;
    }

    // Check if per-shard timeout exceeded
    if (elapsed >= per_shard_timeout_) {
        spdlog::debug(
            "shouldRetry: per-shard timeout ({}ms) exceeded (elapsed: {}ms)",
            per_shard_timeout_.count(),
            elapsed.count());
        return false;
    }

    return true;
}

bool TimeoutPolicy::isOverallTimeoutExceeded(
    std::chrono::milliseconds total_elapsed) const {
    return total_elapsed >= overall_timeout_;
}

void TimeoutPolicy::recordTimeoutEvent(const TimeoutEvent& event) {
    timeout_events_.push_back(event);
    
    std::string type_str;
    switch (event.event_type) {
        case TimeoutEvent::Type::SHARD_TIMEOUT:
            type_str = "SHARD_TIMEOUT";
            break;
        case TimeoutEvent::Type::OVERALL_TIMEOUT:
            type_str = "OVERALL_TIMEOUT";
            break;
        case TimeoutEvent::Type::RETRY_BACKOFF:
            type_str = "RETRY_BACKOFF";
            break;
        case TimeoutEvent::Type::RETRY_EXHAUSTED:
            type_str = "RETRY_EXHAUSTED";
            break;
    }

    spdlog::debug(
        "Timeout event: type={}, shard={}, elapsed={}ms, attempt={}, details={}",
        type_str,
        event.shard_id,
        event.elapsed.count(),
        event.attempt_number,
        event.details);
}

void TimeoutPolicy::recordRetryStats(
    const std::string& shard_id,
    const RetryStats& stats) {
    shard_stats_[shard_id] = stats;

    spdlog::debug(
        "Retry stats recorded for shard {}: successful_attempt={}, "
        "total_attempts={}, total_elapsed={}ms",
        shard_id,
        stats.successful_attempt,
        stats.total_attempts,
        stats.total_elapsed.count());
}

std::optional<TimeoutPolicy::RetryStats> TimeoutPolicy::getRetryStats(
    const std::string& shard_id) const {
    auto it = shard_stats_.find(shard_id);
    if (it != shard_stats_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<TimeoutPolicy::TimeoutEvent> TimeoutPolicy::getTimeoutEvents() const {
    return timeout_events_;
}

void TimeoutPolicy::clearStatistics() {
    shard_stats_.clear();
    timeout_events_.clear();
}

std::string TimeoutPolicy::getStatisticsSummary() const {
    std::string summary = "TimeoutPolicy Statistics:\n";
    summary += "  Config:\n";
    summary += "    - per_shard_timeout: " + std::to_string(per_shard_timeout_.count()) + "ms\n";
    summary += "    - overall_timeout: " + std::to_string(overall_timeout_.count()) + "ms\n";
    summary += "    - max_retries: " + std::to_string(max_retries_) + "\n";
    summary += "  Shard Statistics:\n";
    for (const auto& [shard_id, stats] : shard_stats_) {
        summary += "    - " + shard_id + ":\n";
        summary += "        successful_attempt: " + std::to_string(stats.successful_attempt) + "\n";
        summary += "        total_attempts: " + std::to_string(stats.total_attempts) + "\n";
        summary += "        total_elapsed: " + std::to_string(stats.total_elapsed.count()) + "ms\n";
    }
    summary += "  Timeout Events: " + std::to_string(timeout_events_.size()) + "\n";
    return summary;
}

// ============================================================================
// QueryTimeoutContext Implementation
// ============================================================================

QueryTimeoutContext::QueryTimeoutContext(const TimeoutPolicy& policy)
    : policy_(policy),
      query_start_(std::chrono::steady_clock::now()) {
    spdlog::debug("QueryTimeoutContext created");
}

void QueryTimeoutContext::startShardAttempt(
    const std::string& shard_id,
    int attempt) {
    shard_attempt_start_ = std::chrono::steady_clock::now();
    
    auto& stats = shard_stats_[shard_id];
    stats.total_attempts = attempt + 1;
    
    spdlog::debug("Started shard attempt: shard={}, attempt={}", shard_id, attempt);
}

void QueryTimeoutContext::endShardAttempt(
    const std::string& shard_id,
    bool success,
    const std::string& failure_reason) {
    auto now = std::chrono::steady_clock::now();
    auto attempt_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - shard_attempt_start_);

    auto& stats = shard_stats_[shard_id];
    stats.attempt_latencies.push_back(attempt_duration);
    stats.total_elapsed += attempt_duration;

    if (success) {
        stats.successful_attempt = static_cast<int>(stats.attempt_latencies.size()) - 1;
    } else {
        stats.failure_reasons.push_back(failure_reason);
    }

    spdlog::debug(
        "Ended shard attempt: shard={}, success={}, duration={}ms, "
        "total_elapsed={}ms",
        shard_id,
        success,
        attempt_duration.count(),
        stats.total_elapsed.count());
}

bool QueryTimeoutContext::shouldRetry(const std::string& shard_id) const {
    auto it = shard_stats_.find(shard_id);
    if (it == shard_stats_.end()) {
        return false;
    }

    const auto& stats = it->second;
    int attempt = static_cast<int>(stats.attempt_latencies.size()) - 1;

    return policy_.shouldRetry(stats.total_elapsed, attempt);
}

bool QueryTimeoutContext::isOverallTimeoutExceeded() const {
    auto now = std::chrono::steady_clock::now();
    auto total_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - query_start_);

    return policy_.isOverallTimeoutExceeded(total_elapsed);
}

std::chrono::milliseconds QueryTimeoutContext::getRemainingTime() const {
    auto now = std::chrono::steady_clock::now();
    auto total_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - query_start_);
    
    auto overall_timeout = policy_.getOverallTimeout();
    if (total_elapsed >= overall_timeout) {
        return std::chrono::milliseconds(0);
    }

    return overall_timeout - total_elapsed;
}

std::chrono::milliseconds QueryTimeoutContext::getRemainingShardTime(
    const std::string& shard_id) const {
    auto it = shard_stats_.find(shard_id);
    auto per_shard_timeout = policy_.getPerShardTimeout();

    if (it == shard_stats_.end()) {
        return per_shard_timeout;
    }

    const auto& stats = it->second;
    if (stats.total_elapsed >= per_shard_timeout) {
        return std::chrono::milliseconds(0);
    }

    return per_shard_timeout - stats.total_elapsed;
}

std::chrono::milliseconds QueryTimeoutContext::getTotalElapsed() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        now - query_start_);
}

std::optional<TimeoutPolicy::RetryStats> QueryTimeoutContext::getShardStats(
    const std::string& shard_id) const {
    auto it = shard_stats_.find(shard_id);
    if (it != shard_stats_.end()) {
        return it->second;
    }
    return std::nullopt;
}

} // namespace themis::query
