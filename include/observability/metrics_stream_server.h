/**
 * @file metrics_stream_server.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace observability {

// ---------------------------------------------------------------------------
// Supporting types
// ---------------------------------------------------------------------------

/**
 * @brief A label-based filter applied to incoming metric updates.
 *
 * A filter matches when the metric update carries a label whose name equals
 * @c label and whose value equals @c value.  An empty @c value acts as a
 * wildcard and matches any value for the given label name.
 */
struct MetricFilter {
    /// Label name to match (e.g. "tenant_id").
    std::string label = {};
    /// Required label value; empty = match any value.
    std::string value;
};

/**
 * @brief Subscription registered by a single client.
 *
 * Clients name the metrics they want to watch, supply optional label filters
 * to narrow the stream, and specify a minimum delivery interval to throttle
 * high-frequency updates.
 */
struct StreamSubscription {
    /// Unique client identifier (e.g. WebSocket session ID or SSE client ID).
    std::string client_id;
    /// Metric names the client wants to receive.  Empty means subscribe to all.
    std::vector<std::string> metric_names;
    /// Label filters; a metric update must satisfy ALL filters to be delivered.
    std::vector<MetricFilter> filters;
    /// Minimum time between deliveries for this subscription (0 = unlimited).
    std::chrono::milliseconds update_interval{0};
};

/**
 * @brief A single metric observation pushed to the streaming server.
 *
 * Contains the metric name, its current value, an optional label set, and a
 * wall-clock timestamp.  `pushMetrics()` fans this out to all matching
 * subscriptions.
 */
struct MetricUpdate {
    /// Metric name (e.g. "query_latency_ms").
    std::string metric_name;
    /// Observed value.
    double value{0.0};
    /// Optional label set attached to this observation.
    std::map<std::string, std::string> labels;
    /// Wall-clock timestamp of the observation (defaults to now).
    std::chrono::system_clock::time_point timestamp{
        std::chrono::system_clock::now()};
};

// ---------------------------------------------------------------------------
// MetricsStreamServer
// ---------------------------------------------------------------------------

/**
 * @brief Real-time metric streaming server.
 *
 * Acts as the core dispatch layer for WebSocket / SSE metric streaming.
 * Network I/O is decoupled via a @c SendFn callback so the class can be
 * embedded in any HTTP/WebSocket framework without hard dependency.
 *
 * ### Lifecycle
 * ```cpp
 * MetricsStreamServer srv;
 * srv.setDeliveryCallback([](const std::string& client_id,
 *                            const std::string& payload) {
 *     // forward payload to the client's WebSocket / SSE connection
 * });
 * srv.start("0.0.0.0", 8001);
 *
 * // Register a subscription (typically from a parsed WebSocket frame)
 * StreamSubscription sub;
 * sub.client_id = "client-abc";
 * sub.metric_names = {"query_latency_ms", "cache_hit_rate"};
 * sub.filters = {{"tenant_id", "acme"}};
 * sub.update_interval = std::chrono::milliseconds{1000};
 * srv.subscribe(sub);
 *
 * // Called by MetricsCollector or any instrumented subsystem
 * MetricUpdate upd;
 * upd.metric_name = "query_latency_ms";
 * upd.value = 42.5;
 * upd.labels = {{"tenant_id", "acme"}};
 * srv.pushMetrics(upd);
 *
 * srv.stop();
 * ```
 *
 * ### Delivery format
 * Use @c formatWebSocketMessage() / @c formatSseMessage() to serialise a
 * @c MetricUpdate to the respective wire format before handing the payload
 * to the @c SendFn callback (or to any other transport).
 */
class MetricsStreamServer {
public:
    /// Callback invoked for each eligible subscriber when a metric update
    /// is ready for delivery.  The first argument is the @c client_id, the
    /// second is the JSON-serialised update payload.
    using SendFn =
        std::function<void(const std::string& client_id,
                           const std::string& payload)>;

    /// Aggregated runtime statistics.
    struct Stats {
        /// Number of currently active subscriptions.
        size_t active_subscriptions{0};
        /// Total metric updates pushed via pushMetrics().
        uint64_t total_updates_pushed{0};
        /// Total deliveries sent to subscribers (after filter / rate-limit).
        uint64_t total_deliveries{0};
        /// Deliveries skipped because the subscription interval had not elapsed.
        uint64_t throttled_deliveries{0};
        /// Deliveries skipped because the update did not match the subscription.
        uint64_t filtered_deliveries{0};
    };

    MetricsStreamServer();
    ~MetricsStreamServer();

