/**
 * @file http2_session.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=9, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#ifdef THEMIS_ENABLE_HTTP2

#include "server/http2_session.h"
#include "server/http_server.h"
#include "server/chunked_response_writer.h"
#include "server/tenant_manager.h"
#include "utils/logger.h"
#include <boost/beast/http.hpp>
#include <openssl/ssl.h>
#include <chrono>
#include <cstring>

namespace themis {
namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

// ALPN protocol list for HTTP/2
static const unsigned char alpn_proto_list[] = "\x02h2\x08http/1.1";
static const size_t alpn_proto_list_len = sizeof(alpn_proto_list) - 1;

// ALPN callback for selecting HTTP/2
static int alpn_select_callback(SSL* ssl, const unsigned char** out,
                                unsigned char* outlen, const unsigned char* in,
                                unsigned int inlen, void* arg) {
    // Try to select h2 first, fallback to http/1.1
    if (SSL_select_next_proto((unsigned char**)out, outlen, alpn_proto_list,
                              alpn_proto_list_len, in, inlen) == OPENSSL_NPN_NEGOTIATED) {
        return SSL_TLSEXT_ERR_OK;
    }
    return SSL_TLSEXT_ERR_NOACK;
}

void Http2Handler::configureAlpn([[maybe_unused]] boost::asio::ssl::context& ssl_ctx) {
    SSL_CTX* native_ctx = ssl_ctx.native_handle();
    SSL_CTX_set_alpn_select_cb(native_ctx, alpn_select_callback, nullptr);
    THEMIS_INFO("HTTP/2 ALPN configured (h2, http/1.1)");
}

bool Http2Handler::isHttp2Negotiated([[maybe_unused]] SSL* ssl) {
    const unsigned char* alpn = nullptr;
    unsigned int alpn_len = 0;
    SSL_get0_alpn_selected(ssl, &alpn, &alpn_len);
    
    if (alpn_len == 2 && std::memcmp(alpn, "h2", 2) == 0) {
        THEMIS_DEBUG("HTTP/2 negotiated via ALPN");
        return true;
    }
    THEMIS_DEBUG("HTTP/1.1 negotiated via ALPN (fallback)");
    return false;
}

std::shared_ptr<Http2Session> Http2Handler::createSession(
    tcp::socket socket,
    boost::asio::ssl::context& ssl_ctx,
    HttpServer* server,
    uint32_t max_concurrent_streams,
    uint32_t initial_window_size,
    bool connection_slot_reserved
) {
    return std::make_shared<Http2Session>(
        std::move(socket), ssl_ctx, server,
        max_concurrent_streams, initial_window_size, connection_slot_reserved
    );
}

// ============================================================================
// Http2Session Implementation
// ============================================================================

Http2Session::Http2Session(
    tcp::socket socket,
    boost::asio::ssl::context& ssl_ctx,
    HttpServer* server,
    uint32_t max_concurrent_streams,
    uint32_t initial_window_size,
    bool connection_slot_reserved
)
    : stream_(std::move(socket), ssl_ctx)
    , server_(server)
    , ng2_session_(nullptr)
    , read_timer_(stream_.get_executor())
    , write_timer_(stream_.get_executor())
    , max_concurrent_streams_(max_concurrent_streams)
    , initial_window_size_(initial_window_size)
    , next_push_stream_id_(2) // Server push streams start at 2 (even numbers)
{
    if (!connection_slot_reserved) {
        server_->active_connections_.fetch_add(1, std::memory_order_relaxed);
    }
}

Http2Session::~Http2Session() {
    boost::system::error_code ec;
    read_timer_.cancel(ec);
    write_timer_.cancel(ec);
    if (ng2_session_) {
        nghttp2_session_del(ng2_session_);
    }
    server_->active_connections_.fetch_sub(1, std::memory_order_relaxed);
}

void Http2Session::start() {
    doHandshake();
}

void Http2Session::doHandshake() {
    auto self = shared_from_this();
    armReadTimer();
    stream_.async_handshake(
        boost::asio::ssl::stream_base::server,
        [this, self](boost::system::error_code ec) {
            onHandshake(ec);
        }
    );
}

void Http2Session::onHandshake(boost::system::error_code ec) {
    cancelReadTimer();
    if (ec) {
        THEMIS_ERROR("HTTP/2 TLS handshake failed: {}", ec.message());
        return;
    }
    
    // Check if HTTP/2 was negotiated
    if ([[maybe_unused]] !Http2Handler::isHttp2Negotiated(stream_.native_handle())) {
        THEMIS_WARN("HTTP/2 not negotiated, this session should use HTTP/1.1 handler");
        return;
    }
    
    // Initialize nghttp2 session
    nghttp2_session_callbacks* callbacks;
    nghttp2_session_callbacks_new([[maybe_unused]] &callbacks);
    
    nghttp2_session_callbacks_set_send_callback(callbacks, sendCallback);
    nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, onFrameRecvCallback);
    nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, onDataChunkRecvCallback);
    nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, onStreamCloseCallback);
    nghttp2_session_callbacks_set_on_header_callback(callbacks, onHeaderCallback);
    
    nghttp2_option* option;
    nghttp2_option_new(&option);
    nghttp2_option_set_no_auto_window_update(option, 1);
    
    int rv = nghttp2_session_server_new2(&ng2_session_, callbacks, this, option);
    
    nghttp2_session_callbacks_del([[maybe_unused]] callbacks);
    nghttp2_option_del(option);
    
    if (rv != 0) {
        THEMIS_ERROR("nghttp2_session_server_new2 failed: {}", nghttp2_strerror(rv));
        return;
    }
    
    // Send HTTP/2 connection preface (SETTINGS frame)
    nghttp2_settings_entry settings[] = {
        {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, max_concurrent_streams_},
        {NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, initial_window_size_}
    };
    
    rv = nghttp2_submit_settings(ng2_session_, NGHTTP2_FLAG_NONE, settings, 2);
    if (rv != 0) {
        THEMIS_ERROR("nghttp2_submit_settings failed: {}", nghttp2_strerror(rv));
        return;
    }
    
    THEMIS_INFO("HTTP/2 session initialized, starting read loop");
    doRead();
    doWrite();
}

void Http2Session::doRead() {
    auto self = shared_from_this();
    armReadTimer();
    stream_.async_read_some(
        boost::asio::buffer(read_buffer_),
        [this, self](boost::system::error_code ec, std::size_t bytes_transferred) {
            onRead(ec, bytes_transferred);
        }
    );
}

void Http2Session::onRead(boost::system::error_code ec, std::size_t bytes_transferred) {
    cancelReadTimer();
    if (ec) {
        if (ec != boost::asio::error::eof) {
            THEMIS_ERROR("HTTP/2 read error: {}", ec.message());
        }
        return;
    }
    
    ssize_t readlen = nghttp2_session_mem_recv(ng2_session_, read_buffer_.data(), bytes_transferred);
    if (readlen < 0) {
        THEMIS_ERROR("nghttp2_session_mem_recv failed: {}", nghttp2_strerror((int)readlen));
        return;
    }
    
    doWrite();
    doRead();
}

void Http2Session::doWrite() {
    const uint8_t* data;
    ssize_t datalen = nghttp2_session_mem_send(ng2_session_, &data);
    
    if (datalen < 0) {
        THEMIS_ERROR("nghttp2_session_mem_send failed: {}", nghttp2_strerror((int)datalen));
        return;
    }
    
    if (datalen == 0) {
        return; // Nothing to send
    }
    
    write_buffer_.assign(data, data + datalen);
    
    auto self = shared_from_this();
    armWriteTimer();
    boost::asio::async_write(
        stream_,
        boost::asio::buffer(write_buffer_),
        [this, self](boost::system::error_code ec, std::size_t bytes_transferred) {
            onWrite(ec, bytes_transferred);
        }
    );
}

void Http2Session::onWrite(boost::system::error_code ec, std::size_t bytes_transferred) {
    cancelWriteTimer();
    if (ec) {
        THEMIS_ERROR("HTTP/2 write error: {}", ec.message());
        return;
    }
    THEMIS_DEBUG("HTTP/2 wrote {} bytes", bytes_transferred);
}

void Http2Session::armReadTimer() {
    const uint32_t timeout_ms = server_->hot_request_timeout_ms_.load(std::memory_order_acquire);
    if (timeout_ms == 0) {
      return;
    }
    read_timer_.expires_after(std::chrono::milliseconds(timeout_ms));
    const std::weak_ptr<Http2Session> weak_self = weak_from_this();
    read_timer_.async_wait([weak_self](const boost::system::error_code& ec) {
        if (!ec) {
            auto self = weak_self.lock();
            if (!self) {
                return;
            }
            const uint32_t timeout = self->server_->hot_request_timeout_ms_.load(std::memory_order_relaxed);
            THEMIS_WARN("HTTP/2 handshake/read timeout ({}ms) - closing connection", timeout);
            boost::system::error_code close_ec;
            self->stream_.lowest_layer().shutdown(tcp::socket::shutdown_both, close_ec);
            if (close_ec) {
                THEMIS_DEBUG("HTTP/2 timeout shutdown error: {}", close_ec.message());
            }
            self->stream_.lowest_layer().close(close_ec);
            if (close_ec) {
                THEMIS_DEBUG("HTTP/2 timeout close error: {}", close_ec.message());
            }
        }
    });
}

void Http2Session::cancelReadTimer() {
    read_timer_.cancel();
}

void Http2Session::armWriteTimer() {
    const uint32_t timeout_ms = server_->hot_request_timeout_ms_.load(std::memory_order_acquire);
    if (timeout_ms == 0) {
      return;
    }
    write_timer_.expires_after(std::chrono::milliseconds(timeout_ms));
    const std::weak_ptr<Http2Session> weak_self = weak_from_this();
    write_timer_.async_wait([weak_self](const boost::system::error_code& ec) {
        if (!ec) {
            auto self = weak_self.lock();
            if (!self) {
                return;
            }
            const uint32_t timeout = self->server_->hot_request_timeout_ms_.load(std::memory_order_relaxed);
            THEMIS_WARN("HTTP/2 write timeout ({}ms) - closing connection", timeout);
            boost::system::error_code close_ec;
            self->stream_.lowest_layer().shutdown(tcp::socket::shutdown_both, close_ec);
            if (close_ec) {
                THEMIS_DEBUG("HTTP/2 write-timeout shutdown error: {}", close_ec.message());
            }
            self->stream_.lowest_layer().close(close_ec);
            if (close_ec) {
                THEMIS_DEBUG("HTTP/2 write-timeout close error: {}", close_ec.message());
            }
        }
    });
}

void Http2Session::cancelWriteTimer() {
    write_timer_.cancel();
}

// ============================================================================
// nghttp2 Callbacks
// ============================================================================

ssize_t Http2Session::sendCallback(nghttp2_session* /*session*/, const uint8_t* data,
                                   size_t length, int /*flags*/, void* /*user_data*/) {
    // Data will be sent via mem_send, this callback is not used in our implementation
    return (ssize_t)length;
}

