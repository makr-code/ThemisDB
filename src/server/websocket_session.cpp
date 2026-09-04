/**
 * @file websocket_session.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.48
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=17, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#ifdef THEMIS_ENABLE_WEBSOCKET

#include "server/websocket_session.h"
#include "server/http_server.h"
#include "cdc/changefeed.h"
#include "utils/logger.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <nlohmann/json.hpp>

namespace themis {
namespace server {

using json = nlohmann::json;

// WebSocketSession implementation

WebSocketSession::WebSocketSession(tcp::socket socket, HttpServer* server)
    : server_(server)
    , active_(false)
    , is_tls_(false)
    , writing_(false)
    , close_due_to_backpressure_(false)
    , cdc_subscribed_(false)
    , cdc_from_sequence_(0)
    , cdc_last_sent_sequence_(0)
{
    ws_plain_ = std::make_unique<websocket::stream<beast::tcp_stream>>(std::move(socket));
    
    // Generate unique session ID
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    session_id_ = boost::uuids::to_string(uuid);
    
    THEMIS_INFO("WebSocket session created (plain): {}", session_id_);
}

WebSocketSession::WebSocketSession(beast::ssl_stream<beast::tcp_stream> stream, HttpServer* server)
    : server_(server)
    , active_(false)
    , is_tls_(true)
    , writing_(false)
    , close_due_to_backpressure_(false)
    , cdc_subscribed_(false)
    , cdc_from_sequence_(0)
    , cdc_last_sent_sequence_(0)
{
    ws_tls_ = std::make_unique<websocket::stream<beast::ssl_stream<beast::tcp_stream>>>(std::move(stream));
    
    // Generate unique session ID
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    session_id_ = boost::uuids::to_string(uuid);
    
    THEMIS_INFO("WebSocket session created (TLS): {}", session_id_);
}

WebSocketSession::~WebSocketSession() {
    THEMIS_INFO("WebSocket session destroyed: {}", session_id_);
}

void WebSocketSession::run(http::request<http::string_body> req) {
    // Initialise the CdcWebSocketHandler for the /v2/cdc/stream endpoint so
    // that processMessage() can delegate named-subscription frames to it.
    if (request_path_ == "/v2/cdc/stream") {
        // Pass the ConsumerGroupManager (if available) so that group-protocol
        // subscriptions get durable offset tracking and partition filtering.
        cdc::ConsumerGroupManager* cgm =
            (server_ && server_->consumer_group_manager_)
                ? server_->consumer_group_manager_.get()
                : nullptr;
        cdc_stream_handler_ = std::make_unique<cdc::CdcWebSocketHandler>(
            cdc::CdcWebSocketHandler::kMaxPendingAck, cgm);
    }

    // Set suggested timeout settings for the websocket
    if (is_tls_) {
        ws_tls_->set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
        ws_tls_->set_option(websocket::stream_base::decorator(
            [](websocket::response_type& res) {
                res.set(http::field::server, "ThemisDB WebSocket Server");
            }
        ));
        
        // Accept the WebSocket handshake
        ws_tls_->async_accept(
            req,
            beast::bind_front_handler(&WebSocketSession::onAccept, shared_from_this())
        );
    } else {
        ws_plain_->set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
        ws_plain_->set_option(websocket::stream_base::decorator(
            [](websocket::response_type& res) {
                res.set(http::field::server, "ThemisDB WebSocket Server");
            }
        ));
        
        // Accept the WebSocket handshake
        ws_plain_->async_accept(
            req,
            beast::bind_front_handler(&WebSocketSession::onAccept, shared_from_this())
        );
    }
}

void WebSocketSession::onAccept(beast::error_code ec) {
    if (ec) {
        THEMIS_ERROR("WebSocket accept error ({}): {}", session_id_, ec.message());
        return;
    }
    
    active_.store(true, std::memory_order_release);
    THEMIS_INFO("WebSocket connection accepted: {}", session_id_);
    
    // Send welcome message
    json welcome = {
        {"type", "welcome"},
        {"session_id", session_id_},
        {"server", "ThemisDB"},
        {"protocol", "WebSocket"},
        {"timestamp", std::time(nullptr)}
    };
    send(welcome.dump());
    
    // Start reading messages
    doRead();
}

void WebSocketSession::doRead() {
    if (is_tls_) {
        ws_tls_->async_read(
            buffer_,
            beast::bind_front_handler(&WebSocketSession::onRead, shared_from_this())
        );
    } else {
        ws_plain_->async_read(
            buffer_,
            beast::bind_front_handler(&WebSocketSession::onRead, shared_from_this())
        );
    }
}

void WebSocketSession::onRead(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    
    if (ec == websocket::error::closed) {
        THEMIS_INFO("WebSocket connection closed by client: {}", session_id_);
        active_.store(false, std::memory_order_release);
        return;
    }
    
    if (ec) {
        THEMIS_ERROR("WebSocket read error ({}): {}", session_id_, ec.message());
        active_.store(false, std::memory_order_release);
        return;
    }
    
    // Determine whether the incoming frame is binary or text so that each
    // frame type is routed to the appropriate handler.
    const bool is_binary = is_tls_ ? ws_tls_->got_binary()
                                   : ws_plain_->got_binary();

    if (is_binary) {
        // Binary frame: collect the raw bytes and dispatch to the binary handler.
        const auto* raw = static_cast<const uint8_t*>(buffer_.data().data());
        std::vector<uint8_t> frame_data(raw, raw + buffer_.size());
        buffer_.consume(buffer_.size());

        THEMIS_DEBUG("WebSocket binary frame received ({}): {} bytes",
                     session_id_, frame_data.size());
        processBinaryMessage(frame_data);
    } else {
        // Text frame: decode as UTF-8 string and dispatch to the JSON handler.
        std::string message = beast::buffers_to_string(buffer_.data());
        buffer_.consume(buffer_.size());

        THEMIS_DEBUG("WebSocket message received ({}): {}", session_id_, message);
        processMessage(message);
    }
    
    // Continue reading
    doRead();
}

void WebSocketSession::processMessage(const std::string& message) {
    try {
        // Parse JSON message
        auto msg = json::parse(message);

        // "/v2/cdc/stream" endpoint: delegate entirely to CdcWebSocketHandler
        // which handles the named-subscription protocol (subscribe/unsubscribe/ack).
        if ([[maybe_unused]] request_path_ == "/v2/cdc/stream" && cdc_stream_handler_) {
            auto responses = cdc_stream_handler_->handleFrame([[maybe_unused]] msg);
            for (const auto& resp : responses) {
                send(resp.dump());
            }
            return;
        }

        // "/v2/changes" endpoint uses {"action":"subscribe","collection":"..."} frame format;
        // normalise to the generic {"type":"subscribe","channel":"..."} convention so the
        // rest of the handler remains path-agnostic.
        if (request_path_ == "/v2/changes") {
            if (msg.contains("action") && !msg.contains("type")) {
                msg["type"] = msg["action"];
            }
            if (msg.contains("collection") && !msg.contains("channel")) {
                // Map "collection" to the "changefeed" channel; also preserve as key_prefix
                msg["channel"]    = "changefeed";
                msg["key_prefix"] = msg["collection"].get<std::string>() + ":";
            }
        }

        std::string type = msg.value("type", "unknown");
        
        if (type == "ping") {
            // Respond with pong
            json response = {
                {"type", "pong"},
                {"timestamp", std::time(nullptr)}
            };
            send(response.dump());
        }
        else if (type == "subscribe") {
            // Handle subscription (e.g., to CDC feed)
            std::string channel = msg.value("channel", "");
            THEMIS_INFO("WebSocket subscribe request ({}) to channel: {}", session_id_, channel);
            
            if (channel == "cdc" || channel == "changefeed") {
                // Subscribe to CDC changefeed
                uint64_t from_seq = msg.value("from_sequence", 0);
                std::string key_prefix = msg.value("key_prefix", "");

                // Parse optional filter.type field (e.g. {"filter":{"type":"PUT"}})
                std::set<Changefeed::ChangeEventType> event_types = {};

                if (msg.contains("filter") && msg["filter"].is_object()) {
                    const auto& flt = msg["filter"];
                    if (flt.contains("type") && flt["type"].is_string()) {
                        const std::string& ft = flt["type"].get<std::string>();
                        if (ft == "PUT") {
                            event_types.insert([[maybe_unused]] Changefeed::ChangeEventType::EVENT_PUT);
                        } else if (ft == "DELETE") {
                            event_types.insert([[maybe_unused]] Changefeed::ChangeEventType::EVENT_DELETE);
                        } else if (ft == "TRANSACTION_COMMIT") {
                            event_types.insert([[maybe_unused]] Changefeed::ChangeEventType::EVENT_TRANSACTION_COMMIT);
                        } else if (ft == "TRANSACTION_ROLLBACK") {
                            event_types.insert([[maybe_unused]] Changefeed::ChangeEventType::EVENT_TRANSACTION_ROLLBACK);
                        }
                    }
                }

                subscribeToCDC(from_seq, key_prefix, event_types);
                
                json response = {
                    {"type", "subscribed"},
                    {"channel", channel},
                    {"status", "ok"},
                    {"from_sequence", from_seq},
                    {"key_prefix", key_prefix}
                };
                send(response.dump());
            } else {
                json response = {
                    {"type", "subscribed"},
                    {"channel", channel},
                    {"status", "ok"}
                };
                send(response.dump());
            }
        }
        else if (type == "unsubscribe") {
            // Handle unsubscription
            std::string channel = msg.value("channel", "");
            THEMIS_INFO("WebSocket unsubscribe request ({}) from channel: {}", session_id_, channel);
            
            if (channel == "cdc" || channel == "changefeed") {
                unsubscribeFromCDC();
            }
            
            json response = {
                {"type", "unsubscribed"},
                {"channel", channel},
                {"status", "ok"}
            };
            send(response.dump());
        }
        else if (type == "query") {
            // Dispatch to the server's query handler.
            // WebSocketSession is a friend of HttpServer, so we can access
            // the private query_api_ member directly.
            if (!server_ || !server_->query_api_) {
                json response = {
                    {"type", "query_response"},
                    {"status", "error"},
                    {"message", "Query engine not available"}
                };
                send(response.dump());
                return;
            }

            // Determine handler variant: "aql" key → AQL, otherwise regular query
            const bool is_aql = msg.contains("aql");

            // Build a synthetic HTTP POST request from the WebSocket message.
            // The body is the raw query JSON; strip out the "type" wrapper if present.
            json query_body = msg;
            query_body.erase("type");

            http::request<http::string_body> http_req{http::verb::post,
                                                       is_aql ? "/query/aql" : "/query", 11};
            http_req.set(http::field::content_type, "application/json");
            // Forward any Authorization header the WS client may have sent
            if (msg.contains("authorization"))
                http_req.set(http::field::authorization, msg["authorization"].get<std::string>());
            http_req.body() = query_body.dump();
            http_req.prepare_payload();

            try {
                http::response<http::string_body> http_resp =
                    is_aql ? server_->query_api_->handleQueryAql(http_req)
                           : server_->query_api_->handleQuery(http_req);

                const bool ok = (http_resp.result_int() >= 200 && http_resp.result_int() < 300);
                json ws_resp;
                try {
                    ws_resp = json::parse(http_resp.body());
                } catch (const json::parse_error&) {
                    ws_resp = {{"body", http_resp.body()}};
                }
                ws_resp["type"]        = "query_response";
                ws_resp["status"]      = ok ? "ok" : "error";
                ws_resp["http_status"] = http_resp.result_int();
                send(ws_resp.dump());
            } catch (const std::exception& e) {
                THEMIS_ERROR("WebSocket query error ({}): {}", session_id_, e.what());
                json response = {
                    {"type",    "query_response"},
                    {"status",  "error"},
                    {"message", e.what()}
                };
                send(response.dump());
            }
        }
        else {
            // Unknown message type
            THEMIS_WARN("WebSocket unknown message type ({}): {}", session_id_, type);
            
            json response = {
                {"type", "error"},
                {"message", "Unknown message type"},
                {"received_type", type}
            };
            send(response.dump());
        }
    }
    catch (const json::exception& e) {
        THEMIS_ERROR("WebSocket JSON parse error ({}): {}", session_id_, e.what());
        
        json response = {
            {"type", "error"},
            {"message", "Invalid JSON"},
            {"error", e.what()}
        };
        send(response.dump());
    }
}

void WebSocketSession::processBinaryMessage(const std::vector<uint8_t>& data) {
    // The HTTP WebSocket endpoint (/v1/ws, /v2/changes, /v2/cdc/stream) uses a
    // text/JSON protocol.  Binary frames are not part of its contract.
    //
    // Clients that require binary wire-protocol frame transport should connect
    // to the dedicated wire-protocol WebSocket endpoint on port 8766 where
    // WireProtocolWebSocketSession handles the full binary frame dispatch.
    THEMIS_WARN("WebSocket session {} received unexpected binary frame ({} bytes); "
                "binary frames are not supported on this endpoint",
                session_id_, data.size());

    json response = {
        {"type",     "error"},
        {"status",   "unsupported"},
        {"message",  "Binary WebSocket frames are not supported on the HTTP API endpoint. "
                     "Connect to the wire-protocol WebSocket endpoint (port 8766) for "
                     "binary wire-protocol frame support."},
        {"bytes_received", data.size()}
    };
    send(response.dump());
}

void WebSocketSession::send(const std::string& message) {
    auto self = shared_from_this();
    if (is_tls_) {
        net::dispatch(ws_tls_->get_executor(), [self, message]() mutable {
            self->sendOnExecutor(std::move(message));
        });
    } else {
        net::dispatch(ws_plain_->get_executor(), [self, message]() mutable {
            self->sendOnExecutor(std::move(message));
        });
    }
}

void WebSocketSession::sendBinary(const std::vector<uint8_t>& data) {
    auto self = shared_from_this();
    if (is_tls_) {
        net::dispatch(ws_tls_->get_executor(), [self, data]() mutable {
            self->sendBinaryOnExecutor(std::move(data));
        });
    } else {
        net::dispatch(ws_plain_->get_executor(), [self, data]() mutable {
            self->sendBinaryOnExecutor(std::move(data));
        });
    }
}

void WebSocketSession::sendOnExecutor(std::string message) {
    bool close_now = false;
    {
        std::lock_guard<std::mutex> lock(write_mutex_);

        // Back-pressure: close with code 1011 (Internal Error) when the outbound
        // queue is saturated to avoid unbounded memory growth.
        constexpr size_t kMaxQueueSize = 1000;
        if (static_cast<int>(write_queue_.size()) > = kMaxQueueSize) {
            THEMIS_WARN("WebSocket session {} outbound queue full ({}), closing with 1011",
                        session_id_, write_queue_.size());
            active_.store(false, std::memory_order_release);
            // Signal the in-flight write (if any) to issue a 1011 close frame once
            // the current write drains.  The close frame will be sent via onWrite.
            close_due_to_backpressure_ = true;
            // If there is no in-flight write, onWrite will never run; close now.
            close_now = !writing_;
        } else {
            write_queue_.push({std::move(message), /*is_binary=*/false});
            
            if (!writing_) {
                writing_ = true;
                startWriteLocked();
            }
        }
    }

    if (close_now) {
        closeInternalErrorOnExecutor();
    }
}

