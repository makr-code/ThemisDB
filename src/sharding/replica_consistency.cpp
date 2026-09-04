/**
 * @file replica_consistency.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=11, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/replica_consistency.h"
#include <algorithm>
#include <sstream>
#include <variant>

namespace themisdb {
namespace sharding {

// VectorClock implementation

/** @brief Construct vector clock from node->counter map. */
VectorClock::VectorClock(const std::map<std::string, uint64_t>& timestamps)
    : timestamps_(timestamps) {}

/** @brief Increment logical counter for node_id. */
void VectorClock::increment(const std::string& node_id) {
    timestamps_[node_id]++;
}

/** @brief Merge with another clock using element-wise max per node. */
void VectorClock::update(const VectorClock& other) {
    for (const auto& [node_id, timestamp] : other.timestamps_) {
        timestamps_[node_id] = std::max(timestamps_[node_id], timestamp);
    }
}

/** @brief Return counter for node_id (0 when absent). */
uint64_t VectorClock::get(const std::string& node_id) const {
    auto it = timestamps_.find(node_id);
    return (it != timestamps_.end()) ? it->second : 0;
}

/** @brief Return true when this clock causally precedes other. */
bool VectorClock::happensBefore(const VectorClock& other) const {
    bool less_or_equal = true;
    bool strictly_less = false;
    
    // Check all nodes in this clock
    for (const auto& [node_id, timestamp] : timestamps_) {
        uint64_t other_timestamp = other.get(node_id);
        if (timestamp > other_timestamp) {
            less_or_equal = false;
            break;
        }
        if (timestamp < other_timestamp) {
            strictly_less = true;
        }
    }
    
    // Check nodes only in other clock
    for (const auto& [node_id, timestamp] : other.timestamps_) {
        if (timestamps_.find(node_id) == timestamps_.end()) {
            if (timestamp > 0) {
                strictly_less = true;
            }
        }
    }
    
    return less_or_equal && strictly_less;
}

/** @brief Return true when this clock causally succeeds other. */
bool VectorClock::happensAfter(const VectorClock& other) const {
    return other.happensBefore(*this);
}

/** @brief Return true when no causal ordering exists between clocks. */
bool VectorClock::isConcurrent(const VectorClock& other) const {
    return !happensBefore(other) && !happensAfter(other);
}

/** @brief Serialize vector clock into node:counter comma-separated string. */
std::string VectorClock::serialize() const {
    if (timestamps_.empty()) {
        return "";  // Empty clock
    }
    
    std::ostringstream oss = {};
    bool first = true;
    for (const auto& [node_id, timestamp] : timestamps_) {
        if (!first) {
            oss << ",";
        }
        oss << node_id << ":" << timestamp;
        first = false;
    }
    return oss.str();
}

/** @brief Parse vector clock from node:counter comma-separated string. */
std::optional<VectorClock> VectorClock::deserialize(const std::string& data) {
    if (data.empty()) {
        return VectorClock();  // Empty clock
    }
    
    std::map<std::string, uint64_t> timestamps;
    std::istringstream iss(data);
    std::string token = {};
    
    while (std::getline(iss, token, ',')) {
        size_t colon_pos = token.find(':');
        if (colon_pos == std::string::npos) {
            return std::nullopt;
        }
        
        std::string node_id = token.substr(0, colon_pos);
        try {
            uint64_t timestamp = std::stoull(token.substr(colon_pos + 1));
            timestamps[node_id] = timestamp;
        } catch (const std::invalid_argument&) {
            return std::nullopt;  // Invalid number format
        } catch (const std::out_of_range&) {
            return std::nullopt;  // Number out of range
        }
    }
    
    return VectorClock(timestamps);
}

// VersionedEntry implementation

/** @brief Serialize versioned entry into delimiter-separated text payload. */
std::string VersionedEntry::serialize() const {
    std::ostringstream oss = {};
    oss << node_id << "|" << version.serialize() << "|" 
        << timestamp.time_since_epoch().count() << "|" << data;
    return oss.str();
}