int Http2Session::onFrameRecvCallback(nghttp2_session* /*session*/,
                                      const nghttp2_frame* frame, void* user_data) {
    auto* self = static_cast<Http2Session*>(user_data);
    
    if (frame->hd.type == NGHTTP2_HEADERS && 
        frame->headers.cat == NGHTTP2_HCAT_REQUEST) {
        // Request headers received
        THEMIS_DEBUG("HTTP/2 request headers received for stream {}", frame->hd.stream_id);
        self->streams_[frame->hd.stream_id].headers_complete = true;
    }
    
    return 0;
}

int Http2Session::onDataChunkRecvCallback(nghttp2_session* /*session*/, uint8_t /*flags*/,
                                          int32_t stream_id, const uint8_t* data,
                                          size_t len, void* user_data) {
    auto* self = static_cast<Http2Session*>(user_data);
    
    auto it = self->streams_.find(stream_id);
    if (it != self->streams_.end()) {
        it->second.body.append(reinterpret_cast<const char*>(data), len);
    }
    
    return 0;
}

int Http2Session::onStreamCloseCallback(nghttp2_session* /*session*/, int32_t stream_id,
                                        uint32_t /*error_code*/, void* user_data) {
    auto* self = static_cast<Http2Session*>(user_data);
    
    // Remove from CDC subscription if subscribed
    {
        std::lock_guard<std::mutex> lock(self->push_mutex_);
        self->cdc_subscribed_streams_.erase(stream_id);
    }
    {
        std::lock_guard<std::mutex> lock(self->response_mutex_);
        self->response_buffers_.erase(stream_id);
    }
    
    // Process complete request
    self->processStream(stream_id);
    self->streams_.erase(stream_id);
    
    return 0;
}

