/**
 * @file distributed_coordinator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/distributed_coordinator.h"
#include "utils/logger.h"
#include "utils/thread_join_utils.h"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

namespace themis::sharding {

// CoordinatorTask JSON serialization
/** @brief Serialize coordinator task into JSON payload. */
nlohmann::json DistributedCoordinator::CoordinatorTask::toJson() const {
    nlohmann::json j;
    j["task_id"] = task_id;
    j["type"] = static_cast<uint8_t>(type);
    j["payload"] = payload;
    j["ttl_seconds"] = ttl.count();
    j["created_at"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        created_at.time_since_epoch()).count();
    j["started_at"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        started_at.time_since_epoch()).count();
    j["assigned_leader"] = assigned_leader;
    return j;
}

/** @brief Deserialize coordinator task from JSON payload. */
DistributedCoordinator::CoordinatorTask DistributedCoordinator::CoordinatorTask::fromJson(
    const nlohmann::json& j) {
    CoordinatorTask task;
    task.task_id = j.value("task_id", "");
    task.type = static_cast<TaskType>(j.value("type", 0));
    task.payload = j.value("payload", nlohmann::json::object());
    task.ttl = std::chrono::seconds(j.value("ttl_seconds", 600));
    
    auto created_ms = j.value("created_at", 0LL);
    task.created_at = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(created_ms));
    
    auto started_ms = j.value("started_at", 0LL);
    task.started_at = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(started_ms));
    
    task.assigned_leader = j.value("assigned_leader", "");
    return task;
}

// LeaderInfo JSON serialization
/** @brief Serialize leader info into JSON payload. */
nlohmann::json DistributedCoordinator::LeaderInfo::toJson() const {
    nlohmann::json j;
    j["shard_id"] = shard_id;
    j["role"] = static_cast<uint8_t>(role);
    j["term"] = term;
    j["lease_expires_at"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        lease_expires_at.time_since_epoch()).count();
    j["last_heartbeat"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        last_heartbeat.time_since_epoch()).count();
    return j;
}

// Constructor
/** @brief Construct coordinator with explicit runtime configuration. */
DistributedCoordinator::DistributedCoordinator(
    const std::string& local_shard_id,
    std::shared_ptr<ShardTopology> topology,
    std::shared_ptr<GossipConfigManager> gossip_mgr,
    const Config& config)
    : local_shard_id_(local_shard_id),
      topology_(topology),
      gossip_mgr_(gossip_mgr),
      config_(config) {
    
    THEMIS_INFO("DistributedCoordinator initialized for shard: {}", local_shard_id_);
}

/** @brief Construct coordinator using default config values. */
DistributedCoordinator::DistributedCoordinator(
    const std::string& local_shard_id,
    std::shared_ptr<ShardTopology> topology,
    std::shared_ptr<GossipConfigManager> gossip_mgr)
    : DistributedCoordinator(local_shard_id, topology, gossip_mgr, Config{})
{
}

// Destructor
/** @brief Stop coordinator worker threads during destruction. */
DistributedCoordinator::~DistributedCoordinator() {
    stop();
}

