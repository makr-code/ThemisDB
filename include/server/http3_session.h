/**
 * @file http3_session.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
     * @brief Start.
     */
    void start();
    
    /**
     * @brief Handle Packet.
     * @param[in] data Input parameter.
     * @param[in] len Input parameter.
     * @param[in] peer Input parameter.
     */
    void handlePacket(const uint8_t* data, size_t len, const udp::endpoint& peer);
    
    const udp::endpoint& getRemoteEndpoint() const { return remote_endpoint_; }
    
    /**
     * @brief Is Active.
     * @return True when the operation succeeds.
     */
    bool isActive() const;

    /**
     * @brief On Path Migration.
     * @param[in] new_remote Input parameter.
     */
    void onPathMigration(const udp::endpoint& new_remote);

    Http3ConnectionMetrics::Snapshot getMetricsSnapshot() const {
        return metrics_.snapshot();
    }

    /**
     * @brief Send Datagram.
     * @param[in] context_id Identifier of the context.
     * @param[in] payload Input parameter.
     * @param[in] paylen Input parameter.
     * @return True when the operation succeeds.
     */
    bool sendDatagram(uint64_t context_id,
                      const uint8_t* payload,
                      size_t paylen);

    /**
     * @brief Datagram Dispatcher.
     * @return Return value.
     * @details Implements datagramDispatcher without additional internal calls.
     */
    Http3DatagramDispatcher& datagramDispatcher() { return datagram_dispatcher_; }

    /**
     * @brief Handshake Completed Callback.
     * @param[in,out] conn Input/output parameter.
     * @param[in,out] user_data Input/output parameter.
     * @return Return value.
     */
    static int handshakeCompletedCallback(ngtcp2_conn* conn, void* user_data);
    /**
     * @brief Recv Stream Data Callback.
     * @param[in,out] conn Input/output parameter.
     * @param[in] flags Input parameter.
     * @param[in] stream_id Identifier of the stream.
     * @param[in] offset Input parameter.
     * @param[in] data Input parameter.
     * @param[in] datalen Input parameter.
     * @param[in,out] user_data Input/output parameter.
     * @param[in,out] stream_user_data Input/output parameter.
     * @return Return value.
     */
    static int recvStreamDataCallback(ngtcp2_conn* conn, uint32_t flags,
                                      int64_t stream_id, uint64_t offset,
                                      const uint8_t* data, size_t datalen,
                                      void* user_data, void* stream_user_data);
    /**
     * @brief Ack Stream Data Callback.
     * @param[in,out] conn Input/output parameter.
     * @param[in] stream_id Identifier of the stream.
     * @param[in] offset Input parameter.
     * @param[in] datalen Input parameter.
     * @param[in,out] user_data Input/output parameter.
     * @param[in,out] stream_user_data Input/output parameter.
     * @return Return value.
     */
    static int ackStreamDataCallback(ngtcp2_conn* conn, int64_t stream_id,
                                     uint64_t offset, uint64_t datalen,
                                     void* user_data, void* stream_user_data);
    /**
     * @brief Stream Close Callback.
     * @param[in,out] conn Input/output parameter.
     * @param[in] flags Input parameter.
     * @param[in] stream_id Identifier of the stream.
     * @param[in] app_error_code Input parameter.
     * @param[in,out] user_data Input/output parameter.
     * @param[in,out] stream_user_data Input/output parameter.
     * @return Return value.
     */
    static int streamCloseCallback(ngtcp2_conn* conn, uint32_t flags,
                                   int64_t stream_id, uint64_t app_error_code,
                                   void* user_data, void* stream_user_data);
    /**
     * @brief Get New Connection Id Callback.
     * @param[in,out] conn Input/output parameter.
     * @param[in,out] cid Input/output parameter.
     * @param[in,out] token Input/output parameter.
     * @param[in] cidlen Input parameter.
     * @param[in,out] user_data Input/output parameter.
     * @return Return value.
     */
    static int getNewConnectionIdCallback(ngtcp2_conn* conn, ngtcp2_cid* cid,
                                          uint8_t* token, size_t cidlen,
                                          void* user_data);
    /**
     * @brief Recv Crypto Data Callback.
     * @param[in,out] conn Input/output parameter.
     * @param[in] level Input parameter.
     * @param[in] offset Input parameter.
     * @param[in] data Input parameter.
     * @param[in] datalen Input parameter.
     * @param[in,out] user_data Input/output parameter.
     * @return Return value.
     */
    static int recvCryptoDataCallback(ngtcp2_conn* conn, ngtcp2_encryption_level level,
                                      uint64_t offset, const uint8_t* data,
                                      size_t datalen, void* user_data);
    /**
     * @brief Extend Max Streams Callback.
     * @param[in,out] conn Input/output parameter.
     * @param[in] max_streams Input parameter.
     * @param[in,out] user_data Input/output parameter.
     * @return Return value.
     */
    static int extendMaxStreamsCallback(ngtcp2_conn* conn,
                                        uint64_t max_streams,
                                        void* user_data);
    /**
     * @brief Recv Datagram Callback.
     * @param[in,out] conn Input/output parameter.
     * @param[in] flags Input parameter.
     * @param[in] data Input parameter.
     * @param[in] datalen Input parameter.
     * @param[in,out] user_data Input/output parameter.
     * @return Return value.
     */
    static int recvDatagramCallback(ngtcp2_conn* conn, uint32_t flags,
                                    const uint8_t* data, size_t datalen,
                                    void* user_data);

    /**
     * @brief Http3 Recv Data Callback.
     * @param[in,out] conn Input/output parameter.
     * @param[in] stream_id Identifier of the stream.
     * @param[in] data Input parameter.
     * @param[in] datalen Input parameter.
     * @param[in,out] user_data Input/output parameter.
     * @param[in,out] stream_user_data Input/output parameter.
     * @return Return value.
     */
    static int http3RecvDataCallback(nghttp3_conn* conn, int64_t stream_id,
                                     const uint8_t* data, size_t datalen,
                                     void* user_data, void* stream_user_data);
    /**
     * @brief Http3 Decod Header Callback.
     * @param[in,out] conn Input/output parameter.
     * @param[in] stream_id Identifier of the stream.
     * @param[in] token Input parameter.
     * @param[in,out] name Input/output parameter.
     * @param[in,out] value Input/output parameter.
     * @param[in] flags Input parameter.
     * @param[in,out] user_data Input/output parameter.
     * @param[in,out] stream_user_data Input/output parameter.
     * @return Return value.
     */
    static int http3DecodHeaderCallback(nghttp3_conn* conn, int64_t stream_id,
                                        int32_t token, nghttp3_rcbuf* name,
                                        nghttp3_rcbuf* value, uint8_t flags,
                                        void* user_data, void* stream_user_data);
    /**
     * @brief Http3 End Headers Callback.
     * @param[in,out] conn Input/output parameter.
     * @param[in] stream_id Identifier of the stream.
     * @param[in] fin Input parameter.
     * @param[in,out] user_data Input/output parameter.
     * @param[in,out] stream_user_data Input/output parameter.
     * @return Return value.
     */
    static int http3EndHeadersCallback(nghttp3_conn* conn, int64_t stream_id,
                                       int fin, void* user_data,
                                       void* stream_user_data);
    /**
     * @brief Http3 End Stream Callback.
     * @param[in,out] conn Input/output parameter.
     * @param[in] stream_id Identifier of the stream.
     * @param[in,out] user_data Input/output parameter.
     * @param[in,out] stream_user_data Input/output parameter.
     * @return Return value.
     */
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
    /**
     * @brief Do Read.
     */
    void doRead();
    /**
     * @brief On Read.
     * @param[in] ec Input parameter.
     * @param[in] bytes_transferred Input parameter.
     */
    void onRead(boost::system::error_code ec, std::size_t bytes_transferred);
    /**
     * @brief Do Write.
     */
    void doWrite();
    /**
     * @brief On Timeout.
     */
    void onTimeout();
    /**
     * @brief Schedule Idle Timeout.
     */
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
    
    /**
     * @brief Process Stream.
     * @param[in] stream_id Identifier of the stream.
     */
    void processStream(int64_t stream_id);
    void sendResponse(int64_t stream_id, int status,
                      const std::string& body,
                      const std::unordered_map<std::string, std::string>& headers = {});
    
    // Crypto operations
    /**
     * @brief Setup Crypto.
     * @return Return value.
     */
    int setupCrypto();
    /**
     * @brief Feed Crypto Data.
     * @param[in] level Input parameter.
     * @param[in] data Input parameter.
     * @param[in] len Input parameter.
     * @return Return value.
     */
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
     * @brief Start.
     */
    void start();
    
    /**
     * @brief Stop.
     */
    void stop();
    
    /**
     * @brief Create Ssl Context.
     * @param[in] cert_path Path to the cert.
     * @param[in] key_path Path to the key.
     * @return Pointer to the result.
     */
    static SSL_CTX* createSslContext(const std::string& cert_path,
                                     const std::string& key_path);

    /**
     * @brief Fallback Manager.
     * @return Return value.
     * @details Implements fallbackManager without additional internal calls.
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

    /**
     * @brief Do Accept.
     */
    void doAccept();
    /**
     * @brief On Receive.
     * @param[in] ec Input parameter.
     * @param[in] bytes_transferred Input parameter.
     */
    void onReceive(boost::system::error_code ec, std::size_t bytes_transferred);
    /**
     * @brief Cleanup Inactive Sessions.
     */
    void cleanupInactiveSessions();
    /**
     * @brief Arm Cleanup Timer.
     */
    void armCleanupTimer();

    /**
     * @brief Extract Connection Id.
     * @param[in] data Input parameter.
     * @param[in] len Input parameter.
     * @return Return value.
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