    // Non-copyable, non-movable (owns a mutex)
    MetricsStreamServer(const MetricsStreamServer&) = delete;
    MetricsStreamServer& operator=(const MetricsStreamServer&) = delete;
    MetricsStreamServer(MetricsStreamServer&&) = delete;
    MetricsStreamServer& operator=(MetricsStreamServer&&) = delete;

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    /**
     * @brief Register the delivery callback used to send serialised updates.
     *
     * Must be set before the first call to @c pushMetrics() if actual
     * delivery is required.  Safe to replace at runtime.
     *
     * @param fn  Callback invoked per eligible subscriber (may be null to
     *            disable delivery, e.g. during tests that only inspect stats).
     */
    void setDeliveryCallback(SendFn fn);

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /**
     * @brief Mark the server as running and record its bind coordinates.
     *
     * In an embedded deployment the caller is responsible for creating the
     * actual listening socket (e.g. a Boost.Beast WebSocket acceptor) and
     * routing frames to @c subscribe() / @c unsubscribe() / @c pushMetrics().
     * This method records @p bind_address and @p port for informational
     * purposes and sets the running flag.
     *
     * @throws std::runtime_error if @p bind_address is empty or @p port is 0.
     */
    void start(const std::string& bind_address, uint16_t port);

    /**
     * @brief Mark the server as stopped.
     *
     * Existing subscriptions are preserved; no new deliveries are made after
     * this call returns.
     */
    void stop();

    /** Returns true if the server has been started and not yet stopped. */
    bool isRunning() const noexcept;

    /** Returns the configured bind address (empty if @c start() was not called). */
    std::string bindAddress() const;

    /** Returns the configured port (0 if @c start() was not called). */
    uint16_t port() const;

    // -------------------------------------------------------------------------
    // Subscription management
    // -------------------------------------------------------------------------

    /**
     * @brief Register or replace a client subscription.
     *
     * If a subscription with the same @c client_id already exists it is
     * overwritten.  At most one subscription per @c client_id is supported.
     *
     * @throws std::invalid_argument if @c subscription.client_id is empty.
     */
    void subscribe(const StreamSubscription& subscription);

    /**
     * @brief Remove a client subscription.
     *
     * No-op if @p client_id is not currently subscribed.
     */
    void unsubscribe(const std::string& client_id);

    /** Returns the number of active subscriptions. */
    size_t subscriptionCount() const;

    /**
     * @brief Returns true if a subscription exists for @p client_id,
     *        false otherwise.
     */
    bool hasSubscription(const std::string& client_id) const;

    // -------------------------------------------------------------------------
    // Metric dispatch
    // -------------------------------------------------------------------------

    /**
     * @brief Fan out @p update to all matching subscriptions.
     *
     * For each active subscription, the method:
     *  1. Checks whether @p update.metric_name is in the subscription's
     *     metric_names list (empty list = subscribe to all).
     *  2. Evaluates all @c MetricFilter entries (AND semantics); if any
     *     filter does not match the update is skipped for that client.
     *  3. Enforces the per-subscription rate limit; if the last delivery to
     *     this client was less than @c update_interval ago the update is
     *     throttled.
     *  4. Serialises the update to JSON and invokes the @c SendFn callback.
     *
     * Thread-safe; may be called concurrently from multiple producer threads.
     */
    void pushMetrics(const MetricUpdate& update);

    // -------------------------------------------------------------------------
    // Serialisation helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Serialise @p update as a JSON WebSocket message payload.
     *
     * Output format:
     * ```json
     * {"type":"metric_update","metric_name":"...","value":...,"labels":{...},"timestamp_ms":...}
     * ```
     */
    static std::string formatWebSocketMessage(const MetricUpdate& update);

    /**
     * @brief Serialise @p update as an SSE event.
     *
     * Output format (ready to write directly to an HTTP/1.1 SSE stream):
     * ```
     * data: {"type":"metric_update",...}\n\n
     * ```
     */
    static std::string formatSseMessage(const MetricUpdate& update);

    // -------------------------------------------------------------------------
    // Observability
    // -------------------------------------------------------------------------

    /** Return a snapshot of current runtime statistics. */
    Stats getStats() const;

    /** Reset all statistics counters to zero. */
    void resetStats();

private:
    // Per-subscription runtime state (not part of the public API surface)
    struct SubscriptionState {
        StreamSubscription subscription;
        /// Wall-clock timestamp of the last successful delivery.
        std::chrono::steady_clock::time_point last_delivery{};
    };

    // -------------------------------------------------------------------------
    // Private helpers
    // -------------------------------------------------------------------------

    /// Return true if @p update.metric_name satisfies the name filter.
    static bool matchesMetricNames(const StreamSubscription& sub,
                                   const MetricUpdate& update) noexcept;

    /// Return true if all label filters in @p sub are satisfied by @p update.
    static bool matchesFilters(const StreamSubscription& sub,
                               const MetricUpdate& update) noexcept;

    /// Serialise labels map to a compact JSON object string.
    static std::string labelsToJson(
        const std::map<std::string, std::string>& labels);

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------
    mutable std::mutex mutex_;

    std::string bind_address_;
    uint16_t port_{0};
    std::atomic<bool> running_{false};

    /// Live subscriptions keyed by client_id.
    std::unordered_map<std::string, SubscriptionState> subscriptions_;

    /// Delivery callback (may be null).
    SendFn send_fn_;

    // Statistics
    std::atomic<uint64_t> total_updates_pushed_{0};
    std::atomic<uint64_t> total_deliveries_{0};
    std::atomic<uint64_t> throttled_deliveries_{0};
    std::atomic<uint64_t> filtered_deliveries_{0};
};

} // namespace observability
} // namespace themis