void WebSocketSession::sendBinaryOnExecutor(std::vector<uint8_t> data) {
    bool close_now = false;
    {
        std::lock_guard<std::mutex> lock(write_mutex_);
        
        // Back-pressure: same limit as send()
        if (static_cast<int>(write_queue_.size()) > = kMaxQueueDepth) {
            THEMIS_WARN("WebSocket session {} binary queue depth {} >= {}: closing with 1011",
                        session_id_, write_queue_.size(), kMaxQueueDepth);
            active_.store(false, std::memory_order_release);
            close_due_to_backpressure_ = true;
            // If no write is active, force close immediately.
            close_now = !writing_;
        } else {
            // Store as binary entry so onWrite uses the correct frame type
            write_queue_.push({std::string(data.begin(), data.end()), /*is_binary=*/true});
            
            if (!writing_) {
                writing_ = true;
                startWriteLocked();
            }
        }
    }

    if (close_now) {
        closeInternalErrorOnExecutor();
    }
}

void WebSocketSession::startWriteLocked() {
    if (write_queue_.empty()) {
        return;
    }

    auto& front = write_queue_.front();
    if (is_tls_) {
        ws_tls_->text(!front.is_binary);
        ws_tls_->async_write(
            net::buffer(front.data),
            beast::bind_front_handler(&WebSocketSession::onWrite, shared_from_this())
        );
    } else {
        ws_plain_->text(!front.is_binary);
        ws_plain_->async_write(
            net::buffer(front.data),
            beast::bind_front_handler(&WebSocketSession::onWrite, shared_from_this())
        );
    }
}

