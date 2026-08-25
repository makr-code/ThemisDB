/**
 * @file changefeed_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <optional>
#include <atomic>
#include <vector>
#include <set>
#include <ostream>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include "server/auth_middleware.h"
#include "cdc/delivery_tracker.h"

namespace themis {

// Forward declarations
class RocksDBWrapper;
class Changefeed;

namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

class SseConnectionManager;

/**
 * @brief Async SSE stream for event-driven delivery without polling.
 *
 * Manages the complete lifecycle of an SSE stream subscription:
 * - Subscribes to the changefeed using the event-driven subscription model
 * - Buffers incoming events with backpressure handling (bounded queue)
 * - Handles client disconnections gracefully
 * - Cleans up subscription on stream close (RAII)
 * - Formats events as SSE and writes to the output stream
 *
 * Usage:
 * ```cpp
 * AsyncSSEStream stream(changefeed, output_stream, consumer_id, max_seconds);
 * stream.run(key_prefix, event_types);  // Blocks until stream closes or timeout
 * ```
 */
/**
 * @brief Stream configuration for AsyncSSEStream backpressure and event handling.
 *
 * Defined outside AsyncSSEStream so it can be used as a default parameter
 * in the constructor declaration (GCC 13 restriction on nested-struct
 * aggregate initialization inside a class body).
 */
struct AsyncSSEStreamConfig {
    /// Maximum events to buffer before dropping oldest (backpressure)
    size_t max_buffered_events = 1000;
    /// Heartbeat interval in milliseconds (0 = disabled)
    uint32_t heartbeat_interval_ms = 15000;
    /// Maximum duration before stream auto-closes (in seconds)
    uint32_t max_duration_seconds = 300;
    /// Backpressure strategy: true = drop oldest, false = drop newest
    bool drop_oldest_on_overflow = true;
};

class AsyncSSEStream {
public:
    /// @brief Alias for the stream configuration type.
    using Config = AsyncSSEStreamConfig;

    /**
     * @brief Construct an async SSE stream.
     *
     * @param changefeed Shared changefeed source for subscriptions
     * @param output Output stream for SSE lines
     * @param consumer_id Optional consumer ID for at-least-once tracking
     * @param config Stream configuration
     */
    AsyncSSEStream(
        std::shared_ptr<Changefeed> changefeed,
        std::ostream& output,
        const std::string& consumer_id = "",
        const Config& config = Config{}
    );

    /**
     * @brief Destructor cleans up subscription automatically (RAII).
     */
    ~AsyncSSEStream() noexcept;

    /**
     * @brief Start the async stream with optional filtering.
     *
     * Blocks until the stream times out, client disconnects, or an error occurs.
     * Events are delivered via the subscription callback.
     *
     * @param key_prefix Optional key prefix filter (empty = no filter)
     * @param event_types Optional event type filters (empty = all types)
     * @return Number of events delivered
     */
    size_t run(
        const std::string& key_prefix = "",
        const std::set<Changefeed::ChangeEventType>& event_types = {}
    );

    /**
     * @brief Signal the stream to close gracefully.
     *
     * Called by client disconnect handler or timeout.
     */
    void close() noexcept;

    /**
     * @brief Get the number of events that were delivered.
     */
    size_t getEventCount() const noexcept { return event_count_; }

    /**
     * @brief Get the number of events dropped due to backpressure.
     */
    size_t getDroppedEventCount() const noexcept { return dropped_events_; }

    /**
     * @brief Get the number of heartbeat comments sent.
     */
    size_t getHeartbeatCount() const noexcept { return heartbeat_count_; }

    /**
     * @brief Get the raw events that were delivered.
     * 
     * Returns a copy of the events that were successfully delivered
     * to the output stream. Useful for at-least-once delivery tracking.
     */
    std::vector<Changefeed::ChangeEvent> getDeliveredEvents() const noexcept;

private:
    /// Event queue entry (raw event with additional metadata)
    struct QueuedEvent {
        Changefeed::ChangeEvent event;
        int64_t enqueued_at_ms = 0;
    };

    /// Subscription callback handler (invoked on changefeed events)
    void onChangeEvent(const Changefeed::ChangeEvent& evt) noexcept;

    /// Drain queued events and write them to the output stream
    void drainEventQueue() noexcept;

