/**
 * @file multi_primary_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <atomic>
#include <chrono>
#include <functional>
#include "sharding/wal_manager.h"
#include "sharding/wal_shipper.h"
#include "sharding/replica_topology.h"
#include "sharding/write_concern.h"

namespace themis::sharding {

/**
 * Primary Node State
 */
enum class PrimaryState {
    ACTIVE = 0,       // Accepting writes, shipping to replicas
    STANDBY = 1,      // Receiving replication, ready to promote
    DEGRADED = 2,     // Network issues, partial replication
    OFFLINE = 3       // Not responding to health checks
};

/**
 * Primary Node Info
 */
struct PrimaryNodeInfo {
    std::string node_id;
    std::string endpoint;  // HTTP/gRPC endpoint
    PrimaryState state = PrimaryState::STANDBY;
    LSN last_known_lsn;
    std::chrono::steady_clock::time_point last_heartbeat;
    uint64_t write_count = 0;
    bool is_current = false;  // Is this the current node?
    
    bool isHealthy() const {
        return state == PrimaryState::ACTIVE || state == PrimaryState::STANDBY;
    }
    
    std::chrono::milliseconds timeSinceLastHeartbeat() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - last_heartbeat);
    }
};

/**
 * Multi-Primary Configuration
 */
struct MultiPrimaryConfig {
    std::string current_node_id;
    std::vector<std::string> primary_node_ids;  // All primary nodes in cluster
    std::map<std::string, std::string> primary_endpoints;  // node_id -> endpoint
    
    // Conflict resolution
    bool use_last_write_wins = true;  // LWW conflict resolution
    bool allow_concurrent_writes = true;  // Multiple primaries can write
    
    // Replication
    bool cross_primary_replication = true;  // Primaries replicate to each other
    WriteConcern default_write_concern = WriteConcern::MAJORITY;
    
    // Promotion
    bool auto_promote_on_primary_failure = true;
    std::chrono::milliseconds promotion_timeout{5000};  // 5 seconds
};

/**
 * Write Conflict Resolution
 */
struct WriteConflict {
    LSN lsn1;
    LSN lsn2;
    uint64_t timestamp1;
    uint64_t timestamp2;
    std::string primary_id1;
    std::string primary_id2;
    
    // LWW: choose entry with latest timestamp
    LSN resolveLastWriteWins() const {
        return (timestamp2 > timestamp1) ? lsn2 : lsn1;
    }
};

/**
 * Multi-Primary Coordinator
 * 
 * Coordinates multiple primary nodes without traditional leader election.
 * Features:
 * - Multiple primaries accept writes simultaneously
 * - Conflict resolution via Last-Write-Wins (timestamp-based)
 * - Cross-primary replication for eventual consistency
 * - Manual promotion on primary failure (no automatic election)
 */
class MultiPrimaryCoordinator {
public:
    explicit MultiPrimaryCoordinator(const MultiPrimaryConfig& config);
    ~MultiPrimaryCoordinator();
    
    /**
     * Register a primary node
     */
    void registerPrimary(const std::string& node_id, const std::string& endpoint);
    
    /**
     * Promote a standby to active primary
     * Manual operation, no automatic election
     */
    bool promoteToPrimary(const std::string& node_id);
    
    /**
     * Demote an active primary to standby
     */
    bool demoteToStandby(const std::string& node_id);
    
    /**
     * Mark primary as offline (failed health check)
     */
    void markPrimaryOffline(const std::string& node_id);
    
    /**
     * Update primary heartbeat
     */
    void updateHeartbeat(const std::string& node_id, const LSN& current_lsn);
    
    /**
     * Get all active primaries
     */
    std::vector<PrimaryNodeInfo> getActivePrimaries() const;
    
    /**
     * Get primary info
     */
    std::optional<PrimaryNodeInfo> getPrimaryInfo(const std::string& node_id) const;
    
    /**
     * Check if current node is active primary
     */
    bool isCurrentNodeActive() const;
    
    /**
     * Resolve write conflict (LWW)
     */
    LSN resolveConflict(const WriteConflict& conflict) const;
    
    /**
     * Increment write count for current node
     */
    void recordWrite(const LSN& lsn);
    
    /**
     * Get primary with most recent LSN (for routing reads)
     */
    std::optional<std::string> getMostCurrentPrimary() const;
    
    /**
     * Get statistics
     */
    struct Statistics {
        size_t total_primaries = 0;
        size_t active_primaries = 0;
        size_t standby_primaries = 0;
        size_t offline_primaries = 0;
        uint64_t total_writes = 0;
        uint64_t conflicts_resolved = 0;
    };
    
    Statistics getStatistics() const;

private:
    MultiPrimaryConfig config_;
    mutable std::mutex mutex_;
    
    std::map<std::string, PrimaryNodeInfo> primaries_;
    mutable std::atomic<uint64_t> conflicts_resolved_{0};
};

} // namespace themis::sharding
