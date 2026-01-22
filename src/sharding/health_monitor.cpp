#include "sharding/health_monitor.h"
#include <algorithm>

// Note: In production, use real HTTP client (Boost.Asio, libcurl, etc.)
// For now, we simulate health checks

namespace themis::sharding {

HealthMonitor::HealthMonitor(const HealthMonitorConfig& config,
                             std::shared_ptr<MultiPrimaryCoordinator> primary_coordinator,
                             std::shared_ptr<ReplicaTopology> topology)
    : config_(config),
      primary_coordinator_(primary_coordinator),
      topology_(topology),
      last_failover_time_(std::chrono::steady_clock::time_point::min()) {
}

HealthMonitor::~HealthMonitor() {
    stop();
}

void HealthMonitor::start() {
    if (running_.exchange(true)) {
        return;  // Already running
    }
    
    monitor_thread_ = std::thread([this]() {
        monitoringLoop();
    });
}

void HealthMonitor::stop() {
    if (!running_.exchange(false)) {
        return;  // Already stopped
    }
    
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
}

HealthCheckResult HealthMonitor::checkNodeHealth(const std::string& node_id, 
                                                 const std::string& endpoint) {
    (void)endpoint;
    total_health_checks_++;
    
    HealthCheckResult result;
    result.node_id = node_id;
    result.last_check = std::chrono::steady_clock::now();
    
    // Simulate HTTP health check (in production: use real HTTP client)
    auto start = std::chrono::steady_clock::now();
    
    // TODO: Replace with actual HTTP GET to endpoint + config_.health_check_path
    // Example: GET http://primary1:8765/health
    // For now, simulate success
    bool check_passed = true;  // Simulate success
    
    auto elapsed = std::chrono::steady_clock::now() - start;
    result.response_time = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
    
    if (!check_passed || result.response_time > config_.health_check_timeout) {
        result.status = HealthStatus::SUSPECT;
        result.error_message = "Health check timeout or failed";
        failed_health_checks_++;
    } else {
        result.status = HealthStatus::HEALTHY;
    }
    
    return result;
}

std::map<std::string, HealthCheckResult> HealthMonitor::getAllHealthStatuses() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return health_statuses_;
}

std::optional<HealthCheckResult> HealthMonitor::getHealthStatus(const std::string& node_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = health_statuses_.find(node_id);
    if (it != health_statuses_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

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

void HealthMonitor::setAutoFailoverEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.auto_failover_enabled = enabled;
}

std::vector<FailoverEvent> HealthMonitor::getFailoverHistory(size_t max_events) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t count = std::min(max_events, failover_history_.size());
    return std::vector<FailoverEvent>(
        failover_history_.end() - count,
        failover_history_.end()
    );
}

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

void HealthMonitor::monitoringLoop() {
    while (running_) {
        performHealthChecks();
        std::this_thread::sleep_for(config_.heartbeat_interval);
    }
}

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
            
            // Update consecutive failures
            if (auto it = health_statuses_.find(primary.node_id); it != health_statuses_.end()) {
                if (result.status != HealthStatus::HEALTHY) {
                    result.consecutive_failures = it->second.consecutive_failures + 1;
                } else {
                    result.consecutive_failures = 0;
                }
            }
            
            // Update health status
            health_statuses_[primary.node_id] = result;
            
            // Check if node should be marked DOWN
            if (result.consecutive_failures >= config_.max_consecutive_failures) {
                result.status = HealthStatus::DOWN;
                health_statuses_[primary.node_id] = result;
                
                // Handle failure (may trigger auto-failover)
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
        if (!replica_set) continue;
        
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

bool HealthMonitor::shouldTriggerFailover(const std::string& node_id) const {
    (void)node_id;
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

std::optional<std::string> HealthMonitor::selectStandbyForPromotion() const {
    // Get all primaries
    auto all_primaries = primary_coordinator_->getActivePrimaries();
    
    // Filter for STANDBY nodes with healthy status
    std::vector<std::string> candidates;
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

void HealthMonitor::recordFailoverEvent(const FailoverEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    failover_history_.push_back(event);
    last_failover_time_ = event.timestamp;
    
    // Keep only last 100 events
    if (failover_history_.size() > 100) {
        failover_history_.erase(failover_history_.begin());
    }
}

} // namespace themis::sharding
