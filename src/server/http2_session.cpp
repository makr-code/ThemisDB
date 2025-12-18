#ifdef THEMIS_ENABLE_HTTP2

#include "server/http2_session.h"
#include "server/http_server.h"
#include "utils/logger.h"
#include <openssl/ssl.h>
#include <cstring>

namespace themis {
namespace server {

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

void Http2Handler::configureAlpn(boost::asio::ssl::context& ssl_ctx) {
    SSL_CTX* native_ctx = ssl_ctx.native_handle();
    SSL_CTX_set_alpn_select_cb(native_ctx, alpn_select_callback, nullptr);
    THEMIS_INFO("HTTP/2 ALPN configured (h2, http/1.1)");
}

bool Http2Handler::isHttp2Negotiated(SSL* ssl) {
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
    uint32_t initial_window_size
) {
    return std::make_shared<Http2Session>(
        std::move(socket), ssl_ctx, server,
        max_concurrent_streams, initial_window_size
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
    uint32_t initial_window_size
)
    : stream_(std::move(socket), ssl_ctx)
    , server_(server)
    , ng2_session_(nullptr)
    , max_concurrent_streams_(max_concurrent_streams)
    , initial_window_size_(initial_window_size)
{
}

Http2Session::~Http2Session() {
    if (ng2_session_) {
        nghttp2_session_del(ng2_session_);
    }
}

void Http2Session::start() {
    doHandshake();
}

void Http2Session::doHandshake() {
    auto self = shared_from_this();
    stream_.async_handshake(
        boost::asio::ssl::stream_base::server,
        [this, self](boost::system::error_code ec) {
            onHandshake(ec);
        }
    );
}

void Http2Session::onHandshake(boost::system::error_code ec) {
    if (ec) {
        THEMIS_ERROR("HTTP/2 TLS handshake failed: {}", ec.message());
        return;
    }
    
    // Check if HTTP/2 was negotiated
    if (!Http2Handler::isHttp2Negotiated(stream_.native_handle())) {
        THEMIS_WARN("HTTP/2 not negotiated, this session should use HTTP/1.1 handler");
        return;
    }
    
    // Initialize nghttp2 session
    nghttp2_session_callbacks* callbacks;
    nghttp2_session_callbacks_new(&callbacks);
    
    nghttp2_session_callbacks_set_send_callback(callbacks, sendCallback);
    nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, onFrameRecvCallback);
    nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, onDataChunkRecvCallback);
    nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, onStreamCloseCallback);
    nghttp2_session_callbacks_set_on_header_callback(callbacks, onHeaderCallback);
    
    nghttp2_option* option;
    nghttp2_option_new(&option);
    nghttp2_option_set_no_auto_window_update(option, 1);
    
    int rv = nghttp2_session_server_new2(&ng2_session_, callbacks, this, option);
    
    nghttp2_session_callbacks_del(callbacks);
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
    stream_.async_read_some(
        boost::asio::buffer(read_buffer_),
        [this, self](boost::system::error_code ec, std::size_t bytes_transferred) {
            onRead(ec, bytes_transferred);
        }
    );
}

void Http2Session::onRead(boost::system::error_code ec, std::size_t bytes_transferred) {
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
    boost::asio::async_write(
        stream_,
        boost::asio::buffer(write_buffer_),
        [this, self](boost::system::error_code ec, std::size_t bytes_transferred) {
            onWrite(ec, bytes_transferred);
        }
    );
}

void Http2Session::onWrite(boost::system::error_code ec, std::size_t bytes_transferred) {
    if (ec) {
        THEMIS_ERROR("HTTP/2 write error: {}", ec.message());
        return;
    }
    THEMIS_DEBUG("HTTP/2 wrote {} bytes", bytes_transferred);
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
    
    // Process complete request
    self->processStream(stream_id);
    self->streams_.erase(stream_id);
    
    return 0;
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
    
    // TODO: Convert HTTP/2 request to HttpServer's internal format
    // For now, send a simple response
    THEMIS_INFO("HTTP/2 Processing: {} {}", stream.method, stream.path);
    
    std::string response_body = R"({"status":"ok","message":"HTTP/2 request received","protocol":"h2"})";
    sendResponse(stream_id, 200, response_body, {{"content-type", "application/json"}});
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
    
    // Store response body in class member to ensure lifetime during async operation
    // TODO: Use proper buffer management for production
    struct ResponseBuffer {
        std::string data;
        size_t offset = 0;
    };
    
    auto* resp_buffer = new ResponseBuffer{body, 0};
    
    nghttp2_data_provider data_prd;
    data_prd.source.ptr = resp_buffer;
    data_prd.read_callback = [](nghttp2_session* /*session*/, int32_t /*stream_id*/,
                                 uint8_t* buf, size_t length, uint32_t* data_flags,
                                 nghttp2_data_source* source, void* /*user_data*/) -> ssize_t {
        auto* buffer = static_cast<ResponseBuffer*>(source->ptr);
        size_t remaining = buffer->data.size() - buffer->offset;
        size_t to_copy = std::min(length, remaining);
        
        if (to_copy > 0) {
            std::memcpy(buf, buffer->data.data() + buffer->offset, to_copy);
            buffer->offset += to_copy;
        }
        
        if (buffer->offset >= buffer->data.size()) {
            *data_flags |= NGHTTP2_DATA_FLAG_EOF;
            delete buffer; // Clean up when done
        }
        
        return to_copy;
    };
    
    int rv = nghttp2_submit_response(ng2_session_, stream_id, nva.data(), nva.size(), &data_prd);
    if (rv != 0) {
        THEMIS_ERROR("nghttp2_submit_response failed: {}", nghttp2_strerror(rv));
        delete resp_buffer; // Clean up on error
    }
    
    doWrite();
}

} // namespace server
} // namespace themis

#endif // THEMIS_ENABLE_HTTP2
