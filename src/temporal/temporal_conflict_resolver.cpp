/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            temporal_conflict_resolver.cpp                     ║
  Version:         0.0.25                                             ║
  Last Modified:   2026-02-22 08:22:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     331                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB Temporal Conflict Resolver Implementation
 * 
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "temporal/temporal_conflict_resolver.h"
#include <sstream>
#include <iomanip>
#include <random>

namespace themisdb {
namespace temporal {

// ============================================================================
// TemporalSnapshot Implementation
// ============================================================================

nlohmann::json TemporalSnapshot::toJson() const {
    return {
        {"snapshot_id", snapshot_id},
        {"hlc", {
            {"physical", hlc.physical},
            {"logical", hlc.logical},
            {"node_id", hlc.node_id}
        }},
        {"source_node_id", source_node_id},
        {"data", data},
        {"checksum", checksum}
    };
}

std::optional<TemporalSnapshot> TemporalSnapshot::fromJson(const nlohmann::json& j) {
    try {
        TemporalSnapshot snapshot;
        snapshot.snapshot_id = j.at("snapshot_id").get<std::string>();
        snapshot.hlc.physical = j.at("hlc").at("physical").get<uint64_t>();
        snapshot.hlc.logical = j.at("hlc").at("logical").get<uint32_t>();
        snapshot.hlc.node_id = j.at("hlc").at("node_id").get<std::string>();
        snapshot.source_node_id = j.at("source_node_id").get<std::string>();
        snapshot.data = j.at("data");
        snapshot.checksum = j.at("checksum").get<std::string>();
        return snapshot;
    } catch (const nlohmann::json::exception&) {
        return std::nullopt;
    }
}

// ============================================================================
// TemporalConflictResolver Implementation
// ============================================================================

TemporalConflictResolver::TemporalConflictResolver(ConflictPolicy default_policy)
    : default_policy_(default_policy) {}

TemporalSnapshot TemporalConflictResolver::resolve(
    const TemporalSnapshot& local,
    const TemporalSnapshot& remote,
    std::optional<ConflictPolicy> policy
) {
    total_conflicts_.fetch_add(1, std::memory_order_relaxed);
    
    ConflictPolicy active_policy = policy.value_or(default_policy_);
    
    // Create conflict record
    ConflictRecord record;
    record.conflict_id = generateConflictId();
    record.entity_id = local.snapshot_id;  // Use snapshot_id as entity_id
    record.local_version = local;
    record.remote_version = remote;
    record.resolution_policy = active_policy;
    record.detected_at = std::chrono::system_clock::now();
    record.resolved = false;
    
    TemporalSnapshot winner;
    
    switch (active_policy) {
        case ConflictPolicy::LAST_WRITE_WINS:
            winner = resolveLastWriteWins(local, remote);
            record.winner = (winner.hlc == local.hlc) ? "local" : "remote";
            lww_resolutions_.fetch_add(1, std::memory_order_relaxed);
            record.resolved = true;
            break;
            
        case ConflictPolicy::FIRST_WRITE_WINS:
            winner = resolveFirstWriteWins(local, remote);
            record.winner = (winner.hlc == local.hlc) ? "local" : "remote";
            fww_resolutions_.fetch_add(1, std::memory_order_relaxed);
            record.resolved = true;
            break;
            
        case ConflictPolicy::NODE_PRIORITY:
            winner = resolveNodePriority(local, remote);
            record.winner = (winner.hlc == local.hlc) ? "local" : "remote";
            record.resolved = true;
            break;
            
        case ConflictPolicy::CRDT_MERGE:
            winner = resolveCRDT(local, remote);
            record.winner = "merged";
            crdt_merges_.fetch_add(1, std::memory_order_relaxed);
            record.resolved = true;
            break;
            
        case ConflictPolicy::MANUAL:
            {
                std::lock_guard<std::mutex> lock(mutex_);
                unresolved_conflicts_[record.conflict_id] = record;
            }
            return local;  // Keep local until manual resolution
    }
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        conflict_history_.push_back(record);
    }
    
