/**
 * @file event_stream.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Replication Event Stream API
 *
 * Captures replication lifecycle events and delivers them to registered
 * callbacks.  Supports:
 *   - Selective subscription by EventType
 *   - Wildcard (subscribeAll) for audit / analytics pipelines
 *   - RAII subscription handles that auto-unsubscribe on destruction
 *   - Historical event query over an in-memory ring buffer (default: 10 000 events)
 *
 * ReplicationEventStream implements IReplicationListener, so it can be
 * registered directly with ReplicationManager::addListener().
 *
 * Design constraints:
 *   - Event callbacks are invoked on the replication background thread; they
 *     must not block for more than 1 ms or spawn synchronous I/O.
 *   - Subscription handles auto-unsubscribe when destroyed (RAII).
 *   - getEvents() is O(N) in buffer size; use appropriate time ranges.
 *
 * Target: v1.7.0
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "replication/replication_manager.h"

#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace themisdb {
namespace replication {

// ============================================================================
// EventStream configuration (at namespace scope to avoid GCC nested-struct
// default-argument issues)
// ============================================================================

struct ReplicationEventStreamConfig {
    size_t max_history_events  = 10000; ///< Ring-buffer capacity
    bool   drop_oldest_on_full = true;  ///< If false, new events are dropped when full
};

/**
 * ReplicationEventStream
 *
 * Captures all replication lifecycle events that flow through
 * IReplicationListener and delivers them to subscriber callbacks.
 *
 * Usage:
 * @code
 *   auto stream = std::make_shared<ReplicationEventStream>();
 *   repl_mgr.addListener(stream);
 *
 *   // Subscribe to a specific event type
 *   auto handle = stream->subscribe(
 *       ReplicationEventStream::EventType::FAILOVER_COMPLETED,
 *       [](const ReplicationEventStream::Event& ev) { ... }
 *   );
 *   // handle auto-unsubscribes when it goes out of scope
 *
 *   // Wildcard subscription
 *   auto audit_handle = stream->subscribeAll([](const auto& ev) { log(ev); });
 * @endcode
 */
class ReplicationEventStream
    : public IReplicationListener
    , public std::enable_shared_from_this<ReplicationEventStream> {
public:
    // -----------------------------------------------------------------------
    // Event type enumeration
    // -----------------------------------------------------------------------
    enum class EventType {
        WRITE_REPLICATED,
        ROLE_CHANGED,
        LEADER_ELECTED,
        REPLICA_ADDED,
        REPLICA_REMOVED,
        FAILOVER_STARTED,
        FAILOVER_COMPLETED,
        CONFLICT_DETECTED,
        CONFLICT_RESOLVED,
        LAG_WARNING,
        NETWORK_PARTITION
    };

    // -----------------------------------------------------------------------
    // Event record
    // -----------------------------------------------------------------------
    struct Event {
        EventType                             type;
        std::chrono::system_clock::time_point timestamp;
        std::string                           node_id;   ///< Source node (may be empty)
        std::map<std::string, std::string>    data;      ///< Key-value payload
    };

    using EventCallback = std::function<void(const Event&)>;

    // -----------------------------------------------------------------------
    // RAII subscription handle
    // -----------------------------------------------------------------------

    /**
     * Subscription
     *
     * Returned by subscribe()/subscribeAll().  Calls unsubscribe() automatically
     * when destroyed.  Move-only; non-copyable.
     */
    class Subscription {
    public:
        Subscription() = default;
        Subscription(std::shared_ptr<ReplicationEventStream> stream, uint64_t id)
            : stream_(std::move(stream)), id_(id) {}
        ~Subscription() { cancel(); }

        // Move-only
        Subscription(Subscription&&) noexcept = default;
        Subscription& operator=(Subscription&&) noexcept = default;
        Subscription(const Subscription&)            = delete;
        Subscription& operator=(const Subscription&) = delete;

        /** Manually cancel this subscription before destruction. */
        void cancel() {
            if (auto s = stream_.lock()) {
                s->unsubscribe(id_);
            }
            stream_.reset();
        }

        uint64_t id() const { return id_; }

    private:
        std::weak_ptr<ReplicationEventStream> stream_;
        uint64_t                              id_ = 0;
    };

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------
    using StreamConfig = ReplicationEventStreamConfig;

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------
    explicit ReplicationEventStream(const StreamConfig& config = StreamConfig{});
    ~ReplicationEventStream() override = default;

    ReplicationEventStream(const ReplicationEventStream&)            = delete;
    ReplicationEventStream& operator=(const ReplicationEventStream&) = delete;

    // -----------------------------------------------------------------------
    // Subscription API
    // -----------------------------------------------------------------------

    /**
     * Subscribe to a specific event type.
     * Returns a Subscription handle; the subscription is active until the
     * handle is destroyed or cancel() is called.
     *
     * @param type     Only events of this type are delivered to callback.
     * @param callback Invoked synchronously on the emitting thread; must not block.
     */
    Subscription subscribe(EventType type, EventCallback callback);

    /**
     * Subscribe to all event types.
     * @param callback Invoked for every event regardless of type.
     */
    Subscription subscribeAll(EventCallback callback);

    /**
     * Manually unsubscribe a previously registered handler.
     * Safe to call after the stream is destroyed (no-op).
     */
    void unsubscribe(uint64_t subscription_id);

    // -----------------------------------------------------------------------
    // Historical query
    // -----------------------------------------------------------------------

    /**
     * Return events from the in-memory ring buffer within [start, end).
     * If filter is set only events of that type are returned.
     * Events are returned in chronological order.
     */
    std::vector<Event> getEvents(
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end,
        std::optional<EventType>              filter = std::nullopt
    ) const;

    /** Number of events currently held in the ring buffer. */
    size_t bufferedEventCount() const;

    // -----------------------------------------------------------------------
    // IReplicationListener overrides — all translate to Event records
    // -----------------------------------------------------------------------
    void onRoleChange(ReplicationRole from, ReplicationRole to) override;
    void onLeaderElected(const std::string& leader_id) override;
    void onReplicaAdded(const ReplicaInfo& replica) override;
    void onReplicaRemoved(const std::string& node_id) override;
    void onConflictDetected(const std::string& document_id) override;
    void onReplicationLagWarning(int64_t lag_ms) override;
    void onReplicaHealthChanged(const std::string& node_id,
                                HealthStatus old_status,
                                HealthStatus new_status) override;
    void onFailoverStarted(const std::string& failed_node,
                           const std::string& new_leader) override;
    void onFailoverCompleted(const std::string& new_leader,
                             bool success) override;
    void onNetworkPartitionDetected(const std::vector<std::string>& affected) override;
    void onWALEntryApplied(const WALEntry& entry) override;

private:
    StreamConfig config_;

    // Registered subscriptions
    struct SubscriptionRecord {
        uint64_t                    id = 0;
        std::optional<EventType>    filter; ///< nullopt = wildcard
        EventCallback               callback;
    };

    mutable std::mutex               subs_mutex_;
    std::vector<SubscriptionRecord>  subscriptions_;
    std::atomic<uint64_t>            next_id_{1};

    // Ring buffer
    mutable std::mutex               buffer_mutex_;
    std::deque<Event>                buffer_;

    /** Emit an event: append to ring buffer and invoke matching callbacks. */
    void emit(Event ev);
};

} // namespace replication
} // namespace themisdb