/** @brief Deserialize versioned entry from delimiter-separated payload. */
std::optional<VersionedEntry> VersionedEntry::deserialize(const std::string& data) {
    size_t pos1 = data.find('|');
    if (pos1 == std::string::npos) {
      return std::nullopt;
    }
    
    size_t pos2 = data.find('|', pos1 + 1);
    if (pos2 == std::string::npos) {
      return std::nullopt;
    }
    
    size_t pos3 = data.find('|', pos2 + 1);
    if (pos3 == std::string::npos) {
      return std::nullopt;
    }
    
    VersionedEntry entry;
    entry.node_id = data.substr(0, pos1);
    
    auto clock_opt = VectorClock::deserialize(data.substr(pos1 + 1, pos2 - pos1 - 1));
    if (!clock_opt) {
      return std::nullopt;
    }
    entry.version = *clock_opt;
    
    auto timestamp_count = std::stoull(data.substr(pos2 + 1, pos3 - pos2 - 1));
    entry.timestamp = std::chrono::system_clock::time_point(
        std::chrono::system_clock::duration(timestamp_count));
    
    entry.data = data.substr(pos3 + 1);
    
    return entry;
}

// ReplicaConsistencyManager implementation

/** @brief Construct consistency manager with configured conflict policies. */
ReplicaConsistencyManager::ReplicaConsistencyManager(const Config& config)
    : config_(config) {}

/** @brief Record write and append versioned entry to per-key history. */
VersionedEntry ReplicaConsistencyManager::recordWrite(
    const std::string& key,
    const std::string& data,
    const std::string& node_id) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    stats_.total_writes++;
    
    // Increment vector clock for this node
    node_clocks_[node_id].increment(node_id);
    
    // Create versioned entry
    VersionedEntry entry;
    entry.data = data;
    entry.version = node_clocks_[node_id];
    entry.node_id = node_id;
    entry.timestamp = std::chrono::system_clock::now();
    
    // Add to version history
    auto& history = version_history_[key];
    history.push_back(entry);
    
    // Trim history if needed
    if (static_cast<int>(history.size()) > config_.max_version_history) {
        history.erase(history.begin());
    }
    
    return entry;
}

/** @brief Merge replica versions, detecting and resolving conflicts as configured. */
std::variant<VersionedEntry, VersionConflict> 
ReplicaConsistencyManager::mergeReplicas(
    const std::string& key,
    const std::vector<VersionedEntry>& entries) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    stats_.merges_performed++;
    
    if (entries.empty()) {
        return VersionConflict{};  // Empty conflict
    }
    
    if (static_cast<int>(entries.size()) == 1) {
        return entries[0];  // No conflict
    }
    
    // Check for conflicts
    auto conflict = detectConflict(key, entries);
    if (conflict.has_value()) {
        stats_.conflicts_detected++;
        
        if (config_.auto_resolve_conflicts) {
            auto resolved = autoResolveConflict(*conflict);
            stats_.conflicts_resolved++;
            return resolved;
        }
        
        return *conflict;
    }
    
    // No conflict - select latest version
    return selectWinningVersion(entries, config_.default_strategy);
}

/** @brief Apply manual conflict resolution result and store resolved version. */
void ReplicaConsistencyManager::resolveConflict(
    const VersionConflict& conflict,
    const VersionedEntry& resolved_entry) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    stats_.manual_resolutions++;
    
    // Update version history
    auto& history = version_history_[conflict.key];
    history.push_back(resolved_entry);
    
    if (static_cast<int>(history.size()) > config_.max_version_history) {
        history.erase(history.begin());
    }
}

/** @brief Return current vector clock for node_id. */
VectorClock ReplicaConsistencyManager::getVectorClock(const std::string& node_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = node_clocks_.find(node_id);
    return (it != node_clocks_.end()) ? it->second : VectorClock();
}

/** @brief Merge node vector clock with remote causality information. */
void ReplicaConsistencyManager::updateVectorClock(const std::string& node_id,
                                                  const VectorClock& clock) {
    std::lock_guard<std::mutex> lock(mutex_);
    node_clocks_[node_id].update(clock);
}

/** @brief Register callback for custom conflict resolution (thread-safe). */
void ReplicaConsistencyManager::setConflictCallback([[maybe_unused]] ConflictCallback callback) {
    // conflict_callback_ is read under mutex_ by autoResolveConflict; acquire
    // here to prevent a data race when the callback is registered concurrently.
    std::lock_guard<std::mutex> lock(mutex_);
    conflict_callback_ = std::move([[maybe_unused]] callback);
}

/** @brief Return retained version history for key. */
std::vector<VersionedEntry> ReplicaConsistencyManager::getVersionHistory(
    const std::string& key) const {
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = version_history_.find(key);
    return (it != version_history_.end()) ? it->second : std::vector<VersionedEntry>();
}