void WebSocketSession::closeInternalErrorOnExecutor() {
    beast::error_code close_ec;
    if (is_tls_) {
        ws_tls_->close(websocket::close_code::internal_error, close_ec);
    } else {
        ws_plain_->close(websocket::close_code::internal_error, close_ec);
    }

    std::lock_guard<std::mutex> lock(write_mutex_);
    close_due_to_backpressure_ = false;
}

void WebSocketSession::onWrite(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    
    std::lock_guard<std::mutex> lock(write_mutex_);
    
    if (ec) {
        THEMIS_ERROR("WebSocket write error ({}): {}", session_id_, ec.message());
        active_.store(false, std::memory_order_release);
        writing_ = false;
        return;
    }
    
    // Remove sent message from queue
    write_queue_.pop();
    
    // Check if there are more messages to send
    if (!write_queue_.empty()) {
        startWriteLocked();
    } else {
        writing_ = false;

        // If the queue was closed due to back-pressure, send the 1011 close
        // frame now that all pending writes have drained.
        if (close_due_to_backpressure_) {
            close_due_to_backpressure_ = false;
            beast::error_code close_ec;
            if (is_tls_) {
                ws_tls_->close(websocket::close_code::internal_error, close_ec);
            } else {
                ws_plain_->close(websocket::close_code::internal_error, close_ec);
            }
            THEMIS_INFO("WebSocket session {} closed with 1011 after back-pressure",
                        session_id_);
        }
    }
}

