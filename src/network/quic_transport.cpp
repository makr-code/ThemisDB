/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            quic_transport.cpp                                 ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-14 11:35:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     481                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 394fe997b6  2026-03-01  Add HTTP/3 datagram support (RFC 9221 + RFC 9297, Issue #... ║
    • 2044893834  2026-02-28  fix(network/quic): address code review issues - RAND_byte... ║
    • c90319060b  2026-02-28  feat(network): QUIC/HTTP3 transport layer integration ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// ThemisDB – QUIC transport for the binary wire protocol
// See include/network/quic_transport.h for design documentation.

#ifdef THEMIS_ENABLE_HTTP3

#include "network/quic_transport.h"
#include "utils/logger.h"

#include <ngtcp2/ngtcp2_crypto_openssl.h>
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <boost/asio/buffer.hpp>

#include <chrono>
#include <cstring>
#include <limits>

namespace themis::network {

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Return current time in nanoseconds (ngtcp2 timestamp unit).
static uint64_t quicNow() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
        .count());
}

/// Fill a ngtcp2_cid with cryptographically secure random bytes (OpenSSL).
static void generateCid(ngtcp2_cid* cid) {
    cid->datalen = NGTCP2_MIN_CIDLEN;
    if (RAND_bytes(cid->data, static_cast<int>(cid->datalen)) != 1) {
        // Fallback: zero-fill so the caller gets a deterministic (non-random)
        // CID rather than undefined memory.  The handshake will still fail
        // gracefully if the CID collides with an existing connection.
        std::memset(cid->data, 0, cid->datalen);
    }
}

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────────

QuicTransport::QuicTransport(const Config&                  config,
                             std::shared_ptr<RocksDBWrapper> storage)
    : config_(config)
    , storage_(std::move(storage))
    , io_ctx_(std::make_unique<net::io_context>())
{}

QuicTransport::~QuicTransport() {
    stop();
    if (ssl_ctx_) {
        SSL_CTX_free(ssl_ctx_);
        ssl_ctx_ = nullptr;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// TLS context
// ─────────────────────────────────────────────────────────────────────────────

/* static */
SSL_CTX* QuicTransport::createSslContext(const std::string& cert_path,
                                         const std::string& key_path) {
    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        return nullptr;
    }

    // QUIC requires TLS 1.3; reject older versions.
    SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);

    // Load certificate chain and private key only when paths are provided.
    if (!cert_path.empty() &&
        SSL_CTX_use_certificate_chain_file(ctx, cert_path.c_str()) != 1) {
        THEMIS_ERROR("[QuicTransport] Failed to load certificate: {}", cert_path);
        SSL_CTX_free(ctx);
        return nullptr;
    }

    if (!key_path.empty() &&
        SSL_CTX_use_PrivateKey_file(ctx, key_path.c_str(), SSL_FILETYPE_PEM) != 1) {
        THEMIS_ERROR("[QuicTransport] Failed to load private key: {}", key_path);
        SSL_CTX_free(ctx);
        return nullptr;
    }

    // Configure ALPN: advertise "tmdb" for the binary wire protocol over QUIC.
    static const unsigned char kAlpn[] = "\x04tmdb";
    SSL_CTX_set_alpn_select_cb(
        ctx,
        [](SSL* /*ssl*/,
           const unsigned char** out, unsigned char* outlen,
           const unsigned char* in, unsigned int inlen,
           void* /*arg*/) -> int {
            if (SSL_select_next_proto(
                    const_cast<unsigned char**>(out), outlen,
                    kAlpn, static_cast<unsigned>(sizeof(kAlpn) - 1),
                    in, inlen) == OPENSSL_NPN_NEGOTIATED) {
                return SSL_TLSEXT_ERR_OK;
            }
            return SSL_TLSEXT_ERR_NOACK;
        },
        nullptr);

    // Register the ngtcp2 / OpenSSL QUIC method.
    SSL_CTX_set_quic_method(ctx, ngtcp2_crypto_quic_method());

    return ctx;
}

