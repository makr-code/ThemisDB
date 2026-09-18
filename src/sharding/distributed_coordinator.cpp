/**
 * @file distributed_coordinator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
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

namespace {

void sleepWhileRunning(
    const std::atomic<bool>& running,
    std::chrono::milliseconds total,
    std::chrono::milliseconds quantum = std::chrono::milliseconds(100))
{
    while (running.load(std::memory_order_acquire) && total.count() > 0) {
        const auto slice = std::min(total, quantum);
        std::this_thread::sleep_for(slice);
        total -= slice;
    }
}

}  // namespace

// CoordinatorTask JSON serialization
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

/**
 * @brief From Json.
 * @param[in] j Input parameter.
 * @return Return value.
 * @details Calls: value(), nlohmann::json::object(), std::chrono::seconds(), std::chrono::system_clock::time_point(), std::chrono::milliseconds().
 */
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

DistributedCoordinator::DistributedCoordinator(
    const std::string& local_shard_id,
    std::shared_ptr<ShardTopology> topology,
    std::shared_ptr<GossipConfigManager> gossip_mgr)
    : DistributedCoordinator(local_shard_id, topology, gossip_mgr, Config{})
{
}

// Destructor
DistributedCoordinator::~DistributedCoordinator() {
    stop();
}

// Lifecycle methods
/**
 * @brief Start.
 * @details Calls: exchange(), THEMIS_WARN(), std::thread(), load(), electionLoop(), sleepWhileRunning(), std::chrono::milliseconds(), isLeader().
 */
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
            sleepWhileRunning(
                running_,
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
            sleepWhileRunning(
                running_,
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
            sleepWhileRunning(running_, std::chrono::seconds(1));
        }
    });
    
    THEMIS_INFO("DistributedCoordinator started");
}

/**
 * @brief Stop.
 * @details Calls: exchange(), THEMIS_INFO(), themis::utils::joinThreadWithin(), THEMIS_WARN().
 */
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
std::optional<std::string> DistributedCoordinator::getCurrentLeader() const {
    /**
     * @brief Lock.
     * @param[in] leader_mutex_ Input parameter.
     * @return Return value.
     */
    std::shared_lock<std::shared_mutex> lock(leader_mutex_);
    return current_leader_;
}

// Leader election
/**
 * @brief Start Election.
 * @details Calls: lock(), load(), THEMIS_WARN(), store(), THEMIS_INFO(), requestVotes(), what(), sleepWhileRunning().
 */
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
    sleepWhileRunning(
        running_,
        std::chrono::milliseconds(config_.election_timeout_ms)
    );

    if (!running_.load(std::memory_order_acquire)) {
        return;
    }
    
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

/**
 * @brief Become Leader.
 * @details Calls: cb_lock(), lock(), store(), std::chrono::system_clock::now(), std::chrono::seconds(), THEMIS_INFO(), load(), callback().
 */
void DistributedCoordinator::becomeLeader() {
    // Capture callback before taking lock to avoid deadlock
    LeaderElectedCallback callback;
    {
        std::lock_guard<std::mutex> cb_lock(callback_mutex_);
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
    if (callback) {
        callback(local_shard_id_);
    }
}

/**
 * @brief Step Down.
 * @details Calls: lock(), load(), THEMIS_INFO(), store(), reset().
 */
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
/**
 * @brief Schedule Task.
 * @param[in] task Input parameter.
 * @return Return value.
 * @throws std::runtime_error if an error occurs.
 * @details Calls: isLeader(), lock(), push_back(), broadcastTask(), THEMIS_INFO().
 */
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

/**
 * @brief Cancel Task.
 * @param[in] task_id Identifier of the task.
 * @return True when the operation succeeds.
 * @details Calls: isLeader(), THEMIS_WARN(), lock(), std::find_if(), begin(), end(), erase(), THEMIS_INFO().
 */
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

std::vector<DistributedCoordinator::CoordinatorTask> 
DistributedCoordinator::getPendingTasks() const {
    /**
     * @brief Lock.
     * @param[in] tasks_mutex_ Input parameter.
     * @return Return value.
     */
    std::shared_lock<std::shared_mutex> lock(tasks_mutex_);
    return pending_tasks_;
}

// Task execution callback
/**
 * @brief Set Task Executor.
 * @param[in] executor Input parameter.
 * @details Calls: lock().
 */
void DistributedCoordinator::setTaskExecutor(TaskExecutor executor) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    task_executor_ = executor;
}

