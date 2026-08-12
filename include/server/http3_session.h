/**
 * @file http3_session.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#ifdef THEMIS_ENABLE_HTTP3

#include <nghttp3/nghttp3.h>
#include <ngtcp2/ngtcp2.h>
#include <ngtcp2/ngtcp2_crypto.h>
#include <boost/asio.hpp>
#include <openssl/ssl.h>
#include <atomic>
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>
#include "server/http3_datagram.h"
#include "server/http3_production_config.h"

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
        uint32_t max_idle_timeout_ms,
        const Http3ProductionConfig& prod_cfg = Http3ProductionConfig{}
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

    /**
     * @brief Notify the session that the client has migrated to a new path.
     *
     * Called by Http3Handler when a packet from an already-tracked connection
     * ID arrives from a different address.  Increments migration_count and
     * updates the remote endpoint so subsequent sends reach the new address.
     */
    void onPathMigration(const udp::endpoint& new_remote);

    /**
     * @brief Return a snapshot of the per-connection performance metrics.
     */
    Http3ConnectionMetrics::Snapshot getMetricsSnapshot() const {
        return metrics_.snapshot();
    }

    /**
     * @brief Send an HTTP/3 datagram on the given context (Quarter Stream ID).
     *
     * Encodes the datagram frame (Quarter Stream ID varint + payload) and
     * writes it via ngtcp2_conn_write_datagram().  Returns true on success.
     * Requires the QUIC peer to have negotiated datagram support
     * (max_datagram_frame_size > 0).
     *
     * @param context_id  Quarter Stream ID (stream_id / 4).
     * @param payload     Application payload bytes.
     * @param paylen      Length of @p payload.
     */
    bool sendDatagram(uint64_t context_id,
                      const uint8_t* payload,
                      size_t paylen);

    /**
     * @brief Access the datagram dispatcher for registering/unregistering
     *        context handlers.
     */
    Http3DatagramDispatcher& datagramDispatcher() { return datagram_dispatcher_; }

    /**
     * @brief QUIC and HTTP/3 callback entry points used by ngtcp2/nghttp3.
     *
     * These callbacks are part of the session's externally registered protocol
     * surface and are also exercised directly by protocol-focused unit tests to
     * validate fail-closed behavior on invalid inputs.
     */
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
    static int getNewConnectionIdCallback(ngtcp2_conn* conn, ngtcp2_cid* cid,
                                          uint8_t* token, size_t cidlen,
                                          void* user_data);
    static int recvCryptoDataCallback(ngtcp2_conn* conn, ngtcp2_encryption_level level,
                                      uint64_t offset, const uint8_t* data,
                                      size_t datalen, void* user_data);
    static int extendMaxStreamsCallback(ngtcp2_conn* conn,
                                        uint64_t max_streams,
                                        void* user_data);
    static int recvDatagramCallback(ngtcp2_conn* conn, uint32_t flags,
                                    const uint8_t* data, size_t datalen,
                                    void* user_data);

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

private:
    struct SslCtxDeleter {
        void operator()(SSL_CTX* ctx) const noexcept {
            if (ctx) {
                SSL_CTX_free(ctx);
            }
        }
    };

    struct QuicConnDeleter {
        void operator()(ngtcp2_conn* conn) const noexcept {
            if (!conn) {
                return;
            }
            if (void* tls_handle = ngtcp2_conn_get_tls_native_handle(conn)) {
                SSL_free(static_cast<SSL*>(tls_handle));
            }
            ngtcp2_conn_del(conn);
        }
    };

    struct Http3ConnDeleter {
        void operator()(nghttp3_conn* conn) const noexcept {
            if (conn) {
                nghttp3_conn_del(conn);
            }
        }
    };

    using SslCtxOwner  = std::unique_ptr<SSL_CTX, SslCtxDeleter>;
    using QuicConnOwner = std::unique_ptr<ngtcp2_conn, QuicConnDeleter>;
    using Http3ConnOwner = std::unique_ptr<nghttp3_conn, Http3ConnDeleter>;

    // QUIC connection management
    void doRead();
    void onRead(boost::system::error_code ec, std::size_t bytes_transferred);
    void doWrite();
    void onTimeout();
    void scheduleIdleTimeout();
    
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
    
    QuicConnOwner quic_conn_;
    Http3ConnOwner http3_conn_;
    SSL_CTX* ssl_ctx_;
    SSL* ssl_;
    
    std::array<uint8_t, 65536> read_buffer_;
    std::vector<uint8_t> write_buffer_;
    std::unordered_map<int64_t, StreamData> streams_;
    
    net::steady_timer idle_timer_;
    uint32_t max_idle_timeout_ms_;
    bool handshake_complete_;

    Http3DatagramDispatcher datagram_dispatcher_;

    // Production-readiness additions
    Http3ProductionConfig prod_cfg_;
    Http3ConnectionMetrics metrics_;
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
        uint32_t max_idle_timeout_ms = 30000,
        const Http3ProductionConfig& prod_cfg = Http3ProductionConfig{}
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

    /**
     * @brief Access the fallback manager to check / record QUIC health.
     */
    Http3FallbackManager& fallbackManager() { return fallback_manager_; }
    const Http3FallbackManager& fallbackManager() const { return fallback_manager_; }

private:
    struct SslCtxDeleter {
        void operator()(SSL_CTX* ctx) const noexcept {
            if (ctx) {
                SSL_CTX_free(ctx);
            }
        }
    };

    using SslCtxOwner = std::unique_ptr<SSL_CTX, SslCtxDeleter>;

    void doAccept();
    void onReceive(boost::system::error_code ec, std::size_t bytes_transferred);
    void cleanupInactiveSessions();
    void armCleanupTimer();

    /**
     * @brief Extract the QUIC destination-connection-ID from a raw UDP payload.
     *
     * Parses the first bytes of a QUIC long-header packet to retrieve the
     * Destination Connection ID (DCID) that ngtcp2 assigned during the
     * Initial handshake.  The DCID is returned as a lowercase hex string so
     * it can be used as a map key in @c cid_to_session_key_.
     *
     * @param data  Pointer to the UDP datagram payload.
     * @param len   Length of the datagram in bytes.
     * @return      Hex-encoded DCID string, or an empty string when parsing
     *              fails (packet too short, short-header format, or DCID
     *              length out of range).
     */
    static std::string extractConnectionId(const uint8_t* data, size_t len);
    
    net::io_context& ioc_;
    udp::socket socket_;
    udp::endpoint remote_endpoint_;
    HttpServer* server_;
    SslCtxOwner ssl_ctx_;
    
    std::array<uint8_t, 65536> recv_buffer_;

    // Primary session map: remote IP:port → session
    std::unordered_map<std::string, std::shared_ptr<Http3Session>> sessions_;

    // Secondary index: hex connection-id string → session key (IP:port)
    // Enables routing after connection migration.
    std::unordered_map<std::string, std::string> cid_to_session_key_;
    
    uint32_t max_idle_timeout_ms_;
    net::steady_timer cleanup_timer_;
    std::atomic<bool> running_{false};
    Http3ProductionConfig prod_cfg_;
    Http3FallbackManager fallback_manager_;
};

} // namespace server
} // namespace themis

#endif // THEMIS_ENABLE_HTTP3
