/**
 * @file sse_connection_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <set>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include "cdc/changefeed.h"

namespace themis {

namespace server {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;

class SseConnectionManager {
public:
    struct ConnectionConfig {
        uint32_t heartbeat_interval_ms = 15000;  // Send heartbeat every 15s
        uint32_t max_buffered_events = 1000;     // Max events per connection buffer
        uint32_t event_poll_interval_ms = 500;   // Poll changefeed every 500ms
        uint32_t retry_ms = 3000;                // SSE client reconnect delay hint
        uint32_t max_events_per_second = 0;      // 0 = unlimited (server-side rate control)
        bool drop_oldest_on_overflow = true;     // Backpressure policy: drop oldest if buffer full
    };

    struct ConnectionStats {
        size_t active_connections = 0;
        uint64_t total_events_sent = 0;
        uint64_t total_heartbeats_sent = 0;
        uint64_t total_disconnects = 0;
        uint64_t total_dropped_events = 0;
    };

    /**
     * @brief Sse Connection Manager.
     * @param[in] changefeed Input parameter.
     * @param[in,out] ioc Input/output parameter.
     * @return Return value.
     */
    explicit SseConnectionManager(
        std::shared_ptr<Changefeed> changefeed,
        boost::asio::io_context& ioc
    );

    /**
     * @brief Sse Connection Manager.
     * @param[in] changefeed Input parameter.
     * @param[in,out] ioc Input/output parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit SseConnectionManager(
        std::shared_ptr<Changefeed> changefeed,
        boost::asio::io_context& ioc,
        const ConnectionConfig& config
    );

    ~SseConnectionManager();

    uint64_t registerConnection(
        uint64_t from_seq,
        const std::string& key_prefix = "",
        const std::set<Changefeed::ChangeEventType>& event_types = {}
    );

    /**
     * @brief Unregister Connection.
     * @param[in] conn_id Identifier of the conn.
     */
    void unregisterConnection(uint64_t conn_id);

    std::vector<Changefeed::ChangeEvent> pollRawEvents(uint64_t conn_id, size_t max_events = 100);

    std::vector<std::string> pollEvents(uint64_t conn_id, size_t max_events = 100);

    /**
     * @brief Needs Heartbeat.
     * @param[in] conn_id Identifier of the conn.
     * @return True when the operation succeeds.
     */
    bool needsHeartbeat(uint64_t conn_id) const;

    /**
     * @brief Record Heartbeat.
     * @param[in] conn_id Identifier of the conn.
     */
    void recordHeartbeat(uint64_t conn_id);

    /**
     * @brief Get Stats.
     * @return Return value.
     */
    ConnectionStats getStats() const;

    /**
     * @brief Shutdown.
     */
    void shutdown();

private:
    struct Connection {
        uint64_t id = 0;
        std::atomic<uint64_t> current_sequence{0};
        std::string key_prefix;
        std::set<Changefeed::ChangeEventType> event_types;
        std::chrono::steady_clock::time_point last_activity;
        std::chrono::steady_clock::time_point last_heartbeat;
        std::vector<std::string> buffered_events;
        std::vector<Changefeed::ChangeEvent> raw_buffered_events;
        std::atomic<bool> active{true};
        // Backpressure accounting
        uint64_t dropped_events{0};
        // Simple rate window (optional)
        uint32_t sent_in_window{0};
        std::chrono::steady_clock::time_point window_start{std::chrono::steady_clock::now()};
    };

    /**
     * @brief Background Poll Task.
     */
    void backgroundPollTask();

    std::shared_ptr<Changefeed> changefeed_;
    boost::asio::io_context& ioc_;
    ConnectionConfig config_;

    mutable std::shared_mutex connections_mutex_;
    std::unordered_map<uint64_t, std::shared_ptr<Connection>> connections_;
    std::atomic<uint64_t> next_conn_id_{1};

    // Background polling
    std::unique_ptr<boost::asio::steady_timer> poll_timer_;
    mutable std::mutex poll_timer_mutex_;
    std::atomic<bool> running_{false};

    // Stats
    std::atomic<uint64_t> total_events_sent_{0};
    std::atomic<uint64_t> total_heartbeats_sent_{0};
    std::atomic<uint64_t> total_disconnects_{0};
    std::atomic<uint64_t> total_dropped_events_{0};
};

} // namespace server
} // namespace themis