    /// Send a heartbeat comment
    void sendHeartbeat() noexcept;

    std::shared_ptr<Changefeed> changefeed_;
    std::ostream& output_;
    std::string consumer_id_;
    Config config_;

    /// Subscription handle (auto-unsubscribes on destruction)
    Changefeed::SubscriptionHandle subscription_handle_;

    /// Thread-safe event queue for backpressure handling
    mutable std::mutex queue_mutex_;
    std::vector<QueuedEvent> event_queue_;
    
    /// Delivered events for at-least-once tracking
    mutable std::mutex delivered_mutex_;
    std::vector<Changefeed::ChangeEvent> delivered_events_;

    /// Stream state
    std::atomic<bool> active_{true};
    std::atomic<bool> should_close_{false};

    /// Statistics
    std::atomic<size_t> event_count_{0};
    std::atomic<size_t> dropped_events_{0};
    std::atomic<size_t> heartbeat_count_{0};

    /// Heartbeat timing
    std::chrono::steady_clock::time_point last_heartbeat_time_;
    std::chrono::steady_clock::time_point stream_start_time_;
};

/**
 * @brief Handler for Changefeed (CDC) Operations
 * 
 * This handler manages all changefeed-related endpoints:
 * - GET /changefeed - Get changefeed events (polling)
 * - GET /changefeed/stream - Stream changefeed events via Server-Sent Events (SSE)
 * - GET /changefeed/stats - Get changefeed statistics
 * - POST /changefeed/retention - Configure retention policy
 * 
 * Features:
 * - Change Data Capture (CDC)
 * - Real-time event streaming
 * - SSE (Server-Sent Events) support
 * - Event filtering and pagination
 * - Retention management
 * 
 * Extracted from http_server.cpp (~400 lines) to improve maintainability.
 */
class ChangefeedApiHandler {
public:
    // ─── Async SSE stream writer bridge (stub #305 resolution) ──────────────

    /**
     * @brief Injection bridge for a production async SSE write loop.
     *
     * When registered via `setSseStreamWriterFn()`, the SSE keep-alive
     * handler delegates the entire event-push lifecycle to this function
     * instead of running its own sync busy-wait poll loop.  This allows the
     * caller (e.g., an Asio-backed HTTP/2 session) to drive writes through
     * its own async strand.
     *
     * The function must:
     *  - Call `mgr.pollRawEvents(conn_id, …)` to drain buffered raw events.
     *  - Format each event as SSE (`"id: N\ndata: {...}\n\n"`) and write to
     *    `body`.
     *  - Send heartbeat comments (`": heartbeat\n\n"`) as needed.
     *  - Return when `max_duration` has elapsed or the client disconnects.
     *
     * The handler's delivery_tracker is NOT passed here; callers that need
     * at-least-once tracking should obtain raw events from the manager and
     * call the tracker themselves via a separate injection.
     *
     * @param mgr              Active SSE connection manager.
     * @param conn_id          Connection ID returned by registerConnection().
     * @param body             Output stream for SSE lines.
     * @param max_duration     Maximum wall-clock duration for the stream.
     * @param heartbeat_ms     Heartbeat interval hint (0 = use manager default).
     * @param max_events_per_poll Maximum events to drain per poll cycle.
     */
    using SseStreamWriterFn = std::function<void(
        SseConnectionManager& mgr,
        uint64_t conn_id,
        std::ostream& body,
        std::chrono::seconds max_duration,
        uint32_t heartbeat_ms,
        size_t max_events_per_poll
    )>;

    /**
     * @brief Register an async SSE stream writer for keep-alive connections.
     *
     * Thread-safe.  Replaces any previously registered writer.
     *
     * @param fn  Writer function; may be empty to clear.
     */
    static void setSseStreamWriterFn(SseStreamWriterFn fn);

    /**
     * @brief Clear any previously registered SSE stream writer.
     *
     * After this call the handler reverts to its built-in sync poll loop.
     * Thread-safe.
     */
    static void clearSseStreamWriterFn();