void WebSocketSession::close() {
    // Use exchange so only one caller wins and actually issues the close.
    // The atomic exchange ensures that concurrent calls (e.g. from
    // WebSocketManager::closeAll and the io_context read loop) are idempotent.
    if (!active_.exchange(false, std::memory_order_acq_rel)) {
        return;  // already inactive — nothing to do
    }

    unsubscribeFromCDC();

    // Dispatch doClose() onto the io_context executor that owns the stream.
    // Beast WebSocket streams are NOT thread-safe; calling close() from a
    // thread other than the executor thread (e.g. from the destructor thread
    // via WebSocketManager::closeAll) while an async_read is pending causes
    // undefined behaviour.  Dispatching ensures the close is serialised with
    // any pending async operations.
    auto self = shared_from_this();
    if (is_tls_) {
        net::dispatch(ws_tls_->get_executor(), [self]() { self->doClose(); });
    } else {
        net::dispatch(ws_plain_->get_executor(), [self]() { self->doClose(); });
    }
}

void WebSocketSession::doClose() {
    beast::error_code ec;
    
    if (is_tls_) {
        ws_tls_->close(websocket::close_code::normal, ec);
    } else {
        ws_plain_->close(websocket::close_code::normal, ec);
    }
    
    if (ec) {
        THEMIS_ERROR("WebSocket close error ({}): {}", session_id_, ec.message());
    } else {
        THEMIS_INFO("WebSocket connection closed: {}", session_id_);
    }
}

