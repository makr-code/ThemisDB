/**
 * @file federation_resilience.cpp
 * @brief Implementation of resilience patterns for federated queries
 * @version 0.0.1
 * @date 2026-08-05
 * @copyright Apache-2.0, (c) 2026 ThemisDB Contributors
 */

#include "query/federation_resilience.h"
#include <algorithm>
#include <chrono>
#include <spdlog/spdlog.h>

namespace themis::query {

// ============================================================================
// CircuitBreaker Implementation
// ============================================================================

CircuitBreaker::CircuitBreaker(
    const std::string& shard_id,
    const Config& config)
    : shard_id_(shard_id),
      config_(config),
      last_failure_time_(std::chrono::steady_clock::now()) {
    spdlog::debug("CircuitBreaker created for shard: {}", shard_id_);
}

void CircuitBreaker::recordSuccess() {
    if (state_ == State::HALF_OPEN) {
        state_ = State::CLOSED;
        failure_count_ = 0;
        half_open_requests_ = 0;
        spdlog::info("CircuitBreaker[{}]: HALF_OPEN -> CLOSED (recovery successful)", 
                     shard_id_);
    } else if (state_ == State::CLOSED) {
        failure_count_ = 0;
    }
}

void CircuitBreaker::recordFailure(const std::string& failure_reason) {
    last_failure_reason_ = failure_reason;
    last_failure_time_ = std::chrono::steady_clock::now();

    if (state_ == State::CLOSED) {
        failure_count_++;
        spdlog::debug("CircuitBreaker[{}]: failure recorded ({}/{})", 
                      shard_id_, failure_count_, config_.failure_threshold);

        if (failure_count_ >= config_.failure_threshold) {
            state_ = State::OPEN;
            open_time_ = std::chrono::steady_clock::now();
            spdlog::warn(
                "CircuitBreaker[{}]: CLOSED -> OPEN (threshold exceeded). Reason: {}",
                shard_id_,
                failure_reason);
        }
    } else if (state_ == State::HALF_OPEN) {
        state_ = State::OPEN;
        open_time_ = std::chrono::steady_clock::now();
        half_open_requests_ = 0;
        spdlog::warn(
            "CircuitBreaker[{}]: HALF_OPEN -> OPEN (recovery failed). Reason: {}",
            shard_id_,
            failure_reason);
    }
}

bool CircuitBreaker::allowRequest() const {
    if (state_ == State::CLOSED) {
        return true;
    }

    if (state_ == State::OPEN) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - open_time_);

        if (elapsed >= config_.timeout) {
            // Time to try recovery
            const_cast<CircuitBreaker*>(this)->state_ = State::HALF_OPEN;
            const_cast<CircuitBreaker*>(this)->half_open_requests_ = 0;
            spdlog::info("CircuitBreaker[{}]: OPEN -> HALF_OPEN (trying recovery)",
                        shard_id_);
            return true;
        }
        return false;
    }

    if (state_ == State::HALF_OPEN) {
        if (const_cast<CircuitBreaker*>(this)->half_open_requests_ <
            config_.half_open_max_requests) {
            const_cast<CircuitBreaker*>(this)->half_open_requests_++;
            return true;
        }
        return false;
    }

    return false;
}

CircuitBreaker::State CircuitBreaker::getState() const {
    return state_;
}

const std::string& CircuitBreaker::getShardId() const {
    return shard_id_;
}

std::optional<std::string> CircuitBreaker::getFailureReason() const {
    return last_failure_reason_;
}

std::string CircuitBreaker::getStatistics() const {
    std::string state_str;
    switch (state_) {
        case State::CLOSED:
            state_str = "CLOSED";
            break;
        case State::OPEN:
            state_str = "OPEN";
            break;
        case State::HALF_OPEN:
            state_str = "HALF_OPEN";
            break;
    }

    std::string stats = "CircuitBreaker[" + shard_id_ + "]:\n";
    stats += "  state: " + state_str + "\n";
    stats += "  failure_count: " + std::to_string(failure_count_) + "\n";
    stats += "  threshold: " + std::to_string(config_.failure_threshold) + "\n";
    if (last_failure_reason_) {
        stats += "  last_failure: " + *last_failure_reason_ + "\n";
    }
    return stats;
}

void CircuitBreaker::reset() {
    state_ = State::CLOSED;
    failure_count_ = 0;
    half_open_requests_ = 0;
    last_failure_reason_.reset();
    spdlog::info("CircuitBreaker[{}]: reset to CLOSED", shard_id_);
}

// ============================================================================
// DegradedModeExecutor Implementation
// ============================================================================