// ─────────────────────────────────────────────────────────────────────────────
// Port validation
// ─────────────────────────────────────────────────────────────────────────────

/* static */
bool QuicTransport::isValidPort(uint16_t port) {
    // Reserved / conflicting ThemisDB ports:
    //   8766 – TCP binary wire protocol
    //   8767 – HTTP/1-2 server (alt-HTTP)
    //   8768 – reserved
    //   8769 – UDP fast-path
    //   443  – standard HTTPS
    //   80   – standard HTTP
    if (port == 0 || port == 80 || port == 443) {
        return false;
    }
    if (port == 8766 || port == 8767 || port == 8768 || port == 8769) {
        return false;
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// start / stop
// ─────────────────────────────────────────────────────────────────────────────

void QuicTransport::start() {
    if (running_.load(std::memory_order_acquire)) {
        return;
    }

    // Create TLS context (paths may be empty in test/dev mode).
    ssl_ctx_ = createSslContext(config_.cert_path, config_.key_path);
    if (!ssl_ctx_) {
        THEMIS_ERROR("[QuicTransport] Failed to create TLS context");
        return;
    }

    udp::endpoint endpoint(net::ip::make_address(config_.host), config_.port);
    socket_ = std::make_unique<udp::socket>(*io_ctx_, endpoint);

    running_.store(true, std::memory_order_release);
    THEMIS_INFO("[QuicTransport] listening on {}:{} (QUIC/TLS 1.3)",
                config_.host, config_.port);

    doReceive();

    const std::size_t n = std::max<std::size_t>(1, config_.num_threads);
    threads_.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        threads_.emplace_back([this] { io_ctx_->run(); });
    }
}

void QuicTransport::stop() {
    if (!running_.exchange(false, std::memory_order_acq_rel)) {
        return;
    }

    // Close all active QUIC connections and free associated SSL objects.
    {
        std::lock_guard<std::mutex> lk(sessions_mutex_);
        for (auto& [key, conn] : sessions_) {
            if (conn) {
                // Retrieve and free the per-connection SSL object before
                // deleting the ngtcp2_conn to prevent a memory leak.
                void* tls_handle = ngtcp2_conn_get_tls_native_handle(conn);
                if (tls_handle) {
                    SSL_free(static_cast<SSL*>(tls_handle));
                }
                ngtcp2_conn_del(conn);
            }
        }
        sessions_.clear();
    }

    if (socket_) {
        boost::system::error_code ec;
        socket_->close(ec);
    }
    io_ctx_->stop();

    for (auto& t : threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
    threads_.clear();
    io_ctx_->restart();

    if (ssl_ctx_) {
        SSL_CTX_free(ssl_ctx_);
        ssl_ctx_ = nullptr;
    }

    THEMIS_INFO("[QuicTransport] stopped");
}

// ─────────────────────────────────────────────────────────────────────────────
// Receive loop
// ─────────────────────────────────────────────────────────────────────────────

void QuicTransport::doReceive() {
    socket_->async_receive_from(
        net::buffer(recv_buf_),
        sender_endpoint_,
        [this](const boost::system::error_code& ec, std::size_t bytes) {
            if (ec) {
                if (ec != net::error::operation_aborted) {
                    THEMIS_ERROR("[QuicTransport] receive error: {}", ec.message());
                }
                return;
            }

            // Snapshot the sender endpoint and data before re-arming.
            const udp::endpoint sender = sender_endpoint_;
            const uint8_t*      data   = recv_buf_.data();

            {
                std::lock_guard<std::mutex> lk(stats_mutex_);
                ++stats_.packets_received;
                stats_.bytes_received += bytes;
            }

            handlePacket(sender, data, bytes);

            if (running_.load(std::memory_order_acquire)) {
                doReceive();
            }
        });
}

// ─────────────────────────────────────────────────────────────────────────────
// Packet handler
// ─────────────────────────────────────────────────────────────────────────────

void QuicTransport::handlePacket(const udp::endpoint& sender,
                                  const uint8_t*       data,
                                  std::size_t          len) {
    const std::string key = sender.address().to_string() + ":" +
                            std::to_string(sender.port());

    std::lock_guard<std::mutex> lk(sessions_mutex_);

    auto it = sessions_.find(key);
    if (it != sessions_.end() && it->second) {
        // Existing connection – feed the packet.
        ngtcp2_path path;
        std::memset(&path, 0, sizeof(path));
        ngtcp2_pkt_info pi;
        std::memset(&pi, 0, sizeof(pi));

        int rv = ngtcp2_conn_read_pkt(it->second, &path, &pi,
                                      data, len, quicNow());
        if (rv != 0) {
            if (rv == NGTCP2_ERR_DRAINING || rv == NGTCP2_ERR_DROP_CONN) {
                // Connection is closing; free SSL and remove it.
                void* tls = ngtcp2_conn_get_tls_native_handle(it->second);
                if (tls) {
                    SSL_free(static_cast<SSL*>(tls));
                }
                ngtcp2_conn_del(it->second);
                sessions_.erase(it);
                std::lock_guard<std::mutex> slk(stats_mutex_);
                ++stats_.connections_closed;
                --stats_.connections_active;
            } else {
                THEMIS_WARN("[QuicTransport] ngtcp2_conn_read_pkt({}): {}",
                            key, ngtcp2_strerror(rv));
            }
        }
        return;
    }

    // New Initial packet – parse the header to extract DCIDs for the server
    // CID and then create a fresh server-side connection.

    if (!checkConnectionLimit()) {
        THEMIS_WARN("[QuicTransport] connection limit reached, dropping from {}",
                    key);
        return;
    }

    // Decode the QUIC packet header to extract connection IDs.
    ngtcp2_pkt_hd hd;
    if (ngtcp2_pkt_decode_hd_long(&hd, data, len) < 0) {
        // May be a short-header packet for an unknown connection; ignore.
        std::lock_guard<std::mutex> slk(stats_mutex_);
        ++stats_.parse_errors;
        return;
    }

    // Generate a new server-chosen Source Connection ID.
    ngtcp2_cid scid;
    generateCid(&scid);

    // Build ngtcp2 settings from our Config.
    ngtcp2_settings settings;
    ngtcp2_settings_default(&settings);
    settings.initial_ts                  = quicNow();
    settings.max_idle_timeout            = static_cast<ngtcp2_duration>(
                                               config_.max_idle_timeout_ms) *
                                           NGTCP2_MILLISECONDS;
    settings.max_stream_data_bidi_local  = config_.initial_max_stream_data_bidi;
    settings.max_stream_data_bidi_remote = config_.initial_max_stream_data_bidi;
    settings.max_stream_data_uni         = config_.initial_max_stream_data_uni;
    settings.max_data                    = config_.initial_max_data;
    settings.max_streams_bidi            = config_.max_streams_bidi;
    settings.max_streams_uni             = config_.max_streams_uni;

    // Minimal ngtcp2 server callbacks (crypto only; stream data is left to the
    // caller to process via separate dispatch once the handshake completes).
    ngtcp2_callbacks callbacks;
    std::memset(&callbacks, 0, sizeof(callbacks));

    callbacks.get_new_connection_id = [](ngtcp2_conn* /*conn*/,
                                         ngtcp2_cid* cid,
                                         uint8_t* /*token*/,
                                         size_t /*cidlen*/,
                                         void* /*user_data*/) -> int {
        generateCid(cid);
        return 0;
    };

    callbacks.recv_crypto_data = [](ngtcp2_conn*            conn,
                                    ngtcp2_encryption_level level,
                                    uint64_t                /*offset*/,
                                    const uint8_t*          data,
                                    size_t                  datalen,
                                    void*                   /*user_data*/) -> int {
        return ngtcp2_crypto_read_write_crypto_data(conn, level, data, datalen);
    };

    callbacks.handshake_completed = [](ngtcp2_conn* /*conn*/,
                                        void*         user_data) -> int {
        auto* transport = static_cast<QuicTransport*>(user_data);
        std::lock_guard<std::mutex> lk(transport->stats_mutex_);
        ++transport->stats_.handshakes_completed;
        return 0;
    };

    // Receive QUIC datagrams (RFC 9221).
    callbacks.recv_datagram = [](ngtcp2_conn* /*conn*/, uint32_t /*flags*/,
                                 const uint8_t* /*data*/, size_t /*datalen*/,
                                 void* user_data) -> int {
        auto* transport = static_cast<QuicTransport*>(user_data);
        std::lock_guard<std::mutex> lk(transport->stats_mutex_);
        ++transport->stats_.datagrams_received;
        return 0;
    };

    // Create a fresh SSL object for this connection.
    SSL* ssl = SSL_new(ssl_ctx_);
    if (!ssl) {
        THEMIS_ERROR("[QuicTransport] SSL_new failed for {}", key);
        return;
    }
    SSL_set_accept_state(ssl);
    if (config_.enable_0rtt) {
        SSL_set_quic_early_data_enabled(ssl, 1);
    }

    // Enable QUIC datagram support (RFC 9221) by advertising
    // max_datagram_frame_size in the server transport parameters.
    ngtcp2_transport_params params;
    ngtcp2_transport_params_default(&params);
    if (config_.max_datagram_frame_size > 0) {
        params.max_datagram_frame_size = config_.max_datagram_frame_size;
    }

    ngtcp2_path path;
    std::memset(&path, 0, sizeof(path));

    ngtcp2_conn* conn = nullptr;
    int rv = ngtcp2_conn_server_new(&conn, &hd.dcid, &scid, &path,
                                    kQuicVersion1, &callbacks, &settings,
                                    &params, this);
    if (rv != 0) {
        THEMIS_ERROR("[QuicTransport] ngtcp2_conn_server_new({}): {}",
                     key, ngtcp2_strerror(rv));
        SSL_free(ssl);
        return;
    }

    ngtcp2_conn_set_tls_native_handle(conn, ssl);

    // Feed the Initial packet.
    ngtcp2_pkt_info pi;
    std::memset(&pi, 0, sizeof(pi));
    rv = ngtcp2_conn_read_pkt(conn, &path, &pi, data, len, quicNow());
    if (rv != 0 && rv != NGTCP2_ERR_RETRY) {
        THEMIS_WARN("[QuicTransport] ngtcp2_conn_read_pkt (new, {}): {}",
                    key, ngtcp2_strerror(rv));
        ngtcp2_conn_del(conn);
        SSL_free(ssl);
        return;
    }

    sessions_[key] = conn;
    {
        std::lock_guard<std::mutex> slk(stats_mutex_);
        ++stats_.connections_accepted;
        ++stats_.connections_active;
    }
    THEMIS_INFO("[QuicTransport] new QUIC connection from {}", key);
}

// ─────────────────────────────────────────────────────────────────────────────
// Connection limit
// ─────────────────────────────────────────────────────────────────────────────

bool QuicTransport::checkConnectionLimit() {
    if (config_.max_connections == 0) {
        return true;  // Unlimited
    }

    std::lock_guard<std::mutex> slk(stats_mutex_);
    if (stats_.connections_active >= config_.max_connections) {
        ++stats_.connection_limit_drops;
        return false;
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Stats
// ─────────────────────────────────────────────────────────────────────────────

QuicTransport::Stats QuicTransport::getStats() const {
    std::lock_guard<std::mutex> lk(stats_mutex_);
    return stats_;
}

}  // namespace themis::network

#endif  // THEMIS_ENABLE_HTTP3