void WebSocketSession::subscribeToCDC(uint64_t from_sequence, const std::string& key_prefix,
                                      const std::set<Changefeed::ChangeEventType>& event_types) {
    std::lock_guard<std::mutex> lock(cdc_mutex_);
    cdc_subscribed_ = true;
    cdc_from_sequence_ = from_sequence;
    // Set last_sent to from_sequence - 1 so first poll gets events > from_sequence
    cdc_last_sent_sequence_ = (from_sequence > 0) ? (from_sequence - 1) : 0;
    cdc_key_prefix_ = key_prefix;
    cdc_event_types_ = event_types;
    
    THEMIS_INFO("WebSocket session {} subscribed to CDC (from_seq={}, last_sent={}, prefix='{}', event_types={})", 
                session_id_, from_sequence, cdc_last_sent_sequence_, key_prefix, event_types.size());
}

void WebSocketSession::unsubscribeFromCDC() {
    std::lock_guard<std::mutex> lock(cdc_mutex_);
    if (cdc_subscribed_) {
        cdc_subscribed_ = false;
        THEMIS_INFO("WebSocket session {} unsubscribed from CDC", session_id_);
    }
}

void WebSocketSession::updateCDCLastSentSequence([[maybe_unused]] uint64_t sequence) {
    std::lock_guard<std::mutex> lock(cdc_mutex_);
    cdc_last_sent_sequence_ = sequence;
}

