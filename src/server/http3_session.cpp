#ifdef THEMIS_ENABLE_HTTP3

#include "server/http3_session.h"
#include "server/http_server.h"
#include "utils/logger.h"
#include <ngtcp2/ngtcp2_crypto_openssl.h>
#include <cstring>

namespace themis {
namespace server {

// ============================================================================
// Http3Handler Implementation
// ============================================================================

Http3Handler::Http3Handler(
    net::io_context& ioc,
    const std::string& host,
    uint16_t port,
    HttpServer* server,
    uint32_t max_idle_timeout_ms
)
    : ioc_(ioc)
    , socket_(ioc, udp::endpoint(net::ip::make_address(host), port))
    , server_(server)
    , ssl_ctx_(nullptr)
    , max_idle_timeout_ms_(max_idle_timeout_ms)
    , cleanup_timer_(ioc)
{
    THEMIS_INFO("HTTP/3 handler initialized on UDP {}:{}", host, port);
}

Http3Handler::~Http3Handler() {
    if (ssl_ctx_) {
        SSL_CTX_free(ssl_ctx_);
    }
}

SSL_CTX* Http3Handler::createSslContext(const std::string& cert_path,
                                        const std::string& key_path) {
    SSL_CTX* ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (!ssl_ctx) {
        THEMIS_ERROR("SSL_CTX_new failed");
        return nullptr;
    }
    
    SSL_CTX_set_min_proto_version(ssl_ctx, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(ssl_ctx, TLS1_3_VERSION);
    
    if (SSL_CTX_use_certificate_chain_file(ssl_ctx, cert_path.c_str()) != 1) {
        THEMIS_ERROR("SSL_CTX_use_certificate_chain_file failed");
        SSL_CTX_free(ssl_ctx);
        return nullptr;
    }
    
    if (SSL_CTX_use_PrivateKey_file(ssl_ctx, key_path.c_str(), SSL_FILETYPE_PEM) != 1) {
        THEMIS_ERROR("SSL_CTX_use_PrivateKey_file failed");
        SSL_CTX_free(ssl_ctx);
        return nullptr;
    }
    
    // Configure ALPN for HTTP/3
    static const unsigned char alpn[] = "\x02h3";
    SSL_CTX_set_alpn_select_cb(ssl_ctx, 
        [](SSL* /*ssl*/, const unsigned char** out, unsigned char* outlen,
           const unsigned char* in, unsigned int inlen, void* /*arg*/) -> int {
            if (SSL_select_next_proto((unsigned char**)out, outlen, alpn, sizeof(alpn) - 1,
                                      in, inlen) == OPENSSL_NPN_NEGOTIATED) {
                return SSL_TLSEXT_ERR_OK;
            }
            return SSL_TLSEXT_ERR_NOACK;
        }, nullptr);
    
    THEMIS_INFO("HTTP/3 SSL context created with TLS 1.3 and h3 ALPN");
    return ssl_ctx;
}

void Http3Handler::start() {
    THEMIS_INFO("HTTP/3 handler started, waiting for QUIC connections");
    doAccept();
    
    // Start cleanup timer (every 30 seconds)
    cleanup_timer_.expires_after(std::chrono::seconds(30));
    cleanup_timer_.async_wait([this](boost::system::error_code ec) {
        if (!ec) {
            cleanupInactiveSessions();
        }
    });
}

void Http3Handler::stop() {
    socket_.close();
    cleanup_timer_.cancel();
    sessions_.clear();
    THEMIS_INFO("HTTP/3 handler stopped");
}

void Http3Handler::doAccept() {
    socket_.async_receive_from(
        boost::asio::buffer(recv_buffer_),
        remote_endpoint_,
        [this](boost::system::error_code ec, std::size_t bytes_transferred) {
            onReceive(ec, bytes_transferred);
        }
    );
}

void Http3Handler::onReceive(boost::system::error_code ec, std::size_t bytes_transferred) {
    if (ec) {
        THEMIS_ERROR("HTTP/3 UDP receive error: {}", ec.message());
        doAccept(); // Continue accepting
        return;
    }
    
    std::string session_key = remote_endpoint_.address().to_string() + ":" + 
                              std::to_string(remote_endpoint_.port());
    
    auto it = sessions_.find(session_key);
    if (it != sessions_.end()) {
        // Existing session
        it->second->handlePacket(recv_buffer_.data(), bytes_transferred, remote_endpoint_);
    } else {
        // New session
        THEMIS_INFO("HTTP/3 new QUIC connection from {}", session_key);
        
        auto session = std::make_shared<Http3Session>(
            socket_, remote_endpoint_, server_, ssl_ctx_, max_idle_timeout_ms_
        );
        sessions_[session_key] = session;
        session->start();
        session->handlePacket(recv_buffer_.data(), bytes_transferred, remote_endpoint_);
    }
    
    doAccept(); // Continue accepting
}

void Http3Handler::cleanupInactiveSessions() {
    for (auto it = sessions_.begin(); it != sessions_.end(); ) {
        if (!it->second->isActive()) {
            THEMIS_DEBUG("HTTP/3 cleaning up inactive session: {}", it->first);
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
    
    // Reschedule cleanup
    cleanup_timer_.expires_after(std::chrono::seconds(30));
    cleanup_timer_.async_wait([this](boost::system::error_code ec) {
        if (!ec) {
            cleanupInactiveSessions();
        }
    });
}

// ============================================================================
// Http3Session Implementation (Stub)
// ============================================================================

Http3Session::Http3Session(
    udp::socket& socket,
    const udp::endpoint& remote_endpoint,
    HttpServer* server,
    SSL_CTX* ssl_ctx,
    uint32_t max_idle_timeout_ms
)
    : socket_(socket)
    , remote_endpoint_(remote_endpoint)
    , server_(server)
    , quic_conn_(nullptr)
    , http3_conn_(nullptr)
    , ssl_ctx_(ssl_ctx)
    , ssl_(nullptr)
    , idle_timer_(socket.get_executor())
    , max_idle_timeout_ms_(max_idle_timeout_ms)
    , handshake_complete_(false)
{
}

Http3Session::~Http3Session() {
    if (http3_conn_) {
        nghttp3_conn_del(http3_conn_);
    }
    if (quic_conn_) {
        ngtcp2_conn_del(quic_conn_);
    }
    if (ssl_) {
        SSL_free(ssl_);
    }
}

void Http3Session::start() {
    // Initialize QUIC connection
    // This is a stub - full implementation requires:
    // 1. ngtcp2_conn_server_new()
    // 2. Setup callbacks for packet handling, crypto, stream management
    // 3. Initialize TLS 1.3 handshake
    // 4. Setup nghttp3 connection on top of QUIC
    
    THEMIS_WARN("HTTP/3 session start() - STUB IMPLEMENTATION");
    THEMIS_WARN("Full HTTP/3 implementation requires ngtcp2 + nghttp3 integration");
    
    // TODO: Implement full QUIC handshake and HTTP/3 framing
    // For now, this is a placeholder to allow compilation
}

void Http3Session::handlePacket(const uint8_t* data, size_t len, const udp::endpoint& peer) {
    // Process QUIC packet
    // This is a stub - full implementation requires:
    // 1. ngtcp2_conn_read_pkt() to process incoming QUIC packets
    // 2. Extract HTTP/3 frames from QUIC streams
    // 3. Pass to nghttp3 for HTTP processing
    
    THEMIS_DEBUG("HTTP/3 received {} bytes from {}", len, peer.address().to_string());
    
    // TODO: Implement packet processing
}

bool Http3Session::isActive() const {
    // TODO: Implement proper activity check
    // Should check:
    // - QUIC connection state (not closed)
    // - Idle timeout not expired
    // - Handshake completed successfully
    // For stub implementation, always return false to avoid keeping dead sessions
    return false;
}

void Http3Session::processStream(int64_t stream_id) {
    auto it = streams_.find(stream_id);
    if (it == streams_.end()) {
        return;
    }
    
    auto& stream = it->second;
    
    // TODO: Process HTTP/3 request
    THEMIS_INFO("HTTP/3 Processing: {} {}", stream.method, stream.path);
    
    std::string response_body = R"({"status":"ok","message":"HTTP/3 request received","protocol":"h3"})";
    sendResponse(stream_id, 200, response_body, {{"content-type", "application/json"}});
}

void Http3Session::sendResponse(int64_t stream_id, int status,
                                const std::string& body,
                                const std::unordered_map<std::string, std::string>& headers) {
    // Send HTTP/3 response
    // This is a stub - full implementation requires:
    // 1. Build HTTP/3 headers using nghttp3
    // 2. Submit response via nghttp3_conn_submit_response()
    // 3. Write data to QUIC stream
    
    THEMIS_DEBUG("HTTP/3 sending response: status={}, body_size={}", status, body.size());
    
    // TODO: Implement response sending
}

int Http3Session::setupCrypto() {
    // Setup TLS 1.3 for QUIC
    // This requires integration with ngtcp2_crypto_openssl
    return 0;
}

int Http3Session::feedCryptoData(ngtcp2_encryption_level level, const uint8_t* data, size_t len) {
    // Feed crypto data to TLS engine
    return 0;
}

// Stub callbacks - would be implemented in full version
int Http3Session::handshakeCompletedCallback(ngtcp2_conn* /*conn*/, void* user_data) {
    auto* self = static_cast<Http3Session*>(user_data);
    self->handshake_complete_ = true;
    THEMIS_INFO("HTTP/3 QUIC handshake completed");
    return 0;
}

int Http3Session::recvStreamDataCallback(ngtcp2_conn* /*conn*/, uint32_t /*flags*/,
                                         int64_t stream_id, uint64_t /*offset*/,
                                         const uint8_t* data, size_t datalen,
                                         void* user_data, void* /*stream_user_data*/) {
    auto* self = static_cast<Http3Session*>(user_data);
    auto& stream = self->streams_[stream_id];
    stream.body.append(reinterpret_cast<const char*>(data), datalen);
    return 0;
}

// Additional stub callbacks would go here...

} // namespace server
} // namespace themis

#endif // THEMIS_ENABLE_HTTP3
