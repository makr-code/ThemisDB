/**
 * @file auto_recovery_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB RAID Auto-Recovery Manager
 * 
 * Automated monitoring and recovery system for RAID redundancy:
 * - Background health checks
 * - Automatic degraded document detection
 * - Self-healing with configurable policies
 * - Alerting and notifications
 */

#pragma once

#include "sharding/redundancy_strategy.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include "sharding/shard_repair_engine.h"
#include <thread>
#include <atomic>
#include <memory>
#include <queue>
#include <condition_variable>
#include <functional>

namespace themisdb {
namespace sharding {

// ═══════════════════════════════════════════════════════════
// Auto-Recovery Configuration
// ═══════════════════════════════════════════════════════════

struct AutoRecoveryConfig {
    // Monitoring intervals
    std::chrono::seconds health_check_interval{60};
    std::chrono::seconds repair_check_interval{300};
    
    // Recovery policies
    bool enable_auto_repair = true;
    bool enable_proactive_rebalancing = false;
    uint32_t max_concurrent_repairs = 4;
    uint32_t repair_batch_size = 100;
    
    // Thresholds
    float degraded_threshold = 0.1f;  // Trigger repair if >10% degraded
    float critical_threshold = 0.05f;  // Alert if >5% critical
    
    // Alerting
    bool enable_alerts = true;
    std::function<void(const std::string&)> alert_callback;
};

// ═══════════════════════════════════════════════════════════
// Health Status
// ═══════════════════════════════════════════════════════════

struct HealthStatus {
    uint64_t total_documents = 0;
    uint64_t healthy_documents = 0;
    uint64_t degraded_documents = 0;
    uint64_t critical_documents = 0;
    
    std::chrono::system_clock::time_point last_check;
    
    float getHealthPercentage() const {
        if (total_documents == 0) return 100.0f;
        return (healthy_documents * 100.0f) / total_documents;
    }
    
    float getDegradedPercentage() const {
        if (total_documents == 0) return 0.0f;
        return (degraded_documents * 100.0f) / total_documents;
    }
    
    float getCriticalPercentage() const {
        if (total_documents == 0) return 0.0f;
        return (critical_documents * 100.0f) / total_documents;
    }
};

// ═══════════════════════════════════════════════════════════
// Auto-Recovery Manager
// ═══════════════════════════════════════════════════════════

/** @brief Auto-Recovery Manager. */
class AutoRecoveryManager {
public:
    explicit AutoRecoveryManager(
        const AutoRecoveryConfig& config,
        RedundancyStrategy& strategy,
        ConsistentHashRing& ring,
        ShardTopology& topology
    );
    
    ~AutoRecoveryManager();
    
    // Lifecycle
    void start();
    void stop();
    bool isRunning() const;
    
    // Manual operations
    void triggerHealthCheck();
    void triggerRepair();
    void pauseAutoRepair();
    void resumeAutoRepair();

    /**
     * Attach a ShardRepairEngine to be used for actual document recovery.
     * When set, repairDocument() delegates to ShardRepairEngine::triggerDocumentRepair()
     * instead of returning false (the old stub behaviour).
     */
    void setRepairEngine(std::shared_ptr<themis::sharding::ShardRepairEngine> engine);
    
    // Status
    HealthStatus getHealthStatus() const;
    std::vector<std::string> getDegradedDocuments() const;
    std::vector<std::string> getCriticalDocuments() const;
    
    // Statistics
    struct Stats {
        uint64_t health_checks_performed = 0;
        uint64_t repairs_attempted = 0;
        uint64_t repairs_successful = 0;
        uint64_t repairs_failed = 0;
        uint64_t alerts_sent = 0;
        std::chrono::milliseconds avg_repair_time{0};
    };
    
    Stats getStats() const;
    void resetStats();
    
private:
    // Background threads
    void healthCheckLoop();
    void repairLoop();
    
    // Health monitoring
    HealthStatus performHealthCheck();
    void checkDocumentHealth(const std::string& doc_id);
    
    // Recovery operations
    bool repairDocument(const std::string& doc_id);
    void processRepairQueue();
    
    // Alerting
    void sendAlert(const std::string& message);
    
    // Configuration and state
    AutoRecoveryConfig config_;
    RedundancyStrategy& strategy_;
    ConsistentHashRing& ring_;
    ShardTopology& topology_;
    std::shared_ptr<themis::sharding::ShardRepairEngine> repair_engine_;
    
    // Threading
    std::atomic<bool> running_{false};
    std::atomic<bool> repair_paused_{false};
    std::thread health_check_thread_;
    std::thread repair_thread_;
    
    // Health tracking
    mutable std::shared_mutex health_mutex_;
    HealthStatus current_health_;
    std::set<std::string> degraded_docs_;
    std::set<std::string> critical_docs_;
    
