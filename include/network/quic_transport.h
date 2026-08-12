/**
 * @file quic_transport.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB – QUIC transport for the binary wire protocol
//
// Provides a QUIC-based (UDP + TLS 1.3) alternative to the TCP wire protocol
// server on port 8766 and the connectionless UDP fast-path on port 8769.
//
// Key properties:
//   - Port 8770 (dedicated, does not conflict with TCP or UDP fast-path)
//   - Full-duplex, connection-oriented (unlike the UDP fast-path)
//   - Multiplexed QUIC streams carry binary wire protocol frames directly
//   - 0-RTT connection resumption for low-latency reconnects
//   - Connection migration: clients survive Wi-Fi ↔ cellular handover
//   - TLS 1.3 mandatory (QUIC always encrypts; plain QUIC is not supported)
//   - Per-connection rate limiting and configurable connection limits
//
// Guarded by THEMIS_ENABLE_HTTP3 (requires ngtcp2 + OpenSSL).

#pragma once

#ifdef THEMIS_ENABLE_HTTP3

#include <boost/asio.hpp>
#include <ngtcp2/ngtcp2.h>
#include <ngtcp2/ngtcp2_crypto.h>
#include <openssl/ssl.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace themis {

class RocksDBWrapper;

namespace network {

namespace net = boost::asio;
using udp = net::ip::udp;

/// Default port for the QUIC binary wire protocol transport.
constexpr uint16_t kQuicTransportDefaultPort = 8770;

/// QUIC version 1 (RFC 9000).
constexpr uint32_t kQuicVersion1 = NGTCP2_PROTO_VER_V1;

/// Minimum initial flow-control window (64 KB).
constexpr uint64_t kQuicMinInitialMaxData = 64 * 1024;

// ─────────────────────────────────────────────────────────────────────────────
// QuicTransport
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief QUIC transport for ThemisDB binary wire protocol (network module).
 *
 * Binds a UDP socket, listens for QUIC Initial packets, and multiplexes
 * binary wire protocol frames over per-connection QUIC streams.
 *
 * Unlike the HTTP/3 session handler in the server module (which carries HTTP
 * semantics), this class exposes the same binary framing used by the TCP wire
 * protocol server, providing 0-RTT, head-of-line-blocking-free multiplexing
 * and connection migration for wire protocol clients.
 *
 * Thread safety: start()/stop() must not be called concurrently.
 */
class QuicTransport {
public:
    // ── Configuration ────────────────────────────────────────────────────────

    struct Config {
        std::string host = "0.0.0.0";
        uint16_t    port = kQuicTransportDefaultPort;

        /// Number of I/O + dispatch threads.
        std::size_t num_threads = 2;

        /// TLS certificate chain (PEM) and private key (PEM) paths.
        /// Both are required; the transport will not start without them.
        std::string cert_path;
        std::string key_path;

        /// Maximum idle timeout in milliseconds before a connection is closed.
        uint32_t max_idle_timeout_ms = 30000;

        /// Maximum concurrent bidirectional streams per connection.
        uint64_t max_streams_bidi = 100;

        /// Maximum concurrent unidirectional streams per connection.
        uint64_t max_streams_uni = 3;

        /// Initial connection-level flow-control window (bytes).
        uint64_t initial_max_data = 1024 * 1024;  // 1 MB

        /// Initial stream-level flow-control window for bidirectional streams.
        uint64_t initial_max_stream_data_bidi = 256 * 1024;  // 256 KB

        /// Initial stream-level flow-control window for unidirectional streams.
        uint64_t initial_max_stream_data_uni = 256 * 1024;  // 256 KB

        /// Enable 0-RTT connection resumption (requires session ticket support).
        ///
        /// WARNING: When 0-RTT is enabled, early data sent by the client is
        /// subject to replay attacks if the server is restarted or the session
        /// ticket is replayed.  All wire protocol operations that may arrive
        /// in early data MUST be idempotent (e.g., read-only GET/QUERY).
        /// Mutating operations sent in 0-RTT data MUST be deduplicated by the
        /// application layer.  Set to false to disable early data entirely.
        bool enable_0rtt = true;

