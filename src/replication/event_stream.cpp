/**
 * @file event_stream.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
    /**
     * @brief Lock.
     * @param[in] subs_mutex_ Input parameter.
     * @return Return value.
     */
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
    /**
     * @brief Lock.
     * @param[in] subs_mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(subs_mutex_);
    const uint64_t id = next_id_++;
    subscriptions_.push_back({id, std::nullopt, std::move(callback)});
    return Subscription(
        std::static_pointer_cast<ReplicationEventStream>(shared_from_this()),
        id);
}

/**
 * @brief Unsubscribe.
 * @param[in] subscription_id Identifier of the subscription.
 */
void ReplicationEventStream::unsubscribe(uint64_t subscription_id)
{
    /**
     * @brief Lock.
     * @param[in] subs_mutex_ Input parameter.
     * @return Return value.
     */
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
    /**
     * @brief Lock.
     * @param[in] buffer_mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    std::vector<Event> result = {};

    for (const auto& ev : buffer_) {
        if (ev.timestamp < start) {
          continue;
        }
        if (ev.timestamp >= end) {
          continue;
        }
        if (filter && ev.type != *filter) {
          continue;
        }
        result.push_back(ev);
    }
    return result;
}

size_t ReplicationEventStream::bufferedEventCount() const
{
    /**
     * @brief Lock.
     * @param[in] buffer_mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    return buffer_.size();
}

// ---------------------------------------------------------------------------
// Internal emit
// ---------------------------------------------------------------------------

/**
 * @brief Emit.
 * @param[in] ev Input parameter.
 */
void ReplicationEventStream::emit(Event ev)
{
    // Append to ring buffer
    {
        /**
         * @brief Lock.
         * @param[in] buffer_mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        if (buffer_.size() >= config_.max_history_events) {
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
        /**
         * @brief Lock.
         * @param[in] subs_mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(subs_mutex_);
        snapshot = subscriptions_;
    }
    for (const auto& sub : snapshot) {
        if (!sub.filter || *sub.filter == ev.type) {
            sub.callback(ev);
        }
    }
}

// ---------------------------------------------------------------------------
// IReplicationListener overrides
// ---------------------------------------------------------------------------

/**
 * @brief Role To String.
 * @param[in] role Input parameter.
 * @return Return value.
 * @details Implements roleToString without additional internal calls.
 */
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

/**
 * @brief Health To String.
 * @param[in] s Input parameter.
 * @return Return value.
 * @details Implements healthToString without additional internal calls.
 */
static std::string healthToString(HealthStatus s) {
    switch (s) {
        case HealthStatus::HEALTHY:  return "HEALTHY";
        case HealthStatus::DEGRADED: return "DEGRADED";
        case HealthStatus::FAILED:   return "FAILED";
        case HealthStatus::UNKNOWN:  return "UNKNOWN";
    }
    return "UNKNOWN";
}

/**
 * @brief On Role Change.
 * @param[in] from Input parameter.
 * @param[in] to Input parameter.
 */
void ReplicationEventStream::onRoleChange(
    ReplicationRole from, ReplicationRole to)
{
    Event ev = Event();
    ev.type      = EventType::ROLE_CHANGED;
    ev.timestamp = std::chrono::system_clock::now();
    ev.data["from"] = roleToString(from);
    ev.data["to"]   = roleToString(to);
    emit(std::move(ev));
}

/**
 * @brief On Leader Elected.
 * @param[in] leader_id Identifier of the leader.
 */
void ReplicationEventStream::onLeaderElected(const std::string& leader_id)
{
    Event ev = Event();
    ev.type      = EventType::LEADER_ELECTED;
    ev.timestamp = std::chrono::system_clock::now();
    ev.node_id   = leader_id;
    ev.data["leader_id"] = leader_id;
    emit(std::move(ev));
}

/**
 * @brief On Replica Added.
 * @param[in] replica Input parameter.
 */
void ReplicationEventStream::onReplicaAdded(const ReplicaInfo& replica)
{
    Event ev = Event();
    ev.type      = EventType::REPLICA_ADDED;
    ev.timestamp = std::chrono::system_clock::now();
    ev.node_id   = replica.node_id;
    ev.data["node_id"]  = replica.node_id;
    ev.data["endpoint"] = replica.endpoint;
    ev.data["role"]     = roleToString(replica.role);
    emit(std::move(ev));
}

/**
 * @brief On Replica Removed.
 * @param[in] node_id Identifier of the node.
 */
void ReplicationEventStream::onReplicaRemoved(const std::string& node_id)
{
    Event ev = Event();
    ev.type      = EventType::REPLICA_REMOVED;
    ev.timestamp = std::chrono::system_clock::now();
    ev.node_id   = node_id;
    ev.data["node_id"] = node_id;
    emit(std::move(ev));
}

/**
 * @brief On Conflict Detected.
 * @param[in] document_id Identifier of the document.
 */
void ReplicationEventStream::onConflictDetected(const std::string& document_id)
{
    Event ev = Event();
    ev.type      = EventType::CONFLICT_DETECTED;
    ev.timestamp = std::chrono::system_clock::now();
    ev.data["document_id"] = document_id;
    emit(std::move(ev));
}

/**
 * @brief On Replication Lag Warning.
 * @param[in] lag_ms Input parameter.
 */
void ReplicationEventStream::onReplicationLagWarning(int64_t lag_ms)
{
    Event ev = Event();
    ev.type      = EventType::LAG_WARNING;
    ev.timestamp = std::chrono::system_clock::now();
    ev.data["lag_ms"] = std::to_string(lag_ms);
    emit(std::move(ev));
}

/**
 * @brief On Replica Health Changed.
 * @param[in] node_id Identifier of the node.
 * @param[in] old_status Input parameter.
 * @param[in] new_status Input parameter.
 */
void ReplicationEventStream::onReplicaHealthChanged(
    const std::string& node_id,
    HealthStatus old_status,
    HealthStatus new_status)
{
    Event ev = Event();
    ev.type      = EventType::ROLE_CHANGED; // health change reuses ROLE_CHANGED bucket
    ev.timestamp = std::chrono::system_clock::now();
    ev.node_id   = node_id;
    ev.data["node_id"]    = node_id;
    ev.data["old_status"] = healthToString(old_status);
    ev.data["new_status"] = healthToString(new_status);
    emit(std::move(ev));
}

/**
 * @brief On Failover Started.
 * @param[in] failed_node Input parameter.
 * @param[in] new_leader Input parameter.
 */
void ReplicationEventStream::onFailoverStarted(
    const std::string& failed_node,
    const std::string& new_leader)
{
    Event ev = Event();
    ev.type      = EventType::FAILOVER_STARTED;
    ev.timestamp = std::chrono::system_clock::now();
    ev.data["failed_node"] = failed_node;
    ev.data["new_leader"]  = new_leader;
    emit(std::move(ev));
}

/**
 * @brief On Failover Completed.
 * @param[in] new_leader Input parameter.
 * @param[in] success Input parameter.
 */
void ReplicationEventStream::onFailoverCompleted(
    const std::string& new_leader,
    bool success)
{
    Event ev = Event();
    ev.type      = EventType::FAILOVER_COMPLETED;
    ev.timestamp = std::chrono::system_clock::now();
    ev.node_id   = new_leader;
    ev.data["new_leader"] = new_leader;
    ev.data["success"]    = success ? "true" : "false";
    emit(std::move(ev));
}

/**
 * @brief On Network Partition Detected.
 * @param[in] affected Input parameter.
 */
void ReplicationEventStream::onNetworkPartitionDetected(
    const std::vector<std::string>& affected)
{
    Event ev = Event();
    ev.type      = EventType::NETWORK_PARTITION;
    ev.timestamp = std::chrono::system_clock::now();
    std::ostringstream nodes_stream = {};
    for (size_t i = 0; i < affected.size(); ++i) {
        if (i > 0) {
          nodes_stream << ',';
        }
        nodes_stream << affected[i];
    }
    ev.data["affected_nodes"] = nodes_stream.str();
    emit(std::move(ev));
}

/**
 * @brief On WALEntry Applied.
 * @param[in] entry Input parameter.
 */
void ReplicationEventStream::onWALEntryApplied(const WALEntry& entry)
{
    Event ev = Event();
    ev.type      = EventType::WRITE_REPLICATED;
    ev.timestamp = std::chrono::system_clock::now();
    ev.data["sequence"]   = std::to_string(entry.sequence_number);
    ev.data["collection"] = entry.collection;
    ev.data["operation"]  = entry.operation;
    emit(std::move(ev));
}

} // namespace replication
} // namespace themisdb