// Lifecycle methods
/** @brief Start election, heartbeat, and task execution worker threads. */
void DistributedCoordinator::start() {
    // Use acquire semantics to ensure all thread-local initialization is visible to worker threads.
    if (running_.exchange(true, std::memory_order_release)) {
        THEMIS_WARN("DistributedCoordinator already running");
        return;
    }
    
    // Start election monitoring
    election_thread_ = std::thread([this]() {
        while (running_.load(std::memory_order_acquire)) {
            electionLoop();
            std::this_thread::sleep_for(
                std::chrono::milliseconds(config_.election_timeout_ms)
            );
        }
    });
    
    // Start heartbeat thread (only active if leader)
    heartbeat_thread_ = std::thread([this]() {
        while (running_.load(std::memory_order_acquire)) {
            if (isLeader()) {
                sendHeartbeat();
            }
            std::this_thread::sleep_for(
                std::chrono::milliseconds(config_.heartbeat_interval_ms)
            );
        }
    });
    
    // Start task executor (only active if leader)
    task_executor_thread_ = std::thread([this]() {
        while (running_.load(std::memory_order_acquire)) {
            if (isLeader()) {
                taskExecutorLoop();
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    
    THEMIS_INFO("DistributedCoordinator started");
}

/** @brief Stop coordinator worker threads and wait for shutdown. */
void DistributedCoordinator::stop() {
    // Use release semantics to ensure all pending operations are visible to threads before exit.
    if (!running_.exchange(false, std::memory_order_release)) {
        return;
    }
    
    THEMIS_INFO("Stopping DistributedCoordinator");
    
    // thread_join_no_timeout (W4): bounded join via joinThreadWithin
    if (!themis::utils::joinThreadWithin(election_thread_)) {
        THEMIS_WARN("[DistributedCoordinator] election thread did not finish within shutdown deadline; detaching.");
    }
    if (!themis::utils::joinThreadWithin(heartbeat_thread_)) {
        THEMIS_WARN("[DistributedCoordinator] heartbeat thread did not finish within shutdown deadline; detaching.");
    }
    if (!themis::utils::joinThreadWithin(task_executor_thread_)) {
        THEMIS_WARN("[DistributedCoordinator] task executor thread did not finish within shutdown deadline; detaching.");
    }
    
    THEMIS_INFO("DistributedCoordinator stopped");
}

// Role management
/** @brief Return currently known leader shard identifier when available. */
std::optional<std::string> DistributedCoordinator::getCurrentLeader() const {
    std::shared_lock<std::shared_mutex> lock(leader_mutex_);
    return current_leader_;
}

// Leader election
/** @brief Start local election round and decide winner using simplified policy. */
void DistributedCoordinator::startElection() {
    uint32_t election_term = 0;
    
    {
        std::lock_guard<std::shared_mutex> lock(leader_mutex_);
        
        // Memory order: acquire before modifying term to ensure previous leader lease is visible.
        if (role_.load(std::memory_order_acquire) == CoordinatorRole::LEADER) {
            THEMIS_WARN("Already leader, skipping election");
            return;
        }
        
        // Transition to CANDIDATE and increment term with release semantics for visibility.
        role_.store(CoordinatorRole::CANDIDATE, std::memory_order_release);
        election_term = ++current_term_;  // Atomic increment; capture for logging
    }
    
    stats_.elections_started++;
    
    THEMIS_INFO("Starting leader election (term: {})", election_term);
    
    // Simplified election: broadcast candidacy via gossip
    // NOTE: This is called outside the lock to avoid deadlock risk with gossip manager
    try {
        requestVotes();
    } catch (const std::exception& e) {
        THEMIS_WARN("Election vote request failed: {}", e.what());
    }
    
    // Wait for election timeout
    // Note: This blocks the calling thread by design for simplicity.
    // Production implementations should make this asynchronous or allow interruption.
    std::this_thread::sleep_for(
        std::chrono::milliseconds(config_.election_timeout_ms)
    );
    
    // Check if we won (simplified: highest shard_id wins)
    // In production: use Raft-style voting
    try {
        auto all_shards = topology_->getAllShards();
        bool won = true;
        for (const auto& shard : all_shards) {
            if (shard.shard_id > local_shard_id_ && shard.is_healthy) {
                won = false;
                break;
            }
        }
        
        if (won) {
            becomeLeader();
        } else {
            {
                std::lock_guard<std::shared_mutex> lock(leader_mutex_);
                // Memory order: release to ensure role change is visible to all threads
                role_.store(CoordinatorRole::FOLLOWER, std::memory_order_release);
            }
            stats_.elections_lost++;
            THEMIS_INFO("Lost election in term {}", election_term);
        }
    } catch (const std::exception& e) {
        THEMIS_WARN("Election decision failed: {}", e.what());
        {
            std::lock_guard<std::shared_mutex> lock(leader_mutex_);
            // Memory order: release to ensure role change is visible to all threads
            role_.store(CoordinatorRole::FOLLOWER, std::memory_order_release);
        }
    }
}

/** @brief Promote local node to leader and initialize lease state. */
void DistributedCoordinator::becomeLeader() {
    // Capture callback before taking lock to avoid deadlock
    LeaderElectedCallback callback;
    {
        std::lock_guard<std::mutex> cb_lock([[maybe_unused]] callback_mutex_);
        callback = leader_elected_callback_;
    }
    
    {
        std::lock_guard<std::shared_mutex> lock(leader_mutex_);
        
        // Memory order: release to ensure leader promotion is visible to all threads
        role_.store(CoordinatorRole::LEADER, std::memory_order_release);
        current_leader_ = local_shard_id_;
        leader_lease_expires_ = std::chrono::system_clock::now() + 
                                std::chrono::seconds(config_.leader_lease_seconds);
        last_leader_heartbeat_ = std::chrono::system_clock::now();
        
        stats_.elections_won++;
        
        // Memory order: acquire to read current term safely
        THEMIS_INFO("Became leader for term {}", current_term_.load(std::memory_order_acquire));
    }
    
    // Trigger callback outside of locks to avoid deadlock
    if ([[maybe_unused]] callback) {
        callback([[maybe_unused]] local_shard_id_);
    }
}

/** @brief Step down from leader role to follower state. */
void DistributedCoordinator::stepDown() {
    std::lock_guard<std::shared_mutex> lock(leader_mutex_);
    
    // Memory order: acquire before checking role, release when updating
    if (role_.load(std::memory_order_acquire) == CoordinatorRole::LEADER) {
        THEMIS_INFO("Stepping down from leadership");
        // Memory order: release to ensure role change is visible
        role_.store(CoordinatorRole::FOLLOWER, std::memory_order_release);
        current_leader_.reset();
    }
}

// Task coordination
/** @brief Schedule a new coordinator task and broadcast to peers. */
std::string DistributedCoordinator::scheduleTask(const CoordinatorTask& task) {
    if (!isLeader()) {
        throw std::runtime_error("Only leader can schedule tasks");
    }
    
    std::lock_guard<std::shared_mutex> lock(tasks_mutex_);
    
    pending_tasks_.push_back(task);
    stats_.tasks_coordinated++;
    
    // Broadcast task via gossip
    broadcastTask(task);
    
    THEMIS_INFO("Scheduled task: {} (type: {})", task.task_id, static_cast<int>(task.type));
    
    return task.task_id;
}

/** @brief Cancel pending task by identifier when local node is leader. */
bool DistributedCoordinator::cancelTask(const std::string& task_id) {
    if (!isLeader()) {
        THEMIS_WARN("Only leader can cancel tasks");
        return false;
    }
    
    std::lock_guard<std::shared_mutex> lock(tasks_mutex_);
    
    auto it = std::find_if(pending_tasks_.begin(), pending_tasks_.end(),
        [&task_id](const CoordinatorTask& task) {
            return task.task_id == task_id;
        });
    
    if (it != pending_tasks_.end()) {
        pending_tasks_.erase(it);
        THEMIS_INFO("Cancelled task: {}", task_id);
        return true;
    }
    
    return false;
}

/** @brief Return snapshot of pending coordinator tasks. */
std::vector<DistributedCoordinator::CoordinatorTask> 
DistributedCoordinator::getPendingTasks() const {
    std::shared_lock<std::shared_mutex> lock(tasks_mutex_);
    return pending_tasks_;
}

// Task execution callback
/** @brief Register callback used to execute coordinator tasks. */
void DistributedCoordinator::setTaskExecutor(TaskExecutor executor) {
    std::lock_guard<std::mutex> lock([[maybe_unused]] callback_mutex_);
    task_executor_ = executor;
}

// Leader info
/** @brief Build current leader information snapshot. */
DistributedCoordinator::LeaderInfo DistributedCoordinator::getLeaderInfo() const {
    std::shared_lock<std::shared_mutex> lock(leader_mutex_);
    
    LeaderInfo info;
    info.shard_id = current_leader_.value_or("");
    // Memory order: acquire to ensure role is read consistently
    info.role = role_.load(std::memory_order_acquire);
    info.lease_expires_at = leader_lease_expires_;
    info.last_heartbeat = last_leader_heartbeat_;
    // Memory order: acquire to ensure term is read consistently
    info.term = current_term_.load(std::memory_order_acquire);
    
    return info;
}

// Callbacks
/** @brief Register callback notified when leadership changes. */
void DistributedCoordinator::setLeaderElectedCallback([[maybe_unused]] LeaderElectedCallback callback) {
    std::lock_guard<std::mutex> lock([[maybe_unused]] callback_mutex_);
    leader_elected_callback_ = callback;
}

// Statistics
/** @brief Return copy of coordinator runtime statistics. */
DistributedCoordinator::Statistics DistributedCoordinator::getStatistics() const {
    // Return a copy of statistics with acquire semantics for consistency
    Statistics stats;
    stats.elections_started.store(stats_.elections_started.load(std::memory_order_acquire), std::memory_order_relaxed);
    stats.elections_won.store(stats_.elections_won.load(std::memory_order_acquire), std::memory_order_relaxed);
    stats.elections_lost.store(stats_.elections_lost.load(std::memory_order_acquire), std::memory_order_relaxed);
    stats.leader_failures_detected.store(stats_.leader_failures_detected.load(std::memory_order_acquire), std::memory_order_relaxed);
    stats.tasks_coordinated.store(stats_.tasks_coordinated.load(std::memory_order_acquire), std::memory_order_relaxed);
    stats.avg_lease_duration_seconds.store(stats_.avg_lease_duration_seconds.load(std::memory_order_acquire), std::memory_order_relaxed);
    return stats;
}

/** @brief Return coordinator runtime statistics as JSON payload. */
nlohmann::json DistributedCoordinator::getStatisticsJson() const {
    nlohmann::json j;
    j["elections_started"] = stats_.elections_started.load(std::memory_order_acquire);
    j["elections_won"] = stats_.elections_won.load(std::memory_order_acquire);
    j["elections_lost"] = stats_.elections_lost.load(std::memory_order_acquire);
    j["leader_failures_detected"] = stats_.leader_failures_detected.load(std::memory_order_acquire);
    j["tasks_coordinated"] = stats_.tasks_coordinated.load(std::memory_order_acquire);
    j["avg_lease_duration_seconds"] = stats_.avg_lease_duration_seconds.load(std::memory_order_acquire);
    return j;
}

// Private methods - Election logic
/** @brief Periodic election loop: detect failures and trigger elections. */
void DistributedCoordinator::electionLoop() {
    // Check if there's a leader
    detectLeaderFailure();
    
    // If no leader and automatic failover is enabled, start election
    if (!current_leader_.has_value() && config_.enable_automatic_failover) {
        THEMIS_INFO("No leader detected, starting election");
        startElection();
    }
}

/** @brief Heartbeat loop placeholder handled by dedicated worker lambda. */
void DistributedCoordinator::heartbeatLoop() {
    // Handled in the main heartbeat thread
}

/** @brief Execute pending tasks via registered executor callback. */
void DistributedCoordinator::taskExecutorLoop() {
    std::vector<CoordinatorTask> tasks_to_execute;
    
    {
        std::shared_lock<std::shared_mutex> lock(tasks_mutex_);
        tasks_to_execute = pending_tasks_;
    }
    
    // Get executor outside of any locks to avoid deadlock
    TaskExecutor executor;
    {
        std::lock_guard<std::mutex> cb_lock([[maybe_unused]] callback_mutex_);
        executor = task_executor_;
    }
    
    // Execute tasks without holding any locks
    for (auto& task : tasks_to_execute) {
        if (executor) {
            try {
                THEMIS_DEBUG("Executing task: {} (type: {})", task.task_id, static_cast<int>(task.type));
                bool success = executor(task);
                if (success) {
                    // Remove task from pending
                    THEMIS_INFO("Task completed successfully: {}", task.task_id);
                    cancelTask(task.task_id);
                } else {
                    THEMIS_WARN("Task execution returned false: {} (type: {})", 
                                task.task_id, static_cast<int>(task.type));
                }
            } catch (const std::bad_alloc& e) {
                // Out-of-memory: log with severity; don't retry
                THEMIS_ERROR("Task execution OOM error: {}; task={}", e.what(), task.task_id);
            } catch (const std::exception& e) {
                // Other exceptions: log with full context
                THEMIS_WARN("Task execution exception: {}; task_id={}; task_type={}; what={}", 
                            typeid(e).name(), task.task_id, static_cast<int>(task.type), e.what());
            } catch (...) {
                // Unknown exception: log without attempting to extract details
                THEMIS_ERROR("Task execution unknown exception; task_id={}", task.task_id);
            }
        }
    }
}

// Leader detection
/** @brief Detect leader failure when lease expiration is observed. */
void DistributedCoordinator::detectLeaderFailure() {
    std::lock_guard<std::shared_mutex> lock(leader_mutex_);
    
    if (!current_leader_.has_value()) {
        return;
    }
    
    // Check if leader lease has expired
    auto now = std::chrono::system_clock::now();
    if (now > leader_lease_expires_) {
        THEMIS_WARN("Leader lease expired for shard: {}", current_leader_.value());
        stats_.leader_failures_detected++;
        
        // Clear current leader
        current_leader_.reset();
    }
}

/** @brief Return whether leader heartbeat freshness indicates healthy leader. */
bool DistributedCoordinator::isLeaderHealthy() const {
    std::shared_lock<std::shared_mutex> lock(leader_mutex_);
    
    if (!current_leader_.has_value()) {
        return false;
    }
    
    // Check heartbeat recency
    auto now = std::chrono::system_clock::now();
    auto heartbeat_age = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_leader_heartbeat_).count();
    
    return heartbeat_age < config_.heartbeat_interval_ms * 2;
}

/** @brief Return whether the coordinator is healthy (leader healthy and running). */
bool DistributedCoordinator::isHealthy() const {
    return isRunning() && isLeaderHealthy();
}

// Heartbeats
/** @brief Send leader heartbeat and renew local lease state. */
void DistributedCoordinator::sendHeartbeat() {
    if (!isLeader()) {
        return;
    }
    
    // Memory order: acquire to read current term safely
    THEMIS_DEBUG("Sending leader heartbeat (term: {})", current_term_.load(std::memory_order_acquire));
    
    // Renew lease
    renewLease();
    
    // In production: broadcast heartbeat via gossip to all shards
    // For now, just update local timestamp
    {
        std::lock_guard<std::shared_mutex> lock(leader_mutex_);
        last_leader_heartbeat_ = std::chrono::system_clock::now();
    }
}

/** @brief Process heartbeat received from remote leader candidate/leader. */
void DistributedCoordinator::receiveHeartbeat(const std::string& leader_id, uint32_t term) {
    std::lock_guard<std::shared_mutex> lock(leader_mutex_);
    
    // Update leader info if term is newer or same
    // Memory order: acquire-release for term update visibility
    if (term >= current_term_.load(std::memory_order_acquire)) {
        current_term_.store(term, std::memory_order_release);
        current_leader_ = leader_id;
        last_leader_heartbeat_ = std::chrono::system_clock::now();
        leader_lease_expires_ = std::chrono::system_clock::now() + 
                                std::chrono::seconds(config_.leader_lease_seconds);
        
        // If we were candidate or leader, step down
        // Memory order: acquire before check, release when updating
        if (role_.load(std::memory_order_acquire) != CoordinatorRole::FOLLOWER) {
            role_.store(CoordinatorRole::FOLLOWER, std::memory_order_release);
        }
    }
}

// Election
/** @brief Broadcast vote request for current term (stubbed transport). */
void DistributedCoordinator::requestVotes() {
    // Memory order: acquire to read current term safely
    THEMIS_DEBUG("Requesting votes for term {}", current_term_.load(std::memory_order_acquire));
    
    // In production: broadcast vote request via gossip
    // For simplified implementation, we just log
}

/** @brief Process vote request and emit vote response. */
void DistributedCoordinator::receiveVoteRequest(const std::string& candidate_id, uint32_t term) {
    // Simplified voting logic
    // Memory order: acquire to ensure current term is read before comparison
    bool should_vote = term > current_term_.load(std::memory_order_acquire);
    sendVote(candidate_id, should_vote);
}

/** @brief Emit vote decision for candidate (diagnostic path). */
void DistributedCoordinator::sendVote(const std::string& candidate_id, bool granted) {
    THEMIS_DEBUG("Voting for candidate {} (granted: {})", candidate_id, granted);
}

// Task distribution
/** @brief Broadcast coordinator task to remote shards (transport stub). */
void DistributedCoordinator::broadcastTask(const CoordinatorTask& task) {
    THEMIS_DEBUG("Broadcasting task: {}", task.task_id);
    
    // In production: broadcast task via gossip to all shards
    // For simplified implementation, we just store locally
}

/** @brief Accept coordinator task announced by remote leader. */
void DistributedCoordinator::receiveTask(const CoordinatorTask& task) {
    if (isLeader()) {
        // Leaders don't receive tasks, they create them
        return;
    }
    
    std::lock_guard<std::shared_mutex> lock(tasks_mutex_);
    pending_tasks_.push_back(task);
    
    THEMIS_INFO("Received task: {}", task.task_id);
}

// Lease management
/** @brief Return true when local leader lease has not expired. */
bool DistributedCoordinator::hasValidLease() const {
    std::shared_lock<std::shared_mutex> lock(leader_mutex_);
    
    auto now = std::chrono::system_clock::now();
    return now < leader_lease_expires_;
}

/** @brief Renew local leader lease expiration timestamp. */
void DistributedCoordinator::renewLease() {
    std::lock_guard<std::shared_mutex> lock(leader_mutex_);
    
    leader_lease_expires_ = std::chrono::system_clock::now() + 
                            std::chrono::seconds(config_.leader_lease_seconds);
    
    THEMIS_DEBUG("Renewed leader lease until {}", 
        std::chrono::duration_cast<std::chrono::seconds>(
            leader_lease_expires_.time_since_epoch()).count());
}

// Graceful handoff
/** @brief Attempt graceful leadership transfer and step down. */
void DistributedCoordinator::transferLeadership(const std::string& new_leader) {
    if (!isLeader()) {
        THEMIS_WARN("Only leader can transfer leadership");
        return;
    }
    
    THEMIS_INFO("Transferring leadership to shard: {}", new_leader);
    
    // In production: send leadership transfer message to new_leader
    // Then step down
    stepDown();
}

// Transaction visibility

/** @brief Wire optional transaction coordinator used for visibility queries. */
void DistributedCoordinator::setTransactionCoordinator(
    themisdb::sharding::CrossShardTransactionCoordinator* txn_coordinator)
{
    std::lock_guard<std::shared_mutex> lock(txn_coordinator_mutex_);
    txn_coordinator_ = txn_coordinator;
    THEMIS_INFO("DistributedCoordinator: transaction coordinator {}",
                txn_coordinator ? "registered" : "detached");
}

/** @brief Return in-flight transactions from wired transaction coordinator. */
std::vector<themisdb::sharding::CrossShardTransaction>
DistributedCoordinator::listInFlightTransactions() const
{
    themisdb::sharding::CrossShardTransactionCoordinator* txn_coordinator = nullptr;
    {
        std::shared_lock<std::shared_mutex> lock(txn_coordinator_mutex_);
        txn_coordinator = txn_coordinator_;
    }

    if (!txn_coordinator) {
        return {};
    }
    return txn_coordinator->getActiveTransactions();
}

/** @brief Lookup one transaction by ID via wired transaction coordinator. */
std::optional<themisdb::sharding::CrossShardTransaction>
DistributedCoordinator::getTransaction(const std::string& txn_id) const
{
    themisdb::sharding::CrossShardTransactionCoordinator* txn_coordinator = nullptr;
    {
        std::shared_lock<std::shared_mutex> lock(txn_coordinator_mutex_);
        txn_coordinator = txn_coordinator_;
    }

    if (!txn_coordinator) {
        return std::nullopt;
    }
    return txn_coordinator->getTransaction(txn_id);
}

} // namespace themis::sharding

