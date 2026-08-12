/**
 * @file quic_server.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.9
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB – QUIC Protocol Support (v2.0.0)
//
// Provides QUICServer and QUICClient for HTTP/3-compatible low-latency
// communication using QUIC (RFC 9000) over UDP + TLS 1.3.
//
// Key properties:
//   - Port 8769 (default; configurable; plain QUIC is never allowed)
//   - 0-RTT connection establishment for resumed connections
//   - Multiple concurrent streams per connection (no head-of-line blocking)
//   - Connection migration: clients survive Wi-Fi ↔ cellular handover
//   - TLS 1.3 mandatory (QUIC always encrypts)
//   - Configurable congestion control: BBR or Cubic
//   - Per-connection QUIC-specific metrics (0-RTT, migration events)
//   - HTTP/3 ALPN negotiation for compatibility with HTTP/3 clients
//
// Guarded by THEMIS_ENABLE_HTTP3 (requires ngtcp2 + OpenSSL).
//
// Performance characteristics:
//   - Initial connection:   ~1 RTT  (vs 3 RTT for TCP+TLS)
//   - 0-RTT connection:     0 RTT   (vs 3 RTT for TCP+TLS)
//   - Stream multiplexing:  No head-of-line blocking
//   - Loss recovery:        Faster than TCP (per-packet retransmit)

#pragma once

#ifdef THEMIS_ENABLE_HTTP3

#include <boost/asio.hpp>
#include <ngtcp2/ngtcp2.h>
#include <ngtcp2/ngtcp2_crypto.h>
#include <openssl/ssl.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace themis {

class RocksDBWrapper;
class SecondaryIndexManager;

namespace network {

namespace net = boost::asio;
using udp     = net::ip::udp;

/// Default port for the QUIC HTTP/3 server.
constexpr uint16_t kQuicServerDefaultPort = 8769;

/// QUIC version 1 (RFC 9000) — used by QUICServer.
constexpr uint32_t kQuicServerVersion1 = NGTCP2_PROTO_VER_V1;

/// ALPN identifier advertised by QUICServer for HTTP/3 compatibility.
constexpr const char* kQuicServerAlpn = "h3";

/// Minimum initial flow-control window advertised by QUICServer (64 KB).
constexpr uint64_t kQuicServerMinInitialMaxData = 64 * 1024;

// ─────────────────────────────────────────────────────────────────────────────
// QUICServer
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief High-level QUIC (HTTP/3) server for ThemisDB (network module).
 *
 * Binds a UDP socket and accepts inbound QUIC connections.  Each connection
 * may carry up to `max_streams_per_connection` concurrent bidirectional QUIC
 * streams, each of which maps to an independent ThemisDB request/response
 * cycle.  There is no head-of-line blocking between streams.
 *
 * Key features
 * ────────────
 * - **0-RTT resumption** (enable_0rtt=true): resumed connections send the
 *   first request with zero additional round-trips.  Only idempotent
 *   operations should be allowed in 0-RTT data; the implementation enforces
 *   this via the @c enable_0rtt config flag.
 * - **Connection migration**: QUIC connection IDs decouple the logical
 *   connection from the underlying UDP 4-tuple.  When a client's IP address
 *   changes (Wi-Fi → cellular) the server automatically discovers the new
 *   path and the logical connection continues uninterrupted.
 * - **Congestion control**: BBR (default) or Cubic can be selected at server
 *   startup.  BBR provides faster convergence on high-bandwidth, high-latency
 *   paths; Cubic is the traditional TCP-compatible algorithm.
 * - **TLS 1.3 mandatory**: plain QUIC (without TLS) is not supported.
 *   Certificate and key paths must be provided before calling start().
 * - **HTTP/3 ALPN**: the server advertises "h3" during TLS negotiation so
 *   standard HTTP/3 clients can connect.
 *
 * Thread safety: start()/stop() must not be called concurrently.
 */
class QUICServer {
public:
    // ── Configuration ────────────────────────────────────────────────────────

    struct Config {
        /// Listening address (IPv4/IPv6).
        std::string host = "0.0.0.0";

        /// Listening UDP port.
        uint16_t port = kQuicServerDefaultPort;

        /// Number of I/O threads.
        std::size_t num_threads = 2;

        /// Maximum concurrent bidirectional QUIC streams per connection.
        /// Maps directly to ngtcp2 max_streams_bidi transport parameter.
        uint32_t max_streams_per_connection = 100;

        /// Enable 0-RTT connection resumption (session ticket replay).
        ///
        /// WARNING: 0-RTT data is subject to replay attacks.  Only idempotent
        /// operations (e.g., read-only GET/QUERY) must be sent in 0-RTT data.
        /// Mutating operations sent in 0-RTT MUST be deduplicated at the
        /// application layer.  Disable this flag to prevent early data
        /// entirely.
        bool enable_0rtt = true;

        /// Congestion control algorithm: "bbr" (default) or "cubic".
        /// "bbr"   – Bottleneck Bandwidth and RTT; faster convergence on
        ///           high-BDP paths.
        /// "cubic" – Traditional TCP-compatible CUBIC algorithm.
        std::string congestion_control = "bbr";

