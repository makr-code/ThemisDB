/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            websocket_session.h                                ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     252                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 48054ea22  2026-02-22  fix(server/network): finalize WebSocket upgrade – ROADMAP... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#ifdef THEMIS_ENABLE_WEBSOCKET

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <queue>
#include <mutex>
#include <functional>

namespace themis {

// Forward declaration
class Changefeed;

namespace server {

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

// Forward declarations
class HttpServer;

/**
 * @brief WebSocket Session Handler
 * 
 * Manages a single WebSocket connection for bidirectional real-time communication.
 * Supports both plain and TLS WebSocket connections.
 */
class WebSocketSession : public std::enable_shared_from_this<WebSocketSession> {
public:
    /**
     * @brief Create WebSocket session from HTTP upgrade (plain)
     */
    WebSocketSession(
        tcp::socket socket,
        HttpServer* server
    );
    
    /**
     * @brief Create WebSocket session from HTTPS upgrade (TLS)
     */
    WebSocketSession(
        beast::ssl_stream<beast::tcp_stream> stream,
        HttpServer* server
    );
    
    ~WebSocketSession();

    /**
     * @brief Start the WebSocket session after HTTP upgrade
     */
    void run(http::request<http::string_body> req);
    
    /**
     * @brief Send a text message to the client
     */
    void send(const std::string& message);
    
    /**
     * @brief Send a binary message to the client
     */
    void sendBinary(const std::vector<uint8_t>& data);
    
    /**
     * @brief Close the WebSocket connection
     */
    void close();
    
    /**
     * @brief Check if the session is active
     */
    bool isActive() const { return active_; }
    
    /**
     * @brief Get session ID
     */
    const std::string& getSessionId() const { return session_id_; }

    /// Maximum number of pending outbound frames per connection.
    /// Exceeding this triggers a 1011 close to prevent unbounded memory growth.
    static constexpr std::size_t kMaxQueueDepth = 1000;

    /**
     * @brief Subscribe to CDC changefeed
     */
    void subscribeToCDC(uint64_t from_sequence = 0, const std::string& key_prefix = "");
    
    /**
     * @brief Unsubscribe from CDC changefeed
     */
    void unsubscribeFromCDC();
    
    /**
     * @brief Check if subscribed to CDC
     */
    bool isSubscribedToCDC() const { return cdc_subscribed_; }
    
    /**
     * @brief Update CDC last sent sequence
     */
    void updateCDCLastSentSequence(uint64_t sequence);
    
    /**
     * @brief Get CDC subscription details
     */
    struct CDCSubscription {
        uint64_t from_sequence;
        std::string key_prefix;
        uint64_t last_sent_sequence;
    };
    CDCSubscription getCDCSubscription() const;

private:
    void onAccept(beast::error_code ec);
    void doRead();
    void onRead(beast::error_code ec, std::size_t bytes_transferred);
    void onWrite(beast::error_code ec, std::size_t bytes_transferred);
    void processMessage(const std::string& message);
    void doClose();
    
    // WebSocket stream (plain or TLS)
    std::unique_ptr<websocket::stream<beast::tcp_stream>> ws_plain_;
    std::unique_ptr<websocket::stream<beast::ssl_stream<beast::tcp_stream>>> ws_tls_;
    
    HttpServer* server_;
    beast::flat_buffer buffer_;
    std::string session_id_;
    bool active_;
    bool is_tls_;
    
    // Back-pressure: the maximum queue depth is declared public as kMaxQueueDepth above.

    // Message queue for outgoing messages.
    // Each entry records the payload and whether it is a binary frame so that
    // onWrite can restore the correct frame type when draining the queue.
    struct WriteEntry {
        std::string data;
        bool        is_binary = false;
    };
    std::queue<WriteEntry> write_queue_;
    std::mutex write_mutex_;
    bool writing_;
    // Set to true when the connection is closed due to back-pressure so that
    // onWrite can emit a close frame after the current write drains.
    bool close_due_to_backpressure_;
    
    // CDC subscription state
    bool cdc_subscribed_;
    uint64_t cdc_from_sequence_;
    uint64_t cdc_last_sent_sequence_;
    std::string cdc_key_prefix_;
    mutable std::mutex cdc_mutex_;
};

/**
 * @brief WebSocket Connection Manager
 * 
 * Manages all active WebSocket connections and supports broadcasting.
 * Includes CDC/Changefeed integration for real-time data change notifications.
 */
class WebSocketManager {
public:
    /**
     * @brief Constructor
     * @param changefeed Optional Changefeed instance for CDC support
     * @param cdc_poll_interval_ms CDC polling interval in milliseconds (default: 500ms)
     */
    explicit WebSocketManager(Changefeed* changefeed = nullptr, uint32_t cdc_poll_interval_ms = 500);
    
    ~WebSocketManager();
    
    /**
     * @brief Start background CDC polling (if changefeed is set)
     */
    void startCDCPolling(net::io_context& ioc, uint32_t interval_ms = 500);
    
    /**
     * @brief Stop background CDC polling
     */
    void stopCDCPolling();
    
    /**
     * @brief Add a new WebSocket session
     */
    void addSession(std::shared_ptr<WebSocketSession> session);
    
    /**
     * @brief Remove a WebSocket session
     */
    void removeSession(const std::string& session_id);
    
    /**
     * @brief Broadcast a message to all active sessions
     */
    void broadcast(const std::string& message);
    
    /**
     * @brief Send message to a specific session
     */
    void sendToSession(const std::string& session_id, const std::string& message);
    
    /**
     * @brief Get number of active sessions
     */
    size_t getActiveSessionCount() const;
    
    /**
     * @brief Close all sessions
     */
    void closeAll();
    
    /**
     * @brief Get all sessions subscribed to CDC
     */
    std::vector<std::shared_ptr<WebSocketSession>> getCDCSubscribedSessions() const;

private:
    void pollCDCEvents();
    
    std::unordered_map<std::string, std::shared_ptr<WebSocketSession>> sessions_;
    mutable std::mutex sessions_mutex_;
    
    // CDC integration
    Changefeed* changefeed_;
    std::unique_ptr<net::steady_timer> cdc_poll_timer_;
    std::atomic<bool> cdc_polling_active_;
    uint32_t cdc_poll_interval_ms_;
};

} // namespace server
} // namespace themis

#endif // THEMIS_ENABLE_WEBSOCKET
