/**
 * @file temporal_conflict_resolver.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
        default: break;
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
    // Delegate to the injected MergeResolver if one has been set; otherwise
    // fall back to the built-in LWW-per-field strategy.
    std::shared_ptr<MergeResolver> resolver;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        resolver = merge_resolver_;
    }

    if (resolver) {
        return resolver->merge(local, remote);
    }
    // Fallback: built-in LWW-per-field strategy
    return LWWFieldMergeResolver{}.merge(local, remote);
}

std::vector<ConflictRecord> TemporalConflictResolver::getUnresolvedConflicts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ConflictRecord> result = {};

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
            default: break;
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
        {"unresolved_conflicts",static_cast<int>(unresolved_conflicts_.size())}
    };
}

std::string TemporalConflictResolver::generateConflictId() const {
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();

    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist;

    std::ostringstream oss = {};
    oss << "conflict_" << now_ms << "_" << dist(gen);
    return oss.str();
}

// ============================================================================
// TemporalConflictDetector Implementation
// ============================================================================

// ---------------------------------------------------------------------------
// detectConflicts
// ---------------------------------------------------------------------------

std::vector<Conflict> TemporalConflictDetector::detectConflicts(
    const std::string& table_name,
    const TemporalSnapshot& local,
    const TemporalSnapshot& remote
) {
    std::vector<Conflict> conflicts;

    // Run each sub-detector and collect results.
    auto concurrent = detectConcurrentUpdate(local, remote);
    if (concurrent) {
        concurrent->table_name = table_name;
        concurrent->entity_id  = local.snapshot_id;
        conflicts.push_back(std::move(*concurrent));
    }

    auto overlapping = detectOverlappingPeriods(local, remote);
    if (overlapping) {
        overlapping->table_name = table_name;
        overlapping->entity_id  = local.snapshot_id;
        conflicts.push_back(std::move(*overlapping));
    }

    auto refint = detectReferentialIntegrity(local, remote);
    if (refint) {
        refint->table_name = table_name;
        refint->entity_id  = local.snapshot_id;
        conflicts.push_back(std::move(*refint));
    }

    auto uniq = detectUniquenessViolation(local, remote);
    if (uniq) {
        uniq->table_name = table_name;
        uniq->entity_id  = local.snapshot_id;
        conflicts.push_back(std::move(*uniq));
    }

    return conflicts;
}

// ---------------------------------------------------------------------------
// autoResolveConflict
// ---------------------------------------------------------------------------

std::optional<TemporalSnapshot> TemporalConflictDetector::autoResolveConflict(
    const Conflict& conflict,
    ConflictPolicy policy
) {
    if (policy == ConflictPolicy::MANUAL) {
        return std::nullopt;
    }
    TemporalConflictResolver resolver(policy);
    return resolver.resolve(conflict.local_version, conflict.remote_version, policy);
}

// ---------------------------------------------------------------------------
// queueForManualResolution
// ---------------------------------------------------------------------------

bool TemporalConflictDetector::queueForManualResolution(const std::string& table_name,
                                                        const Conflict& conflict) {
    const std::string key = makeQueueKey(table_name, conflict);
    std::lock_guard<std::mutex> lock(queue_mutex_);
    if (manual_queue_.count(key)) {
        return false;  // already queued
    }
    // Store a copy with table_name set so the queued entry is self-consistent.
    Conflict stored = conflict;
    stored.table_name = table_name;
    manual_queue_[key] = std::move(stored);
    return true;
}

std::vector<Conflict> TemporalConflictDetector::getQueuedConflicts() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    std::vector<Conflict> result = {};

    result.reserve(manual_queue_.size());
    for (const auto& [k, v] : manual_queue_) {
        result.push_back(v);
    }
    return result;
}

void TemporalConflictDetector::clearQueue() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    manual_queue_.clear();
}

// ---------------------------------------------------------------------------
// makeQueueKey (private static)
// ---------------------------------------------------------------------------

std::string TemporalConflictDetector::makeQueueKey(
    const std::string& table_name,
    const Conflict& conflict
) {
    std::string type_str = {};
    switch (conflict.type) {
        case ConflictType::CONCURRENT_UPDATE:     type_str = "CONCURRENT_UPDATE";     break;
        case ConflictType::OVERLAPPING_PERIODS:   type_str = "OVERLAPPING_PERIODS";   break;
        case ConflictType::REFERENTIAL_INTEGRITY: type_str = "REFERENTIAL_INTEGRITY"; break;
        case ConflictType::UNIQUENESS_VIOLATION:  type_str = "UNIQUENESS_VIOLATION";  break;
        default: break;
    }
    return table_name + "|" + conflict.entity_id + "|" + type_str
         + "|" + conflict.local_version.snapshot_id
         + "|" + conflict.remote_version.snapshot_id;
}

// ---------------------------------------------------------------------------
// Sub-detectors (private static)
// ---------------------------------------------------------------------------

std::optional<Conflict> TemporalConflictDetector::detectConcurrentUpdate(
    const TemporalSnapshot& local,
    const TemporalSnapshot& remote
) {
    // For conflict detection we treat HLC causality on physical/logical parts.
    // Node-ID tie-breakers are used for deterministic winner selection, not
    // for deciding whether two writes are concurrent.
    const auto& l = local.hlc;
    const auto& r = remote.hlc;

    bool local_before_remote =
        (l.physical < r.physical) ||
        (l.physical == r.physical && l.logical < r.logical);
    bool remote_before_local =
        (r.physical < l.physical) ||
        (r.physical == l.physical && r.logical < l.logical);

    // Strictly ordered: no concurrent update conflict.
    if (local_before_remote || remote_before_local) {
        return std::nullopt;
    }

    // Equal HLC with same node: identical write — no conflict.
    if (l.node_id == r.node_id && local.data == remote.data) {
        return std::nullopt;
    }

    // Collect differing data keys as affected columns.
    std::vector<std::string> affected = {};

    if (local.data.is_object() && remote.data.is_object()) {
        for (auto& [key, val] : local.data.items()) {
            if (remote.data.contains(key) && remote.data[key] != val) {
                affected.push_back(key);
            }
        }
    } else if (local.data != remote.data) {
        affected.push_back("data");
    }

    Conflict c;
    c.type             = ConflictType::CONCURRENT_UPDATE;
    c.local_version    = local;
    c.remote_version   = remote;
    c.affected_columns = std::move(affected);
    return c;
}

std::optional<Conflict> TemporalConflictDetector::detectOverlappingPeriods(
    const TemporalSnapshot& local,
    const TemporalSnapshot& remote
) {
    // Snapshots may carry valid_start / valid_end fields in their JSON data.
    // If both snapshots have these fields, check for a valid-time overlap.
    const auto& ld = local.data;
    const auto& rd = remote.data;

    if (!ld.is_object() || !rd.is_object()) {
        return std::nullopt;
    }
    if (!ld.contains("valid_start") || !ld.contains("valid_end") ||
        !rd.contains("valid_start") || !rd.contains("valid_end")) {
        return std::nullopt;
    }

    const auto& l_start_val = ld.at("valid_start");
    const auto& l_end_val   = ld.at("valid_end");
    const auto& r_start_val = rd.at("valid_start");
    const auto& r_end_val   = rd.at("valid_end");

    // Treat non-integer valid-time fields as "no period information present".
    if (!l_start_val.is_number_integer() || !l_end_val.is_number_integer() ||
        !r_start_val.is_number_integer() || !r_end_val.is_number_integer()) {
        return std::nullopt;
    }

    int64_t l_start = l_start_val.get<int64_t>();
    int64_t l_end   = l_end_val.get<int64_t>();
    int64_t r_start = r_start_val.get<int64_t>();
    int64_t r_end   = r_end_val.get<int64_t>();

    // Half-open [start, end) overlap: l_start < r_end && r_start < l_end
    bool overlap = (l_start < r_end) && (r_start < l_end);
    if (!overlap) {
        return std::nullopt;
    }

    // Only a conflict when the data also differs (otherwise it is the same version).
    if (local.data == remote.data) {
        return std::nullopt;
    }

    Conflict c;
    c.type             = ConflictType::OVERLAPPING_PERIODS;
    c.local_version    = local;
    c.remote_version   = remote;
    c.affected_columns = {"valid_start", "valid_end"};
    return c;
}

std::optional<Conflict> TemporalConflictDetector::detectReferentialIntegrity(
    const TemporalSnapshot& local,
    const TemporalSnapshot& remote
) {
    // Check for a "ref_entity_id" field.  If both snapshots carry it but the
    // values differ, that signals a referential integrity conflict.
    const auto& ld = local.data;
    const auto& rd = remote.data;

    if (!ld.is_object() || !rd.is_object()) {
        return std::nullopt;
    }
    if (!ld.contains("ref_entity_id") || !rd.contains("ref_entity_id")) {
        return std::nullopt;
    }

    if (ld.at("ref_entity_id") == rd.at("ref_entity_id")) {
        return std::nullopt;  // references agree
    }

    Conflict c;
    c.type             = ConflictType::REFERENTIAL_INTEGRITY;
    c.local_version    = local;
    c.remote_version   = remote;
    c.affected_columns = {"ref_entity_id"};
    return c;
}

std::optional<Conflict> TemporalConflictDetector::detectUniquenessViolation(
    const TemporalSnapshot& local,
    const TemporalSnapshot& remote
) {
    // A uniqueness conflict exists when both snapshots claim to be the current
    // version of the same entity (same snapshot_id prefix or same source node)
    // but carry different data.
    if (local.source_node_id == remote.source_node_id) {
        return std::nullopt;  // same origin — not a distributed uniqueness conflict
    }

    if (local.data == remote.data) {
        return std::nullopt;  // identical content — no violation
    }

    // Collect affected columns as the symmetric difference of diverging keys:
    // - Keys present in both but with different values
    // - Keys present in only one snapshot
    std::vector<std::string> affected = {};

    if (local.data.is_object() && remote.data.is_object()) {
        // Keys in local: diverging or missing in remote
        for (auto& [key, val] : local.data.items()) {
            if (!remote.data.contains(key) || remote.data[key] != val) {
                affected.push_back(key);
            }
        }
        // Keys present only in remote (not already captured above)
        for (auto& [key, val] : remote.data.items()) {
            if (!local.data.contains(key)) {
                affected.push_back(key);
            }
        }
    } else {
        affected.push_back("data");
    }

    if (affected.empty()) {
        return std::nullopt;  // no common keys diverge — no violation
    }

    Conflict c;
    c.type             = ConflictType::UNIQUENESS_VIOLATION;
    c.local_version    = local;
    c.remote_version   = remote;
    c.affected_columns = std::move(affected);
    return c;
}

// ============================================================================
// MergeResolver Built-in Implementations
// ============================================================================

// ─── LWWFieldMergeResolver ───────────────────────────────────────────────────

TemporalSnapshot LWWFieldMergeResolver::merge(
    const TemporalSnapshot& local,
    const TemporalSnapshot& remote
) const {
    // LWW-Register per field: for each JSON field keep the value from the
    // snapshot with the higher HLC timestamp.  Fields present only in one
    // snapshot are always included.

    const TemporalSnapshot& newer = (local.hlc < remote.hlc) ? remote : local;
    const TemporalSnapshot& older = (local.hlc < remote.hlc) ? local  : remote;

    nlohmann::json merged = older.data;

    if (newer.data.is_object() && older.data.is_object()) {
        for (const auto& [key, value] : newer.data.items()) {
            merged[key] = value;
        }
    } else {
        merged = newer.data;
    }

    TemporalSnapshot result = newer;
    result.data = std::move(merged);
    return result;
}

// ─── UnionMergeResolver ──────────────────────────────────────────────────────

TemporalSnapshot UnionMergeResolver::merge(
    const TemporalSnapshot& local,
    const TemporalSnapshot& remote
) const {
    // Union: keep every field from either snapshot; for fields present in
    // both, take the value from the snapshot with the higher HLC.
    // Non-object payloads fall back to LWW.

    const TemporalSnapshot& newer = (local.hlc < remote.hlc) ? remote : local;
    const TemporalSnapshot& older = (local.hlc < remote.hlc) ? local  : remote;

    nlohmann::json merged;

    if (newer.data.is_object() && older.data.is_object()) {
        // Start with older fields as the baseline (union semantics: keep all).
        merged = older.data;
        // Overwrite/add all fields from the newer snapshot.
        for (const auto& [key, value] : newer.data.items()) {
            merged[key] = value;
        }
    } else {
        merged = newer.data;
    }

    TemporalSnapshot result = newer;
    result.data = std::move(merged);
    return result;
}

// ─── CustomMergeResolver ─────────────────────────────────────────────────────

CustomMergeResolver::CustomMergeResolver(MergeFn fn)
    : fn_(std::move(fn))
{}

TemporalSnapshot CustomMergeResolver::merge(
    const TemporalSnapshot& local,
    const TemporalSnapshot& remote
) const {
    return fn_(local, remote);
}

// ============================================================================
// TemporalConflictResolver – MergeResolver accessors
// ============================================================================

void TemporalConflictResolver::setMergeResolver(std::shared_ptr<MergeResolver> resolver) {
    std::lock_guard<std::mutex> lk(mutex_);
    merge_resolver_ = std::move(resolver);
}

std::shared_ptr<MergeResolver> TemporalConflictResolver::getMergeResolver() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return merge_resolver_;
}

} // namespace temporal
} // namespace themisdb