        /// TLS certificate chain (PEM) path.  Required before start().
        std::string tls_cert_path;

        /// TLS private key (PEM) path.  Required before start().
        std::string tls_key_path;

        /// Maximum idle timeout in seconds before a connection is closed.
        uint32_t max_idle_timeout_sec = 60;

        /// Initial connection-level flow-control window (bytes).
        uint64_t initial_max_data = 1024 * 1024;  // 1 MB

        /// Initial stream-level flow-control window for bidirectional streams.
        uint64_t initial_max_stream_data_bidi = 256 * 1024;  // 256 KB

        /// Initial stream-level flow-control window for unidirectional streams.
        uint64_t initial_max_stream_data_uni = 256 * 1024;  // 256 KB

        /// Maximum total active connections (0 = unlimited).
        uint32_t max_connections = 0;

        Config() = default;
    };

    // ── QUIC-specific statistics ──────────────────────────────────────────────

    struct Stats {
        /// Total QUIC connections accepted since start().
        uint64_t total_connections = 0;

        /// Currently active QUIC connections.
        uint64_t active_connections = 0;

        /// Total QUIC streams opened across all connections.
        uint64_t total_streams = 0;

        /// 0-RTT early data accepted (successful 0-RTT resumptions).
        uint64_t zero_rtt_accepted = 0;

        /// 0-RTT early data rejected (server forced a full handshake).
        uint64_t zero_rtt_rejected = 0;

        /// Connection migration events observed (IP/port change survived).
        uint64_t migrations = 0;

        /// Full TLS 1.3 handshakes completed.
        uint64_t handshakes_completed = 0;

        /// Connections dropped because max_connections was reached.
        uint64_t connection_limit_drops = 0;

        /// Packets received (all QUIC connections combined).
        uint64_t packets_received = 0;

        /// Packets sent (all QUIC connections combined).
        uint64_t packets_sent = 0;

        /// Bytes received.
        uint64_t bytes_received = 0;

        /// Bytes sent.
        uint64_t bytes_sent = 0;
    };

    // ── Lifecycle ────────────────────────────────────────────────────────────

    /**
     * @param config     Server configuration.
     * @param storage    Optional RocksDB handle forwarded to stream handlers.
     * @param index_mgr  Optional secondary-index manager forwarded to
     *                   stream handlers.
     */
    QUICServer(const Config&                  config,
               std::shared_ptr<RocksDBWrapper> storage    = nullptr,
               SecondaryIndexManager*          index_mgr  = nullptr);

    ~QUICServer();

    /// Bind the UDP socket, initialise TLS context, and start I/O threads.
    /// Logs an error and returns without starting if cert/key are missing or
    /// the TLS context cannot be created.
    void start();

    /// Gracefully close all connections and join all I/O threads.
    void stop();

    /// Returns true after start() succeeds and before stop() is called.
    bool isRunning() const { return running_.load(std::memory_order_acquire); }

    /// Snapshot of accumulated QUIC-specific metrics.
    Stats getStats() const;

    // ── Static helpers (public for unit-test access) ─────────────────────────

    /**
     * @brief Create an OpenSSL context configured for QUIC / TLS 1.3 with
     *        HTTP/3 ALPN ("h3").
     *
     * @param cert_path  PEM certificate chain file (may be empty in test mode).
     * @param key_path   PEM private key file (may be empty in test mode).
     * @return New SSL_CTX owned by the caller, or nullptr on failure.
     */
    static SSL_CTX* createSslContext(const std::string& cert_path,
                                     const std::string& key_path);

    /**
     * @brief Validate @p algo as a known congestion control name.
     * @return true for "bbr" or "cubic" (case-insensitive); false otherwise.
     */
    static bool isValidCongestionControl(const std::string& algo);

    /**
     * @brief Return true if @p port is usable by QUICServer.
     *
     * Rejects 0, 80, 443 and known ThemisDB transport ports that QUICServer
     * must not shadow: 8766 (TCP wire), 8770 (QuicTransport), 8771 (gRPC),
     * 8772 (DPDK), 8773 (io_uring), 8774 (Raft).
     */
    static bool isValidPort(uint16_t port);

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

    using SslCtxOwner = std::unique_ptr<SSL_CTX, SslCtxDeleter>;
    using QuicConnOwner = std::unique_ptr<ngtcp2_conn, QuicConnDeleter>;

    void doReceive();
    void handlePacket(const udp::endpoint& sender,
                      const uint8_t*       data,
                      std::size_t          len);
    bool checkConnectionLimit();

    Config                          config_;
    std::shared_ptr<RocksDBWrapper> storage_;
    SecondaryIndexManager*          index_mgr_{nullptr};

    SslCtxOwner ssl_ctx_{nullptr};

    std::unique_ptr<net::io_context> io_ctx_;
    std::unique_ptr<udp::socket>     socket_;
    std::vector<std::thread>         threads_;

    std::atomic<bool> running_{false};

    std::array<uint8_t, 65536> recv_buf_;
    udp::endpoint               sender_endpoint_;