/** @brief Merge partitioned Raft log streams by term/index ordering. */
std::vector<LogEntry> ReplicaConsistencyManager::mergePartitionedLogs(
    const std::vector<LogEntry>& local_entries,
    const std::vector<LogEntry>& remote_entries) {
    
    // Merge logs using term and index
    std::vector<LogEntry> merged;
    size_t local_idx = 0;
    size_t remote_idx = 0;
    
    while (local_idx <static_cast<int>(local_entries.size())  && static_cast<size_t>(remote_idx) <static_cast<int>(remote_entries.size())) {
        const auto& local = local_entries[local_idx];
        const auto& remote = remote_entries[remote_idx];
        
        // Compare by term, then by index
        if (local.term < remote.term || 
            (local.term == remote.term && local.index < remote.index)) {
            merged.push_back(local);
            local_idx++;
        } else if (remote.term < local.term ||
                   (remote.term == local.term && remote.index < local.index)) {
            merged.push_back(remote);
            remote_idx++;
        } else {
            // Same term and index - prefer local (or use timestamp)
            merged.push_back(local);
            local_idx++;
            remote_idx++;
        }
    }
    
    // Add remaining entries
    while (static_cast<size_t>(local_idx) <static_cast<int>(local_entries.size())) {
        merged.push_back(local_entries[local_idx++]);
    }
    while (static_cast<size_t>(remote_idx) <static_cast<int>(remote_entries.size())) {
        merged.push_back(remote_entries[remote_idx++]);
    }
    
    return merged;
}

/** @brief Detect concurrent versions indicating conflict for key. */
std::optional<VersionConflict> ReplicaConsistencyManager::detectConflict(
    const std::string& key,
    const std::vector<VersionedEntry>& entries) {
    
    // Check if any entries are concurrent
    for (size_t i = 0; i <static_cast<int>(entries.size()); ++i) {
        for (size_t j = i + 1; j <static_cast<int>(entries.size()); ++j) {
            if (entries[i].version.isConcurrent(entries[j].version)) {
                // Found conflict
                VersionConflict conflict;
                conflict.key = key;
                conflict.conflicting_versions = entries;
                conflict.resolution_strategy = config_.default_strategy;
                conflict.needs_manual_resolution = !config_.auto_resolve_conflicts;
                return conflict;
            }
        }
    }
    
    return std::nullopt;
}

/** @brief Auto-resolve conflict via callback or strategy selection. */
VersionedEntry ReplicaConsistencyManager::autoResolveConflict(
    const VersionConflict& conflict) {
    
    if ([[maybe_unused]] conflict_callback_) {
        return conflict_callback_([[maybe_unused]] conflict);
    }
    
    return selectWinningVersion(conflict.conflicting_versions,
                               conflict.resolution_strategy);
}

/** @brief Select winning version according to provided resolution strategy. */
VersionedEntry ReplicaConsistencyManager::selectWinningVersion(
    const std::vector<VersionedEntry>& entries,
    ConflictResolutionStrategy strategy) {
    
    if (entries.empty()) {
        return VersionedEntry{};
    }
    
    switch (strategy) {
        case ConflictResolutionStrategy::LAST_WRITE_WINS: {
            // Use timestamp
            auto it = std::max_element(entries.begin(), entries.end(),
                [](const VersionedEntry& a, const VersionedEntry& b) {
                    return a.timestamp < b.timestamp;
                });
            return *it;
        }
        
        case ConflictResolutionStrategy::VECTOR_CLOCK_ORDERING: {
            // Find entry that happens after all others
            for (const auto& entry : entries) {
                bool is_latest = true;
                for (const auto& other : entries) {
                    if (&entry != &other && !entry.version.happensAfter(other.version)) {
                        is_latest = false;
                        break;
                    }
                }
                if (is_latest) {
                    return entry;
                }
            }
            // If no clear winner, fall back to timestamp
            return selectWinningVersion(entries, ConflictResolutionStrategy::LAST_WRITE_WINS);
        }
        
        case ConflictResolutionStrategy::HIGHEST_NODE_ID: {
            // Deterministic by node ID
            auto it = std::max_element(entries.begin(), entries.end(),
                [](const VersionedEntry& a, const VersionedEntry& b) {
                    return a.node_id < b.node_id;
                });
            return *it;
        }
        
        default:
            return entries[0];
    }
}

}  // namespace sharding
}  // namespace themisdb