    /**
     * @brief Construct a new Changefeed API Handler
     * 
     * @param storage Storage backend
     * @param changefeed Changefeed manager
     * @param sse_manager SSE connection manager (optional)
     * @param auth Authentication/authorization middleware
     * @param feature_cdc Whether CDC feature is enabled
     */
    ChangefeedApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<Changefeed> changefeed,
        std::shared_ptr<SseConnectionManager> sse_manager,
        std::shared_ptr<themis::AuthMiddleware> auth,
        bool feature_cdc
    );

    /**
     * @brief Handle GET /changefeed request
     * @param req HTTP request with query parameters (since, limit, filter)
     * @return HTTP response with changefeed events
     */
    http::response<http::string_body> handleGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /changefeed/stream request (SSE)
     * @param req HTTP request for SSE stream
     * @return HTTP response with SSE stream
     */
    http::response<http::string_body> handleStreamSse(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /changefeed/stats request
     * @param req HTTP request
     * @return HTTP response with changefeed statistics
     */
    http::response<http::string_body> handleStats(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /changefeed/retention request
     * @param req HTTP request with retention configuration
     * @return HTTP response with update status
     */
    http::response<http::string_body> handleRetention(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /changefeed/retention request
     *
     * Returns the current retention status (log size, oldest event age,
     * next scheduled cleanup time, compact_on_cleanup flag).
     */
    http::response<http::string_body> handleRetentionGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle PUT /changefeed/retention request
     *
     * Updates the retention policy at runtime.  Accepted JSON fields:
     *   enabled (bool), max_age_hours (uint32), max_event_count (uint64),
     *   max_size_bytes (uint64), cleanup_interval_minutes (uint32),
     *   compact_on_cleanup (bool)
     */
    http::response<http::string_body> handleRetentionPut(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /changefeed/compact request
     *
     * Triggers a key-based log compaction: removes superseded entries,
     * keeping only the latest event per document key.
     *
     * @param req HTTP request
     * @return HTTP response with compaction statistics
     */
    http::response<http::string_body> handleCompact(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /changefeed/stream/ack request
     *
     * Acknowledges delivery of SSE change events, clearing them from the
     * at-least-once in-flight tracking state for the given consumer.
     *
     * Accepted JSON body fields:
     *   consumer_id      (string, required) – opaque consumer identifier
     *   up_to_sequence   (uint64, required) – highest sequence number to acknowledge
     *
     * @param req HTTP request with JSON body
     * @return HTTP response with acknowledgement result
     */
    http::response<http::string_body> handleStreamAck(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /changefeed/redact request (GDPR right to erasure)
     *
     * Scrubs PII from all change log entries whose key starts with the
     * supplied key prefix.  Replaces the value, before_snapshot, and
     * after_snapshot fields with "[REDACTED]" / nullopt and sets
     * redacted = true.  Audit-critical fields (sequence, type, key,
     * timestamp_ms) are preserved.
     *
     * Accepted JSON body fields:
     *   key_prefix   (string, required) – data-subject key prefix, e.g. "user:42"
     *   tenant_id    (string, optional) – tenant scope for the audit record
     *   operator_id  (string, optional) – identity of the requesting operator
     *
     * @param req HTTP request with JSON body
     * @return HTTP response with GDPRRedactionResult JSON
     */
    http::response<http::string_body> handleGdprRedact(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<Changefeed> changefeed_;
    std::shared_ptr<SseConnectionManager> sse_manager_;
    std::shared_ptr<themis::AuthMiddleware> auth_;
    bool feature_cdc_;

    /// At-least-once delivery tracker for SSE consumers.
    /// Tracks in-flight events per consumer_id until the client ACKs them.
    cdc::DeliveryTracker delivery_tracker_;

    // Static async SSE stream writer bridge (guarded by sse_writer_mutex_).
    static SseStreamWriterFn sse_stream_writer_fn_;
    static std::mutex        sse_writer_mutex_;

    // Helper methods (to be implemented)
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
    
    // Authorization helper
    std::optional<http::response<http::string_body>> checkAuth(
        const http::request<http::string_body>& req, const std::string& required_scope);
    
    // Tenant isolation helper
    struct TenantAuthContext {
        std::string user_id;
        std::string tenant_id;
        std::vector<std::string> groups;
    };
    std::optional<http::response<http::string_body>> checkAuthAndResolveTenant(
        const http::request<http::string_body>& req, 
        const std::string& required_scope,
        TenantAuthContext& out_context);
    
    // Governance headers
    void applyGovernanceHeaders(
        const http::request<http::string_body>& req,
        http::response<http::string_body>& res);
};

} // namespace server
} // namespace themis
