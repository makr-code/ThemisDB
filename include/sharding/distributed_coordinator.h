/**
 * @file distributed_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.9.0-beta
 * @note Maturity: PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "sharding/gossip_config_manager.h"
#include "sharding/shard_topology.h"
#include "sharding/cross_shard_transaction.h"
#include <nlohmann/json.hpp>
#include <atomic>
#include <optional>
#include <shared_mutex>
#include <thread>
#include <chrono>

namespace themis::sharding {

/** @brief Distributed coordinator component. */
class DistributedCoordinator {
public:
    /** @brief Runtime role of this node in distributed coordination. */
    enum class CoordinatorRole : uint8_t {
        FOLLOWER = 0,     ///< Passive shard without active coordination lease.
        CANDIDATE = 1,    ///< Node currently campaigning for leadership.
        LEADER = 2        ///< Node holding active coordinator lease.
    };

    /** @brief Categorization of coordinator-managed cluster tasks. */
    enum class TaskType : uint8_t {
        REBALANCE = 0,        ///< Data rebalancing between shards.
        REPAIR = 1,           ///< Repair/recovery of degraded shard state.
        MAINTENANCE = 2,      ///< General maintenance operation.
        SCHEMA_MIGRATION = 3, ///< Metadata/schema evolution operation.
        BACKUP = 4,           ///< Backup task coordination.
        RESTORE = 5           ///< Restore task coordination.
    };

    /** @brief Serializable unit of leader-coordinated work. */
    struct CoordinatorTask {
        std::string task_id;                                ///< Unique task identifier.
        TaskType type;                                      ///< Task category.
        nlohmann::json payload;                             ///< Task-specific payload.
        std::chrono::seconds ttl{600};                      ///< Task time-to-live (default 10 min).
        std::chrono::system_clock::time_point created_at;   ///< Task creation timestamp.
        std::chrono::system_clock::time_point started_at;   ///< Task execution start timestamp.
        std::string assigned_leader;                        ///< Leader responsible for execution.

        /** @brief Serialize task into JSON payload. */
        nlohmann::json toJson() const;
        /** @brief Deserialize task from JSON payload. */
        static CoordinatorTask fromJson(const nlohmann::json& j);
    };

    /** @brief Runtime tuning for election, heartbeat, and lease behavior. */
    struct Config {
        uint32_t leader_lease_seconds = 30;   ///< Leader lease duration in seconds.
        uint32_t heartbeat_interval_ms = 5000; ///< Leader heartbeat cadence in milliseconds.
        uint32_t election_timeout_ms = 10000;  ///< Election timeout in milliseconds.
        bool enable_automatic_failover = true; ///< Auto-start elections on leader failure.
        bool enable_leader_stickiness = true;  ///< Prefer current/previous leader continuity.
        float leader_stickiness_bonus = 0.3f;  ///< Re-election bias factor for sticky leader behavior.
    };

    /** @brief Snapshot of current coordinator leader status. */
    struct LeaderInfo {
        std::string shard_id;                               ///< Current leader shard ID.
        CoordinatorRole role;                               ///< Local role at time of snapshot.
        std::chrono::system_clock::time_point lease_expires_at; ///< Leader lease expiry time.
        std::chrono::system_clock::time_point last_heartbeat;   ///< Timestamp of last leader heartbeat.
        uint32_t term;                                      ///< Raft-inspired coordination term.

        /** @brief Serialize leader info into JSON payload. */
        nlohmann::json toJson() const;
    };

    /** @brief Runtime coordination statistics counters. */
    struct Statistics {
        std::atomic<uint64_t> elections_started{0};         ///< Number of election rounds initiated.
        std::atomic<uint64_t> elections_won{0};             ///< Number of elections won.
        std::atomic<uint64_t> elections_lost{0};            ///< Number of elections lost.
        std::atomic<uint64_t> leader_failures_detected{0};  ///< Number of detected leader-failure events.
        std::atomic<uint64_t> tasks_coordinated{0};         ///< Number of coordinated tasks accepted.
        std::atomic<double> avg_lease_duration_seconds{0.0}; ///< Average effective leader lease duration.