ssize_t Http2Session::responseDataReadCallback(nghttp2_session* /*session*/, int32_t stream_id,
                                               uint8_t* buf, size_t length, uint32_t* data_flags,
                                               nghttp2_data_source* /*source*/, void* user_data) {
    auto* self = static_cast<Http2Session*>(user_data);
    std::shared_ptr<ResponseBuffer> buffer;
    {
        std::lock_guard<std::mutex> lock(self->response_mutex_);
        auto it = self->response_buffers_.find(stream_id);
        if (it != self->response_buffers_.end()) {
            buffer = it->second;
        }
    }

    if (!buffer) {
        *data_flags |= NGHTTP2_DATA_FLAG_EOF;
        return 0;
    }

    const size_t remaining = buffer->data.size() - buffer->offset;
    const size_t to_copy = std::min(length, remaining);

    if (to_copy > 0) {
        std::memcpy(buf, buffer->data.data() + buffer->offset, to_copy);
        buffer->offset += to_copy;
    }

    if (buffer->offset >= buffer->data.size()) {
        *data_flags |= NGHTTP2_DATA_FLAG_EOF;
        std::lock_guard<std::mutex> lock(self->response_mutex_);
        self->response_buffers_.erase(stream_id);
    }

    return static_cast<ssize_t>(to_copy);
}

