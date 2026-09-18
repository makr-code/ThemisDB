/**
 * @file multi_primary_coordinator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/multi_primary_coordinator.h"
#include <algorithm>

namespace themis::sharding {

MultiPrimaryCoordinator::MultiPrimaryCoordinator(const MultiPrimaryConfig& config)
    : config_(config) {
    
    // Register all configured primaries as STANDBY
    for (const auto& node_id : config_.primary_node_ids) {
        PrimaryNodeInfo info;
        info.node_id = node_id;
        info.state = PrimaryState::STANDBY;
        info.is_current = (node_id == config_.current_node_id);
        info.last_heartbeat = std::chrono::steady_clock::now();
        
        if (config_.primary_endpoints.count(node_id)) {
            info.endpoint = config_.primary_endpoints.at(node_id);
        }
        
        primaries_[node_id] = info;
    }
    
    // Promote current node to ACTIVE by default
    if (primaries_.count(config_.current_node_id)) {
        primaries_[config_.current_node_id].state = PrimaryState::ACTIVE;
    }
}

MultiPrimaryCoordinator::~MultiPrimaryCoordinator() = default;

/**
 * @brief Register Primary.
 * @param[in] node_id Identifier of the node.
 * @param[in] endpoint Input parameter.
 * @details Calls: lock(), std::chrono::steady_clock::now().
 */
void MultiPrimaryCoordinator::registerPrimary(const std::string& node_id, const std::string& endpoint) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    PrimaryNodeInfo info;
    info.node_id = node_id;
    info.endpoint = endpoint;
    info.state = PrimaryState::STANDBY;
    info.is_current = (node_id == config_.current_node_id);
    info.last_heartbeat = std::chrono::steady_clock::now();
    
    primaries_[node_id] = info;
}

/**
 * @brief Promote To Primary.
 * @param[in] node_id Identifier of the node.
 * @return True when the operation succeeds.
 * @details Calls: lock(), find(), end(), std::chrono::steady_clock::now().
 */
bool MultiPrimaryCoordinator::promoteToPrimary(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = primaries_.find(node_id);
    if (it == primaries_.end()) {
        return false;  // Unknown node
    }
    
    // Can only promote STANDBY or DEGRADED nodes
    if (it->second.state != PrimaryState::STANDBY && 
        it->second.state != PrimaryState::DEGRADED) {
        return false;
    }
    
    it->second.state = PrimaryState::ACTIVE;
    it->second.last_heartbeat = std::chrono::steady_clock::now();
    
    return true;
}

/**
 * @brief Demote To Standby.
 * @param[in] node_id Identifier of the node.
 * @return True when the operation succeeds.
 * @details Calls: lock(), find(), end().
 */
bool MultiPrimaryCoordinator::demoteToStandby(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = primaries_.find(node_id);
    if (it == primaries_.end()) {
        return false;
    }
    
    // Can demote ACTIVE or DEGRADED
    if (it->second.state == PrimaryState::ACTIVE || 
        it->second.state == PrimaryState::DEGRADED) {
        it->second.state = PrimaryState::STANDBY;
        return true;
    }
    
    return false;
}

/**
 * @brief Mark Primary Offline.
 * @param[in] node_id Identifier of the node.
 * @details Calls: lock(), find(), end().
 */
void MultiPrimaryCoordinator::markPrimaryOffline(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = primaries_.find(node_id);
    if (it != primaries_.end()) {
        it->second.state = PrimaryState::OFFLINE;
    }
}

/**
 * @brief Update Heartbeat.
 * @param[in] node_id Identifier of the node.
 * @param[in] current_lsn Input parameter.
 * @details Calls: lock(), find(), end(), std::chrono::steady_clock::now().
 */
void MultiPrimaryCoordinator::updateHeartbeat(const std::string& node_id, const LSN& current_lsn) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = primaries_.find(node_id);
    if (it != primaries_.end()) {
        it->second.last_heartbeat = std::chrono::steady_clock::now();
        it->second.last_known_lsn = current_lsn;
        
        // If node was offline, bring it back to STANDBY
        if (it->second.state == PrimaryState::OFFLINE) {
            it->second.state = PrimaryState::STANDBY;
        }
    }
}

std::vector<PrimaryNodeInfo> MultiPrimaryCoordinator::getActivePrimaries() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<PrimaryNodeInfo> result = {};

    for (const auto& [node_id, info] : primaries_) {
        if (info.state == PrimaryState::ACTIVE) {
            result.push_back(info);
        }
    }
    
    return result;
}

std::optional<PrimaryNodeInfo> MultiPrimaryCoordinator::getPrimaryInfo(const std::string& node_id) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = primaries_.find(node_id);
    if (it != primaries_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

bool MultiPrimaryCoordinator::isCurrentNodeActive() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = primaries_.find(config_.current_node_id);
    if (it != primaries_.end()) {
        return it->second.state == PrimaryState::ACTIVE;
    }
    
    return false;
}

LSN MultiPrimaryCoordinator::resolveConflict(const WriteConflict& conflict) const {
    conflicts_resolved_++;
    
    if (config_.use_last_write_wins) {
        return conflict.resolveLastWriteWins();
    }
    
    // Fallback: use higher LSN
    return (conflict.lsn2 > conflict.lsn1) ? conflict.lsn2 : conflict.lsn1;
}

/**
 * @brief Record Write.
 * @param[in] lsn Input parameter.
 * @details Calls: lock(), find(), end().
 */
void MultiPrimaryCoordinator::recordWrite(const LSN& lsn) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = primaries_.find(config_.current_node_id);
    if (it != primaries_.end()) {
        it->second.write_count++;
        it->second.last_known_lsn = lsn;
    }
}

std::optional<std::string> MultiPrimaryCoordinator::getMostCurrentPrimary() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string best_primary = {};
    LSN max_lsn(0, 0);
    
    for (const auto& [node_id, info] : primaries_) {
        if (info.state == PrimaryState::ACTIVE && info.last_known_lsn > max_lsn) {
            max_lsn = info.last_known_lsn;
            best_primary = node_id;
        }
    }
    
    if (best_primary.empty()) {
        return std::nullopt;
    }
    
    return best_primary;
}

MultiPrimaryCoordinator::Statistics MultiPrimaryCoordinator::getStatistics() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    
    Statistics stats;
    stats.total_primaries = primaries_.size();
    stats.conflicts_resolved = conflicts_resolved_.load();
    
    uint64_t total_writes = 0;
    for (const auto& [node_id, info] : primaries_) {
        switch (info.state) {
            case PrimaryState::ACTIVE:
                stats.active_primaries++;
                break;
            case PrimaryState::STANDBY:
                stats.standby_primaries++;
                break;
            case PrimaryState::OFFLINE:
                stats.offline_primaries++;
                break;
            default:
                break;
        }
        total_writes += info.write_count;
    }
    
    stats.total_writes = total_writes;
    
    return stats;
}

} // namespace themis::sharding
