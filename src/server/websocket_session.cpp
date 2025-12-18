#ifdef THEMIS_ENABLE_WEBSOCKET

#include "server/websocket_session.h"
#include "server/http_server.h"
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
    
    active_ = true;
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
        active_ = false;
        return;
    }
    
    if (ec) {
        THEMIS_ERROR("WebSocket read error ({}): {}", session_id_, ec.message());
        active_ = false;
        return;
    }
    
    // Process the received message
    std::string message = beast::buffers_to_string(buffer_.data());
    buffer_.consume(buffer_.size());
    
    THEMIS_DEBUG("WebSocket message received ({}): {}", session_id_, message);
    
    processMessage(message);
    
    // Continue reading
    doRead();
}

void WebSocketSession::processMessage(const std::string& message) {
    try {
        // Parse JSON message
        auto msg = json::parse(message);
        
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
            
            json response = {
                {"type", "subscribed"},
                {"channel", channel},
                {"status", "ok"}
            };
            send(response.dump());
        }
        else if (type == "unsubscribe") {
            // Handle unsubscription
            std::string channel = msg.value("channel", "");
            THEMIS_INFO("WebSocket unsubscribe request ({}) from channel: {}", session_id_, channel);
            
            json response = {
                {"type", "unsubscribed"},
                {"channel", channel},
                {"status", "ok"}
            };
            send(response.dump());
        }
        else if (type == "query") {
            // Handle query request
            // TODO: Integrate with HttpServer query handlers
            json response = {
                {"type", "query_response"},
                {"status", "not_implemented"},
                {"message", "Query via WebSocket not yet implemented"}
            };
            send(response.dump());
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

void WebSocketSession::send(const std::string& message) {
    std::lock_guard<std::mutex> lock(write_mutex_);
    
    write_queue_.push(message);
    
    if (!writing_) {
        writing_ = true;
        
        if (is_tls_) {
            ws_tls_->async_write(
                net::buffer(write_queue_.front()),
                beast::bind_front_handler(&WebSocketSession::onWrite, shared_from_this())
            );
        } else {
            ws_plain_->async_write(
                net::buffer(write_queue_.front()),
                beast::bind_front_handler(&WebSocketSession::onWrite, shared_from_this())
            );
        }
    }
}

void WebSocketSession::sendBinary(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(write_mutex_);
    
    // Convert binary data to string for queue
    std::string binary_str(data.begin(), data.end());
    write_queue_.push(binary_str);
    
    if (!writing_) {
        writing_ = true;
        
        if (is_tls_) {
            ws_tls_->binary(true);
            ws_tls_->async_write(
                net::buffer(write_queue_.front()),
                beast::bind_front_handler(&WebSocketSession::onWrite, shared_from_this())
            );
        } else {
            ws_plain_->binary(true);
            ws_plain_->async_write(
                net::buffer(write_queue_.front()),
                beast::bind_front_handler(&WebSocketSession::onWrite, shared_from_this())
            );
        }
    }
}

void WebSocketSession::onWrite(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    
    std::lock_guard<std::mutex> lock(write_mutex_);
    
    if (ec) {
        THEMIS_ERROR("WebSocket write error ({}): {}", session_id_, ec.message());
        active_ = false;
        writing_ = false;
        return;
    }
    
    // Remove sent message from queue
    write_queue_.pop();
    
    // Check if there are more messages to send
    if (!write_queue_.empty()) {
        if (is_tls_) {
            ws_tls_->text(true); // Reset to text mode
            ws_tls_->async_write(
                net::buffer(write_queue_.front()),
                beast::bind_front_handler(&WebSocketSession::onWrite, shared_from_this())
            );
        } else {
            ws_plain_->text(true); // Reset to text mode
            ws_plain_->async_write(
                net::buffer(write_queue_.front()),
                beast::bind_front_handler(&WebSocketSession::onWrite, shared_from_this())
            );
        }
    } else {
        writing_ = false;
    }
}

void WebSocketSession::close() {
    if (!active_) {
        return;
    }
    
    active_ = false;
    doClose();
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

// WebSocketManager implementation

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