WebSocketSession::CDCSubscription WebSocketSession::getCDCSubscription() const {
    std::lock_guard<std::mutex> lock(cdc_mutex_);
    return CDCSubscription{
        cdc_from_sequence_,
        cdc_key_prefix_,
        cdc_last_sent_sequence_,
        cdc_event_types_
    };
}

// WebSocketManager implementation

WebSocketManager::WebSocketManager(Changefeed* changefeed, uint32_t cdc_poll_interval_ms)
    : changefeed_(changefeed)
    , cdc_polling_active_(false)
    , cdc_poll_interval_ms_(cdc_poll_interval_ms)
{
    THEMIS_INFO("WebSocketManager created with CDC support: {}, poll_interval={}ms", 
                changefeed != nullptr, cdc_poll_interval_ms);
}

WebSocketManager::~WebSocketManager() {
    stopCDCPolling();
    closeAll();
}

void WebSocketManager::startCDCPolling(net::io_context& ioc, uint32_t interval_ms) {
    if (!changefeed_) {
        THEMIS_WARN("Cannot start CDC polling: no changefeed configured");
        return;
    }
    
    if (cdc_polling_active_.load()) {
        THEMIS_WARN("CDC polling already active");
        return;
    }
    
    cdc_poll_interval_ms_ = interval_ms;
    cdc_polling_active_ = true;
    cdc_poll_timer_ = std::make_unique<net::steady_timer>(ioc);
    
    THEMIS_INFO("Starting CDC polling for WebSocket (interval={}ms)", interval_ms);
    pollCDCEvents();
}

void WebSocketManager::stopCDCPolling() {
    if (cdc_polling_active_.load()) {
        cdc_polling_active_ = false;
        if (cdc_poll_timer_) {
            cdc_poll_timer_->cancel();
            cdc_poll_timer_.reset();
        }
        THEMIS_INFO("CDC polling stopped for WebSocket");
    }
}