DegradedModeExecutor::DegradedModeExecutor(Strategy strategy)
    : strategy_(strategy) {
    spdlog::debug("DegradedModeExecutor created with strategy: {}",
                  static_cast<int>(strategy_));
}

void DegradedModeExecutor::setStrategy(Strategy strategy) {
    strategy_ = strategy;
    spdlog::debug("DegradedModeExecutor strategy changed to: {}",
                  static_cast<int>(strategy_));
}

DegradedModeExecutor::Strategy DegradedModeExecutor::getStrategy() const {
    return strategy_;
}

bool DegradedModeExecutor::shouldProceedDegraded(
    size_t available_shards,
    size_t total_shards) const {
    if (total_shards == 0) {
        return false;
    }

    const double coverage = 100.0 * static_cast<double>(available_shards) /
                           static_cast<double>(total_shards);

    switch (strategy_) {
        case Strategy::FAIL_FAST:
            return available_shards == total_shards;
        case Strategy::PARTIAL_RESULTS:
            return coverage >= minimum_coverage_pct_;
        case Strategy::FALLBACK_REPLICA:
            return available_shards > 0;
        case Strategy::BEST_EFFORT:
            return available_shards > 0;
    }
    return false;
}

double DegradedModeExecutor::calculateConfidence(
    size_t available_shards,
    size_t total_shards) const {
    if (total_shards == 0) {
        return 0.0;
    }
    return 100.0 * static_cast<double>(available_shards) /
           static_cast<double>(total_shards);
}

double DegradedModeExecutor::getMinimumCoverage() const {
    return minimum_coverage_pct_;
}

void DegradedModeExecutor::setMinimumCoverage([[maybe_unused]] double coverage_pct) {
    if (coverage_pct < 0.0 || coverage_pct > 100.0) {
        spdlog::warn("Invalid coverage percentage: {}; ignoring", coverage_pct);
        return;
    }
    minimum_coverage_pct_ = coverage_pct;
}

std::string DegradedModeExecutor::getStatistics() const {
    std::string stats = "DegradedModeExecutor:\n";
    stats += "  strategy: ";
    switch (strategy_) {
        case Strategy::FAIL_FAST:
            stats += "FAIL_FAST";
            break;
        case Strategy::PARTIAL_RESULTS:
            stats += "PARTIAL_RESULTS";
            break;
        case Strategy::FALLBACK_REPLICA:
            stats += "FALLBACK_REPLICA";
            break;
        case Strategy::BEST_EFFORT:
            stats += "BEST_EFFORT";
            break;
    }
    stats += "\n";
    stats += "  minimum_coverage: " + std::to_string(minimum_coverage_pct_) + "%\n";
    stats += "  degraded_executions: " + std::to_string(degraded_executions_) + "\n";
    stats += "  partial_result_queries: " + std::to_string(partial_result_queries_) +
             "\n";
    return stats;
}

// ============================================================================
// RecoveryTimeTracker Implementation
// ============================================================================

RecoveryTimeTracker::RecoveryTimeTracker(
    const std::string& shard_id,
    uint64_t recovery_sla_ms)
    : shard_id_(shard_id),
      recovery_sla_ms_(recovery_sla_ms) {
    spdlog::debug("RecoveryTimeTracker created for shard {}: SLA={}ms",
                  shard_id_, recovery_sla_ms_);
}

void RecoveryTimeTracker::markDegraded() {
    if (!is_degraded_) {
        is_degraded_ = true;
        degradation_start_ = std::chrono::steady_clock::now();
        spdlog::info("RecoveryTimeTracker[{}]: marked as degraded", shard_id_);
    }
}

void RecoveryTimeTracker::markRecovered() {
    if (is_degraded_) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - degradation_start_);
        total_degradation_time_ms_ += elapsed.count();
        recovery_event_count_++;

        bool sla_met = elapsed.count() <= static_cast<int64_t>(recovery_sla_ms_);
        spdlog::info(
            "RecoveryTimeTracker[{}]: recovered in {}ms (SLA: {}ms) - {}",
            shard_id_,
            elapsed.count(),
            recovery_sla_ms_,
            sla_met ? "OK" : "VIOLATED");

        is_degraded_ = false;
    }
}

bool RecoveryTimeTracker::isSLAMet() const {
    if (!is_degraded_) {
        return true;  // Not degraded
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - degradation_start_);
    return elapsed.count() <= static_cast<int64_t>(recovery_sla_ms_);
}

uint64_t RecoveryTimeTracker::getTimeSinceDegradation() const {
    if (!is_degraded_) {
        return 0;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - degradation_start_);
    return elapsed.count();
}

