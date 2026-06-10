/**
 * @file subscription_multiplexer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: subscription_multiplexer.h | Version: 0.1.0 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 128
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace api {

// ---------------------------------------------------------------------------
// SubscriptionFilter — describes a single topic subscription with optional
// resume point and AQL-style filter expression
// ---------------------------------------------------------------------------

/**
 * @brief Filter descriptor for a single topic subscription.
 *
 * A connection may hold multiple SubscriptionFilters simultaneously.
 * `filter_expr` is evaluated server-side before delivery so that only
 * matching events are forwarded to the subscriber.
 */
struct SubscriptionFilter {
    std::string topic;            ///< Topic/channel to subscribe to (non-empty).
    std::string filter_expr;      ///< Optional AQL-style filter expression.
    int64_t     last_event_id = -1; ///< Resume from event ID; -1 = from latest.
};

// ---------------------------------------------------------------------------
// SubscriptionEvent — a single event delivered to subscribers of a topic
// ---------------------------------------------------------------------------

/**
 * @brief A single event published to a topic.
 *
 * `event_id` is monotonically increasing per topic and is used by
 * resumable subscriptions to request missed events on reconnect.
 */
struct SubscriptionEvent {
    int64_t     event_id       = 0;
    std::string topic;
    std::string payload_json;
    std::chrono::system_clock::time_point timestamp;
};

// ---------------------------------------------------------------------------
// ISubscriptionMultiplexer — multi-subscription fan-out interface
// ---------------------------------------------------------------------------

/**
 * @brief Pure-virtual interface for multi-subscription fan-out.
 *
 * Manages the mapping from connection IDs to sets of topic filters and
 * delivers published events to all matching subscribers.
 *
 * ### Thread safety
 * All methods must be safe to call concurrently from multiple threads.
 *
 * ### Contract
 * - `subscribe()` replaces any existing filters for the given connection
 *   on the listed topics; it does not clear topics not mentioned.
 * - `unsubscribe()` with an empty `topics` vector removes the connection
 *   entirely (all topic subscriptions are dropped).
 * - `publish()` returns the number of connections the event was delivered to.
 */
class ISubscriptionMultiplexer {
public:
    virtual ~ISubscriptionMultiplexer() = default;

    /**
     * @brief Subscribe a connection to a set of topics.
     *
     * @param connection_id  Opaque client connection identifier (non-empty).
     * @param filters        One or more topic filters to apply.
     * @return `true` on success; `false` if the connection_id or any
     *         filter is invalid.
     */
    virtual bool subscribe(
        const std::string& connection_id,
        const std::vector<SubscriptionFilter>& filters
    ) = 0;

    /**
     * @brief Unsubscribe a connection from specific topics.
     *
     * @param connection_id  Connection to modify.
     * @param topics         Topics to remove; if empty, removes the
     *                       connection from all topics.
     * @return `true` if the connection existed and was modified.
     */
    virtual bool unsubscribe(
        const std::string& connection_id,
        const std::vector<std::string>& topics = {}
    ) = 0;

    /**
     * @brief Publish an event to all subscribers of the event's topic.
     *
     * Filter expressions are evaluated before delivery; only connections
     * whose filter matches the payload are counted and notified.
     *
     * @return Number of connections the event was delivered to.
     */
    virtual size_t publish(const SubscriptionEvent& event) = 0;

    /// Return the number of active subscribers for @p topic.
    virtual size_t subscriberCount(const std::string& topic) const = 0;

    /// Return all topics that have at least one active subscriber.
    virtual std::vector<std::string> activeTopics() const = 0;

    /// Return the total number of tracked connections.
    virtual size_t connectionCount() const = 0;
};

} // namespace api
} // namespace themis