void WebSocketManager::pollCDCEvents() {
    if (!cdc_polling_active_.load() || !cdc_poll_timer_) {
        return;
    }
    
    // Get all CDC-subscribed sessions
    auto cdc_sessions = getCDCSubscribedSessions();
    
    if (!cdc_sessions.empty()) {
        // Poll changefeed for each subscribed session
        for (auto& session : cdc_sessions) {
            if (!session->isActive()) {
                continue;
            }

            // /v2/cdc/stream sessions: delegate to CdcWebSocketHandler which
            // tracks named subscriptions and implements at-least-once delivery.
            if ([[maybe_unused]] auto* handler = session->getCdcStreamHandler()) {
                if (!handler->hasSubscriptions()) {
                  continue;
                }
                try {
                    auto frames = handler->pollEvents([[maybe_unused]] *changefeed_);
                    for (const auto& frame : frames) {
                        session->send(frame.dump());
                    }
                    auto redeliveries = handler->checkRedelivery();
                    for (const auto& frame : redeliveries) {
                        session->send(frame.dump());
                    }
                    if (!frames.empty() || !redeliveries.empty()) {
                        THEMIS_DEBUG("Sent {} new + {} redelivered CDC events via "
                                     "CdcWebSocketHandler to session {}",
                                     frames.size(), redeliveries.size(),
                                     session->getSessionId());
                    }
                } catch (const std::exception& e) {
                    THEMIS_ERROR("Error polling CDC stream handler for session {}: {}",
                                 session->getSessionId(), e.what());
                }
                continue;
            }

            // Legacy /v2/changes polling path.
            auto sub = session->getCDCSubscription();
            
            // Get new events since last sent sequence (use last_sent + 1 to avoid re-sending)
            Changefeed::ListOptions options;
            options.from_sequence = sub.last_sent_sequence;  // ListOptions already excludes from_sequence
            options.limit = 100;
            options.long_poll_ms = 0; // No blocking
            if (!sub.key_prefix.empty()) {
                options.key_prefix = sub.key_prefix;
            }
            // Apply per-subscription event-type filter if set
            if ([[maybe_unused]] !sub.event_types.empty()) {
                options.event_types = sub.event_types;
            }
            
            try {
                auto events = changefeed_->listEvents([[maybe_unused]] options);
                
                // Reuse JSON object for better performance
                json cdc_message;
                cdc_message["type"] = "cdc_event";
                
                for ([[maybe_unused]] const auto& event : events) {
                    cdc_message["sequence"] = event.sequence;
                    cdc_message["event_type"] = static_cast<int>([[maybe_unused]] event.type);
                    cdc_message["key"] = event.key;
                    cdc_message["timestamp_ms"] = event.timestamp_ms;
                    
                    if ([[maybe_unused]] event.value.has_value()) {
                        cdc_message["value"] = event.value.value();
                    } else {
                        cdc_message.erase("value");
                    }
                    
                    if ([[maybe_unused]] !event.metadata.empty()) {
                        cdc_message["metadata"] = event.metadata;
                    } else {
                        cdc_message.erase("metadata");
                    }
                    
                    session->send(cdc_message.dump());
                }
                
                // Always update last sent sequence after polling (even if empty)
                if ([[maybe_unused]] !events.empty()) {
                    session->updateCDCLastSentSequence([[maybe_unused]] events.back().sequence);
                    
                    THEMIS_DEBUG("Sent {} CDC events to WebSocket session {}", 
                                events.size(), session->getSessionId());
                }
            } catch (const std::exception& e) {
                THEMIS_ERROR("Error polling CDC for WebSocket session {}: {}", 
                            session->getSessionId(), e.what());
            }
        }
    }
    
    // Schedule next poll
    cdc_poll_timer_->expires_after(std::chrono::milliseconds(cdc_poll_interval_ms_));
    cdc_poll_timer_->async_wait([this](beast::error_code ec) {
        if (!ec && cdc_polling_active_.load()) {
            pollCDCEvents();
        }
    });
}

std::vector<std::shared_ptr<WebSocketSession>> WebSocketManager::getCDCSubscribedSessions() const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    std::vector<std::shared_ptr<WebSocketSession>> cdc_sessions;
    for (const auto& [id, session] : sessions_) {
        if (session->isActive() && session->isSubscribedToCDC()) {
            cdc_sessions.push_back(session);
        }
    }
    return cdc_sessions;
}

void WebSocketManager::addSession(std::shared_ptr<WebSocketSession> session) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    sessions_[session->getSessionId()] = session;
    THEMIS_INFO("WebSocket session added to manager: {} (total: {})", 
                session->getSessionId(), sessions_.size());
}

void WebSocketManager::removeSession(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    sessions_.erase(session_id);
    THEMIS_INFO("WebSocket session removed from manager: {} (remaining: {})", 
                session_id, sessions_.size());
}

void WebSocketManager::broadcast(const std::string& message) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    THEMIS_DEBUG("Broadcasting WebSocket message to {} sessions", sessions_.size());
    
    for (auto& [id, session] : sessions_) {
        if (session->isActive()) {
            session->send(message);
        }
    }
}

void WebSocketManager::sendToSession(const std::string& session_id, const std::string& message) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = sessions_.find(session_id);
    if (it != sessions_.end() && it->second->isActive()) {
        it->second->send(message);
    } else {
        THEMIS_WARN("WebSocket session not found or inactive: {}", session_id);
    }
}

size_t WebSocketManager::getActiveSessionCount() const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    size_t count = 0;
    for (const auto& [id, session] : sessions_) {
        if (session->isActive()) {
            count++;
        }
    }
    return count;
}

void WebSocketManager::closeAll() {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    THEMIS_INFO("Closing all WebSocket sessions ({} total)", sessions_.size());
    
    for (auto& [id, session] : sessions_) {
        if (session->isActive()) {
            session->close();
        }
    }
    
    sessions_.clear();
}

} // namespace server
} // namespace themis

#endif // THEMIS_ENABLE_WEBSOCKET
