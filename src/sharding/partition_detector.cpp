// THEMIS_GAP_STATS: gaps=1 unimpl=0 stub=0 mock=0 sim=0 todo=0 debt=0 scanned=2026-05-18
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            partition_detector.cpp                             ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:50:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     323                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/partition_detector.h"
#include <algorithm>
#include <thread>
#include <condition_variable>

namespace themisdb {
namespace sharding {

PartitionDetector::PartitionDetector(const PartitionDetectorConfig& config)
    : config_(config) {}

PartitionDetector::~PartitionDetector() {
    stop();
}

void PartitionDetector::start() {
    if (running_.exchange(true)) {
        return;  // Already running
    }
    
    health_check_thread_ = std::thread(&PartitionDetector::healthCheckLoop, this);
}

void PartitionDetector::stop() {
    if (!running_.exchange(false)) {
        return;  // Already stopped
    }
    
    if (health_check_thread_.joinable()) {
        health_check_thread_.join();
    }
}

void PartitionDetector::addNode(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    NodeConnectivity conn;
    conn.node_id = node_id;
    conn.reachable = true;
    conn.last_rtt = std::chrono::milliseconds(0);
    conn.last_successful_contact = std::chrono::steady_clock::now();
    conn.consecutive_failures = 0;
    conn.packet_loss_rate = 0.0;
    
    nodes_[node_id] = conn;
}

void PartitionDetector::removeNode(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    nodes_.erase(node_id);
}

void PartitionDetector::recordHeartbeat(const std::string& node_id,
                                       std::chrono::milliseconds rtt) {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    auto it = nodes_.find(node_id);
    if (it == nodes_.end()) {
        return;
    }
    
    auto& conn = it->second;
    conn.reachable = true;
    conn.last_rtt = rtt;
    conn.last_successful_contact = std::chrono::steady_clock::now();
    conn.consecutive_failures = 0;
    
    // Update packet loss rate (exponential moving average)
    conn.packet_loss_rate = conn.packet_loss_rate * 0.9;  // Decay
}

void PartitionDetector::recordFailure(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    auto it = nodes_.find(node_id);
    if (it == nodes_.end()) {
        return;
    }
    
    auto& conn = it->second;
    conn.consecutive_failures++;
    
    // Update packet loss rate
    conn.packet_loss_rate = conn.packet_loss_rate * 0.9 + 0.1;  // Increase
    
    // Mark as unreachable if too many failures
    if (conn.consecutive_failures >= config_.max_consecutive_failures) {
        conn.reachable = false;
    }
    
    stats_.failed_health_checks++;
}

NetworkHealth PartitionDetector::getNetworkHealth() const {
    return current_health_.load();
}

std::vector<NodeConnectivity> PartitionDetector::getNodeConnectivity() const {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    std::vector<NodeConnectivity> result;
    for (const auto& pair : nodes_) {
        result.push_back(pair.second);
    }
    return result;
}

bool PartitionDetector::isSplitBrainDetected() const {
    return split_brain_detected_.load();
}

std::vector<PartitionEvent> PartitionDetector::getPartitionHistory() const {
    std::lock_guard<std::mutex> lock(events_mutex_);
    return partition_history_;
}

void PartitionDetector::setHealthCheckCallback(HealthCheckCallback callback) {
    health_check_callback_ = callback;
}

void PartitionDetector::setPartitionCallback(PartitionCallback callback) {
    partition_callback_ = callback;
}

void PartitionDetector::healthCheckLoop() {
    while (running_) {
        stats_.total_health_checks++;
        
        // Perform health checks
        if (health_check_callback_) {
            std::vector<std::string> node_ids;
            {
                std::lock_guard<std::mutex> lock(nodes_mutex_);
                for (const auto& pair : nodes_) {
                    node_ids.push_back(pair.first);
                }
            }
            
            for (const auto& node_id : node_ids) {
                auto start = std::chrono::steady_clock::now();
                bool success = health_check_callback_(node_id);
                auto end = std::chrono::steady_clock::now();
                auto rtt = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                
                if (success) {
                    recordHeartbeat(node_id, rtt);
                } else {
                    recordFailure(node_id);
                }
            }
        }
        
        // Update network health and detect partitions
        updateNetworkHealth();
        detectPartition();
        
        if (config_.enable_auto_healing) {
            checkPartitionHealing();
        }
        
        std::this_thread::sleep_for(config_.health_check_interval);
    }
}

void PartitionDetector::detectPartition() {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    // Count unreachable nodes
    size_t unreachable_count = 0;
    std::vector<std::string> unreachable_nodes;
    std::vector<std::string> reachable_nodes;
    
    for (const auto& pair : nodes_) {
        if (!pair.second.reachable) {
            unreachable_count++;
            unreachable_nodes.push_back(pair.first);
        } else {
            reachable_nodes.push_back(pair.first);
        }
    }
    
    // Detect partition if significant portion unreachable
    size_t total_nodes = nodes_.size();
    bool partition_detected = false;
    
    if (config_.enable_split_brain_detection) {
        // Split-brain: roughly equal partitions
        double unreachable_ratio = static_cast<double>(unreachable_count) / total_nodes;
        if (unreachable_ratio >= 0.3 && unreachable_ratio <= 0.7) {
            partition_detected = true;
            split_brain_detected_.store(true);
        }
    }
    
    if (partition_detected) {
        stats_.partitions_detected++;
        stats_.last_partition_time = std::chrono::steady_clock::now();
        
        // Create partition event
        PartitionEvent event;
        event.detected_at = std::chrono::steady_clock::now();
        event.partition_a = reachable_nodes;
        event.partition_b = unreachable_nodes;
        event.is_healed = false;
        event.description = "Network partition detected: " + 
                           std::to_string(reachable_nodes.size()) + " vs " +
                           std::to_string(unreachable_nodes.size()) + " nodes";
        
        {
            std::lock_guard<std::mutex> events_lock(events_mutex_);
            partition_history_.push_back(event);
        }
        
        // Notify callback
        if (partition_callback_) {
            partition_callback_(event);
        }
    }
}

void PartitionDetector::checkPartitionHealing() {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    // Check if all nodes are reachable again
    bool all_reachable = true;
    for (const auto& pair : nodes_) {
        if (!pair.second.reachable) {
            all_reachable = false;
            break;
        }
    }
    
    if (all_reachable && split_brain_detected_.load()) {
        split_brain_detected_.store(false);
        stats_.partitions_healed++;
        
        // Mark last partition event as healed
        std::lock_guard<std::mutex> events_lock(events_mutex_);
        if (!partition_history_.empty()) {
            auto& last_event = partition_history_.back();
            if (!last_event.is_healed) {
                last_event.is_healed = true;
                last_event.healed_at = std::chrono::steady_clock::now();
                
                // Notify callback
                if (partition_callback_) {
                    partition_callback_(last_event);
                }
            }
        }
    }
}

void PartitionDetector::updateNetworkHealth() {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    size_t unreachable_count = 0;
    size_t degraded_count = 0;
    
    auto now = std::chrono::steady_clock::now();
    
    for (const auto& pair : nodes_) {
        const auto& conn = pair.second;
        
        if (!conn.reachable) {
            unreachable_count++;
        } else {
            // Check if degraded (high latency or packet loss)
            auto time_since_contact = now - conn.last_successful_contact;
            if (conn.last_rtt > config_.heartbeat_timeout / 2 ||
                conn.packet_loss_rate > config_.packet_loss_threshold / 2 ||
                time_since_contact > config_.heartbeat_timeout) {
                degraded_count++;
            }
        }
    }
    
    size_t total_nodes = nodes_.size();
    
    if (total_nodes == 0) {
        current_health_.store(NetworkHealth::HEALTHY);
        return;
    }
    
    double unreachable_ratio = static_cast<double>(unreachable_count) / total_nodes;
    double degraded_ratio = static_cast<double>(degraded_count) / total_nodes;
    
    if (unreachable_ratio >= 0.3) {
        current_health_.store(NetworkHealth::PARTITIONED);
    } else if (unreachable_ratio > 0 || degraded_ratio > 0.3) {
        current_health_.store(NetworkHealth::DEGRADED);
    } else {
        current_health_.store(NetworkHealth::HEALTHY);
    }
}

}  // namespace sharding
}  // namespace themisdb