int Http2Session::onHeaderCallback(nghttp2_session* /*session*/,
                                   const nghttp2_frame* frame,
                                   const uint8_t* name, size_t namelen,
                                   const uint8_t* value, size_t valuelen,
                                   uint8_t /*flags*/, void* user_data) {
    auto* self = static_cast<Http2Session*>(user_data);
    
    if (frame->hd.type != NGHTTP2_HEADERS || 
        frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
        return 0;
    }
    
    int32_t stream_id = frame->hd.stream_id;
    auto& stream = self->streams_[stream_id];
    stream.stream_id = stream_id;
    
    std::string header_name(reinterpret_cast<const char*>(name), namelen);
    std::string header_value(reinterpret_cast<const char*>(value), valuelen);
    
    if (header_name == ":method") {
        stream.method = header_value;
    } else if (header_name == ":path") {
        stream.path = header_value;
    } else if (header_name != ":scheme" && header_name != ":authority") {
        stream.headers[header_name] = header_value;
    }
    
    return 0;
}

// ============================================================================
// Request Processing
// ============================================================================

void Http2Session::processStream(int32_t stream_id) {
    auto it = streams_.find(stream_id);
    if (it == streams_.end()) {
        return;
    }
    
    auto& stream = it->second;
    
    THEMIS_INFO("HTTP/2 Processing: {} {}", stream.method, stream.path);
    
    // Convert HTTP/2 request to Boost.Beast HTTP/1.1 request format
    // This allows us to reuse all existing HttpServer handlers
    boost::beast::http::request<boost::beast::http::string_body> req;
    
    // Set method
    if (stream.method == "GET") {
        req.method(boost::beast::http::verb::get);
    } else if (stream.method == "POST") {
        req.method(boost::beast::http::verb::post);
    } else if (stream.method == "PUT") {
        req.method(boost::beast::http::verb::put);
    } else if (stream.method == "DELETE") {
        req.method(boost::beast::http::verb::delete_);
    } else if (stream.method == "PATCH") {
        req.method(boost::beast::http::verb::patch);
    } else if (stream.method == "HEAD") {
        req.method(boost::beast::http::verb::head);
    } else if (stream.method == "OPTIONS") {
        req.method(boost::beast::http::verb::options);
    } else {
        THEMIS_WARN("HTTP/2 unsupported method: {}", stream.method);
        sendResponse(stream_id, 405, R"({"error":"Method not allowed"})", 
                    {{"content-type", "application/json"}});
        return;
    }
    
    // Set target (path)
    req.target(stream.path);
    
    // Set HTTP version (doesn't matter for routing, but set it correctly)
    req.version(11); // HTTP/1.1
    
    // Copy headers
    for (const auto& [name, value] : stream.headers) {
        req.set(name, value);
    }
    
    // Set body
    req.body() = stream.body;
    req.prepare_payload();
    
    // Check if this is a CDC subscription request
    if (stream.path == "/cdc/subscribe" || stream.path == "/api/v1/cdc/subscribe") {
        subscribeToCDC(stream_id);
        sendResponse(stream_id, 200, R"({"status":"subscribed","message":"HTTP/2 Server Push enabled for CDC events"})", 
                    {{"content-type", "application/json"}});
        return;
    }
    
    // Rewrite path for tenant-prefixed namespace routing.
    // When the URL path contains the tenant prefix ("/tenants/{id}/..."),
    // extract the tenant ID, set it as X-Tenant-ID header (if not already
    // present), and strip the prefix so the request reaches normal API handlers.
    {
        const auto rw = themis::TenantManager::instance()
                            .rewriteTenantPath(req.target());
        if (rw.rewritten) {
            if (req.find("X-Tenant-ID") == req.end()) {
                req.set("X-Tenant-ID", rw.tenant_id);
            }
            req.target(rw.effective_path);
        }
    }

    // Route the request using HttpServer's existing routing logic
    auto response = server_->routeRequest(req);

    // HTTP/2 forbids Transfer-Encoding: chunked (RFC 7540 §8.1.2.2).
    // When a handler returns a pre-encoded chunked body (e.g. from
    // ChunkedResponseWriter), decode it back to plain bytes and strip the
    // forbidden header before forwarding over the HTTP/2 connection.
    std::string response_body = response.body();
    auto te_it = response.find(http::field::transfer_encoding);
    bool is_chunked = (te_it != response.end() &&
                       boost::beast::iequals(te_it->value(), "chunked"));
    if (is_chunked) {
        response_body = ChunkedResponseWriter::decodeChunkedBody(response_body);
    }

    // Convert response headers to HTTP/2 format, omitting hop-by-hop headers
    // forbidden in HTTP/2 by RFC 7540 §8.1.2.2 (transfer-encoding, connection,
    // keep-alive, upgrade, proxy-connection, te).  Use beast::iequals for
    // case-insensitive comparison as HTTP header names are case-insensitive.
    static constexpr std::string_view kHopByHop[] = {
        "transfer-encoding", "connection", "keep-alive", "upgrade",
        "proxy-connection", "te"
    };
    std::unordered_map<std::string, std::string> response_headers = {};

    for (const auto& header : response) {
        bool skip = false;
        for (const auto& hop : kHopByHop) {
            if (boost::beast::iequals(header.name_string(), hop)) { skip = true; break; }
        }
        if (!skip) {
            response_headers[std::string(header.name_string())] = header.value();
        }
    }

    // Send HTTP/2 response
    sendResponse(stream_id, response.result_int(), response_body, response_headers);
}