        Statistics() = default;
        Statistics(const Statistics& other) {
            elections_started.store(other.elections_started.load());
            elections_won.store(other.elections_won.load());
            elections_lost.store(other.elections_lost.load());
            leader_failures_detected.store(other.leader_failures_detected.load());
            tasks_coordinated.store(other.tasks_coordinated.load());
            avg_lease_duration_seconds.store(other.avg_lease_duration_seconds.load());
        }
        Statistics& operator=(const Statistics& other) {
            if (this != &other) {
                elections_started.store(other.elections_started.load());
                elections_won.store(other.elections_won.load());
                elections_lost.store(other.elections_lost.load());
                leader_failures_detected.store(other.leader_failures_detected.load());
                tasks_coordinated.store(other.tasks_coordinated.load());
                avg_lease_duration_seconds.store(other.avg_lease_duration_seconds.load());
            }
            return *this;
        }
    };

    /** @brief Task execution callback for coordinated tasks. */
    using TaskExecutor = std::function<bool(const CoordinatorTask&)>;
    /** @brief Callback invoked when a leader is elected or changed. */
    using LeaderElectedCallback = std::function<void(const std::string& leader_id)>;

    /**
     * @brief Construct coordinator with explicit configuration.
     * @param local_shard_id Local shard identity.
     * @param topology Shared shard topology provider.
     * @param gossip_mgr Shared gossip configuration manager.
     * @param config Coordinator runtime configuration.
     */
    explicit DistributedCoordinator(
        const std::string& local_shard_id,
        std::shared_ptr<ShardTopology> topology,
        std::shared_ptr<GossipConfigManager> gossip_mgr,
        const Config& config
    );

    /**
     * @brief Construct coordinator with default configuration values.
     * @param local_shard_id Local shard identity.
     * @param topology Shared shard topology provider.
     * @param gossip_mgr Shared gossip configuration manager.
     */
    DistributedCoordinator(
        const std::string& local_shard_id,
        std::shared_ptr<ShardTopology> topology,
        std::shared_ptr<GossipConfigManager> gossip_mgr
    );

    /** @brief Stop worker threads and release coordinator resources. */
    ~DistributedCoordinator();

    // Lifecycle
    /** @brief Start election, heartbeat, and task-executor worker loops. */
    void start();
    /** @brief Stop all worker loops and join their threads. */
    void stop();
    /** @brief Return true when coordinator workers are active. */
    bool isRunning() const { return running_.load(); }

    // Role management
    /** @brief Return local coordinator role. */
    CoordinatorRole getRole() const { return role_.load(); }
    /** @brief Return true when this node is current coordinator leader. */
    bool isLeader() const { return role_.load() == CoordinatorRole::LEADER; }
    /** @brief Return currently known leader shard ID when available. */
    std::optional<std::string> getCurrentLeader() const;

    // Local node identity
    /** @brief Return immutable local shard ID. */
    const std::string& getLocalShardId() const { return local_shard_id_; }

    // Leader election (Gossip-based, no centralized coordination)
    /** @brief Start a leader election round from local node. */
    void startElection();
    /** @brief Promote local node into leader role and initialize lease. */
    void becomeLeader();
    /** @brief Step down from leader/candidate role to follower. */
    void stepDown();

    // Task coordination (only if leader)
    /**
     * @brief Schedule new coordinated task (leader only).
     * @param task Task descriptor to schedule.
     * @return Scheduled task identifier.
     */
    std::string scheduleTask(const CoordinatorTask& task);
    /**
     * @brief Cancel previously scheduled task (leader only).
     * @param task_id Task identifier.
     * @return True when task was found and removed.
     */
    bool cancelTask(const std::string& task_id);
    /** @brief Return current pending task queue snapshot. */
    std::vector<CoordinatorTask> getPendingTasks() const;

    // Task execution callback
    /** @brief Register callback that executes coordinated tasks. */
    void setTaskExecutor(TaskExecutor executor);

    // Leader info
    /** @brief Return structured snapshot of current leader information. */
    LeaderInfo getLeaderInfo() const;

    // Callbacks
    /** @brief Register callback invoked after leader election events. */
    void setLeaderElectedCallback(LeaderElectedCallback callback);

    // Statistics
    /** @brief Return copy of current runtime statistics counters. */
    Statistics getStatistics() const;
    /** @brief Return runtime statistics as JSON payload. */
    nlohmann::json getStatisticsJson() const;

    // Transaction visibility – wired to a CrossShardTransactionCoordinator
    // so that OrphanDetector can query in-flight transactions via this class.

    /**
     * Register the transaction coordinator whose in-flight transactions this
     * DistributedCoordinator will expose.  The caller must ensure the
     * coordinator outlives this object (or call with nullptr to detach).
     */
    void setTransactionCoordinator(
        themisdb::sharding::CrossShardTransactionCoordinator* txn_coordinator);