    // Active QUIC connections keyed by "<addr>:<port>" string.
    mutable std::mutex                              sessions_mutex_;
    std::unordered_map<std::string, QuicConnOwner>  sessions_;

    mutable std::mutex stats_mutex_;
    Stats              stats_;
};

// ─────────────────────────────────────────────────────────────────────────────
// QUICClient
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief QUIC (HTTP/3) client for ThemisDB (network module).
 *
 * Establishes a QUIC connection to a QUICServer (or any RFC 9000 compliant
 * server) and exposes independent bidirectional streams for multiplexed
 * request/response cycles with no head-of-line blocking.
 *
 * Usage example
 * ─────────────
 * @code
 * QUICClient::Config cfg;
 * cfg.enable_0rtt    = true;
 * cfg.congestion_control = "bbr";
 *
 * QUICClient client("quic://server.example.com:8769", cfg);
 * client.connect();   // 0-RTT if a session ticket exists
 *
 * auto stream1 = client.openStream();
 * stream1->send({0x01, 0x02});          // Sends on stream 1
 *
 * auto stream2 = client.openStream();
 * stream2->send({0x03, 0x04});          // Sends on stream 2 (independent)
 * @endcode
 *
 * Thread safety: connect()/disconnect() must not be called concurrently.
 * openStream() is thread-safe after connect() returns.
 */
class QUICClient {
public:
    // ── Stream ───────────────────────────────────────────────────────────────

    /**
     * @brief A single QUIC bidirectional stream.
     *
     * Streams are independent; loss on one stream does not block others.
     * Each Stream is owned by the QUICClient that created it; the client must
     * not be destroyed while any Stream is still in use.
     */
    class Stream {
    public:
        explicit Stream(int64_t stream_id, QUICClient* owner);
        ~Stream();

        /// Send @p data on this stream (best-effort; returns immediately).
        void send(const std::vector<uint8_t>& data);

        /// Receive bytes available on this stream (non-blocking; may be empty).
        std::vector<uint8_t> receive();

        /// Close this stream (sends a STREAM_FIN frame).
        void close();

        /// Return the QUIC stream ID assigned by the server.
        int64_t streamId() const { return stream_id_; }

        /// Return true until close() is called or the peer resets the stream.
        bool isOpen() const { return open_.load(std::memory_order_acquire); }

    private:
        int64_t       stream_id_;
        QUICClient*   owner_;
        std::atomic<bool> open_{true};
        mutable std::mutex buf_mutex_;
        std::vector<uint8_t> recv_buf_;
    };

    // ── Configuration ────────────────────────────────────────────────────────

    struct Config {
        /// Timeout for the initial connect() call.
        std::chrono::seconds connect_timeout{5};

        /// Enable 0-RTT early data on resumed connections.
        bool enable_0rtt = true;

        /// Verify the server TLS certificate against system CA store.
        bool verify_tls = true;

        /// Congestion control algorithm: "bbr" (default) or "cubic".
        std::string congestion_control = "bbr";
    };

    // ── Lifecycle ────────────────────────────────────────────────────────────

    /**
     * @param url     Connection target, e.g. "quic://server.example.com:8769"
     * @param config  Client configuration.
     */
    explicit QUICClient(const std::string& url,
                        const Config&      config = Config{});

    ~QUICClient();

    /// Establish a QUIC connection to the target URL.
    /// Performs a full 1-RTT handshake (or 0-RTT if a session ticket exists
    /// and enable_0rtt is true).
    /// Throws std::runtime_error on connection failure or timeout.
    void connect();

    /// Gracefully close the QUIC connection (GOAWAY + CONNECTION_CLOSE).
    void disconnect();

    /// Returns true after connect() succeeds and before disconnect().
    bool isConnected() const { return connected_.load(std::memory_order_acquire); }

    /// Open a new bidirectional QUIC stream.
    /// Throws std::runtime_error if not connected or stream limit exceeded.
    std::unique_ptr<Stream> openStream();

    // ── Static helpers ────────────────────────────────────────────────────────

    /**
     * @brief Parse a "quic://<host>:<port>" URL.
     * @param[in]  url   URL string to parse.
     * @param[out] host  Extracted hostname (may be empty on failure).
     * @param[out] port  Extracted port (0 on failure).
     * @return true on success; false if the URL is malformed.
     */
    static bool parseUrl(const std::string& url,
                         std::string&       host,
                         uint16_t&          port);

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

    using SslCtxOwner = std::unique_ptr<SSL_CTX, SslCtxDeleter>;
    using QuicConnOwner = std::unique_ptr<ngtcp2_conn, QuicConnDeleter>;

    Config      config_;
    std::string url_;
    std::string host_;
    uint16_t    port_{0};

    SslCtxOwner  ssl_ctx_{nullptr};
    QuicConnOwner conn_{nullptr};

    std::atomic<bool> connected_{false};

    int64_t next_stream_id_{0};  // Client-initiated bidi streams: 0, 4, 8, …

    mutable std::mutex streams_mutex_;
    std::unordered_map<int64_t, Stream*> streams_;

    mutable std::mutex stats_mutex_;
};

}  // namespace network
}  // namespace themis

#endif  // THEMIS_ENABLE_HTTP3