void Http2Session::sendResponse(int32_t stream_id, int status,
                                const std::string& body,
                                const std::unordered_map<std::string, std::string>& headers) {
    std::vector<nghttp2_nv> nva;
    
    // Status header
    std::string status_str = std::to_string(status);
    nva.push_back({
        (uint8_t*)":status",
        (uint8_t*)status_str.c_str(),
        7,
        status_str.size(),
        NGHTTP2_NV_FLAG_NONE
    });
    
    // Custom headers
    for (const auto& [name, value] : headers) {
        nva.push_back({
            (uint8_t*)name.c_str(),
            (uint8_t*)value.c_str(),
            name.size(),
            value.size(),
            NGHTTP2_NV_FLAG_NONE
        });
    }
    
    auto resp_buffer = std::make_shared<ResponseBuffer>(ResponseBuffer{body, 0});
    {
        std::lock_guard<std::mutex> lock(response_mutex_);
        response_buffers_[stream_id] = resp_buffer;
    }
    
    nghttp2_data_provider data_prd;
    data_prd.source.ptr = nullptr;
    data_prd.read_callback = responseDataReadCallback;
    
    int rv = nghttp2_submit_response(ng2_session_, stream_id, nva.data(),static_cast<int>(nva.size()), &data_prd);
    if (rv != 0) {
        THEMIS_ERROR("nghttp2_submit_response failed: {}", nghttp2_strerror(rv));
        std::lock_guard<std::mutex> lock(response_mutex_);
        response_buffers_.erase(stream_id);
    }

    doWrite();
}

// ============================================================================
// HTTP/2 Server Push for CDC
// ============================================================================

