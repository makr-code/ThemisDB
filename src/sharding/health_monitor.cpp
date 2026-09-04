/**
 * @file health_monitor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=0, H=7, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/health_monitor.h"
#include "utils/logger.h"
#include "utils/thread_join_utils.h"
#include <algorithm>
#include <stdexcept>

namespace themis::sharding {

/**
 * @brief Construct monitor with internally created HTTP client pool.
 */
HealthMonitor::HealthMonitor(const HealthMonitorConfig& config,
                             std::shared_ptr<MultiPrimaryCoordinator> primary_coordinator,
                             std::shared_ptr<ReplicaTopology> topology)
    : config_(config),
      primary_coordinator_(primary_coordinator),
      topology_(topology),
      last_failover_time_(std::chrono::steady_clock::time_point::min()) {
    // Create default HTTP client pool
    utils::HTTPClientPool::Config pool_config;
    // Convert timeout to seconds, ensuring minimum of 1 second
    auto timeout_seconds = std::max(
        std::chrono::seconds(1),
        std::chrono::duration_cast<std::chrono::seconds>(config_.health_check_timeout)
    );
    pool_config.connect_timeout = timeout_seconds;
    pool_config.request_timeout = timeout_seconds;
    pool_config.max_connections = 20;
    http_pool_ = std::make_shared<utils::HTTPClientPool>(pool_config);
}

/**
 * @brief Construct monitor with caller-provided HTTP client pool.
 */
HealthMonitor::HealthMonitor(const HealthMonitorConfig& config,
                             std::shared_ptr<MultiPrimaryCoordinator> primary_coordinator,
                             std::shared_ptr<ReplicaTopology> topology,
                             std::shared_ptr<utils::HTTPClientPool> http_pool)
    : config_(config),
      primary_coordinator_(primary_coordinator),
      topology_(topology),
      http_pool_(http_pool),
      last_failover_time_(std::chrono::steady_clock::time_point::min()) {
}

/**
 * @brief Construct monitor with custom HTTP pool and thread pool manager.
 */
HealthMonitor::HealthMonitor(const HealthMonitorConfig& config,
                             std::shared_ptr<MultiPrimaryCoordinator> primary_coordinator,
                             std::shared_ptr<ReplicaTopology> topology,
                             std::shared_ptr<utils::HTTPClientPool> http_pool,
                             std::shared_ptr<utils::ThreadPoolManager> thread_pool)
    : config_(config),
      primary_coordinator_(primary_coordinator),
      topology_(topology),
      http_pool_(http_pool),
      thread_pool_(thread_pool),
      last_failover_time_(std::chrono::steady_clock::time_point::min()) {
}

/** @brief Destructor; ensures monitoring loop is stopped. */
HealthMonitor::~HealthMonitor() {
    stop();
}

/** @brief Start monitoring loop using thread pool when available. */
void HealthMonitor::start() {
    if (running_.exchange(true)) {
        return;  // Already running
    }
    
    // If thread pool is available, use it to schedule health checks
    if (thread_pool_) {
        // Schedule periodic health check task on thread pool
        thread_pool_->submitTask(
            utils::ThreadPoolManager::PoolType::IO,
            [this]() { monitoringLoop(); },
            "HealthMonitor::monitoringLoop",
            utils::Task::Priority::NORMAL
        );
    } else {
        // Fallback to dedicated thread (backward compatibility)
        monitor_thread_ = std::thread([this]() {
            monitoringLoop();
        });
    }
}

/** @brief Stop monitoring loop and join fallback thread when needed. */
void HealthMonitor::stop() {
    if (!running_.exchange(false)) {
        return;  // Already stopped
    }
    
    // thread_join_no_timeout (W4): bounded join via joinThreadWithin
    if (!themis::utils::joinThreadWithin(monitor_thread_)) {
        THEMIS_WARN("[HealthMonitor] monitor thread did not finish within shutdown deadline; detaching.");
    }
}

/**
 * @brief Perform synchronous health check against node endpoint.
 * @param node_id Node identifier.
 * @param endpoint HTTP health endpoint.
 * @return Health-check result including status and response time.
 */
HealthCheckResult HealthMonitor::checkNodeHealth(const std::string& node_id, 
                                                 const std::string& endpoint) {
    total_health_checks_++;
    
    HealthCheckResult result;
    result.node_id = node_id;
    result.last_check = std::chrono::steady_clock::now();
    
    // Perform actual HTTP health check
    auto start = std::chrono::steady_clock::now();
    
    bool check_passed = performHealthCheck(endpoint);
    
    auto elapsed = std::chrono::steady_clock::now() - start;
    result.response_time = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
    
    if (!check_passed || result.response_time > config_.health_check_timeout) {
        result.status = HealthStatus::SUSPECT;
        if (result.response_time > config_.health_check_timeout) {
            result.error_message = "Health check timeout";
        } else {
            result.error_message = "Health check failed";
        }
        failed_health_checks_++;
    } else {
        result.status = HealthStatus::HEALTHY;
    }
    
    return result;
}