    /**
     * Return all transactions that are currently active / in-flight according
     * to the registered CrossShardTransactionCoordinator.
     * Returns an empty vector when no coordinator has been registered.
     */
    std::vector<themisdb::sharding::CrossShardTransaction>
    listInFlightTransactions() const;

    /**
     * Look up a specific transaction by ID in the registered coordinator.
     * Returns std::nullopt when no coordinator is registered or the
     * transaction does not exist.
     */
    std::optional<themisdb::sharding::CrossShardTransaction>
    getTransaction(const std::string& txn_id) const;

private:
    std::string local_shard_id_;                    ///< Local shard identity.
    std::shared_ptr<ShardTopology> topology_;       ///< Shared topology provider.
    std::shared_ptr<GossipConfigManager> gossip_mgr_; ///< Shared gossip manager.
    Config config_;                                 ///< Runtime configuration snapshot.

    std::atomic<bool> running_{false};              ///< Worker lifecycle flag.
    std::atomic<CoordinatorRole> role_{CoordinatorRole::FOLLOWER}; ///< Current local role.
    std::atomic<uint32_t> current_term_{0};         ///< Current coordination term.
    
    // Leader state
    std::optional<std::string> current_leader_;     ///< Known current leader shard ID.
    std::chrono::system_clock::time_point leader_lease_expires_; ///< Leader lease expiration timestamp.
    std::chrono::system_clock::time_point last_leader_heartbeat_; ///< Last heartbeat timestamp from leader.
    mutable std::shared_mutex leader_mutex_;
    
    // Tasks (only used if leader)
    std::vector<CoordinatorTask> pending_tasks_;    ///< Leader-maintained pending tasks queue.
    mutable std::shared_mutex tasks_mutex_;
    
    // Threads
    std::thread election_thread_;                   ///< Election monitor thread.
    std::thread heartbeat_thread_;                  ///< Heartbeat emission thread.
    std::thread task_executor_thread_;              ///< Task execution thread.
    
    // Callbacks
    TaskExecutor task_executor_;                    ///< Task execution callback.
    LeaderElectedCallback leader_elected_callback_; ///< Leader election callback.
    std::mutex callback_mutex_;
    
    // Statistics
    Statistics stats_;                              ///< Runtime counters.

    // Optional wired transaction coordinator (non-owning pointer).
    themisdb::sharding::CrossShardTransactionCoordinator* txn_coordinator_{nullptr}; ///< Optional non-owning tx coordinator bridge.
    mutable std::shared_mutex txn_coordinator_mutex_;

    // Election logic (Raft-inspired but gossip-based)
    /** @brief Periodic election-monitor loop body. */
    void electionLoop();
    /** @brief Heartbeat-loop body (kept for symmetry/extensibility). */
    void heartbeatLoop();
    /** @brief Task-executor loop body for pending coordinated tasks. */
    void taskExecutorLoop();
    
    // Leader detection
    /** @brief Detect and mark leader failure based on lease expiration. */
    void detectLeaderFailure();
    /** @brief Return whether current leader heartbeat is still fresh. */
    bool isLeaderHealthy() const;
    
    // Heartbeats
    /** @brief Emit leader heartbeat and renew lease metadata. */
    void sendHeartbeat();
    /** @brief Process incoming leader heartbeat and update role/term state. */
    void receiveHeartbeat(const std::string& leader_id, uint32_t term);
    
    // Election
    /** @brief Broadcast vote requests for current election term. */
    void requestVotes();
    /** @brief Process incoming vote request from candidate. */
    void receiveVoteRequest(const std::string& candidate_id, uint32_t term);
    /** @brief Emit vote response to election candidate. */
    void sendVote(const std::string& candidate_id, bool granted);
    
    // Task distribution (gossip-based)
    /** @brief Broadcast task announcement through gossip channel. */
    void broadcastTask(const CoordinatorTask& task);
    /** @brief Handle task announcement received from remote leader. */
    void receiveTask(const CoordinatorTask& task);
    
    // Lease management
    /** @brief Return true when local view of leader lease is still valid. */
    bool hasValidLease() const;
    /** @brief Renew local leader lease expiration timestamp. */
    void renewLease();
    
    // Graceful handoff
    /** @brief Transfer leadership intent to another shard and step down. */
    void transferLeadership(const std::string& new_leader);
};

} // namespace themis::sharding