        /// Maximum total active connections (0 = unlimited).
        uint32_t max_connections = 0;

        /// Maximum QUIC datagram frame size advertised to peers (RFC 9221).
        /// Set to 0 to disable datagram support.
        uint64_t max_datagram_frame_size = 65535;

        Config() = default;
    };

    // ── Statistics ───────────────────────────────────────────────────────────

    struct Stats {
        uint64_t connections_accepted  = 0;
        uint64_t connections_active    = 0;
        uint64_t connections_closed    = 0;
        uint64_t packets_received      = 0;
        uint64_t packets_sent          = 0;
        uint64_t bytes_received        = 0;
        uint64_t bytes_sent            = 0;
        uint64_t parse_errors          = 0;
        uint64_t handshakes_completed  = 0;
        uint64_t zero_rtt_accepted     = 0;
        uint64_t migrations            = 0;
        uint64_t connection_limit_drops = 0;
        uint64_t datagrams_received    = 0;  ///< QUIC datagrams received (RFC 9221)
        uint64_t datagrams_sent        = 0;  ///< QUIC datagrams sent
    };

    // ── Lifecycle ────────────────────────────────────────────────────────────

    /**
     * @param config   Transport configuration.
     * @param storage  Optional storage handle forwarded to stream handlers.
     */
    explicit QuicTransport(const Config&                  config,
                           std::shared_ptr<RocksDBWrapper> storage = nullptr);

    ~QuicTransport();

    /// Bind the UDP socket, create the TLS context, and start I/O threads.
    void start();

    /// Gracefully close all connections and join all threads.
    void stop();

    bool isRunning() const { return running_.load(std::memory_order_acquire); }

    Stats getStats() const;

    // ── Helpers (public for unit-test access) ────────────────────────────────

    /**
     * @brief Create an OpenSSL context configured for QUIC / TLS 1.3.
     *
     * Exported so tests and embedders can reuse the TLS context creation
     * logic without instantiating a full transport.
     *
     * @param cert_path  PEM certificate chain file.
     * @param key_path   PEM private key file.
     * @return New SSL_CTX owned by the caller, or nullptr on failure.
     */
    static SSL_CTX* createSslContext(const std::string& cert_path,
                                     const std::string& key_path);

    /**
     * @brief Return true if @p port is a valid wire protocol port.
     *
     * Validates that the port is in the ThemisDB reserved range and does not
     * conflict with the TCP wire protocol (8766), HTTP/1-2 server (8080/443),
     * or UDP fast-path (8769).
     */
    static bool isValidPort(uint16_t port);

private:
    // ── Internal helpers ─────────────────────────────────────────────────────

    void doReceive();

    void handlePacket(const udp::endpoint& sender,
                      const uint8_t*       data,
                      std::size_t          len);

    /// Enforce max_connections limit.  Returns false and increments the stat
    /// when the limit is exceeded.
    bool checkConnectionLimit();

    // ── Members ──────────────────────────────────────────────────────────────

    Config config_;
    std::shared_ptr<RocksDBWrapper> storage_;

    SSL_CTX* ssl_ctx_{nullptr};

    std::unique_ptr<net::io_context> io_ctx_;
    std::unique_ptr<udp::socket>     socket_;
    std::vector<std::thread>         threads_;

    std::atomic<bool> running_{false};

    std::array<uint8_t, 65536> recv_buf_;
    udp::endpoint               sender_endpoint_;

    // Active QUIC connections indexed by "<addr>:<port>" string.
    // Each entry owns the ngtcp2_conn* for that peer.
    mutable std::mutex                               sessions_mutex_;
    std::unordered_map<std::string, ngtcp2_conn*>    sessions_;

    mutable std::mutex stats_mutex_;
    Stats              stats_;
};

}  // namespace network
}  // namespace themis

#endif  // THEMIS_ENABLE_HTTP3