/** @brief Return status snapshots for all tracked nodes. */
std::map<std::string, HealthCheckResult> HealthMonitor::getAllHealthStatuses() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return health_statuses_;
}

/**
 * @brief Return health snapshot for a single node.
 * @param node_id Node identifier.
 * @return Optional status snapshot.
 */
std::optional<HealthCheckResult> HealthMonitor::getHealthStatus(const std::string& node_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = health_statuses_.find(node_id);
    if (it != health_statuses_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

/**
 * @brief Trigger manual failover and record event on success.
 * @param failed_node_id Failed source node.
 * @param promote_node_id Target node for promotion.
 * @return true when promotion succeeds.
 */
bool HealthMonitor::triggerManualFailover(const std::string& failed_node_id,
                                          const std::string& promote_node_id) {
    manual_failovers_++;
    
    // Mark failed node offline
    primary_coordinator_->markPrimaryOffline(failed_node_id);
    
    // Promote specified standby
    bool promoted = primary_coordinator_->promoteToPrimary(promote_node_id);
    
    if (promoted) {
        FailoverEvent event;
        event.failed_node_id = failed_node_id;
        event.promoted_node_id = promote_node_id;
        event.action = FailoverAction::PROMOTE_STANDBY;
        event.timestamp = std::chrono::steady_clock::now();
        event.reason = "Manual failover triggered";
        
        recordFailoverEvent(event);
    }
    
    return promoted;
}

/** @brief Enable/disable automatic failover policy. */
void HealthMonitor::setAutoFailoverEnabled([[maybe_unused]] bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.auto_failover_enabled = enabled;
}

/**
 * @brief Return most recent failover events up to max_events.
 * @param max_events Maximum number of events.
 * @return Failover event tail.
 */
std::vector<FailoverEvent> HealthMonitor::getFailoverHistory([[maybe_unused]] size_t max_events) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t count = std::min(max_events, failover_history_.size());
    return std::vector<FailoverEvent>(
        failover_history_.end() - count,
        failover_history_.end()
    );
}

/** @brief Return monitor statistics snapshot. */
HealthMonitor::Statistics HealthMonitor::getStatistics() const {
    Statistics stats;
    stats.total_health_checks = total_health_checks_.load();
    stats.failed_health_checks = failed_health_checks_.load();
    stats.auto_failovers_triggered = auto_failovers_.load();
    stats.manual_failovers_triggered = manual_failovers_.load();
    stats.last_failover_time = last_failover_time_;
    
    return stats;
}

// Private methods

/** @brief Periodic monitoring loop body. */
void HealthMonitor::monitoringLoop() {
    while (running_) {
        performHealthChecks();
        std::this_thread::sleep_for(config_.heartbeat_interval);
    }
}

/** @brief Execute one monitoring iteration across primaries and replicas. */
void HealthMonitor::performHealthChecks() {
    // Check all active primaries
    auto primaries = primary_coordinator_->getActivePrimaries();
    
    for (const auto& primary : primaries) {
        if (primary.endpoint.empty()) {
            continue;  // No endpoint configured
        }
        
        auto result = checkNodeHealth(primary.node_id, primary.endpoint);
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            
            // Get previous status if it exists
            auto it = health_statuses_.find(primary.node_id);
            HealthStatus previous_status = HealthStatus::HEALTHY;
            uint32_t prev_consecutive_failures = 0;
            uint32_t prev_consecutive_successes = 0;
            
            if (it != health_statuses_.end()) {
                previous_status = it->second.status;
                prev_consecutive_failures = it->second.consecutive_failures;
                prev_consecutive_successes = it->second.consecutive_successes;
            }
            
            // Update consecutive counters based on result
            if (result.status == HealthStatus::HEALTHY) {
                result.consecutive_failures = 0;
                result.consecutive_successes = prev_consecutive_successes + 1;
            } else {
                result.consecutive_failures = prev_consecutive_failures + 1;
                result.consecutive_successes = 0;
            }
            
            // State machine transitions
            if (previous_status == HealthStatus::HEALTHY && result.consecutive_failures >= 1) {
                result.status = HealthStatus::SUSPECT;
            } else if ((previous_status == HealthStatus::SUSPECT || previous_status == HealthStatus::HEALTHY) && 
                       result.consecutive_failures >= config_.max_consecutive_failures) {
                result.status = HealthStatus::DOWN;
            } else if (previous_status == HealthStatus::DOWN && result.consecutive_successes >= 1) {
                result.status = HealthStatus::RECOVERING;
            } else if (previous_status == HealthStatus::RECOVERING && 
                       result.consecutive_successes >= config_.successes_for_recovery) {
                result.status = HealthStatus::HEALTHY;
            } else if (previous_status == HealthStatus::RECOVERING && result.consecutive_failures >= 1) {
                // Failed during recovery, go back to DOWN
                result.status = HealthStatus::DOWN;
            } else {
                // Maintain previous status if no transition
                result.status = previous_status;
            }
            
            // Update health status
            health_statuses_[primary.node_id] = result;
            
            // Handle node failure if marked DOWN
            if (result.status == HealthStatus::DOWN && previous_status != HealthStatus::DOWN) {
                handleNodeFailure(primary.node_id);
            }
        }
        
        // Update primary coordinator heartbeat
        if (result.status == HealthStatus::HEALTHY) {
            primary_coordinator_->updateHeartbeat(primary.node_id, primary.last_known_lsn);
        }
    }
    
    // Check replicas (from topology)
    auto all_shards = topology_->getAllShards();
    for (const auto& shard_id : all_shards) {
        auto replica_set = topology_->getReplicaSet(shard_id);
        if (!replica_set) {
          continue;
        }
        
        for (const auto& replica_id : replica_set->replicas) {
            // In production: check replica health via HTTP
            // For now, mark all replicas as healthy (mock)
            HealthCheckResult result;
            result.node_id = replica_id;
            result.status = HealthStatus::HEALTHY;
            result.last_check = std::chrono::steady_clock::now();
            
            std::lock_guard<std::mutex> lock(mutex_);
            health_statuses_[replica_id] = result;
        }
    }
}

