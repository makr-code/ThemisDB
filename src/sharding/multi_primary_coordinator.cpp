/**
 * @file multi_primary_coordinator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=1, M=1, L=0
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

void MultiPrimaryCoordinator::markPrimaryOffline(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = primaries_.find(node_id);
    if (it != primaries_.end()) {
        it->second.state = PrimaryState::OFFLINE;
    }
}

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
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = primaries_.find(node_id);
    if (it != primaries_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

bool MultiPrimaryCoordinator::isCurrentNodeActive() const {
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

void MultiPrimaryCoordinator::recordWrite(const LSN& lsn) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = primaries_.find(config_.current_node_id);
    if (it != primaries_.end()) {
        it->second.write_count++;
        it->second.last_known_lsn = lsn;
    }
}

std::optional<std::string> MultiPrimaryCoordinator::getMostCurrentPrimary() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string best_primary;
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
