/**
 * @file event_stream.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=7, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Replication Event Stream Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "replication/event_stream.h"

#include <algorithm>
#include <string>
#include <sstream>

namespace themisdb {
namespace replication {

// ============================================================================
// Lock Hierarchy Documentation (event_stream.cpp)
// ============================================================================
//
// This module implements a 2-level lock hierarchy for thread-safe event
// handling in replication systems without deadlocks.
//
// LOCK HIERARCHY (ordered from outermost to innermost):
//
//   Level 1: ReplicationEventStream::subs_mutex_
//            - Purpose: Protects subscription list
//            - Scope: Subscribe/unsubscribe operations
//            - Hold time: MINIMAL (~microseconds)
//            - Pattern: Acquire → copy subscriptions → release → invoke outside
//
//   Level 2: ReplicationEventStream::buffer_mutex_
//            - Purpose: Protects event history buffer
//            - Scope: Buffer append and historical queries
//            - Hold time: MINIMAL (~microseconds)
//            - Pattern: Acquire → buffer op → release
//
// CRITICAL INVARIANT:
//   All callback invocations (emit → callbacks) happen OUTSIDE both locks.
//   This prevents callbacks from acquiring locks that could cause circular wait.
//
// ============================================================================

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

ReplicationEventStream::ReplicationEventStream(const StreamConfig& config)
    : config_(config)
{}

// ---------------------------------------------------------------------------
// Subscription
// ---------------------------------------------------------------------------

ReplicationEventStream::Subscription
ReplicationEventStream::subscribe(EventType type, EventCallback callback)
{
    std::lock_guard<std::mutex> lock(subs_mutex_);
    const uint64_t id = next_id_++;
    subscriptions_.push_back({id, type, std::move(callback)});
    return Subscription(
        std::static_pointer_cast<ReplicationEventStream>(shared_from_this()),
        id);
}

ReplicationEventStream::Subscription
ReplicationEventStream::subscribeAll(EventCallback callback)
{
    std::lock_guard<std::mutex> lock(subs_mutex_);
    const uint64_t id = next_id_++;
    subscriptions_.push_back({id, std::nullopt, std::move(callback)});
    return Subscription(
        std::static_pointer_cast<ReplicationEventStream>(shared_from_this()),
        id);
}

void ReplicationEventStream::unsubscribe(uint64_t subscription_id)
{
    std::lock_guard<std::mutex> lock(subs_mutex_);
    subscriptions_.erase(
        std::remove_if(subscriptions_.begin(), subscriptions_.end(),
            [subscription_id](const SubscriptionRecord& r) {
                return r.id == subscription_id;
            }),
        subscriptions_.end());
}

// ---------------------------------------------------------------------------
// Historical query
// ---------------------------------------------------------------------------

std::vector<ReplicationEventStream::Event>
ReplicationEventStream::getEvents(
    std::chrono::system_clock::time_point start,
    std::chrono::system_clock::time_point end,
    std::optional<EventType> filter) const
{
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    std::vector<Event> result;
    for (const auto& ev : buffer_) {
        if (ev.timestamp < start) continue;
        if (ev.timestamp >= end)  continue;
        if (filter && ev.type != *filter) continue;
        result.push_back(ev);
    }
    return result;
}

size_t ReplicationEventStream::bufferedEventCount() const
{
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    return buffer_.size();
}

// ---------------------------------------------------------------------------
// Internal emit
// ---------------------------------------------------------------------------

void ReplicationEventStream::emit(Event ev)
{
    // Append to ring buffer
    {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        if ([[maybe_unused]] buffer_.size() >= config_.max_history_events) {
            if (config_.drop_oldest_on_full) {
                buffer_.pop_front();
            } else {
                return; // drop new event
            }
        }
        buffer_.push_back(ev);
    }

    // Invoke matching callbacks (outside buffer lock to avoid deadlock)
    std::vector<SubscriptionRecord> snapshot;
    {
        std::lock_guard<std::mutex> lock(subs_mutex_);
        snapshot = subscriptions_;
    }
    for (const auto& sub : snapshot) {
        if (!sub.filter || *sub.filter == ev.type) {
            sub.callback([[maybe_unused]] ev);
        }
    }
}

// ---------------------------------------------------------------------------
// IReplicationListener overrides
// ---------------------------------------------------------------------------

static std::string roleToString(ReplicationRole role) {
    switch (role) {
        case ReplicationRole::LEADER:    return "LEADER";
        case ReplicationRole::FOLLOWER:  return "FOLLOWER";
        case ReplicationRole::CANDIDATE: return "CANDIDATE";
        case ReplicationRole::OBSERVER:  return "OBSERVER";
        case ReplicationRole::WITNESS:   return "WITNESS";
    }
    return "UNKNOWN";
}

static std::string healthToString(HealthStatus s) {
    switch (s) {
        case HealthStatus::HEALTHY:  return "HEALTHY";
        case HealthStatus::DEGRADED: return "DEGRADED";
        case HealthStatus::FAILED:   return "FAILED";
        case HealthStatus::UNKNOWN:  return "UNKNOWN";
    }
    return "UNKNOWN";
}

void ReplicationEventStream::onRoleChange(
    ReplicationRole from, ReplicationRole to)
{
    Event ev;
    ev.type      = EventType::ROLE_CHANGED;
    ev.timestamp = std::chrono::system_clock::now();
    ev.data["from"] = roleToString(from);
    ev.data["to"]   = roleToString(to);
    emit(std::move(ev));
}

void ReplicationEventStream::onLeaderElected(const std::string& leader_id)
{
    Event ev;
    ev.type      = EventType::LEADER_ELECTED;
    ev.timestamp = std::chrono::system_clock::now();
    ev.node_id   = leader_id;
    ev.data["leader_id"] = leader_id;
    emit(std::move(ev));
}

void ReplicationEventStream::onReplicaAdded(const ReplicaInfo& replica)
{
    Event ev;
    ev.type      = EventType::REPLICA_ADDED;
    ev.timestamp = std::chrono::system_clock::now();
    ev.node_id   = replica.node_id;
    ev.data["node_id"]  = replica.node_id;
    ev.data["endpoint"] = replica.endpoint;
    ev.data["role"]     = roleToString(replica.role);
    emit(std::move(ev));
}

void ReplicationEventStream::onReplicaRemoved(const std::string& node_id)
{
    Event ev;
    ev.type      = EventType::REPLICA_REMOVED;
    ev.timestamp = std::chrono::system_clock::now();
    ev.node_id   = node_id;
    ev.data["node_id"] = node_id;
    emit(std::move(ev));
}

void ReplicationEventStream::onConflictDetected(const std::string& document_id)
{
    Event ev;
    ev.type      = EventType::CONFLICT_DETECTED;
    ev.timestamp = std::chrono::system_clock::now();
    ev.data["document_id"] = document_id;
    emit(std::move(ev));
}

void ReplicationEventStream::onReplicationLagWarning(int64_t lag_ms)
{
    Event ev;
    ev.type      = EventType::LAG_WARNING;
    ev.timestamp = std::chrono::system_clock::now();
    ev.data["lag_ms"] = std::to_string(lag_ms);
    emit(std::move(ev));
}

void ReplicationEventStream::onReplicaHealthChanged(
    const std::string& node_id,
    HealthStatus old_status,
    HealthStatus new_status)
{
    Event ev;
    ev.type      = EventType::ROLE_CHANGED; // health change reuses ROLE_CHANGED bucket
    ev.timestamp = std::chrono::system_clock::now();
    ev.node_id   = node_id;
    ev.data["node_id"]    = node_id;
    ev.data["old_status"] = healthToString(old_status);
    ev.data["new_status"] = healthToString(new_status);
    emit(std::move(ev));
}

void ReplicationEventStream::onFailoverStarted(
    const std::string& failed_node,
    const std::string& new_leader)
{
    Event ev;
    ev.type      = EventType::FAILOVER_STARTED;
    ev.timestamp = std::chrono::system_clock::now();
    ev.data["failed_node"] = failed_node;
    ev.data["new_leader"]  = new_leader;
    emit(std::move(ev));
}

void ReplicationEventStream::onFailoverCompleted(
    const std::string& new_leader,
    bool success)
{
    Event ev;
    ev.type      = EventType::FAILOVER_COMPLETED;
    ev.timestamp = std::chrono::system_clock::now();
    ev.node_id   = new_leader;
    ev.data["new_leader"] = new_leader;
    ev.data["success"]    = success ? "true" : "false";
    emit(std::move(ev));
}

void ReplicationEventStream::onNetworkPartitionDetected(
    const std::vector<std::string>& affected)
{
    Event ev;
    ev.type      = EventType::NETWORK_PARTITION;
    ev.timestamp = std::chrono::system_clock::now();
    std::ostringstream nodes_stream;
    for (size_t i = 0; i < affected.size(); ++i) {
        if (i > 0) nodes_stream << ',';
        nodes_stream << affected[i];
    }
    ev.data["affected_nodes"] = nodes_stream.str();
    emit(std::move(ev));
}

void ReplicationEventStream::onWALEntryApplied(const WALEntry& entry)
{
    Event ev;
    ev.type      = EventType::WRITE_REPLICATED;
    ev.timestamp = std::chrono::system_clock::now();
    ev.data["sequence"]   = std::to_string(entry.sequence_number);
    ev.data["collection"] = entry.collection;
    ev.data["operation"]  = entry.operation;
    emit(std::move(ev));
}

} // namespace replication
} // namespace themisdb