/** @brief Process node DOWN transition and optionally trigger auto-failover. */
void HealthMonitor::handleNodeFailure(const std::string& node_id) {
    if (!shouldTriggerFailover(node_id)) {
        return;  // Failover cooldown or disabled
    }
    
    // Mark primary offline
    primary_coordinator_->markPrimaryOffline(node_id);
    
    // Auto-promote standby if enabled
    if (config_.auto_promote_standby) {
        auto standby_node = selectStandbyForPromotion();
        if (standby_node) {
            bool promoted = primary_coordinator_->promoteToPrimary(*standby_node);
            
            if (promoted) {
                auto_failovers_++;
                
                FailoverEvent event;
                event.failed_node_id = node_id;
                event.promoted_node_id = *standby_node;
                event.action = FailoverAction::PROMOTE_STANDBY;
                event.timestamp = std::chrono::steady_clock::now();
                event.reason = "Auto-failover: primary health check failed";
                
                recordFailoverEvent(event);
            }
        }
    }
}

/** @brief Return whether automatic failover may execute now. */
bool HealthMonitor::shouldTriggerFailover([[maybe_unused]] const std::string& node_id) const {
    if (!config_.auto_failover_enabled) {
        return false;
    }
    
    // Check cooldown period
    auto now = std::chrono::steady_clock::now();
    auto time_since_last = now - last_failover_time_;
    
    if (time_since_last < config_.failover_cooldown) {
        return false;  // Too soon after last failover
    }
    
    return true;
}

/** @brief Select first healthy standby node eligible for promotion. */
std::optional<std::string> HealthMonitor::selectStandbyForPromotion() const {
    // Get all primaries
    auto all_primaries = primary_coordinator_->getActivePrimaries();
    
    // Filter for STANDBY nodes with healthy status
    std::vector<std::string> candidates = {};

    for (const auto& primary : all_primaries) {
        auto info = primary_coordinator_->getPrimaryInfo(primary.node_id);
        if (info && info->state == PrimaryState::STANDBY) {
            // Check if healthy
            auto health = getHealthStatus(primary.node_id);
            if (health && health->isHealthy()) {
                candidates.push_back(primary.node_id);
            }
        }
    }
    
    if (candidates.empty()) {
        return std::nullopt;
    }
    
    // Select first healthy standby (in production: use more sophisticated logic)
    return candidates[0];
}

/** @brief Record failover event and enforce bounded in-memory history size. */
void HealthMonitor::recordFailoverEvent(const FailoverEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    failover_history_.push_back(event);
    last_failover_time_ = event.timestamp;
    
    // Keep only last 100 events
    if (static_cast<int>(failover_history_.size()) > 100) {
        failover_history_.erase(failover_history_.begin());
    }
}

/** @brief Perform HTTP-based liveness check for one endpoint. */
bool HealthMonitor::performHealthCheck(const std::string& endpoint) {
    if (!http_pool_) {
        return false;  // No HTTP pool available
    }
    
    try {
        // Construct full URL: endpoint + health_check_path
        std::string url = endpoint;
        if (!url.empty() && url.back() == '/' && !config_.health_check_path.empty() && config_.health_check_path[0] == '/') {
            url = url.substr(0, url.length() - 1);  // Remove trailing slash
        }
        url += config_.health_check_path;
        
        // Ensure URL has protocol
        if (url.find("://") == std::string::npos) {
            url = "http://" + url;
        }
        
        // Perform HTTP GET request with timeout
        auto future = http_pool_->get(url);
        
        // Wait for response with timeout
        auto status = future.wait_for(config_.health_check_timeout);
        
        if (status == std::future_status::timeout) {
            return false;  // Timeout
        }
        
        auto response = future.get();
        
        // Check response status code
        // 200-299: Healthy
        // 500+: Unhealthy
        // Connection errors: Unhealthy (exception thrown)
        return response.isSuccess();
        
    } catch (...) {
        // Connection error, timeout, or other exception
        return false;
    }
}

} // namespace themis::sharding