void Http2Session::sendServerPush(int32_t stream_id, const std::string& push_path,
                                  const std::string& body,
                                  const std::unordered_map<std::string, std::string>& headers) {
    std::lock_guard<std::mutex> lock(push_mutex_);
    
    // Create push promise headers
    std::vector<nghttp2_nv> nva;
    
    // Required pseudo-headers for push promise
    nva.push_back({
        (uint8_t*)":method",
        (uint8_t*)"GET",
        7,
        3,
        NGHTTP2_NV_FLAG_NONE
    });
    
    nva.push_back({
        (uint8_t*)":path",
        (uint8_t*)push_path.c_str(),
        5,
        push_path.size(),
        NGHTTP2_NV_FLAG_NONE
    });
    
    nva.push_back({
        (uint8_t*)":scheme",
        (uint8_t*)"https",
        7,
        5,
        NGHTTP2_NV_FLAG_NONE
    });
    
    nva.push_back({
        (uint8_t*)":authority",
        (uint8_t*)"localhost",
        10,
        9,
        NGHTTP2_NV_FLAG_NONE
    });
    
    // Submit push promise
    int32_t promised_stream_id = -1;
    int rv = nghttp2_submit_push_promise(ng2_session_, NGHTTP2_FLAG_NONE, stream_id,
                                         nva.data(),static_cast<int>(nva.size()), &promised_stream_id);
    
    if (rv != 0) {
        THEMIS_ERROR("nghttp2_submit_push_promise failed: {}", nghttp2_strerror(rv));
        return;
    }
    
    THEMIS_DEBUG("HTTP/2 Server Push promise created for stream {}, promised stream {}", stream_id, promised_stream_id);
    
    // Now send the actual pushed response
    std::vector<nghttp2_nv> response_nva;
    
    // Status header
    response_nva.push_back({
        (uint8_t*)":status",
        (uint8_t*)"200",
        7,
        3,
        NGHTTP2_NV_FLAG_NONE
    });
    
    // Content-Type header (default to application/json for CDC events)
    std::string content_type = "application/json";
    auto it = headers.find("content-type");
    if (it != headers.end()) {
        content_type = it->second;
    }
    
    response_nva.push_back({
        (uint8_t*)"content-type",
        (uint8_t*)content_type.c_str(),
        12,
        content_type.size(),
        NGHTTP2_NV_FLAG_NONE
    });
    
    // Additional custom headers
    for (const auto& [name, value] : headers) {
        if (name != "content-type") {
            response_nva.push_back({
                (uint8_t*)name.c_str(),
                (uint8_t*)value.c_str(),
                name.size(),
                value.size(),
                NGHTTP2_NV_FLAG_NONE
            });
        }
    }
    
    auto resp_buffer = std::make_shared<ResponseBuffer>(ResponseBuffer{body, 0});
    {
        std::lock_guard<std::mutex> lock(response_mutex_);
        response_buffers_[promised_stream_id] = resp_buffer;
    }
    
    nghttp2_data_provider data_prd;
    data_prd.source.ptr = nullptr;
    data_prd.read_callback = responseDataReadCallback;
    
    rv = nghttp2_submit_response(ng2_session_, promised_stream_id, response_nva.data(), 
                                  response_nva.size(), &data_prd);
    if (rv != 0) {
        THEMIS_ERROR("nghttp2_submit_response for push failed: {}", nghttp2_strerror(rv));
        std::lock_guard<std::mutex> lock(response_mutex_);
        response_buffers_.erase(promised_stream_id);
        return;
    }
    
    doWrite();
}

void Http2Session::subscribeToCDC(int32_t stream_id) {
    std::lock_guard<std::mutex> lock(push_mutex_);
    cdc_subscribed_streams_.insert(stream_id);
    
    auto& stream = streams_[stream_id];
    stream.cdc_subscribed = true;
    stream.cdc_last_sequence = 0;
    
    THEMIS_INFO("HTTP/2 stream {} subscribed to CDC with Server Push", stream_id);
}

void Http2Session::broadcastCDCEvent([[maybe_unused]] const std::string& event_data) {
    std::lock_guard<std::mutex> lock(push_mutex_);
    
    // Push to all subscribed streams
    for (int32_t stream_id : cdc_subscribed_streams_) {
        auto it = streams_.find(stream_id);
        if (it != streams_.end()) {
            // Increment sequence for tracking
            it->second.cdc_last_sequence++;
            
            // Create unique push path for each CDC event
            std::string push_path = "/cdc/event/" + std::to_string([[maybe_unused]] it->second.cdc_last_sequence);
            
            // Send Server Push with CDC event data
            sendServerPush(stream_id, push_path, event_data, 
                          {{"content-type", "application/json"},
                           {"x-cdc-sequence", std::to_string(it->second.cdc_last_sequence)}});
            
            THEMIS_DEBUG("HTTP/2 Server Push sent CDC event to stream {}, sequence {}", 
                        stream_id, it->second.cdc_last_sequence);
        }
    }
}

} // namespace server
} // namespace themis

#endif // THEMIS_ENABLE_HTTP2
