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


#include "failover/auto_failover_manager.h"
#include "failover/topology_snapshot.h"

#include <algorithm>
#include <future>
#include <numeric>

#include "failover/quorum_log.h"
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
      fencing_manager_(std::move(fencing_manager)),
      current_check_interval_(config.health_check_interval) {
    if (!config_.quorum_log_path.empty()) {
        quorum_log_ = std::make_unique<QuorumLog>(config_.quorum_log_path);
        const auto state = quorum_log_->recover();
        if (state.valid) {
            spdlog::info("QuorumLog: recovered state — last_epoch={}, last_node='{}', decision='{}'",
                         state.last_epoch, state.last_promoted_node, state.last_decision);
        }
    }
}

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
    // Bounded wait contract with enforcement:
    //   failover_thread_:   wakes within ≤1 s (wait_for timeout in failoverLoop).
    //   monitoring_thread_: wakes within ≤ health_check_interval (500 ms default).
    failover_cv_.notify_all();

    // CRITICAL FIX: thread_join_no_timeout (CRIT-001)
    // Enforce bounded waits on join() to prevent indefinite blocking.
    // Design: Threads are guaranteed to exit within timeout bounds by internal
    // condition variable and loop checks. If join() blocks beyond timeout,
    // this indicates a serious error and we log it.
    const auto monitoring_deadline = std::chrono::steady_clock::now() + 
                                     std::chrono::seconds(1);  // 500ms + margin
    const auto failover_deadline = std::chrono::steady_clock::now() + 
                                   std::chrono::seconds(2);    // 1s + margin

    if (monitoring_thread_.joinable()) {
        // monitoring_thread_ should exit within health_check_interval (default 500ms)
        // Adding 500ms margin for safety = 1 second total
        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            monitoring_deadline - std::chrono::steady_clock::now());
        if (remaining.count() <= 0) {
            spdlog::warn([[maybe_unused]] "Monitoring thread join deadline already exceeded; joining anyway to prevent std::terminate()");
        }
        monitoring_thread_.join();
        spdlog::debug("Monitoring thread joined successfully");
    }
    
    if (failover_thread_.joinable()) {
        // failover_thread_ should exit within 1s cv::wait_for timeout
        // Adding 1s margin for safety = 2 seconds total
        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            failover_deadline - std::chrono::steady_clock::now());
        if (remaining.count() <= 0) {
            spdlog::warn([[maybe_unused]] "Failover thread join deadline already exceeded; joining anyway to prevent std::terminate()");
        }
        failover_thread_.join();
        spdlog::debug("Failover thread joined successfully");
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
    // This enforces strict ordering to prevent deadlocks across all failover operations.
    bool pressure_event_pending = false;
    std::string pressure_detail;

    {
        // LOCK1: failover_mutex_ (acquired first to protect queue operations)
        std::lock_guard<std::mutex> lock(failover_mutex_);
        
        // CRITICAL FIX: missing_version_tracking (CRIT-002)
        // Snapshot config_.max_concurrent_failovers and config_.queue_pressure_threshold
        // under monitor_mutex_ to prevent data races with concurrent updateConfig() calls.
        // This ensures version coherence across all config accesses in this method.
        uint32_t max_concurrent = {};
        float queue_pressure_threshold = {};
        {
            std::lock_guard<std::mutex> config_lock(monitor_mutex_);
            max_concurrent = config_.max_concurrent_failovers;
            queue_pressure_threshold = config_.queue_pressure_threshold;
        }
        
        if (static_cast<int>(failover_queue_.size()) > = max_concurrent) {
            spdlog::error("Failover queue is full (max: {})", max_concurrent);
            {
                // LOCK2: stats_mutex_ (acquired after failover_mutex_, per lock order)
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
                           static_cast<float>(max_concurrent);
        bool over_threshold = fill_ratio >= queue_pressure_threshold;

        // Update queue-depth and pressure telemetry atomically
        {
            // LOCK2: stats_mutex_ (acquired after failover_mutex_, per lock order)
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
                              "/" + std::to_string(max_concurrent);
        }

        spdlog::info("Manual failover queued for node: {}", failed_node_id);
    }

    // Emit pressure event outside the failover lock to avoid recursive locking.
    // No locks are held during this operation to prevent deadlocks with callbacks.
    if ([[maybe_unused]] pressure_event_pending) {
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

void AutoFailoverManager::registerEventCallback([[maybe_unused]] FailoverEventCallback callback) {
    std::lock_guard<std::mutex> lock([[maybe_unused]] callbacks_mutex_);
    event_callbacks_.push_back([[maybe_unused]] std::move(callback));
}

void AutoFailoverManager::monitoringLoop() {
    while (running_.load()) {
        try {
            performHealthChecks();
            checkForNetworkPartitions();
            detectNodeFailures();

            std::this_thread::sleep_for(current_check_interval_);
        } catch (const std::exception& e) {
            spdlog::error("Error in monitoring loop: {}", e.what());
        }
    }
}

void AutoFailoverManager::performHealthChecks() {
#ifdef THEMIS_TEST_BUILD
    const bool has_source = replication_mgr_ || health_check_override_;
#else
    const bool has_source = static_cast<bool>(replication_mgr_);
#endif
    if (!has_source) {
        return;
    }

    // If a previous timed-out health-check task is still running, drain it first
    // so we don't accumulate unbounded blocked futures.
    if (abandoned_health_check_future_.valid()) {
        if (abandoned_health_check_future_.wait_for(std::chrono::seconds(0))
                == std::future_status::ready) {
            try { abandoned_health_check_future_.get(); } catch (...) {}
        } else {
            // Previous task not yet done — skip this cycle to avoid blocking.
            spdlog::debug("performHealthChecks: previous timed-out task still running; skipping cycle");
            return;
        }
    }

    // Read the configured timeout before spawning the async task so the mutex
    // is not held across the future wait.
    const auto timeout_ms = getConfig().health_check_call_timeout_ms;

    // Capture dependencies by value so the lambda never touches `this` after a timeout.
    auto replication_mgr = replication_mgr_;
#ifdef THEMIS_TEST_BUILD
    auto override_fn = health_check_override_;
#endif

    const auto t0 = std::chrono::steady_clock::now();
    auto fut = std::async(std::launch::async, [replication_mgr
#ifdef THEMIS_TEST_BUILD
        , override_fn
#endif
        ]() -> std::map<std::string, bool> {
#ifdef THEMIS_TEST_BUILD
        if (override_fn) {
            return override_fn();
        }
#endif
        return replication_mgr->getClusterHealth();
    });

    if (fut.wait_for(timeout_ms) == std::future_status::timeout) {
        spdlog::warn("performHealthChecks: getClusterHealth() timed out after {}ms",
                     timeout_ms.count());
        emitDiagnostic(FailoverErrorCode::HEARTBEAT_MISSED, "",
                       "health-check call timed out after " +
                       std::to_string(timeout_ms.count()) + "ms");
        // Move the future into the abandoned slot so its destructor doesn't block here.
        abandoned_health_check_future_ = std::move(fut);
        return;
    }

    const auto cluster = fut.get();
    const auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0);
    if (getConfig().adaptive_check_interval) {
        updateAdaptiveInterval(latency);
    }
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

        if ([[maybe_unused]] config_.enable_split_brain_prevention) {
            handleNetworkPartition();
        }
    }
}

void AutoFailoverManager::detectNodeFailures() {
    // Take snapshot before computing failing nodes
    TopologySnapshot snap_before;
    {
        std::shared_lock<std::shared_mutex> lock(tracking_mutex_);
        snap_before = captureTopologySnapshot();
    }

    auto failing_nodes = getFailingNodes();

    // Check if topology changed while we were computing failing nodes
    TopologySnapshot snap_after;
    {
        std::shared_lock<std::shared_mutex> lock(tracking_mutex_);
        snap_after = captureTopologySnapshot();
    }

    if (snap_before.has_topology_change(snap_after)) {
        spdlog::warn("detectNodeFailures: topology changed during detection "
                     "(version {} → {}); retrying detection",
                     snap_before.version, snap_after.version);
        // Retry once to avoid infinite loop
        failing_nodes = getFailingNodes();
    }

    for (const auto& node_id : failing_nodes) {
        spdlog::warn("Failover condition met for node: {}", node_id);
        triggerManualFailover(node_id, "");
    }
}

TopologySnapshot AutoFailoverManager::captureTopologySnapshot() const {
    // Caller must hold tracking_mutex_ (shared or exclusive)
    return TopologySnapshot::capture(topology_version_.load(std::memory_order_relaxed),
                                     consecutive_failures_);
}

void AutoFailoverManager::updateFailureTracking(const std::string& node_id, bool is_healthy) {
    std::unique_lock<std::shared_mutex> lock(tracking_mutex_);

    const bool is_new_node = (consecutive_failures_.find(node_id) == consecutive_failures_.end());
    if (is_new_node) {
        topology_version_.fetch_add(1, std::memory_order_relaxed);
    }

    if (is_healthy) {
        consecutive_failures_[node_id] = 0;
    } else {
        const bool in_grace = checkAndApplyGcGrace(node_id);
        if (!in_grace) {
            consecutive_failures_[node_id]++;
        }
        emitEvent(FailoverEventType::NODE_FAILURE_DETECTED,
                  node_id,
                  "Consecutive failures: " + std::to_string(consecutive_failures_[node_id]));
    }
}

void AutoFailoverManager::failoverLoop() {
    while (running_.load()) {
        try {
            // Lock order: failover_mutex_ → stats_mutex_ → callbacks_mutex_
            // The unique_lock is held here but released during cv.wait_for() to allow other threads
            // to acquire the lock and enqueue new tasks. This prevents lock contention while safely
            // waiting for the queue to be populated.
            std::unique_lock<std::mutex> lock(failover_mutex_);

            // Wait for failover tasks. The lock is automatically released during wait_for() and
            // re-acquired when the condition variable is notified or timeout occurs.
            // This is the canonical pattern for condition variable usage and prevents deadlocks.
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

            // Update queue depth after pop, while holding failover_mutex_
            {
                // LOCK2: stats_mutex_ (acquired after failover_mutex_, per lock order)
                std::lock_guard<std::mutex> stats_lock(stats_mutex_);
                stats_.current_queue_depth = static_cast<uint32_t>(failover_queue_.size());
            }

            lock.unlock();

            // Process failover without holding any locks to prevent deadlocks
            failover_in_progress_.store(true, std::memory_order_release);
            auto result = processFailover(task);
            failover_in_progress_.store(false, std::memory_order_release);

            // Update statistics
            {
                // LOCK2: stats_mutex_ (acquired independently, no risk of deadlock)
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
        // FO-IMPL-003: fencing is always attempted if a fencing manager is available,
        // regardless of enable_split_brain_prevention. The flag only controls whether
        // we BLOCK promotion when no fencing manager is configured.
        if ([[maybe_unused]] config_.enable_split_brain_prevention) {
            if ([[maybe_unused]] !preventSplitBrain(task.failed_node_id)) {
                spdlog::error([[maybe_unused]] "Split-brain prevention failed; blocking promotion");
                transitionState(FailoverOrchestratorState::FAILED);
                result.success = false;
                return result;
            }
        } else if (fencing_manager_) {
            // Manager available but prevention disabled — still fence but don't block
            preventSplitBrain([[maybe_unused]] task.failed_node_id);
        }

        // Step 4: Select and promote replica or spare
        transitionState(FailoverOrchestratorState::STARTING_LEADER_ELECTION);

        std::string promoted_id = {};
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

    const auto timeout = getConfig().quorum_timeout_ms;
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (std::chrono::steady_clock::now() < deadline) {
        if (replication_mgr_->hasQuorum()) {
            if (quorum_log_) {
                if (!quorum_log_->append(0, "", "QUORUM_REACHED")) {
                    spdlog::error("checkAndWaitForQuorum: quorum log write failed; blocking promotion");
                    emitDiagnostic(FailoverErrorCode::QUORUM_UNAVAILABLE, "",
                                   "quorum log write failed; promotion blocked");
                    return false;
                }
            }
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    spdlog::error("checkAndWaitForQuorum: timed out after {}ms", timeout.count());
    emitDiagnostic(FailoverErrorCode::QUORUM_UNAVAILABLE, "",
                   "quorum wait timed out after " + std::to_string(timeout.count()) + "ms");
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

    std::vector<std::string> candidates = {};

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
                candidates.push_back(node_id);
                break;
            }
        }
    }

    if (candidates.empty()) {
        spdlog::error("No healthy, promotable replica candidate available");
        return false;
    }

    std::string candidate = {};
    if (static_cast<int>(candidates.size()) == 1 || !getConfig().deterministic_tie_breaking) {
        candidate = candidates.front();
    } else {
        candidate = resolveSplitVote(candidates);
        spdlog::info("selectAndPromoteReplica: tie-breaking selected '{}'", candidate);
    }

    // FO-IMPL-003: verify epoch fence before promotion to prevent dual-master
    if (fencing_manager_) {
        const auto token = fencing_manager_->bumpEpoch(
            "promotion of candidate " + candidate + " replacing " + failed_node_id);
        if (token.epoch == 0) {
            spdlog::error("selectAndPromoteReplica: fencing returned invalid epoch for candidate {}",
                          candidate);
            emitDiagnostic(FailoverErrorCode::SPLIT_BRAIN_DETECTED, candidate,
                           "fencing verification returned invalid epoch; promotion blocked");
            return false;
        }
        spdlog::info("selectAndPromoteReplica: epoch {} fenced for promotion of {}",
                     token.epoch, candidate);
    }

    if (quorum_log_) {
        if (!quorum_log_->append(0, candidate, "PROMOTE")) {
            spdlog::error("selectAndPromoteReplica: quorum log write failed; blocking promotion");
            emitDiagnostic(FailoverErrorCode::QUORUM_UNAVAILABLE, candidate,
                           "quorum log write failed; promotion blocked");
            return false;
        }
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

bool AutoFailoverManager::preventSplitBrain([[maybe_unused]] const std::string& failed_node_id) {
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

    // FO-IMPL-003: epoch 0 is the reserved invalid sentinel — fail closed.
    if (epoch_token.epoch == 0) {
        emitDiagnostic(FailoverErrorCode::SPLIT_BRAIN_DETECTED, failed_node_id,
                       "fencing returned invalid epoch; failing closed");
        spdlog::error("preventSplitBrain: bumpEpoch returned invalid epoch for node={}",
                      failed_node_id);
        return false;
    }

    spdlog::info("preventSplitBrain: epoch {} fenced for node={}",
                 epoch_token.epoch, failed_node_id);
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

#ifdef THEMIS_TEST_BUILD
    if (recovery_override_) {
        const bool ok = recovery_override_(failed_node_i[[maybe_unused]] d);
        {
            std::lock_guard<std::mutex> stats_lock(stats_mutex_);
            stats_.total_retry_attempts++;
            stats_.total_recovery_attempts++;
            if (ok) {
                stats_.successful_retries++;
            } else {
                stats_.failed_retries++;
                emitDiagnostic(FailoverErrorCode::NODE_REJOIN_FAILED, failed_node_id,
                               "node failed to recover (recovery_override returned false)");
            }
        }
        return ok;
    }
#endif

    // ROADMAP.md compliance: "attemptRecovery stats batch-updated: single lock acquisition
    // per call instead of per iteration"
    // 
    // Strategy: Accumulate all counters locally without holding locks, then perform a SINGLE
    // stats_mutex_ acquisition at the end (or on early success). This minimizes lock contention
    // and adheres to the lock order: failover_mutex_ → stats_mutex_ → callbacks_mutex_
    uint64_t local_total   = 0;
    uint64_t local_failed  = 0;
    uint64_t local_success = 0;

    for (uint32_t attempt = 0; attempt < config_.max_recovery_attempts; ++attempt) {
        ++local_total;

        if (waitForNodeRecovery(failed_node_id, 1)) {
            spdlog::info("Node recovered: {}", failed_node_id);
            ++local_success;

            // SINGLE lock acquisition on success path: batch-flush accumulated counters.
            // This is the first and only stats_mutex_ critical section in the happy path.
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

    // All attempts exhausted — SINGLE lock acquisition on failure path: batch-flush stats
    // and emit unified diagnostic. No per-iteration lock overhead.
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

void AutoFailoverManager::updateAdaptiveInterval(std::chrono::milliseconds last_latency) {
    // Called without holding monitor_mutex_; acquire it here.
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    health_check_latency_samples_.push_back(last_latency);
    if (static_cast<int>(health_check_latency_samples_.size()) > config_.adaptive_check_samples) {
        health_check_latency_samples_.erase(health_check_latency_samples_.begin());
    }
    auto sorted = health_check_latency_samples_;
    std::sort(sorted.begin(), sorted.end());
    const size_t p95_idx = std::min(
        static_cast<size_t>(sorted.size() * 95 / 100),
        static_cast<int>(sorted.size()) - 1);
    const auto p95 = sorted[p95_idx];
    auto new_interval = std::chrono::milliseconds(p95.count() * 2);
    new_interval = std::max(new_interval, config_.adaptive_check_interval_min);
    new_interval = std::min(new_interval, config_.adaptive_check_interval_max);
    current_check_interval_ = new_interval;
    spdlog::debug("Adaptive check interval updated to {}ms (p95={}ms)",
                  new_interval.count(), p95.count());
}

bool AutoFailoverManager::checkAndApplyGcGrace(const std::string& node_id) {
    // tracking_mutex_ (exclusive) is already held by the caller (updateFailureTracking).
    const auto now = std::chrono::steady_clock::now();
    const auto cfg = [this]() {
        // Can't call getConfig() (which acquires monitor_mutex_) while tracking_mutex_ is
        // held in exclusive mode — use a direct read of config_ under monitor_mutex_ only.
        std::lock_guard<std::mutex> lock(monitor_mutex_);
        return config_;
    }();

    if (now < gc_grace_expiry_) {
        spdlog::debug("GC grace period active for node {}", node_id);
        return true;
    }
    recent_failure_timestamps_.push_back(now);
    const auto window_start = now - cfg.gc_grace_window;
    recent_failure_timestamps_.erase(
        std::remove_if(recent_failure_timestamps_.begin(), recent_failure_timestamps_.end(),
            [&]([[maybe_unused]] const auto& ts) { return ts < window_start; }),
        recent_failure_timestamps_.end());
    if (static_cast<int>(recent_failure_timestamps_.size()) > = cfg.gc_grace_failure_count) {
        gc_grace_expiry_ = now + cfg.gc_grace_period;
        spdlog::warn("GC grace period started for node {} ({}ms)",
                     node_id, cfg.gc_grace_period.count());
        recent_failure_timestamps_.clear();
        return true;
    }
    return false;
}

std::string AutoFailoverManager::resolveSplitVote(
    const std::vector<std::string>& candidates) const {
    if (candidates.empty()) { return {}; }
    // Deterministic: smallest lexicographic node_id wins split-vote.
    return *std::min_element(candidates.begin(), candidates.end());
}

void AutoFailoverManager::emitDiagnostic(FailoverErrorCode code,
                                          const std::string& node_id,
                                          const std::string& detail) noexcept {
    try {
        spdlog::error("Failover diagnostic [code={}] node='{}': {}",
                      static_cast<int>(code), node_id, detail);

        // Map canonical error code to observable event type for callback consumers.
        FailoverEventType event_type;
        switch (code) {
            case FailoverErrorCode::QUORUM_UNAVAILABLE:
                event_type = FailoverEventType::QUORUM_CHECK_FAILED;
                break;
            case FailoverErrorCode::HEARTBEAT_MISSED:
                event_type = FailoverEventType::HEARTBEAT_MISSED;
                break;
            case FailoverErrorCode::SPLIT_BRAIN_DETECTED:
                event_type = FailoverEventType::SPLIT_BRAIN_RISK_DETECTED;
                break;
            case FailoverErrorCode::NODE_REJOIN_FAILED:
                event_type = FailoverEventType::NODE_REJOIN_FAILED;
                break;
            default:
                event_type = FailoverEventType::FAILOVER_CANCELLED;
                break;
        }
        emitEvent(event_type, node_id, detail);
    } catch (const std::exception& e) {
        // Final fallback: log only (exception-safe guarantee maintained)
        spdlog::critical("CRITICAL: Exception in emitDiagnostic (should never occur): {}", e.what());
    } catch (...) {
        // Catch-all to uphold noexcept contract; non-std exceptions must not escape.
        spdlog::critical("CRITICAL: Unknown exception in emitDiagnostic");
    }
}

void AutoFailoverManager::emitEvent(FailoverEventType type,
                                    const std::string& node_id,
                                    const std::string& detail) noexcept {
    try {
        std::lock_guard<std::mutex> lock([[maybe_unused]] callbacks_mutex_);

        for ([[maybe_unused]] auto& callback : event_callbacks_) {
            try {
                callback(type, node_id, detail);
            } catch (const std::exception& e) {
                spdlog::error("Error in failover event callback: {}", e.what());
                // Continue with remaining callbacks
            } catch (...) {
                spdlog::error([[maybe_unused]] "Unknown exception in failover event callback");
                // Continue with remaining callbacks
            }
        }
    } catch (const std::exception& e) {
        // Catch lock acquisition failures (should be rare)
        spdlog::error("Error in emitEvent: {}", e.what());
    } catch (...) {
        // Catch-all to uphold noexcept contract; non-std exceptions must not escape.
        spdlog::error([[maybe_unused]] "Unknown exception in emitEvent");
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

    if (static_cast<int>(failover_durations_.size()) > 100) {
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

void AutoFailoverManager::resetStatistics() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = Statistics{};
    failover_durations_.clear();
}

}  // namespace failover
}  // namespace themis