    return winner;
}

TemporalSnapshot TemporalConflictResolver::resolveLastWriteWins(
    const TemporalSnapshot& local,
    const TemporalSnapshot& remote
) {
    // Compare HLC timestamps using operator< and operator==
    if (local.hlc < remote.hlc) {
        return remote;  // remote is newer
    } else if (remote.hlc < local.hlc) {
        return local;   // local is newer
    } else {
        // HLC timestamps are equal → Use node_id as tiebreaker
        if (remote.hlc.node_id > local.hlc.node_id) {
            return remote;
        } else {
            return local;
        }
    }
}

TemporalSnapshot TemporalConflictResolver::resolveFirstWriteWins(
    const TemporalSnapshot& local,
    const TemporalSnapshot& remote
) {
    // Opposite of Last-Write-Wins - oldest wins
    if (local.hlc < remote.hlc) {
        return local;   // local is older
    } else if (remote.hlc < local.hlc) {
        return remote;  // remote is older
    } else {
        // Use node_id as tiebreaker (favor lower ID)
        if (local.hlc.node_id < remote.hlc.node_id) {
            return local;
        } else {
            return remote;
        }
    }
}

TemporalSnapshot TemporalConflictResolver::resolveNodePriority(
    const TemporalSnapshot& local,
    const TemporalSnapshot& remote
) {
    // Node priority based on node_id lexicographic order
    // Can be extended with configuration-based priority mapping
    if (local.source_node_id < remote.source_node_id) {
        return local;
    } else {
        return remote;
    }
}

TemporalSnapshot TemporalConflictResolver::resolveCRDT(
    const TemporalSnapshot& local,
    const TemporalSnapshot& remote
) {
    // LWW-Register per field: for each field, keep the value from the
    // snapshot with the higher HLC timestamp (Last-Write-Wins per field).
    // For fields present only in one snapshot the single value is kept.
    // This is the standard LWW-Element-Register CRDT strategy.

    const TemporalSnapshot& newer =
        (local.hlc < remote.hlc) ? remote : local;
    const TemporalSnapshot& older =
        (local.hlc < remote.hlc) ? local : remote;

    // Start with the older snapshot's fields as the baseline
    nlohmann::json merged = older.data;

    // Override/add with all fields from the newer snapshot
    if (newer.data.is_object() && older.data.is_object()) {
        for (auto& [key, value] : newer.data.items()) {
            merged[key] = value;
        }
    } else {
        // Non-object payloads: the newer value wins outright
        merged = newer.data;
    }

    TemporalSnapshot result = newer;
    result.data = std::move(merged);
    return result;
}

std::vector<ConflictRecord> TemporalConflictResolver::getUnresolvedConflicts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ConflictRecord> result;
    result.reserve(unresolved_conflicts_.size());
    for (const auto& [id, record] : unresolved_conflicts_) {
        result.push_back(record);
    }
    return result;
}

void TemporalConflictResolver::resolveManually(const std::string& conflict_id, const std::string& winner) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = unresolved_conflicts_.find(conflict_id);
    if (it != unresolved_conflicts_.end()) {
        it->second.winner = winner;
        it->second.resolved = true;
        conflict_history_.push_back(it->second);
        unresolved_conflicts_.erase(it);
        manual_resolutions_.fetch_add(1, std::memory_order_relaxed);
    }
}

std::vector<ConflictRecord> TemporalConflictResolver::getConflictHistory() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ConflictRecord> result = conflict_history_;
    // Append unresolved conflicts so callers see the complete picture
    for (const auto& [id, record] : unresolved_conflicts_) {
        result.push_back(record);
    }
    return result;
}

nlohmann::json TemporalConflictResolver::exportAuditLog() const {
    std::lock_guard<std::mutex> lock(mutex_);
    nlohmann::json log = nlohmann::json::array();

    auto policyName = [](ConflictPolicy p) -> std::string {
        switch (p) {
            case ConflictPolicy::LAST_WRITE_WINS:  return "LWW";
            case ConflictPolicy::FIRST_WRITE_WINS: return "FWW";
            case ConflictPolicy::NODE_PRIORITY:    return "NODE_PRIORITY";
            case ConflictPolicy::MANUAL:           return "MANUAL";
            case ConflictPolicy::CRDT_MERGE:       return "CRDT_MERGE";
        }
        return "UNKNOWN";
    };

    auto appendEntry = [&](const ConflictRecord& r) {
        auto detected_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                               r.detected_at.time_since_epoch())
                               .count();
        log.push_back({
            {"conflict_id",    r.conflict_id},
            {"entity_id",      r.entity_id},
            {"winner",         r.winner},
            {"policy",         policyName(r.resolution_policy)},
            {"resolved",       r.resolved},
            {"detected_at_ms", detected_ms}
        });
    };

    for (const auto& r : conflict_history_) {
        appendEntry(r);
    }
    for (const auto& [id, r] : unresolved_conflicts_) {
        appendEntry(r);
    }
    return log;
}

nlohmann::json TemporalConflictResolver::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return {
        {"total_conflicts", total_conflicts_.load()},
        {"lww_resolutions", lww_resolutions_.load()},
        {"fww_resolutions", fww_resolutions_.load()},
        {"manual_resolutions", manual_resolutions_.load()},
        {"crdt_merges", crdt_merges_.load()},
        {"unresolved_conflicts", unresolved_conflicts_.size()}
    };
}

std::string TemporalConflictResolver::generateConflictId() const {
    // Generate a unique conflict ID based on timestamp and random number
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist;
    
    std::ostringstream oss;
    oss << "conflict_" << now_ms << "_" << dist(gen);
    return oss.str();
}

} // namespace temporal
} // namespace themisdb
