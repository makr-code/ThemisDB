/**
 * @file distributed_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: distributed_coordinator.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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

class DistributedCoordinator {
public:
    enum class CoordinatorRole : uint8_t {
        FOLLOWER = 0,     // Normal shard (default)
        CANDIDATE = 1,    // Requesting to become leader
        LEADER = 2        // Current coordinator
    };
    
    enum class TaskType : uint8_t {
        REBALANCE = 0,
        REPAIR = 1,
        MAINTENANCE = 2,
        SCHEMA_MIGRATION = 3,
        BACKUP = 4,
        RESTORE = 5
    };
    
    struct CoordinatorTask {
        std::string task_id;
        TaskType type;
        nlohmann::json payload;
        std::chrono::seconds ttl{600};  // 10 min default
        std::chrono::system_clock::time_point created_at;
        std::chrono::system_clock::time_point started_at;
        std::string assigned_leader;
        
        nlohmann::json toJson() const;
        static CoordinatorTask fromJson(const nlohmann::json& j);
    };
    
    struct Config {
        uint32_t leader_lease_seconds = 30;          // 30s lease
        uint32_t heartbeat_interval_ms = 5000;       // 5s heartbeats
        uint32_t election_timeout_ms = 10000;        // 10s election timeout
        bool enable_automatic_failover = true;
        bool enable_leader_stickiness = true;        // Prefer current leader
        float leader_stickiness_bonus = 0.3f;        // 30% bonus for re-election
    };
    
    struct LeaderInfo {
        std::string shard_id;
        CoordinatorRole role;
        std::chrono::system_clock::time_point lease_expires_at;
        std::chrono::system_clock::time_point last_heartbeat;
        uint32_t term;  // Raft-inspired term number
        
        nlohmann::json toJson() const;
    };
    
    struct Statistics {
        std::atomic<uint64_t> elections_started{0};
        std::atomic<uint64_t> elections_won{0};
        std::atomic<uint64_t> elections_lost{0};
        std::atomic<uint64_t> leader_failures_detected{0};
        std::atomic<uint64_t> tasks_coordinated{0};
        std::atomic<double> avg_lease_duration_seconds{0.0};

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
    
    using TaskExecutor = std::function<bool(const CoordinatorTask&)>;
    using LeaderElectedCallback = std::function<void(const std::string& leader_id)>;
    
    explicit DistributedCoordinator(
        const std::string& local_shard_id,
        std::shared_ptr<ShardTopology> topology,
        std::shared_ptr<GossipConfigManager> gossip_mgr,
        const Config& config
    );

    DistributedCoordinator(
        const std::string& local_shard_id,
        std::shared_ptr<ShardTopology> topology,
        std::shared_ptr<GossipConfigManager> gossip_mgr
    );
    
    ~DistributedCoordinator();
    
    // Lifecycle
    void start();
    void stop();
    bool isRunning() const { return running_.load(); }
    
    // Role management
    CoordinatorRole getRole() const { return role_.load(); }
    bool isLeader() const { return role_.load() == CoordinatorRole::LEADER; }
    std::optional<std::string> getCurrentLeader() const;
    
    // Local node identity
    const std::string& getLocalShardId() const { return local_shard_id_; }
    
    // Leader election (Gossip-based, no centralized coordination)
    void startElection();
    void becomeLeader();
    void stepDown();
    
    // Task coordination (only if leader)
    std::string scheduleTask(const CoordinatorTask& task);
    bool cancelTask(const std::string& task_id);
    std::vector<CoordinatorTask> getPendingTasks() const;
    
    // Task execution callback
    void setTaskExecutor(TaskExecutor executor);
    
    // Leader info
    LeaderInfo getLeaderInfo() const;
    
    // Callbacks
    void setLeaderElectedCallback(LeaderElectedCallback callback);
    
    // Statistics
    Statistics getStatistics() const;
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
    std::string local_shard_id_;
    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<GossipConfigManager> gossip_mgr_;
    Config config_;
    
    std::atomic<bool> running_{false};
    std::atomic<CoordinatorRole> role_{CoordinatorRole::FOLLOWER};
    std::atomic<uint32_t> current_term_{0};
    
    // Leader state
    std::optional<std::string> current_leader_;
    std::chrono::system_clock::time_point leader_lease_expires_;
    std::chrono::system_clock::time_point last_leader_heartbeat_;
    mutable std::shared_mutex leader_mutex_;
    
    // Tasks (only used if leader)
    std::vector<CoordinatorTask> pending_tasks_;
    mutable std::shared_mutex tasks_mutex_;
    
    // Threads
    std::thread election_thread_;
    std::thread heartbeat_thread_;
    std::thread task_executor_thread_;
    
    // Callbacks
    TaskExecutor task_executor_;
    LeaderElectedCallback leader_elected_callback_;
    std::mutex callback_mutex_;
    
    // Statistics
    Statistics stats_;

    // Optional wired transaction coordinator (non-owning pointer).
    themisdb::sharding::CrossShardTransactionCoordinator* txn_coordinator_{nullptr};
    mutable std::shared_mutex txn_coordinator_mutex_;

    // Election logic (Raft-inspired but gossip-based)
    void electionLoop();
    void heartbeatLoop();
    void taskExecutorLoop();
    
    // Leader detection
    void detectLeaderFailure();
    bool isLeaderHealthy() const;
    
    // Heartbeats
    void sendHeartbeat();
    void receiveHeartbeat(const std::string& leader_id, uint32_t term);
    
    // Election
    void requestVotes();
    void receiveVoteRequest(const std::string& candidate_id, uint32_t term);
    void sendVote(const std::string& candidate_id, bool granted);
    
    // Task distribution (gossip-based)
    void broadcastTask(const CoordinatorTask& task);
    void receiveTask(const CoordinatorTask& task);
    
    // Lease management
    bool hasValidLease() const;
    void renewLease();
    
    // Graceful handoff
    void transferLeadership(const std::string& new_leader);
};

} // namespace themis::sharding
