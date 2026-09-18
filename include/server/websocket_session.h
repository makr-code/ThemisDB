/**
 * @file websocket_session.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.48
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
#include <set>
#include <atomic>
#include <mutex>
#include <functional>
#include "cdc/changefeed.h"
#include "cdc/cdc_ws_handler.h"

namespace themis {

namespace server {

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

// Forward declarations
class HttpServer;

class WebSocketSession : public std::enable_shared_from_this<WebSocketSession> {
public:
    WebSocketSession(
        tcp::socket socket,
        HttpServer* server
    );
    
    WebSocketSession(
        beast::ssl_stream<beast::tcp_stream> stream,
        HttpServer* server
    );
    
    ~WebSocketSession();

    /**
     * @brief Run.
     * @param[in] req Input parameter.
     */
    void run(http::request<http::string_body> req);
    
    /**
     * @brief Set Auth Token.
     * @param[in] token Input parameter.
     * @details Implements setAuthToken without additional internal calls.
     */
    void setAuthToken(const std::string& token) { auth_token_ = token; }

    /**
     * @brief Set Request Path.
     * @param[in] path Input parameter.
     * @details Implements setRequestPath without additional internal calls.
     */
    void setRequestPath(const std::string& path) { request_path_ = path; }
    
    /**
     * @brief Send.
     * @param[in] message Input parameter.
     */
    void send(const std::string& message);
    
    /**
     * @brief Send Binary.
     * @param[in] data Input parameter.
     */
    void sendBinary(const std::vector<uint8_t>& data);
    
    /**
     * @brief Close.
     */
    void close();
    
    bool isActive() const { return active_.load(std::memory_order_acquire); }
    
    const std::string& getSessionId() const { return session_id_; }

    static constexpr std::size_t kMaxQueueDepth = 1000;

    void subscribeToCDC(uint64_t from_sequence = 0, const std::string& key_prefix = "",
                        const std::set<Changefeed::ChangeEventType>& event_types = {});
    
    /**
     * @brief Unsubscribe From CDC.
     */
    void unsubscribeFromCDC();
    
    bool isSubscribedToCDC() const {
        if (cdc_subscribed_) {
          return true;
        }
        if (cdc_stream_handler_) {
          return cdc_stream_handler_->hasSubscriptions();
        }
        return false;
    }
    
    /**
     * @brief Update CDCLast Sent Sequence.
     * @param[in] sequence Input parameter.
     */
    void updateCDCLastSentSequence(uint64_t sequence);
    
    struct CDCSubscription {
        uint64_t from_sequence = 0;
        std::string key_prefix;
        uint64_t last_sent_sequence;
        std::set<Changefeed::ChangeEventType> event_types;
    };
    /**
     * @brief Get CDCSubscription.
     * @return Return value.
     */
    CDCSubscription getCDCSubscription() const;

    /**
     * @brief Get Cdc Stream Handler.
     * @return Pointer to the result.
     * @details Calls: get().
     */
    cdc::CdcWebSocketHandler* getCdcStreamHandler() {
        return cdc_stream_handler_.get();
    }

private:
    /**
     * @brief On Accept.
     * @param[in] ec Input parameter.
     */
    void onAccept(beast::error_code ec);
    /**
     * @brief Do Read.
     */
    void doRead();
    /**
     * @brief On Read.
     * @param[in] ec Input parameter.
     * @param[in] bytes_transferred Input parameter.
     */
    void onRead(beast::error_code ec, std::size_t bytes_transferred);
    /**
     * @brief On Write.
     * @param[in] ec Input parameter.
     * @param[in] bytes_transferred Input parameter.
     */
    void onWrite(beast::error_code ec, std::size_t bytes_transferred);
    /**
     * @brief Send On Executor.
     * @param[in] message Input parameter.
     */
    void sendOnExecutor(std::string message);
    /**
     * @brief Send Binary On Executor.
     * @param[in] data Input parameter.
     */
    void sendBinaryOnExecutor(std::vector<uint8_t> data);
    /**
     * @brief Start Write Locked.
     */
    void startWriteLocked();
    /**
     * @brief Close Internal Error On Executor.
     */
    void closeInternalErrorOnExecutor();
    /**
     * @brief Process Message.
     * @param[in] message Input parameter.
     */
    void processMessage(const std::string& message);
    /**
     * @brief Process Binary Message.
     * @param[in] data Input parameter.
     */
    void processBinaryMessage(const std::vector<uint8_t>& data);
    /**
     * @brief Do Close.
     */
    void doClose();
    
    // WebSocket stream (plain or TLS)
    std::unique_ptr<websocket::stream<beast::tcp_stream>> ws_plain_;
    std::unique_ptr<websocket::stream<beast::ssl_stream<beast::tcp_stream>>> ws_tls_;
    
    HttpServer* server_;
    beast::flat_buffer buffer_;
    std::string session_id_;
    std::string request_path_;   ///< Target path from the HTTP upgrade request
    std::string auth_token_;     ///< JWT extracted from the HTTP upgrade Authorization header
    std::atomic<bool> active_;
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
    
    // CDC subscription state (legacy /v2/changes protocol)
    bool cdc_subscribed_;
    uint64_t cdc_from_sequence_;
    uint64_t cdc_last_sent_sequence_;
    std::string cdc_key_prefix_;
    std::set<Changefeed::ChangeEventType> cdc_event_types_;  ///< Filtered event types; empty = all
    mutable std::mutex cdc_mutex_;

    // CDC subscription manager for /v2/cdc/stream (named subscriptions + acks).
    // Null for legacy /v2/changes sessions; initialised in run() for the new endpoint.
    std::unique_ptr<cdc::CdcWebSocketHandler> cdc_stream_handler_;
};

class WebSocketManager {
public:
    explicit WebSocketManager(Changefeed* changefeed = nullptr, uint32_t cdc_poll_interval_ms = 500);
    
    ~WebSocketManager();
    
    void startCDCPolling(net::io_context& ioc, uint32_t interval_ms = 500);
    
    /**
     * @brief Stop CDCPolling.
     */
    void stopCDCPolling();
    
    /**
     * @brief Add Session.
     * @param[in] session Input parameter.
     */
    void addSession(std::shared_ptr<WebSocketSession> session);
    
    /**
     * @brief Remove Session.
     * @param[in] session_id Identifier of the session.
     */
    void removeSession(const std::string& session_id);
    
    /**
     * @brief Broadcast.
     * @param[in] message Input parameter.
     */
    void broadcast(const std::string& message);
    
    /**
     * @brief Send To Session.
     * @param[in] session_id Identifier of the session.
     * @param[in] message Input parameter.
     */
    void sendToSession(const std::string& session_id, const std::string& message);
    
    /**
     * @brief Get Active Session Count.
     * @return Return value.
     */
    size_t getActiveSessionCount() const;
    
    /**
     * @brief Close All.
     */
    void closeAll();
    
    /**
     * @brief Get CDCSubscribed Sessions.
     * @return Return value.
     */
    std::vector<std::shared_ptr<WebSocketSession>> getCDCSubscribedSessions() const;

private:
    /**
     * @brief Poll CDCEvents.
     */
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
