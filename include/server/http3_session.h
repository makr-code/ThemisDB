/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            http3_session.h                                    ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     222                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a9a9edcf2  2026-02-21  server: Phase 2 – HTTP/3 hardening, GraphQL endpoint, API... ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#ifdef THEMIS_ENABLE_HTTP3

#include <nghttp3/nghttp3.h>
#include <ngtcp2/ngtcp2.h>
#include <ngtcp2/ngtcp2_crypto.h>
#include <boost/asio.hpp>
#include <openssl/ssl.h>
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>

namespace themis {
namespace server {

namespace net = boost::asio;
using udp = net::ip::udp;

// Forward declarations
class HttpServer;

/**
 * @brief HTTP/3 (QUIC) Session Handler
 * 
 * Manages a single HTTP/3 connection using nghttp3 and ngtcp2 libraries.
 * Handles QUIC transport, 0-RTT, connection migration, and HTTP/3 framing.
 */
class Http3Session : public std::enable_shared_from_this<Http3Session> {
public:
    Http3Session(
        udp::socket& socket,
        const udp::endpoint& remote_endpoint,
        HttpServer* server,
        SSL_CTX* ssl_ctx,
        uint32_t max_idle_timeout_ms
    );
    
    ~Http3Session();

    /**
     * @brief Start the HTTP/3 session
     * 
     * Initializes QUIC connection and performs TLS 1.3 handshake
     */
    void start();
    
    /**
     * @brief Process incoming QUIC packet
     */
    void handlePacket(const uint8_t* data, size_t len, const udp::endpoint& peer);
    
    /**
     * @brief Get remote endpoint
     */
    const udp::endpoint& getRemoteEndpoint() const { return remote_endpoint_; }
    
    /**
     * @brief Check if session is active
     */
    bool isActive() const;

private:
    // QUIC connection management
    void doRead();
    void onRead(boost::system::error_code ec, std::size_t bytes_transferred);
    void doWrite();
    void onTimeout();
    
    // ngtcp2 callbacks
    static int handshakeCompletedCallback(ngtcp2_conn* conn, void* user_data);
    static int recvStreamDataCallback(ngtcp2_conn* conn, uint32_t flags,
                                      int64_t stream_id, uint64_t offset,
                                      const uint8_t* data, size_t datalen,
                                      void* user_data, void* stream_user_data);
    static int ackStreamDataCallback(ngtcp2_conn* conn, int64_t stream_id,
                                     uint64_t offset, uint64_t datalen,
                                     void* user_data, void* stream_user_data);
    static int streamCloseCallback(ngtcp2_conn* conn, uint32_t flags,
                                   int64_t stream_id, uint64_t app_error_code,
                                   void* user_data, void* stream_user_data);
    static int extendMaxStreamsCallback(ngtcp2_conn* conn,
                                        uint64_t max_streams,
                                        void* user_data);
    
    // nghttp3 callbacks
    static int http3RecvDataCallback(nghttp3_conn* conn, int64_t stream_id,
                                     const uint8_t* data, size_t datalen,
                                     void* user_data, void* stream_user_data);
    static int http3DecodHeaderCallback(nghttp3_conn* conn, int64_t stream_id,
                                        int32_t token, nghttp3_rcbuf* name,
                                        nghttp3_rcbuf* value, uint8_t flags,
                                        void* user_data, void* stream_user_data);
    static int http3EndHeadersCallback(nghttp3_conn* conn, int64_t stream_id,
                                       int fin, void* user_data,
                                       void* stream_user_data);
    static int http3EndStreamCallback(nghttp3_conn* conn, int64_t stream_id,
                                      void* user_data, void* stream_user_data);
    
    // Stream data management
    struct StreamData {
        int64_t stream_id;
        std::string method;
        std::string path;
        std::string scheme;
        std::string authority;
        std::unordered_map<std::string, std::string> headers;
        std::string body;
        bool headers_complete = false;
    };
    
    void processStream(int64_t stream_id);
    void sendResponse(int64_t stream_id, int status,
                      const std::string& body,
                      const std::unordered_map<std::string, std::string>& headers = {});
    
    // Crypto operations
    int setupCrypto();
    int feedCryptoData(ngtcp2_encryption_level level, const uint8_t* data, size_t len);
    
    // Members
    udp::socket& socket_;
    udp::endpoint remote_endpoint_;
    HttpServer* server_;
    
    ngtcp2_conn* quic_conn_;
    nghttp3_conn* http3_conn_;
    SSL_CTX* ssl_ctx_;
    SSL* ssl_;
    
    std::array<uint8_t, 65536> read_buffer_;
    std::vector<uint8_t> write_buffer_;
    std::unordered_map<int64_t, StreamData> streams_;
    
    net::steady_timer idle_timer_;
    uint32_t max_idle_timeout_ms_;
    bool handshake_complete_;
};

/**
 * @brief HTTP/3 Protocol Handler
 * 
 * Manages UDP socket for QUIC connections and session lifecycle
 */
class Http3Handler : public std::enable_shared_from_this<Http3Handler> {
public:
    Http3Handler(
        net::io_context& ioc,
        const std::string& host,
        uint16_t port,
        HttpServer* server,
        SSL_CTX* ssl_ctx,
        uint32_t max_idle_timeout_ms = 30000
    );

    ~Http3Handler();

    /**
     * @brief Start listening for QUIC connections
     */
    void start();
    
    /**
     * @brief Stop accepting new connections
     */
    void stop();
    
    /**
     * @brief Configure TLS context for QUIC
     */
    static SSL_CTX* createSslContext(const std::string& cert_path,
                                     const std::string& key_path);

private:
    void doAccept();
    void onReceive(boost::system::error_code ec, std::size_t bytes_transferred);
    void cleanupInactiveSessions();
    
    net::io_context& ioc_;
    udp::socket socket_;
    udp::endpoint remote_endpoint_;
    HttpServer* server_;
    SSL_CTX* ssl_ctx_;
    
    std::array<uint8_t, 65536> recv_buffer_;
    std::unordered_map<std::string, std::shared_ptr<Http3Session>> sessions_;
    
    uint32_t max_idle_timeout_ms_;
    net::steady_timer cleanup_timer_;
};

} // namespace server
} // namespace themis

#endif // THEMIS_ENABLE_HTTP3
