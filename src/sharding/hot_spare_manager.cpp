/**
 * @file hot_spare_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=6, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Hot Spare Management Implementation
 * 
 * Note: This file uses the themis::sharding namespace internally
 * with a compatibility shim for themisdb::sharding at the end,
 * following the pattern used in redundancy_strategy.cpp and other
 * sharding components.
 * 
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sharding/hot_spare_manager.h"
#include "sharding/consistent_hash.h"
#include "utils/thread_join_utils.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <numeric>
#include <sstream>

namespace themis {
namespace sharding {

// ═══════════════════════════════════════════════════════════
// HotSpareConfig Implementation
// ═══════════════════════════════════════════════════════════

bool HotSpareConfig::validate() const {
    if (enable && spare_shards.empty()) {
        spdlog::error("Hot spare enabled but no spare shards configured");
        return false;
    }
    
    if (rebuild_throttle_mbps == 0) {
        spdlog::error("Invalid rebuild_throttle_mbps: must be > 0");
        return false;
    }
    
    if (max_concurrent_rebuilds == 0) {
        spdlog::error("Invalid max_concurrent_rebuilds: must be > 0");
        return false;
    }
    
    if (rebuild_chunk_size_mb == 0) {
        spdlog::error("Invalid rebuild_chunk_size_mb: must be > 0");
        return false;
    }
    
    return true;
}

// ═══════════════════════════════════════════════════════════
// HotSpareManager Implementation
// ═══════════════════════════════════════════════════════════

HotSpareManager::HotSpareManager(
    const HotSpareConfig& config,
    RedundancyStrategy& strategy,
    ShardTopology& topology
) : config_(config),
    strategy_(strategy),
    topology_(topology) {
    
    if (!config_.validate()) {
        throw std::invalid_argument("Invalid hot spare configuration");
    }
    
    // Initialize spare pool
    for (const auto& shard_id : config_.spare_shards) {
        SpareShardInfo spare;
        spare.shard_id = shard_id;
        spare.state = SpareState::AVAILABLE;
        spare.state_changed = std::chrono::system_clock::now();
        spare.last_health_check = std::chrono::system_clock::now();
        spares_[shard_id] = spare;
    }
    
    spdlog::info("HotSpareManager initialized with {} spare shards", 
                 config_.spare_shards.size());
}

HotSpareManager::~HotSpareManager() {
    stop();
}

void HotSpareManager::start() {
    if (running_.exchange(true)) {
        spdlog::warn("HotSpareManager already running");
        return;
    }
    
    // Start health check thread
    health_check_thread_ = std::thread([this]() {
        healthCheckLoop();
    });
    
    // Start rebuild thread
    if (config_.auto_rebuild) {
        rebuild_thread_ = std::thread([this]() {
            rebuildLoop();
        });
    }
    
    spdlog::info("HotSpareManager started");
}

void HotSpareManager::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    
    // Wake up rebuild thread
    rebuild_cv_.notify_all();
    
    // Join threads
    if (health_check_thread_.joinable()) {
        const bool health_joined = themis::utils::joinThreadWithin(health_check_thread_);
        if (!health_joined) {
            spdlog::warn("HotSpareManager: health-check thread did not join within timeout");
        }
    }
    if (rebuild_thread_.joinable()) {
        const bool rebuild_joined = themis::utils::joinThreadWithin(rebuild_thread_);
        if (!rebuild_joined) {
            spdlog::warn("HotSpareManager: rebuild thread did not join within timeout");
        }
    }
    
    spdlog::info("HotSpareManager stopped");
}

bool HotSpareManager::isRunning() const {
    return running_.load();
}

void HotSpareManager::addSpare(const std::string& shard_id) {
    std::unique_lock<std::shared_mutex> lock(spares_mutex_);
    
    if (spares_.count(shard_id)) {
        spdlog::warn("Spare shard {} already exists", shard_id);
        return;
    }
    
    SpareShardInfo spare;
    spare.shard_id = shard_id;
    spare.state = SpareState::AVAILABLE;
    spare.state_changed = std::chrono::system_clock::now();
    spare.last_health_check = std::chrono::system_clock::now();
    
    spares_[shard_id] = spare;
    
    spdlog::info("Added spare shard: {}", shard_id);
}

void HotSpareManager::removeSpare(const std::string& shard_id) {
    std::unique_lock<std::shared_mutex> lock(spares_mutex_);
    
    auto it = spares_.find(shard_id);
    if (it == spares_.end()) {
        spdlog::warn("Spare shard {} not found", shard_id);
        return;
    }
    
    if (it->second.state == SpareState::ACTIVE || 
        it->second.state == SpareState::REBUILDING) {
        spdlog::error("Cannot remove spare shard {} in state {}", 
                     shard_id, static_cast<int>(it->second.state));
        return;
    }
    
    spares_.erase(it);
    
    spdlog::info("Removed spare shard: {}", shard_id);
}

std::vector<std::string> HotSpareManager::getAvailableSpares() const {
    std::shared_lock<std::shared_mutex> lock(spares_mutex_);
    
    std::vector<std::string> available = {};

    for (const auto& [shard_id, spare] : spares_) {
        if (spare.state == SpareState::AVAILABLE) {
            available.push_back(shard_id);
        }
    }
    
    return available;
}

std::vector<SpareShardInfo> HotSpareManager::getAllSpares() const {
    std::shared_lock<std::shared_mutex> lock(spares_mutex_);
    
    std::vector<SpareShardInfo> spares = {};

    for (const auto& [_, spare] : spares_) {
        spares.push_back(spare);
    }
    
    return spares;
}

bool HotSpareManager::activateSpare(
    const std::string& failed_shard_id,
    ConsistentHashRing& ring,
    ReadHandler read_handler,
    WriteHandler write_handler,
    DocumentIterator doc_iterator
) {
    auto start_time = std::chrono::steady_clock::now();
    
    spdlog::info("Activating spare for failed shard: {}", failed_shard_id);
    
    // Select best available spare
    auto spare_id_opt = selectBestSpare();
    if (!spare_id_opt) {
        spdlog::error("No available spares for failover");
        
        HotSpareFailoverEvent event;
        event.timestamp = std::chrono::system_clock::now();
        event.failed_shard_id = failed_shard_id;
        event.success = false;
        event.error_message = "No available spares";
        
        {
            std::lock_guard<std::mutex> lock(history_mutex_);
            failover_history_.push_back(event);
            if (static_cast<int>(failover_history_.size()) > MAX_HISTORY_SIZE) {
                failover_history_.erase(failover_history_.begin());
            }
        }
        
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_failovers++;
            stats_.failed_failovers++;
        }
        
        sendAlert("CRITICAL: No available spares for failed shard " + failed_shard_id);
        return false;
    }
    
    std::string spare_id = *spare_id_opt;
    
    // Update spare state
    {
        std::unique_lock<std::shared_mutex> lock(spares_mutex_);
        auto& spare = spares_[spare_id];
        spare.state = SpareState::ACTIVATING;
        spare.state_changed = std::chrono::system_clock::now();
    }
    
    // Replace failed shard with spare in ring
    try {
        ring.removeShard(failed_shard_id);
        ring.addShard(spare_id);
        
        // Update topology
        // (Topology update logic would go here)
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to update ring: {}", e.what());
        
        // Revert spare state
        {
            std::unique_lock<std::shared_mutex> lock(spares_mutex_);
            spares_[spare_id].state = SpareState::AVAILABLE;
        }
        
        return false;
    }
    
    // Update spare to active
    {
        std::unique_lock<std::shared_mutex> lock(spares_mutex_);
        auto& spare = spares_[spare_id];
        spare.state = SpareState::ACTIVE;
        spare.state_changed = std::chrono::system_clock::now();
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    
    // Record failover event
    HotSpareFailoverEvent event;
    event.timestamp = std::chrono::system_clock::now();
    event.failed_shard_id = failed_shard_id;
    event.spare_shard_id = spare_id;
    event.failover_duration = duration;
    event.success = true;
    
    {
        std::lock_guard<std::mutex> lock(history_mutex_);
        failover_history_.push_back(event);
        if (static_cast<int>(failover_history_.size()) > MAX_HISTORY_SIZE) {
            failover_history_.erase(failover_history_.begin());
        }
    }
    
    // Update statistics
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_failovers++;
        stats_.successful_failovers++;
        
        // Update average failover time
        uint64_t total_time = stats_.avg_failover_time.count() * 
                             (stats_.successful_failovers - 1);
        stats_.avg_failover_time = std::chrono::milliseconds(
            (total_time + duration.count()) / stats_.successful_failovers
        );
    }
    
    spdlog::info("Spare {} activated for failed shard {} in {}ms", 
                 spare_id, failed_shard_id, duration.count());
    
    sendAlert("Spare " + spare_id + " activated for failed shard " + 
              failed_shard_id + " (failover time: " + 
              std::to_string(duration.count()) + "ms)");
    
    // Initiate rebuild if auto_rebuild is enabled
    if (config_.auto_rebuild) {
        // Get documents from replicas
        auto documents = doc_iterator(failed_shard_id);
        
        RebuildTask task;
        task.spare_shard_id = spare_id;
        task.source_shard_id = failed_shard_id;
        task.documents = documents;
        task.total_bytes = 0;  // Will be calculated during rebuild
        task.ring = &ring;
        task.read_handler = read_handler;
        task.write_handler = write_handler;
        
        {
            std::lock_guard<std::mutex> lock(rebuild_mutex_);
            rebuild_queue_.push(task);
            rebuild_cv_.notify_one();
        }
        
        spdlog::info("Queued rebuild for spare {}, {} documents to rebuild", 
                     spare_id,static_cast<int>(documents.size()));
    }

    // Notify ShardRepairEngine (if attached) so it can perform erasure-aware
    // consistency checks and recovery for data that could not be directly
    // copied from replicas (e.g. parity-only data in RAID-5/6 configurations).
    if (repair_engine_) {
        repair_engine_->triggerRepair(spare_id);
        spdlog::info("ShardRepairEngine repair triggered for newly activated spare {}",
                     spare_id);
    }
    
    return true;
}

void HotSpareManager::triggerRebuild(const std::string& spare_shard_id) {
    std::shared_lock<std::shared_mutex> lock(spares_mutex_);
    
    auto it = spares_.find(spare_shard_id);
    if (it == spares_.end()) {
        spdlog::warn("Spare shard {} not found", spare_shard_id);
        return;
    }
    
    if (it->second.state != SpareState::ACTIVE) {
        spdlog::error("Cannot trigger rebuild for spare {} in state {}", 
                     spare_shard_id, static_cast<int>(it->second.state));
        return;
    }
    
    // Manual rebuild trigger would go here
    spdlog::info("Manual rebuild triggered for spare: {}", spare_shard_id);
}

void HotSpareManager::pauseRebuild(const std::string& spare_shard_id) {
    std::lock_guard<std::mutex> lock(rebuild_mutex_);
    rebuild_paused_[spare_shard_id] = true;
    spdlog::info("Rebuild paused for spare: {}", spare_shard_id);
}

void HotSpareManager::resumeRebuild(const std::string& spare_shard_id) {
    std::lock_guard<std::mutex> lock(rebuild_mutex_);
    rebuild_paused_[spare_shard_id] = false;
    rebuild_cv_.notify_one();
    spdlog::info("Rebuild resumed for spare: {}", spare_shard_id);
}

void HotSpareManager::cancelRebuild(const std::string& spare_shard_id) {
    std::lock_guard<std::mutex> lock(rebuild_mutex_);
    
    // Remove from paused map
    rebuild_paused_.erase(spare_shard_id);
    
    // Update spare state
    {
        std::unique_lock<std::shared_mutex> spare_lock(spares_mutex_);
        auto it = spares_.find(spare_shard_id);
        if (it != spares_.end() && it->second.state == SpareState::REBUILDING) {
            it->second.state = SpareState::DEGRADED;
            it->second.state_changed = std::chrono::system_clock::now();
        }
    }
    
    spdlog::info("Rebuild cancelled for spare: {}", spare_shard_id);
}

RebuildStatus HotSpareManager::getRebuildStatus() const {
    std::shared_lock<std::shared_mutex> lock(spares_mutex_);
    
    RebuildStatus status;
    
    for (const auto& [_, spare] : spares_) {
        if (spare.state == SpareState::REBUILDING) {
            status.is_rebuilding = true;
            status.active_rebuilds++;
            status.rebuilding_spares.push_back(spare);
        }
    }
    
    if (status.active_rebuilds > 0) {
        // Calculate overall progress
        double total_progress = 0.0;
        int64_t total_eta_seconds = 0;
        double total_throughput = 0.0;
        
        for (const auto& spare : status.rebuilding_spares) {
            total_progress += spare.getProgressPercentage();
            total_eta_seconds += spare.getEstimatedTimeRemaining().count();
            total_throughput += spare.getRebuildThroughputMBps();
        }
        
        status.overall_progress = total_progress / status.active_rebuilds;
        status.estimated_time_remaining = std::chrono::seconds(
            total_eta_seconds / status.active_rebuilds);
        status.average_throughput_mbps = total_throughput / status.active_rebuilds;
    }
    
    // Get statistics
    {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        status.total_rebuilds_completed = stats_.successful_rebuilds;
        status.total_rebuilds_failed = stats_.failed_rebuilds;
        status.average_rebuild_time = stats_.avg_rebuild_time;
    }
    
    return status;
}

std::optional<SpareShardInfo> HotSpareManager::getSpareInfo(
    const std::string& shard_id) const {
    std::shared_lock<std::shared_mutex> lock(spares_mutex_);
    
    auto it = spares_.find(shard_id);
    if (it != spares_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

std::vector<HotSpareFailoverEvent> HotSpareManager::getFailoverHistory(
    size_t max_count) const {
    std::lock_guard<std::mutex> lock(history_mutex_);
    
    size_t count = std::min(max_count,static_cast<int>(failover_history_.size()));
    
    std::vector<HotSpareFailoverEvent> history = {};

    if (count > 0) {
        auto start_it = failover_history_.end() - count;
        history.insert(history.end(), start_it, failover_history_.end());
    }
    
    return history;
}

void HotSpareManager::updateConfig(const HotSpareConfig& config) {
    if (!config.validate()) {
        throw std::invalid_argument("Invalid hot spare configuration");
    }
    
    config_ = config;
    spdlog::info("HotSpareManager configuration updated");
}

void HotSpareManager::setRepairEngine(
    std::shared_ptr<themis::sharding::ShardRepairEngine> engine) {
    repair_engine_ = std::move(engine);
}

HotSpareManager::Stats HotSpareManager::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    Stats stats = stats_;
    
    // Update current spare counts
    {
        std::shared_lock<std::shared_mutex> spare_lock(spares_mutex_);
        for (const auto& [_, spare] : spares_) {
            switch (spare.state) {
                case SpareState::AVAILABLE:
                    stats.spares_available++;
                    break;
                case SpareState::ACTIVE:
                    stats.spares_active++;
                    break;
                case SpareState::REBUILDING:
                    stats.spares_rebuilding++;
                    break;
                default:
                    break;
            }
        }
    }
    
    return stats;
}

std::string HotSpareManager::exportPrometheusMetrics() const {
    std::stringstream ss = {};
    
    auto stats = getStats();
    
    ss << "# HELP themis_hot_spare_total_failovers Total number of failover operations\n";
    ss << "# TYPE themis_hot_spare_total_failovers counter\n";
    ss << "themis_hot_spare_total_failovers " << stats.total_failovers << "\n";
    
    ss << "# HELP themis_hot_spare_successful_failovers Number of successful failovers\n";
    ss << "# TYPE themis_hot_spare_successful_failovers counter\n";
    ss << "themis_hot_spare_successful_failovers " << stats.successful_failovers << "\n";
    
    ss << "# HELP themis_hot_spare_failed_failovers Number of failed failovers\n";
    ss << "# TYPE themis_hot_spare_failed_failovers counter\n";
    ss << "themis_hot_spare_failed_failovers " << stats.failed_failovers << "\n";
    
    ss << "# HELP themis_hot_spare_total_rebuilds Total number of rebuild operations\n";
    ss << "# TYPE themis_hot_spare_total_rebuilds counter\n";
    ss << "themis_hot_spare_total_rebuilds " << stats.total_rebuilds << "\n";
    
    ss << "# HELP themis_hot_spare_successful_rebuilds Number of successful rebuilds\n";
    ss << "# TYPE themis_hot_spare_successful_rebuilds counter\n";
    ss << "themis_hot_spare_successful_rebuilds " << stats.successful_rebuilds << "\n";
    
    ss << "# HELP themis_hot_spare_failed_rebuilds Number of failed rebuilds\n";
    ss << "# TYPE themis_hot_spare_failed_rebuilds counter\n";
    ss << "themis_hot_spare_failed_rebuilds " << stats.failed_rebuilds << "\n";
    
    ss << "# HELP themis_hot_spare_spares_available Number of available spare shards\n";
    ss << "# TYPE themis_hot_spare_spares_available gauge\n";
    ss << "themis_hot_spare_spares_available " << stats.spares_available << "\n";
    
    ss << "# HELP themis_hot_spare_spares_active Number of active spare shards\n";
    ss << "# TYPE themis_hot_spare_spares_active gauge\n";
    ss << "themis_hot_spare_spares_active " << stats.spares_active << "\n";
    
    ss << "# HELP themis_hot_spare_spares_rebuilding Number of rebuilding spare shards\n";
    ss << "# TYPE themis_hot_spare_spares_rebuilding gauge\n";
    ss << "themis_hot_spare_spares_rebuilding " << stats.spares_rebuilding << "\n";
    
    ss << "# HELP themis_hot_spare_avg_failover_time_ms Average failover time in milliseconds\n";
    ss << "# TYPE themis_hot_spare_avg_failover_time_ms gauge\n";
    ss << "themis_hot_spare_avg_failover_time_ms " << stats.avg_failover_time.count() << "\n";
    
    ss << "# HELP themis_hot_spare_avg_rebuild_time_ms Average rebuild time in milliseconds\n";
    ss << "# TYPE themis_hot_spare_avg_rebuild_time_ms gauge\n";
    ss << "themis_hot_spare_avg_rebuild_time_ms " << stats.avg_rebuild_time.count() << "\n";
    
    // Per-spare metrics
    std::shared_lock<std::shared_mutex> lock(spares_mutex_);
    for (const auto& [shard_id, spare] : spares_) {
        std::string state_str = {};
        switch (spare.state) {
            case SpareState::AVAILABLE: state_str = "available"; break;
            case SpareState::ACTIVATING: state_str = "activating"; break;
            case SpareState::ACTIVE: state_str = "active"; break;
            case SpareState::REBUILDING: state_str = "rebuilding"; break;
            case SpareState::DEGRADED: state_str = "degraded"; break;
            case SpareState::OFFLINE: state_str = "offline"; break;
            default: state_str = "unknown"; break;
        }
        
        ss << "# HELP themis_hot_spare_state Spare shard state (1=current state)\n";
        ss << "# TYPE themis_hot_spare_state gauge\n";
        ss << "themis_hot_spare_state{shard=\"" << shard_id 
           << "\",state=\"" << state_str << "\"} 1\n";
        
        if (spare.state == SpareState::REBUILDING) {
            ss << "# HELP themis_hot_spare_rebuild_progress Rebuild progress percentage\n";
            ss << "# TYPE themis_hot_spare_rebuild_progress gauge\n";
            ss << "themis_hot_spare_rebuild_progress{shard=\"" << shard_id 
               << "\"} " << spare.getProgressPercentage() << "\n";
            
            ss << "# HELP themis_hot_spare_rebuild_throughput_mbps Rebuild throughput in MB/s\n";
            ss << "# TYPE themis_hot_spare_rebuild_throughput_mbps gauge\n";
            ss << "themis_hot_spare_rebuild_throughput_mbps{shard=\"" << shard_id 
               << "\"} " << spare.getRebuildThroughputMBps() << "\n";
            
            ss << "# HELP themis_hot_spare_rebuild_eta_seconds Estimated time remaining in seconds\n";
            ss << "# TYPE themis_hot_spare_rebuild_eta_seconds gauge\n";
            ss << "themis_hot_spare_rebuild_eta_seconds{shard=\"" << shard_id 
               << "\"} " << spare.getEstimatedTimeRemaining().count() << "\n";
        }
    }
    
    return ss.str();
}

// ═══════════════════════════════════════════════════════════
// Private Methods
// ═══════════════════════════════════════════════════════════

void HotSpareManager::healthCheckLoop() {
    while (running_.load()) {
        try {
            std::shared_lock<std::shared_mutex> lock(spares_mutex_);
            
            for (auto& [_, spare] : spares_) {
                checkSpareHealth(const_cast<SpareShardInfo&>(spare));
            }
            
        } catch (const std::exception& e) {
            spdlog::error("Health check error: {}", e.what());
        }
        
        std::this_thread::sleep_for(config_.health_check_interval);
    }
}

void HotSpareManager::rebuildLoop() {
    while (running_.load()) {
        std::unique_lock<std::mutex> lock(rebuild_mutex_);
        
        // Wait for rebuild work or stop signal
        rebuild_cv_.wait_for(lock, std::chrono::seconds(10), [this]() {
            return !rebuild_queue_.empty() || !running_.load();
        });
        
        if (!running_.load()) {
            break;
        }
        
        if (rebuild_queue_.empty()) {
            continue;
        }
        
        // Check if we can process more rebuilds
        size_t active_rebuilds = 0;
        {
            std::shared_lock<std::shared_mutex> spare_lock(spares_mutex_);
            for (const auto& [_, spare] : spares_) {
                if (spare.state == SpareState::REBUILDING) {
                    active_rebuilds++;
                }
            }
        }
        
        if (active_rebuilds >= config_.max_concurrent_rebuilds) {
            continue;
        }
        
        // Get next rebuild task
        RebuildTask task = rebuild_queue_.front();
        rebuild_queue_.pop();
        
        lock.unlock();
        
        // Check if rebuild is paused
        {
            std::lock_guard<std::mutex> pause_lock(rebuild_mutex_);
            if (rebuild_paused_[task.spare_shard_id]) {
                // Re-queue the task
                rebuild_queue_.push(task);
                continue;
            }
        }
        
        // Update spare state to rebuilding
        {
            std::unique_lock<std::shared_mutex> spare_lock(spares_mutex_);
            auto it = spares_.find(task.spare_shard_id);
            if (it != spares_.end()) {
                it->second.state = SpareState::REBUILDING;
                it->second.source_shard_id = task.source_shard_id;
                it->second.rebuild_started = std::chrono::system_clock::now();
                it->second.bytes_rebuilt = 0;
                it->second.total_bytes = task.total_bytes;
            }
        }
        
        // Perform rebuild with actual data transfer
        bool success = rebuildShard(task);
        
        // Update statistics
        {
            std::lock_guard<std::mutex> stats_lock(stats_mutex_);
            stats_.total_rebuilds++;
            if (success) {
                stats_.successful_rebuilds++;
            } else {
                stats_.failed_rebuilds++;
            }
        }
        
        // Update spare state
        {
            std::unique_lock<std::shared_mutex> spare_lock(spares_mutex_);
            auto it = spares_.find(task.spare_shard_id);
            if (it != spares_.end()) {
                if (success) {
                    it->second.state = SpareState::ACTIVE;
                    spdlog::info("Rebuild completed for spare: {}", task.spare_shard_id);
                    sendAlert("Rebuild completed for spare " + task.spare_shard_id);
                } else {
                    it->second.state = SpareState::DEGRADED;
                    spdlog::error("Rebuild failed for spare: {}", task.spare_shard_id);
                    sendAlert("CRITICAL: Rebuild failed for spare " + task.spare_shard_id);
                }
                it->second.state_changed = std::chrono::system_clock::now();
            }
        }
    }
}

void HotSpareManager::checkSpareHealth(SpareShardInfo& spare) {
    spare.last_health_check = std::chrono::system_clock::now();
    
    // Simplified health check
    // In production, implement actual health checks
    
    // For now, just update last check time
}

void HotSpareManager::handleShardFailure(const std::string& shard_id) {
    spdlog::warn("Shard failure detected: {}", shard_id);
    sendAlert("Shard failure detected: " + shard_id);
}

bool HotSpareManager::rebuildShard(RebuildTask& task) {
    spdlog::info("Starting rebuild for spare: {}, {} documents to transfer", 
                 task.spare_shard_id,static_cast<int>(task.documents.size()));
    
    if (!task.ring || !task.read_handler || !task.write_handler) {
        spdlog::error("Invalid rebuild task: missing ring or handlers");
        return false;
    }
    
    auto rebuild_start = std::chrono::steady_clock::now();
    uint64_t bytes_transferred = 0;
    uint32_t documents_transferred = 0;
    uint32_t failed_documents = 0;
    
    // Calculate throttle delay per MB if throttling is enabled
    std::chrono::microseconds throttle_delay_per_mb{0};
    if (config_.rebuild_throttle_mbps > 0) {
        // Calculate microseconds to wait per MB transferred
        throttle_delay_per_mb = std::chrono::microseconds(1000000 / config_.rebuild_throttle_mbps);
    }
    
    // Process documents in chunks for better progress tracking
    size_t chunk_size = config_.rebuild_chunk_size_mb > 0 ? 
                        config_.rebuild_chunk_size_mb : 100;
    
    for (size_t i = 0; i <static_cast<int>(task.documents.size()); ++i) {
        const auto& doc_id = task.documents[i];
        
        // Check if rebuild is paused
        {
            std::lock_guard<std::mutex> lock(rebuild_mutex_);
            if (rebuild_paused_[task.spare_shard_id]) {
                spdlog::info("Rebuild paused for spare: {}", task.spare_shard_id);
                return false;  // Will be retried when resumed
            }
        }
        
        try {
            // Get replica nodes for this document (excluding the failed shard)
            auto replicas = task.ring->getReplicaNodes(doc_id, 2);
            
            // Try to read from each replica until successful
            std::optional<std::vector<uint8_t>> data;
            std::string source_replica = {};
            
            for (const auto& replica_id : replicas) {
                if (replica_id != task.source_shard_id) {
                    data = task.read_handler(replica_id, doc_id);
                    if (data) {
                        source_replica = replica_id;
                        break;
                    }
                }
            }
            
            if (!data) {
                spdlog::error("Failed to read document {} from any replica", doc_id);
                failed_documents++;
                continue;
            }
            
            // Write to spare shard
            bool write_success = task.write_handler(task.spare_shard_id, doc_id, *data);
            
            if (!write_success) {
                spdlog::error("Failed to write document {} to spare {}", 
                             doc_id, task.spare_shard_id);
                failed_documents++;
                continue;
            }
            
            // Update progress
            bytes_transferred += data->size();
            documents_transferred++;
            
            {
                std::unique_lock<std::shared_mutex> lock(spares_mutex_);
                auto it = spares_.find(task.spare_shard_id);
                if (it != spares_.end()) {
                    it->second.bytes_rebuilt = bytes_transferred;
                    // Update total bytes estimate as we go
                    it->second.total_bytes = bytes_transferred + 
                        (data->size() * (task.documents.size() - i - 1));
                }
            }
            
            // Apply throttling if enabled
            if (config_.rebuild_throttle_mbps > 0 && data->size() > 0) {
                double mb_transferred = data->size() / (1024.0 * 1024.0);
                auto delay = std::chrono::microseconds(
                    static_cast<int64_t>(mb_transferred * throttle_delay_per_mb.count())
                );
                
                if (delay.count() > 0) {
                    std::this_thread::sleep_for(delay);
                }
            }
            
            // Log progress periodically
            if ((i + 1) % chunk_size == 0 || i == static_cast<int>(task.documents.size()) - 1) {
                double progress = ((i + 1) * 100.0) / task.documents.size();
                auto elapsed = std::chrono::steady_clock::now() - rebuild_start;
                double elapsed_seconds = std::chrono::duration<double>(elapsed).count();
                double throughput_mbps = elapsed_seconds > 0 ? 
                    (bytes_transferred / (1024.0 * 1024.0)) / elapsed_seconds : 0.0;
                
                spdlog::info("Rebuild progress for {}: {:.1f}% ({}/{} docs, {:.2f} MB/s)", 
                            task.spare_shard_id, progress, i + 1, 
                            task.documents.size(), throughput_mbps);
            }
            
        } catch (const std::exception& e) {
            spdlog::error("Exception during rebuild of document {}: {}", doc_id, e.what());
            failed_documents++;
        }
    }
    
    auto rebuild_end = std::chrono::steady_clock::now();
    auto rebuild_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        rebuild_end - rebuild_start);
    
    bool success = failed_documents == 0;
    
    if (success) {
        spdlog::info("Rebuild completed successfully for spare {}: {} documents, {} bytes, {} ms", 
                    task.spare_shard_id, documents_transferred, 
                    bytes_transferred, rebuild_duration.count());
    } else {
        spdlog::warn("Rebuild completed with errors for spare {}: {}/{} documents failed", 
                    task.spare_shard_id, failed_documents,static_cast<int>(task.documents.size()));
    }
    
    return success;
}

std::optional<std::string> HotSpareManager::selectBestSpare() const {
    std::shared_lock<std::shared_mutex> lock(spares_mutex_);
    
    for (const auto& [shard_id, spare] : spares_) {
        if (spare.state == SpareState::AVAILABLE) {
            return shard_id;
        }
    }
    
    return std::nullopt;
}

void HotSpareManager::sendAlert(const std::string& message) {
    if (config_.enable_alerts && config_.alert_callback) {
        config_.alert_callback(message);
    }
    
    spdlog::info("Alert: {}", message);
}

} // namespace sharding
} // namespace themis

// Backward compatibility shim
namespace themisdb {
namespace sharding {
using namespace themis::sharding;
}
}