// Leader info
DistributedCoordinator::LeaderInfo DistributedCoordinator::getLeaderInfo() const {
    /**
     * @brief Lock.
     * @param[in] leader_mutex_ Input parameter.
     * @return Return value.
     */
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
/**
 * @brief Set Leader Elected Callback.
 * @param[in] callback Input parameter.
 * @details Calls: lock().
 */
void DistributedCoordinator::setLeaderElectedCallback(LeaderElectedCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    leader_elected_callback_ = callback;
}

// Statistics
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

/**
 * @brief Private methods - Election logic
 * @details Calls: detectLeaderFailure(), has_value(), THEMIS_INFO(), startElection().
 */
void DistributedCoordinator::electionLoop() {
    // Check if there's a leader
    detectLeaderFailure();
    
    // If no leader and automatic failover is enabled, start election
    if (!current_leader_.has_value() && config_.enable_automatic_failover) {
        THEMIS_INFO("No leader detected, starting election");
        startElection();
    }
}

/**
 * @brief Heartbeat Loop.
 * @details Implements heartbeatLoop without additional internal calls.
 */
void DistributedCoordinator::heartbeatLoop() {
    // Handled in the main heartbeat thread
}

/**
 * @brief Task Executor Loop.
 * @details Calls: lock(), cb_lock(), THEMIS_DEBUG(), executor(), THEMIS_INFO(), cancelTask(), THEMIS_WARN(), THEMIS_ERROR().
 */
void DistributedCoordinator::taskExecutorLoop() {
    std::vector<CoordinatorTask> tasks_to_execute;
    
    {
        std::shared_lock<std::shared_mutex> lock(tasks_mutex_);
        tasks_to_execute = pending_tasks_;
    }
    
    // Get executor outside of any locks to avoid deadlock
    TaskExecutor executor;
    {
        std::lock_guard<std::mutex> cb_lock(callback_mutex_);
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
/**
 * @brief Detect Leader Failure.
 * @details Calls: lock(), has_value(), std::chrono::system_clock::now(), THEMIS_WARN(), value(), reset().
 */
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

bool DistributedCoordinator::isLeaderHealthy() const {
    /**
     * @brief Lock.
     * @param[in] leader_mutex_ Input parameter.
     * @return Return value.
     */
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

bool DistributedCoordinator::isHealthy() const {
    return isRunning() && isLeaderHealthy();
}

// Heartbeats
/**
 * @brief Send Heartbeat.
 * @details Calls: isLeader(), THEMIS_DEBUG(), load(), renewLease(), lock(), std::chrono::system_clock::now().
 */
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

/**
 * @brief Receive Heartbeat.
 * @param[in] leader_id Identifier of the leader.
 * @param[in] term Input parameter.
 * @details Calls: lock(), load(), store(), std::chrono::system_clock::now(), std::chrono::seconds().
 */
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
/**
 * @brief Request Votes.
 * @details Calls: THEMIS_DEBUG(), load().
 */
void DistributedCoordinator::requestVotes() {
    // Memory order: acquire to read current term safely
    THEMIS_DEBUG("Requesting votes for term {}", current_term_.load(std::memory_order_acquire));
    
    // In production: broadcast vote request via gossip
    // For simplified implementation, we just log
}

/**
 * @brief Receive Vote Request.
 * @param[in] candidate_id Identifier of the candidate.
 * @param[in] term Input parameter.
 * @details Calls: load(), sendVote().
 */
void DistributedCoordinator::receiveVoteRequest(const std::string& candidate_id, uint32_t term) {
    // Simplified voting logic
    // Memory order: acquire to ensure current term is read before comparison
    bool should_vote = term > current_term_.load(std::memory_order_acquire);
    sendVote(candidate_id, should_vote);
}

/**
 * @brief Send Vote.
 * @param[in] candidate_id Identifier of the candidate.
 * @param[in] granted Input parameter.
 * @details Calls: THEMIS_DEBUG().
 */
void DistributedCoordinator::sendVote(const std::string& candidate_id, bool granted) {
    THEMIS_DEBUG("Voting for candidate {} (granted: {})", candidate_id, granted);
}

// Task distribution
/**
 * @brief Broadcast Task.
 * @param[in] task Input parameter.
 * @details Calls: THEMIS_DEBUG().
 */
void DistributedCoordinator::broadcastTask(const CoordinatorTask& task) {
    THEMIS_DEBUG("Broadcasting task: {}", task.task_id);
    
    // In production: broadcast task via gossip to all shards
    // For simplified implementation, we just store locally
}

/**
 * @brief Receive Task.
 * @param[in] task Input parameter.
 * @details Calls: isLeader(), lock(), push_back(), THEMIS_INFO().
 */
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
bool DistributedCoordinator::hasValidLease() const {
    /**
     * @brief Lock.
     * @param[in] leader_mutex_ Input parameter.
     * @return Return value.
     */
    std::shared_lock<std::shared_mutex> lock(leader_mutex_);
    
    auto now = std::chrono::system_clock::now();
    return now < leader_lease_expires_;
}

/**
 * @brief Renew Lease.
 * @details Calls: lock(), std::chrono::system_clock::now(), std::chrono::seconds(), THEMIS_DEBUG(), time_since_epoch(), count().
 */
void DistributedCoordinator::renewLease() {
    std::lock_guard<std::shared_mutex> lock(leader_mutex_);
    
    leader_lease_expires_ = std::chrono::system_clock::now() + 
                            std::chrono::seconds(config_.leader_lease_seconds);
    
    THEMIS_DEBUG("Renewed leader lease until {}", 
        std::chrono::duration_cast<std::chrono::seconds>(
            leader_lease_expires_.time_since_epoch()).count());
}

// Graceful handoff
/**
 * @brief Transfer Leadership.
 * @param[in] new_leader Input parameter.
 * @details Calls: isLeader(), THEMIS_WARN(), THEMIS_INFO(), stepDown().
 */
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

/**
 * @brief Set Transaction Coordinator.
 * @param[in,out] txn_coordinator Input/output parameter.
 */
void DistributedCoordinator::setTransactionCoordinator(
    themisdb::sharding::CrossShardTransactionCoordinator* txn_coordinator)
{
    /**
     * @brief Lock.
     * @param[in] txn_coordinator_mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::shared_mutex> lock(txn_coordinator_mutex_);
    txn_coordinator_ = txn_coordinator;
    THEMIS_INFO("DistributedCoordinator: transaction coordinator {}",
                txn_coordinator ? "registered" : "detached");
}

std::vector<themisdb::sharding::CrossShardTransaction>
DistributedCoordinator::listInFlightTransactions() const
{
    themisdb::sharding::CrossShardTransactionCoordinator* txn_coordinator = nullptr;
    {
        /**
         * @brief Lock.
         * @param[in] txn_coordinator_mutex_ Input parameter.
         * @return Return value.
         */
        std::shared_lock<std::shared_mutex> lock(txn_coordinator_mutex_);
        txn_coordinator = txn_coordinator_;
    }

    if (!txn_coordinator) {
        return {};
    }
    return txn_coordinator->getActiveTransactions();
}

std::optional<themisdb::sharding::CrossShardTransaction>
DistributedCoordinator::getTransaction(const std::string& txn_id) const
{
    themisdb::sharding::CrossShardTransactionCoordinator* txn_coordinator = nullptr;
    {
        /**
         * @brief Lock.
         * @param[in] txn_coordinator_mutex_ Input parameter.
         * @return Return value.
         */
        std::shared_lock<std::shared_mutex> lock(txn_coordinator_mutex_);
        txn_coordinator = txn_coordinator_;
    }

    if (!txn_coordinator) {
        return std::nullopt;
    }
    return txn_coordinator->getTransaction(txn_id);
}

} // namespace themis::sharding