uint64_t RecoveryTimeTracker::getRecoverySLAMs() const {
    return recovery_sla_ms_;
}

const std::string& RecoveryTimeTracker::getShardId() const {
    return shard_id_;
}

std::string RecoveryTimeTracker::getStatistics() const {
    std::string stats = "RecoveryTimeTracker[" + shard_id_ + "]:\n";
    stats += "  is_degraded: " + std::string(is_degraded_ ? "true" : "false") + "\n";
    stats += "  recovery_sla: " + std::to_string(recovery_sla_ms_) + "ms\n";
    stats += "  total_degradation_time: " + std::to_string(total_degradation_time_ms_) +
             "ms\n";
    stats += "  recovery_events: " + std::to_string(recovery_event_count_) + "\n";
    if (is_degraded_) {
        stats += "  current_degradation_time: " + std::to_string(getTimeSinceDegradation()) +
                 "ms\n";
    }
    return stats;
}

// ============================================================================
// FederationResilienceCoordinator Implementation
// ============================================================================

FederationResilienceCoordinator::FederationResilienceCoordinator(
    DegradedModeExecutor::Strategy default_strategy)
    : degraded_executor_(default_strategy) {
    spdlog::debug("FederationResilienceCoordinator created");
}

CircuitBreaker& FederationResilienceCoordinator::getOrCreateCircuitBreaker(
    const std::string& shard_id,
    const CircuitBreaker::Config& config) {
    auto it = circuit_breakers_.find(shard_id);
    if (it == circuit_breakers_.end()) {
        circuit_breakers_.emplace(shard_id, CircuitBreaker(shard_id, config));
    }
    return circuit_breakers_.at(shard_id);
}

bool FederationResilienceCoordinator::isShardAvailable(
    const std::string& shard_id) const {
    auto it = circuit_breakers_.find(shard_id);
    if (it == circuit_breakers_.end()) {
        return true;  // Unknown shard assumed available
    }
    return it->second.allowRequest();
}

size_t FederationResilienceCoordinator::getAvailableShardCount() const {
    size_t count = 0;
    for (const auto& [shard_id, cb] : circuit_breakers_) {
        if (cb.getState() == CircuitBreaker::State::CLOSED) {
            count++;
        }
    }
    return count;
}

size_t FederationResilienceCoordinator::getDegradedShardCount() const {
    size_t count = 0;
    for (const auto& [shard_id, cb] : circuit_breakers_) {
        if (cb.getState() != CircuitBreaker::State::CLOSED) {
            count++;
        }
    }
    return count;
}

DegradedModeExecutor& FederationResilienceCoordinator::getDegradedModeExecutor() {
    return degraded_executor_;
}

void FederationResilienceCoordinator::registerShardForRecoveryTracking(
    const std::string& shard_id,
    uint64_t recovery_sla_ms) {
    if (recovery_trackers_.find(shard_id) == recovery_trackers_.end()) {
        recovery_trackers_.emplace(shard_id,
                                   RecoveryTimeTracker(shard_id, recovery_sla_ms));
    }
}

std::optional<std::reference_wrapper<RecoveryTimeTracker>>
FederationResilienceCoordinator::getRecoveryTracker(
    const std::string& shard_id) {
    auto it = recovery_trackers_.find(shard_id);
    if (it != recovery_trackers_.end()) {
        return std::reference_wrapper<RecoveryTimeTracker>(it->second);
    }
    return std::nullopt;
}

std::string FederationResilienceCoordinator::getResilienceStatistics() const {
    std::string stats = "FederationResilienceCoordinator Statistics:\n";
    stats += "  total_shards: " + std::to_string(circuit_breakers_.size()) + "\n";
    stats += "  available_shards: " + std::to_string(getAvailableShardCount()) + "\n";
    stats += "  degraded_shards: " + std::to_string(getDegradedShardCount()) + "\n";
    stats += "  recovery_tracked_shards: " + std::to_string(recovery_trackers_.size()) +
             "\n";
    stats += "\n" + degraded_executor_.getStatistics();
    return stats;
}

std::unordered_map<std::string, std::string>
FederationResilienceCoordinator::getShardStatesSummary() const {
    std::unordered_map<std::string, std::string> summary;
    for (const auto& [shard_id, cb] : circuit_breakers_) {
        std::string state_str;
        switch (cb.getState()) {
            case CircuitBreaker::State::CLOSED:
                state_str = "healthy";
                break;
            case CircuitBreaker::State::OPEN:
                state_str = "failed";
                break;
            case CircuitBreaker::State::HALF_OPEN:
                state_str = "recovering";
                break;
        }
        summary[shard_id] = state_str;
    }
    return summary;
}

} // namespace themis::query