    // Repair queue
    std::mutex repair_mutex_;
    std::condition_variable repair_cv_;
    std::queue<std::string> repair_queue_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    Stats stats_;
};

// ═══════════════════════════════════════════════════════════
// Implementation
// ═══════════════════════════════════════════════════════════

inline AutoRecoveryManager::AutoRecoveryManager(
    const AutoRecoveryConfig& config,
    RedundancyStrategy& strategy,
    ConsistentHashRing& ring,
    ShardTopology& topology
) : config_(config),
    strategy_(strategy),
    ring_(ring),
    topology_(topology) {
}

inline AutoRecoveryManager::~AutoRecoveryManager() {
    stop();
}

inline void AutoRecoveryManager::start() {
    if (running_.exchange(true)) {
        return;
    }
    
    // Start health check thread
    health_check_thread_ = std::thread([this]() {
        healthCheckLoop();
    });
    
    // Start repair thread
    if (config_.enable_auto_repair) {
        repair_thread_ = std::thread([this]() {
            repairLoop();
        });
    }
}

inline void AutoRecoveryManager::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    
    // Wake up repair thread
    repair_cv_.notify_all();
    
    // Join threads
    if (health_check_thread_.joinable()) {
        health_check_thread_.join();
    }
    if (repair_thread_.joinable()) {
        repair_thread_.join();
    }
}

inline bool AutoRecoveryManager::isRunning() const {
    return running_.load();
}

inline void AutoRecoveryManager::healthCheckLoop() {
    while (running_.load()) {
        try {
            auto health = performHealthCheck();
            
            {
                std::unique_lock<std::shared_mutex> lock(health_mutex_);
                current_health_ = health;
            }
            
            // Check thresholds and alert if necessary
            if (health.getCriticalPercentage() > config_.critical_threshold * 100) {
                sendAlert("CRITICAL: " + std::to_string(health.critical_documents) + 
                         " documents are in critical state");
            } else if (health.getDegradedPercentage() > config_.degraded_threshold * 100) {
                sendAlert("WARNING: " + std::to_string(health.degraded_documents) + 
                         " documents are degraded");
            }
            
            {
                std::lock_guard<std::mutex> lock(stats_mutex_);
                stats_.health_checks_performed++;
            }
            
        } catch (const std::exception& e) {
            sendAlert("Health check error: " + std::string(e.what()));
        }
        
        std::this_thread::sleep_for(config_.health_check_interval);
    }
}

inline void AutoRecoveryManager::repairLoop() {
    while (running_.load()) {
        std::unique_lock<std::mutex> lock(repair_mutex_);
        
        // Wait for repair work or stop signal
        repair_cv_.wait_for(lock, config_.repair_check_interval, [this]() {
            return !repair_queue_.empty() || !running_.load();
        });
        
        if (!running_.load()) {
            break;
        }
        
        if (repair_paused_.load()) {
            continue;
        }
        
        // Process repair queue
        processRepairQueue();
    }
}

inline HealthStatus AutoRecoveryManager::performHealthCheck() {
    HealthStatus status;
    status.last_check = std::chrono::system_clock::now();
    
    // This is a simplified version
    // In production, iterate through all documents in collections
    
    return status;
}

inline void AutoRecoveryManager::processRepairQueue() {
    std::unique_lock<std::mutex> lock(repair_mutex_);
    
    uint32_t repairs_this_batch = 0;
    
    while (!repair_queue_.empty() && 
           repairs_this_batch < config_.repair_batch_size &&
           repairs_this_batch < config_.max_concurrent_repairs) {
        
        std::string doc_id = repair_queue_.front();
        repair_queue_.pop();
        
        lock.unlock();
        
        // Perform repair
        auto start = std::chrono::steady_clock::now();
        bool success = repairDocument(doc_id);
        auto end = std::chrono::steady_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        {
            std::lock_guard<std::mutex> stats_lock(stats_mutex_);
            stats_.repairs_attempted++;
            if (success) {
                stats_.repairs_successful++;
            } else {
                stats_.repairs_failed++;
            }
            
            // Update average repair time
            uint64_t total_time = stats_.avg_repair_time.count() * (stats_.repairs_attempted - 1);
            stats_.avg_repair_time = std::chrono::milliseconds(
                (total_time + duration.count()) / stats_.repairs_attempted
            );
        }
        
        repairs_this_batch++;
        lock.lock();
    }
}

inline bool AutoRecoveryManager::repairDocument(const std::string& doc_id) {
    if (repair_engine_) {
        // Delegate to the full-featured ShardRepairEngine.
        // triggerDocumentRepair() enqueues an async job; we report success when
        // the job is accepted (further progress tracked via getJobStatus).
        // The second argument is the collection name; empty string uses the
        // engine's configured default_collection.
        repair_engine_->triggerDocumentRepair(doc_id, /* collection= */ "");
        return true;
    }
    // ShardRepairEngine not wired up — no-op (caller should call setRepairEngine).
    return false;
}

inline void AutoRecoveryManager::setRepairEngine(
    std::shared_ptr<themis::sharding::ShardRepairEngine> engine) {
    repair_engine_ = std::move(engine);
}

inline void AutoRecoveryManager::sendAlert(const std::string& message) {
    if (config_.enable_alerts && config_.alert_callback) {
        config_.alert_callback(message);
        
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.alerts_sent++;
    }
}

inline HealthStatus AutoRecoveryManager::getHealthStatus() const {
    std::shared_lock<std::shared_mutex> lock(health_mutex_);
    return current_health_;
}

inline AutoRecoveryManager::Stats AutoRecoveryManager::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

inline void AutoRecoveryManager::pauseAutoRepair() {
    repair_paused_.store(true);
}

inline void AutoRecoveryManager::resumeAutoRepair() {
    repair_paused_.store(false);
    repair_cv_.notify_all();
}

} // namespace sharding
} // namespace themisdb
