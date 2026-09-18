/**
 * @file changefeed_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct AsyncSSEStreamConfig {
    size_t max_buffered_events = 1000;
    uint32_t heartbeat_interval_ms = 15000;
    uint32_t max_duration_seconds = 300;
    bool drop_oldest_on_overflow = true;
};

class AsyncSSEStream {
public:
    using Config = AsyncSSEStreamConfig;

    AsyncSSEStream(
        std::shared_ptr<Changefeed> changefeed,
        std::ostream& output,
        const std::string& consumer_id = "",
        const Config& config = Config{}
    );

    ~AsyncSSEStream() noexcept;

    size_t run(
        const std::string& key_prefix = "",
        const std::set<Changefeed::ChangeEventType>& event_types = {}
    );

    /**
     * @brief Close.
     * @note Exception safety: noexcept.
     */
    void close() noexcept;

    size_t getEventCount() const noexcept { return event_count_; }

    size_t getDroppedEventCount() const noexcept { return dropped_events_; }

    size_t getHeartbeatCount() const noexcept { return heartbeat_count_; }

    /**
     * @brief Get Delivered Events.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    std::vector<Changefeed::ChangeEvent> getDeliveredEvents() const noexcept;

private:
    struct QueuedEvent {
        Changefeed::ChangeEvent event;
        int64_t enqueued_at_ms = 0;
    };

    /**
     * @brief On Change Event.
     * @param[in] evt Input parameter.
     * @note Exception safety: noexcept.
     */
    void onChangeEvent(const Changefeed::ChangeEvent& evt) noexcept;

    /**
     * @brief Drain Event Queue.
     * @note Exception safety: noexcept.
     */
    void drainEventQueue() noexcept;

    /**
     * @brief Send Heartbeat.
     * @note Exception safety: noexcept.
     */
    void sendHeartbeat() noexcept;

    std::shared_ptr<Changefeed> changefeed_;
    std::ostream& output_;
    std::string consumer_id_;
    Config config_;

    Changefeed::SubscriptionHandle subscription_handle_;

    mutable std::mutex queue_mutex_;
    std::vector<QueuedEvent> event_queue_;
    
    mutable std::mutex delivered_mutex_;
    std::vector<Changefeed::ChangeEvent> delivered_events_;

    std::atomic<bool> active_{true};
    std::atomic<bool> should_close_{false};

    std::atomic<size_t> event_count_{0};
    std::atomic<size_t> dropped_events_{0};
    std::atomic<size_t> heartbeat_count_{0};

    std::chrono::steady_clock::time_point last_heartbeat_time_;
    std::chrono::steady_clock::time_point stream_start_time_;
};

class ChangefeedApiHandler {
public:
    // ─── Async SSE stream writer bridge (stub #305 resolution) ──────────────

    using SseStreamWriterFn = std::function<void(
        SseConnectionManager& mgr,
        uint64_t conn_id,
        std::ostream& body,
        std::chrono::seconds max_duration,
        uint32_t heartbeat_ms,
        size_t max_events_per_poll
    )>;

    /**
     * @brief Set Sse Stream Writer Fn.
     * @param[in] fn Input parameter.
     */
    static void setSseStreamWriterFn(SseStreamWriterFn fn);

    /**
     * @brief Clear Sse Stream Writer Fn.
     */
    static void clearSseStreamWriterFn();

    ChangefeedApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<Changefeed> changefeed,
        std::shared_ptr<SseConnectionManager> sse_manager,
        std::shared_ptr<themis::AuthMiddleware> auth,
        bool feature_cdc
    );

    /**
     * @brief Handle Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle Stream Sse.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleStreamSse(const http::request<http::string_body>& req);

    /**
     * @brief Handle Stats.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleStats(const http::request<http::string_body>& req);

    /**
     * @brief Handle Retention.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRetention(const http::request<http::string_body>& req);

    /**
     * @brief Handle Retention Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRetentionGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle Retention Put.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRetentionPut(const http::request<http::string_body>& req);

    /**
     * @brief Handle Compact.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCompact(const http::request<http::string_body>& req);

    /**
     * @brief Handle Stream Ack.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleStreamAck(const http::request<http::string_body>& req);

    /**
     * @brief Handle Gdpr Redact.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGdprRedact(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<Changefeed> changefeed_;
    std::shared_ptr<SseConnectionManager> sse_manager_;
    std::shared_ptr<themis::AuthMiddleware> auth_;
    bool feature_cdc_;

    cdc::DeliveryTracker delivery_tracker_;

    // Static async SSE stream writer bridge (guarded by sse_writer_mutex_).
    static SseStreamWriterFn sse_stream_writer_fn_;
    static std::mutex        sse_writer_mutex_;

    /**
     * @brief Make Error Response.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    /**
     * @brief Make Response.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
    
    // Authorization helper
    /**
     * @brief Check Auth.
     * @param[in] req Input parameter.
     * @param[in] required_scope Input parameter.
     * @return Return value.
     */
    std::optional<http::response<http::string_body>> checkAuth(
        const http::request<http::string_body>& req, const std::string& required_scope);
    
    // Tenant isolation helper
    struct TenantAuthContext {
        std::string user_id;
        std::string tenant_id;
        std::vector<std::string> groups;
    };
    /**
     * @brief Check Auth And Resolve Tenant.
     * @param[in] req Input parameter.
     * @param[in] required_scope Input parameter.
     * @param[in,out] out_context Input/output parameter.
     * @return Return value.
     */
    std::optional<http::response<http::string_body>> checkAuthAndResolveTenant(
        const http::request<http::string_body>& req, 
        const std::string& required_scope,
        TenantAuthContext& out_context);
    
    // Governance headers
    /**
     * @brief Apply Governance Headers.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void applyGovernanceHeaders(
        const http::request<http::string_body>& req,
        http::response<http::string_body>& res);
};

} // namespace server
} // namespace themis
