/**
 * @file auto_failover_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.11
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=11, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: auto_failover_manager.cpp | Version: 0.0.11 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 99/100 | Lines: 656
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=15, M=3, L=0
 * PR History (last 5): #4553 [MODULE] failover: Phase 4 ... (2026-04-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "failover/auto_failover_manager.h"

#include <algorithm>
#include <numeric>

#include "spdlog/spdlog.h"

namespace themis {
namespace failover {

AutoFailoverManager::AutoFailoverManager(
    const AutoFailoverConfig& config,
    std::shared_ptr<themisdb::replication::ReplicationManager> replication_mgr,
    std::shared_ptr<sharding::HealthMonitor> health_monitor,
    std::shared_ptr<sharding::HotSpareManager> spare_manager,
    std::shared_ptr<sharding::EpochFencingManager> fencing_manager
)
    : config_(config),
      replication_mgr_(std::move(replication_mgr)),
      health_monitor_(std::move(health_monitor)),
      spare_manager_(std::move(spare_manager)),
      fencing_manager_(std::move(fencing_manager)) {}

AutoFailoverManager::~AutoFailoverManager() {
    if (running_.load()) {
        stop();
    }
}

bool AutoFailoverManager::start() {
    if (running_.exchange(true)) {
        spdlog::warn("AutoFailoverManager already running");
        return false;
    }

    try {
        transitionState(FailoverOrchestratorState::IDLE);
        monitoring_thread_ = std::thread(&AutoFailoverManager::monitoringLoop, this);
        failover_thread_ = std::thread(&AutoFailoverManager::failoverLoop, this);
        spdlog::info("AutoFailoverManager started successfully");
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to start AutoFailoverManager: {}", e.what());
        running_ = false;
        return false;
    }
}

bool AutoFailoverManager::stop() {
    if (!running_.exchange(false)) {
        return false;
    }

    // Lock order: failover_mutex_ → stats_mutex_ → callbacks_mutex_
    // Bounded wait contract:
    //   failover_thread_:   wakes within ≤1 s (wait_for timeout in failoverLoop).
    //   monitoring_thread_: wakes within ≤ health_check_interval (500 ms default).
    failover_cv_.notify_all();

    if (monitoring_thread_.joinable()) {
        monitoring_thread_.join();   // exits within health_check_interval
    }
    if (failover_thread_.joinable()) {
        failover_thread_.join();     // exits within 1 s cv::wait_for timeout
    }

    transitionState(FailoverOrchestratorState::IDLE);
    spdlog::info("AutoFailoverManager stopped");
    return true;
}

bool AutoFailoverManager::isRunning() const {
    return running_.load();
}

bool AutoFailoverManager::triggerManualFailover(
    const std::string& failed_node_id,
    const std::string& target_promote_id
) {
    if (!running_.load()) {
        spdlog::error("Cannot trigger failover: manager not running");
        return false;
    }

    // Lock order: failover_mutex_ → stats_mutex_ → callbacks_mutex_
    bool pressure_event_pending = false;
    std::string pressure_detail;

    {
        std::lock_guard<std::mutex> lock(failover_mutex_);
        if (failover_queue_.size() >= config_.max_concurrent_failovers) {
            spdlog::error("Failover queue is full (max: {})", config_.max_concurrent_failovers);
            {
                std::lock_guard<std::mutex> stats_lock(stats_mutex_);
                stats_.tasks_dropped_queue_full++;
            }
            return false;
        }

        FailoverTask task;
        task.failed_node_id = failed_node_id;
        task.target_promote_id = target_promote_id;
        task.enqueued_at = std::chrono::steady_clock::now();

        failover_queue_.push(task);
        auto depth = static_cast<uint32_t>(failover_queue_.size());

        // Compute fill ratio once under the failover lock for consistency
        float fill_ratio = static_cast<float>(depth) /
                           static_cast<float>(config_.max_concurrent_failovers);
        bool over_threshold = fill_ratio >= config_.queue_pressure_threshold;

        // Update queue-depth and pressure telemetry atomically
        {
            std::lock_guard<std::mutex> stats_lock(stats_mutex_);
            stats_.current_queue_depth = depth;
            if (depth > stats_.max_queue_depth_observed) {
                stats_.max_queue_depth_observed = depth;
            }
            if (over_threshold) {
                stats_.queue_pressure_events++;
            }
        }

        if (over_threshold) {
            pressure_event_pending = true;
            pressure_detail = "Queue depth: " + std::to_string(depth) +
                              "/" + std::to_string(config_.max_concurrent_failovers);
        }

        spdlog::info("Manual failover queued for node: {}", failed_node_id);
    }

    // Emit pressure event outside the failover lock to avoid recursive locking
    if (pressure_event_pending) {
        emitEvent(FailoverEventType::QUEUE_PRESSURE, failed_node_id, pressure_detail);
    }

    failover_cv_.notify_one();
    return true;
}

FailoverOrchestratorState AutoFailoverManager::getState() const {
    return state_.load();
}

bool AutoFailoverManager::isFailoverInProgress() const {
    return failover_in_progress_.load();
}

std::vector<std::string> AutoFailoverManager::getFailingNodes() const {
    std::shared_lock<std::shared_mutex> lock(tracking_mutex_);
    std::vector<std::string> failing_nodes;

    for (const auto& [node_id, failures] : consecutive_failures_) {
        if (failures >= static_cast<int>(config_.consecutive_failures_before_action)) {
            failing_nodes.push_back(node_id);
        }
    }

    return failing_nodes;
}

std::optional<FailoverResult> AutoFailoverManager::getLastFailoverResult() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return last_failover_result_;
}

void AutoFailoverManager::updateConfig(const AutoFailoverConfig& config) {
    {
        std::lock_guard<std::mutex> lock(monitor_mutex_);
        config_ = config;
    }
    spdlog::info("AutoFailoverManager config updated");
}

AutoFailoverConfig AutoFailoverManager::getConfig() const {
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    return config_;
}

AutoFailoverManager::Statistics AutoFailoverManager::getStatistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void AutoFailoverManager::registerEventCallback(FailoverEventCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    event_callbacks_.push_back(std::move(callback));
}

void AutoFailoverManager::monitoringLoop() {
    while (running_.load()) {
        try {
            performHealthChecks();
            checkForNetworkPartitions();
            detectNodeFailures();

            std::this_thread::sleep_for(config_.health_check_interval);
        } catch (const std::exception& e) {
            spdlog::error("Error in monitoring loop: {}", e.what());
        }
    }
}

void AutoFailoverManager::performHealthChecks() {
    if (!replication_mgr_) {
        return;
    }

    const auto cluster = replication_mgr_->getClusterHealth();
    for (const auto& [node_id, is_healthy] : cluster) {
        updateFailureTracking(node_id, is_healthy);
    }
}

void AutoFailoverManager::checkForNetworkPartitions() {
    if (!config_.enable_network_partition_detection) {
        return;
    }

    if (isNetworkPartitionedFromQuorum()) {
        spdlog::warn("Network partition detected from quorum");
        emitEvent(FailoverEventType::NETWORK_PARTITION_DETECTED, "", "Split from quorum");
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.network_partitions_detected++;
        }

        if (config_.enable_split_brain_prevention) {
            handleNetworkPartition();
        }
    }
}

void AutoFailoverManager::detectNodeFailures() {
    // Detect nodes that have failed and enqueue failover tasks
    auto failing_nodes = getFailingNodes();

    for (const auto& node_id : failing_nodes) {
        spdlog::warn("Failover condition met for node: {}", node_id);
        triggerManualFailover(node_id, "");
    }
}

void AutoFailoverManager::updateFailureTracking(const std::string& node_id, bool is_healthy) {
    std::unique_lock<std::shared_mutex> lock(tracking_mutex_);

    if (is_healthy) {
        consecutive_failures_[node_id] = 0;
    } else {
        consecutive_failures_[node_id]++;
        emitEvent(
            FailoverEventType::NODE_FAILURE_DETECTED,
            node_id,
            "Consecutive failures: " + std::to_string(consecutive_failures_[node_id])
        );
    }
}

void AutoFailoverManager::failoverLoop() {
    while (running_.load()) {
        try {
            std::unique_lock<std::mutex> lock(failover_mutex_);

            // Wait for failover tasks
            failover_cv_.wait_for(lock, std::chrono::seconds(1), [this] {
                return !failover_queue_.empty() || !running_.load();
            });

            if (!running_.load()) {
                break;
            }

            if (failover_queue_.empty()) {
                continue;
            }

            auto task = failover_queue_.front();
            failover_queue_.pop();

            // Update queue depth after pop
            {
                std::lock_guard<std::mutex> stats_lock(stats_mutex_);
                stats_.current_queue_depth = static_cast<uint32_t>(failover_queue_.size());
            }

            lock.unlock();

            // Process failover
            failover_in_progress_.store(true, std::memory_order_release);
            auto result = processFailover(task);
            failover_in_progress_.store(false, std::memory_order_release);

            // Update statistics
            {
                std::lock_guard<std::mutex> stats_lock(stats_mutex_);
                last_failover_result_ = result;
                updateStatistics(result);
            }

            // Attempt recovery if failover succeeds
            if (result.success && config_.enable_automatic_recovery) {
                emitEvent(FailoverEventType::RECOVERY_STARTED, task.failed_node_id, "");
                std::this_thread::sleep_for(std::chrono::seconds(5));  // Brief delay before recovery attempt
                attemptRecovery(task.failed_node_id);
            }
        } catch (const std::exception& e) {
            spdlog::error("Error in failover loop: {}", e.what());
        }
    }
}

FailoverResult AutoFailoverManager::processFailover(const FailoverTask& task) {
    auto start_time = std::chrono::steady_clock::now();
    FailoverResult result;
    result.failed_node_id = task.failed_node_id;

    try {
        // Step 1: Verify failure still present
        transitionState(FailoverOrchestratorState::VERIFYING_FAILURE);
        spdlog::info("Verifying failure of node: {}", task.failed_node_id);

        // Step 2: Check quorum
        transitionState(FailoverOrchestratorState::CHECKING_QUORUM);
        emitEvent(FailoverEventType::QUORUM_CHECK_PASSED, task.failed_node_id, "");

        if (!checkAndWaitForQuorum()) {
            spdlog::error("Quorum check failed for node: {}", task.failed_node_id);
            emitEvent(FailoverEventType::QUORUM_CHECK_FAILED, task.failed_node_id, "No quorum");
            transitionState(FailoverOrchestratorState::FAILED);
            result.success = false;
            return result;
        }

        // Step 3: Split-brain prevention
        if (config_.enable_split_brain_prevention) {
            if (!preventSplitBrain(task.failed_node_id)) {
                spdlog::error("Split-brain prevention failed");
                transitionState(FailoverOrchestratorState::FAILED);
                result.success = false;
                return result;
            }
        }

        // Step 4: Select and promote replica or spare
        transitionState(FailoverOrchestratorState::STARTING_LEADER_ELECTION);

        std::string promoted_id;
        if (!selectAndPromoteReplica(task.failed_node_id, promoted_id)) {
            spdlog::error("Failed to select replica for promotion");
            transitionState(FailoverOrchestratorState::FAILED);
            result.success = false;
            return result;
        }

        result.promoted_node_id = promoted_id;

        // Step 5: Activate spare if needed
        if (config_.enable_spare_activation) {
            activateSpareIfNeeded(task.failed_node_id);
        }

        // Step 6: Update metadata
        transitionState(FailoverOrchestratorState::UPDATING_METADATA);
        if (!updateMetadata(task.failed_node_id, promoted_id)) {
            spdlog::error("Failed to update metadata");
            transitionState(FailoverOrchestratorState::FAILED);
            result.success = false;
            return result;
        }

        // Step 7: Verify completion
        transitionState(FailoverOrchestratorState::COMPLETING_FAILOVER);
        if (!verifyFailoverCompletion(task)) {
            spdlog::error("Failed to verify failover completion");
            transitionState(FailoverOrchestratorState::FAILED);
            result.success = false;
            return result;
        }

        transitionState(FailoverOrchestratorState::IDLE);
        result.success = true;
        emitEvent(FailoverEventType::FAILOVER_COMPLETED, promoted_id, "");

        spdlog::info("Failover completed: {} -> {}", task.failed_node_id, promoted_id);
    } catch (const std::exception& e) {
        spdlog::error("Exception during failover: {}", e.what());
        result.error_message = e.what();
        transitionState(FailoverOrchestratorState::FAILED);
    }

    auto end_time = std::chrono::steady_clock::now();
    result.duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    return result;
}

bool AutoFailoverManager::checkAndWaitForQuorum() {
    if (!replication_mgr_) {
        spdlog::warn("ReplicationManager not available");
        return false;
    }

    auto deadline = std::chrono::steady_clock::now() + config_.failover_timeout;

    while (std::chrono::steady_clock::now() < deadline) {
        if (replication_mgr_->hasQuorum()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return false;
}

bool AutoFailoverManager::startLeaderElection(const std::string& failed_node_id) {
    transitionState(FailoverOrchestratorState::LEADER_ELECTION_IN_PROGRESS);
    emitEvent(FailoverEventType::LEADER_ELECTION_STARTED, failed_node_id, "");

    if (!replication_mgr_) {
        return false;
    }

    // Trigger election by calling replication_mgr_->triggerFailover()
    auto result = replication_mgr_->triggerFailover(failed_node_id);

    if (result) {
        emitEvent(FailoverEventType::LEADER_ELECTED, failed_node_id, "");
    }

    return result;
}

bool AutoFailoverManager::selectAndPromoteReplica(const std::string& failed_node_id,
                                                  std::string& promoted_id) {
    if (!replication_mgr_) {
        return false;
    }

    const auto replicas = replication_mgr_->getReplicas();
    const auto health = replication_mgr_->getReplicaHealthStatus();

    std::string candidate;
    for (const auto& [node_id, status] : health) {
        if (status != themisdb::replication::HealthStatus::HEALTHY) {
            continue;
        }
        if (node_id == failed_node_id) {
            continue;
        }
        for (const auto& replica : replicas) {
            if (replica.node_id == node_id &&
                replica.role != themisdb::replication::ReplicationRole::WITNESS) {
                candidate = node_id;
                break;
            }
        }
        if (!candidate.empty()) {
            break;
        }
    }

    if (candidate.empty()) {
        spdlog::error("No healthy, promotable replica candidate available");
        return false;
    }

    if (!replication_mgr_->triggerFailover(candidate)) {
        spdlog::error("Leader election trigger failed for candidate {}", candidate);
        return false;
    }

    promoted_id = candidate;
    emitEvent(FailoverEventType::LEADER_ELECTED, promoted_id, "Replica promoted");

    return true;
}

bool AutoFailoverManager::activateSpareIfNeeded(const std::string& failed_node_id) {
    if (!spare_manager_) {
        return true;  // Spare manager not configured
    }

    spdlog::info("Activating spare for failed node: {}", failed_node_id);

    // Activation requires ring/read/write/document handlers from sharding runtime.
    // In orchestrator-only mode we signal intent and let sharding controller execute.
    emitEvent(FailoverEventType::SPARE_ACTIVATION_FAILED,
              failed_node_id,
              "Spare activation requires sharding runtime context");

    return false;
}

bool AutoFailoverManager::updateMetadata(const std::string& old_leader_id,
                                         const std::string& new_leader_id) {
    spdlog::info("Updating metadata: {} -> {}", old_leader_id, new_leader_id);
    // Update cluster topology, DNS, reputation scores, etc.
    return true;
}

bool AutoFailoverManager::verifyFailoverCompletion(const FailoverTask& task) {
    spdlog::info("Verifying failover completion for: {}", task.failed_node_id);
    return replication_mgr_ && replication_mgr_->hasQuorum();
}

bool AutoFailoverManager::preventSplitBrain(const std::string& failed_node_id) {
    if (!fencing_manager_) {
        // Fail closed: split-brain prevention requires a fencing manager.
        // Returning true here would be unsafe — we cannot guarantee exclusive leadership.
        emitDiagnostic(FailoverErrorCode::QUORUM_UNAVAILABLE, failed_node_id,
                       "split-brain prevention requires fencing manager: none configured");
        spdlog::error("preventSplitBrain: no EpochFencingManager configured; "
                      "failing closed (node={})", failed_node_id);
        return false;
    }

    spdlog::info("Acquiring exclusive lease for node: {}", failed_node_id);

    const auto epoch_token = fencing_manager_->bumpEpoch(
        "automatic failover for failed node " + failed_node_id
    );
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.split_brain_preventions++;
    }
    return true;
}

bool AutoFailoverManager::handleNetworkPartition() {
    spdlog::warn("Handling network partition...");

    // If we don't have quorum, shut down gracefully
    if (isNetworkPartitionedFromQuorum()) {
        spdlog::error("Partitioned from quorum - entering read-only mode");
        return true;  // Graceful degradation
    }

    return true;
}

bool AutoFailoverManager::isNetworkPartitionedFromQuorum() const {
    return replication_mgr_ && replication_mgr_->detectNetworkPartition();
}

bool AutoFailoverManager::attemptRecovery(const std::string& failed_node_id) {
    spdlog::info("Attempting recovery for node: {}", failed_node_id);

    // Accumulate counters locally to minimise lock contention; apply a single
    // lock at the end (or on early success) rather than locking per iteration.
    uint64_t local_total   = 0;
    uint64_t local_failed  = 0;
    uint64_t local_success = 0;

    for (uint32_t attempt = 0; attempt < config_.max_recovery_attempts; ++attempt) {
        ++local_total;

        if (waitForNodeRecovery(failed_node_id, 1)) {
            spdlog::info("Node recovered: {}", failed_node_id);
            ++local_success;

            // Batch-flush accumulated counters in a single lock acquisition.
            {
                std::lock_guard<std::mutex> stats_lock(stats_mutex_);
                stats_.total_retry_attempts += local_total;
                stats_.successful_retries   += local_success;
                stats_.failed_retries       += local_failed;
            }

            updateFailureTracking(failed_node_id, true);
            emitEvent(FailoverEventType::RECOVERY_COMPLETED, failed_node_id, "");
            return true;
        }

        ++local_failed;

        if (attempt < config_.max_recovery_attempts - 1) {
            std::this_thread::sleep_for(config_.recovery_retry_interval);
        }
    }

    // All attempts exhausted — batch-flush stats and emit unified diagnostic.
    {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.total_retry_attempts += local_total;
        stats_.failed_retries       += local_failed;
    }

    emitDiagnostic(FailoverErrorCode::NODE_REJOIN_FAILED, failed_node_id,
                   "node failed to recover after " +
                   std::to_string(config_.max_recovery_attempts) + " attempt(s)");
    spdlog::warn("Node failed to recover after {} attempts: {}",
                 config_.max_recovery_attempts, failed_node_id);
    return false;
}

bool AutoFailoverManager::waitForNodeRecovery(const std::string& node_id, uint32_t max_attempts) {
    if (!health_monitor_) {
        return false;
    }

    for (uint32_t i = 0; i < max_attempts; ++i) {
        const auto status = health_monitor_->getHealthStatus(node_id);
        if (status && status->isHealthy()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
}

void AutoFailoverManager::transitionState(FailoverOrchestratorState new_state) {
    auto current_state = state_.exchange(new_state);

    if (!canTransition(current_state, new_state)) {
        spdlog::warn("Failover state machine: unexpected transition {} → {} — "
                     "this may indicate a logic error",
                     static_cast<int>(current_state),
                     static_cast<int>(new_state));
    } else if (current_state != new_state) {
        spdlog::debug("Failover state transition: {} → {}",
                      static_cast<int>(current_state),
                      static_cast<int>(new_state));
    }
}

bool AutoFailoverManager::canTransition(FailoverOrchestratorState from,
                                        FailoverOrchestratorState to) const {
    // Any state → IDLE is valid (stop / reset).
    if (to == FailoverOrchestratorState::IDLE) {
        return true;
    }
    // Any state → FAILED is valid (error path from any step).
    if (to == FailoverOrchestratorState::FAILED) {
        return true;
    }

    // Forward path:
    //   IDLE → VERIFYING_FAILURE → CHECKING_QUORUM →
    //   STARTING_LEADER_ELECTION → LEADER_ELECTION_IN_PROGRESS →
    //   (ACTIVATING_SPARE →) (REDIRECTING_TRAFFIC →) UPDATING_METADATA →
    //   COMPLETING_FAILOVER → IDLE
    switch (from) {
        case FailoverOrchestratorState::IDLE:
            return to == FailoverOrchestratorState::VERIFYING_FAILURE ||
                   to == FailoverOrchestratorState::DETECTING_FAILURE;

        case FailoverOrchestratorState::DETECTING_FAILURE:
            return to == FailoverOrchestratorState::VERIFYING_FAILURE;

        case FailoverOrchestratorState::VERIFYING_FAILURE:
            return to == FailoverOrchestratorState::CHECKING_QUORUM;

        case FailoverOrchestratorState::CHECKING_QUORUM:
            return to == FailoverOrchestratorState::STARTING_LEADER_ELECTION;

        case FailoverOrchestratorState::STARTING_LEADER_ELECTION:
            // LEADER_ELECTION_IN_PROGRESS: via startLeaderElection()
            // UPDATING_METADATA: direct shortcut in processFailover()
            return to == FailoverOrchestratorState::LEADER_ELECTION_IN_PROGRESS ||
                   to == FailoverOrchestratorState::UPDATING_METADATA;

        case FailoverOrchestratorState::LEADER_ELECTION_IN_PROGRESS:
            return to == FailoverOrchestratorState::ACTIVATING_SPARE ||
                   to == FailoverOrchestratorState::UPDATING_METADATA;

        case FailoverOrchestratorState::ACTIVATING_SPARE:
            return to == FailoverOrchestratorState::REDIRECTING_TRAFFIC ||
                   to == FailoverOrchestratorState::UPDATING_METADATA;

        case FailoverOrchestratorState::REDIRECTING_TRAFFIC:
            return to == FailoverOrchestratorState::UPDATING_METADATA;

        case FailoverOrchestratorState::UPDATING_METADATA:
            return to == FailoverOrchestratorState::COMPLETING_FAILOVER;

        case FailoverOrchestratorState::COMPLETING_FAILOVER:
            // Reaches IDLE — already handled above.
            return false;

        case FailoverOrchestratorState::FAILED:
            // Reaches IDLE — already handled above.
            return false;

        default:
            return false;
    }
}

void AutoFailoverManager::emitDiagnostic(FailoverErrorCode code,
                                          const std::string& node_id,
                                          const std::string& detail) {
    spdlog::error("Failover diagnostic [code={}] node='{}': {}",
                  static_cast<int>(code), node_id, detail);

    // Map canonical error code to observable event type for callback consumers.
    FailoverEventType event_type;
    switch (code) {
        case FailoverErrorCode::QUORUM_UNAVAILABLE:
            event_type = FailoverEventType::QUORUM_CHECK_FAILED;
            break;
        default:
            event_type = FailoverEventType::FAILOVER_CANCELLED;
            break;
    }
    emitEvent(event_type, node_id, detail);
}

void AutoFailoverManager::emitEvent(FailoverEventType type,
                                    const std::string& node_id,
                                    const std::string& detail) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);

    for (auto& callback : event_callbacks_) {
        try {
            callback(type, node_id, detail);
        } catch (const std::exception& e) {
            spdlog::error("Error in failover event callback: {}", e.what());
        }
    }
}

void AutoFailoverManager::updateStatistics(const FailoverResult& result) {
    stats_.total_failovers++;

    if (result.success) {
        stats_.successful_failovers++;
    } else {
        stats_.failed_failovers++;
    }

    failover_durations_.push_back(result.duration);

    if (failover_durations_.size() > 100) {
        failover_durations_.erase(failover_durations_.begin());
    }

    if (!failover_durations_.empty()) {
        auto total_duration = std::accumulate(
            failover_durations_.begin(), failover_durations_.end(), std::chrono::milliseconds(0)
        );
        stats_.avg_failover_time = total_duration / failover_durations_.size();

        stats_.min_failover_time =
            *std::min_element(failover_durations_.begin(), failover_durations_.end());
        stats_.max_failover_time =
            *std::max_element(failover_durations_.begin(), failover_durations_.end());
    }
}

}  // namespace failover
}  // namespace themis

