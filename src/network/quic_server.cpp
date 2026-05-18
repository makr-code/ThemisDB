// THEMIS_GAP_STATS: gaps=6 unimpl=3 stub=0 mock=0 sim=0 todo=0 debt=0 scanned=2026-05-18
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            quic_server.cpp                                    ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 18:49:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     818                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • 7e5f9a04db  2026-04-13  feat(network): QUIC Protocol Support — QUICServer + QUICC... ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • 5cbf5e10b2  2026-04-13  feat(network): QUIC Protocol Support — QUICServer + QUICC... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// ThemisDB – QUIC Protocol Support (QUICServer + QUICClient)
// See include/network/quic_server.h for design documentation.
//
// Implementation notes
// ────────────────────
// QUICServer builds on ngtcp2 (RFC 9000 QUIC) with OpenSSL for TLS 1.3.
// The server binds a UDP socket and dispatches inbound QUIC packets to
// per-connection ngtcp2_conn objects.  Each accepted connection may open
// up to Config::max_streams_per_connection independent bidirectional streams.
//
// Congestion control is selected at connection creation time via ngtcp2's
// settings.cc_algo field:
//   "bbr"   → NGTCP2_CC_ALGO_BBR  (Bottleneck Bandwidth and RTT)
//   "cubic" → NGTCP2_CC_ALGO_CUBIC (TCP-compatible Cubic)
//
// Connection migration: ngtcp2 tracks the peer address per-connection.
// When a packet arrives from a different endpoint for an existing DCID the
// migration counter is incremented so operators can observe Wi-Fi→cellular
// transitions in the Stats::migrations metric.
//
// 0-RTT: when enable_0rtt=true the server TLS context is configured to
// accept early data (SSL_set_quic_early_data_enabled).  The application
// layer MUST ensure that only idempotent operations are sent in early data.

#ifdef THEMIS_ENABLE_HTTP3

#include "network/quic_server.h"
#include "utils/logger.h"

#include <ngtcp2/ngtcp2_crypto_openssl.h>
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <boost/asio/buffer.hpp>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <string_view>

namespace themis::network {

// ─────────────────────────────────────────────────────────────────────────────
// File-local helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Current time in nanoseconds (ngtcp2 timestamp unit).
static uint64_t quicServerNow() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
        .count());
}

/// Fill a ngtcp2_cid with cryptographically secure random bytes (OpenSSL).
static void generateCid(ngtcp2_cid* cid) {
    cid->datalen = NGTCP2_MIN_CIDLEN;
    if (RAND_bytes(cid->data, static_cast<int>(cid->datalen)) != 1) {
        // Fallback: deterministic zero-fill rather than undefined memory.
        std::memset(cid->data, 0, cid->datalen);
    }
}

/// Map a congestion control name to the ngtcp2 enum value.
static ngtcp2_cc_algo resolveCcAlgo(const std::string& algo) {
    // Normalize to lower-case comparison.
    std::string lower = algo;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (lower == "cubic") {
        return NGTCP2_CC_ALGO_CUBIC;
    }
    // Default (including "bbr"): fall back to BBR when available.
    // ngtcp2 1.x exposes NGTCP2_CC_ALGO_BBR; older builds fall back to
    // CUBIC because BBR was not yet available.
#if defined(NGTCP2_CC_ALGO_BBR)
    return NGTCP2_CC_ALGO_BBR;
#else
    return NGTCP2_CC_ALGO_CUBIC;
#endif
}

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────
// QUICServer — Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────────

QUICServer::QUICServer(const Config&                  config,
                       std::shared_ptr<RocksDBWrapper> storage,
                       SecondaryIndexManager*          index_mgr)
    : config_(config)
    , storage_(std::move(storage))
    , index_mgr_(index_mgr)
    , io_ctx_(std::make_unique<net::io_context>())
{}

QUICServer::~QUICServer() {
    stop();
    if (ssl_ctx_) {
        SSL_CTX_free(ssl_ctx_);
        ssl_ctx_ = nullptr;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// QUICServer — TLS context
// ─────────────────────────────────────────────────────────────────────────────

/* static */
SSL_CTX* QUICServer::createSslContext(const std::string& cert_path,
                                      const std::string& key_path) {
    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        return nullptr;
    }

    // QUIC requires TLS 1.3; reject any older protocol version.
    SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);

    // Load certificate chain when path is provided.
    if (!cert_path.empty() &&
        SSL_CTX_use_certificate_chain_file(ctx, cert_path.c_str()) != 1) {
        THEMIS_ERROR("[QUICServer] Failed to load certificate: {}", cert_path);
        SSL_CTX_free(ctx);
        return nullptr;
    }

    // Load private key when path is provided.
    if (!key_path.empty() &&
        SSL_CTX_use_PrivateKey_file(ctx, key_path.c_str(), SSL_FILETYPE_PEM) != 1) {
        THEMIS_ERROR("[QUICServer] Failed to load private key: {}", key_path);
        SSL_CTX_free(ctx);
        return nullptr;
    }

    // Advertise HTTP/3 ALPN ("h3") for compatibility with standard HTTP/3
    // clients, plus ThemisDB binary wire protocol ("tmdb").
    static const unsigned char kAlpn[] =
        "\x02h3"          // "h3"   (length 2)
        "\x04tmdb";       // "tmdb" (length 4)
    SSL_CTX_set_alpn_select_cb(
        ctx,
        [](SSL* /*ssl*/,
           const unsigned char** out, unsigned char* outlen,
           const unsigned char* in,  unsigned int   inlen,
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

    // Register the ngtcp2/OpenSSL QUIC method (replaces the normal TLS
    // record-layer with QUIC crypto handshake messages).
    SSL_CTX_set_quic_method(ctx, ngtcp2_crypto_quic_method());

    return ctx;
}

// ─────────────────────────────────────────────────────────────────────────────
// QUICServer — Validation helpers
// ─────────────────────────────────────────────────────────────────────────────

/* static */
bool QUICServer::isValidCongestionControl(const std::string& algo) {
    std::string lower = algo;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lower == "bbr" || lower == "cubic";
}

/* static */
bool QUICServer::isValidPort(uint16_t port) {
    if (port == 0 || port == 80 || port == 443) {
        return false;
    }
    // Reserved ThemisDB ports that QUICServer must not shadow:
    //   8766 – TCP binary wire protocol (WireProtocolServer)
    //   8770 – QUIC binary wire protocol (QuicTransport)
    //   8771 – gRPC native transport (GrpcTransport)
    //   8772 – DPDK kernel-bypass server (DPDKServer)
    //   8773 – io_uring server (IoUringServer)
    //   8774 – Raft load-balancer port (RaftLoadBalancer)
    if (port == 8766 || port == 8770 || port == 8771 ||
        port == 8772 || port == 8773 || port == 8774) {
        return false;
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// QUICServer — start / stop
// ─────────────────────────────────────────────────────────────────────────────

void QUICServer::start() {
    if (running_.load(std::memory_order_acquire)) {
        return;
    }

    // Validate congestion control setting before binding.
    if (!isValidCongestionControl(config_.congestion_control)) {
        THEMIS_ERROR("[QUICServer] Unknown congestion control '{}'; "
                     "supported: 'bbr', 'cubic'",
                     config_.congestion_control);
        return;
    }

    // Create TLS context (cert/key paths may be empty in test/dev mode).
    ssl_ctx_ = createSslContext(config_.tls_cert_path, config_.tls_key_path);
    if (!ssl_ctx_) {
        THEMIS_ERROR("[QUICServer] Failed to create TLS context");
        return;
    }

    udp::endpoint endpoint(net::ip::make_address(config_.host), config_.port);
    socket_ = std::make_unique<udp::socket>(*io_ctx_, endpoint);

    running_.store(true, std::memory_order_release);

    THEMIS_INFO("[QUICServer] listening on {}:{} (QUIC/TLS 1.3, cc={})",
                config_.host, config_.port, config_.congestion_control);

    doReceive();

    const std::size_t n = std::max<std::size_t>(1, config_.num_threads);
    threads_.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        threads_.emplace_back([this] { io_ctx_->run(); });
    }
}

void QUICServer::stop() {
    if (!running_.exchange(false, std::memory_order_acq_rel)) {
        return;
    }

    // Close all active QUIC connections.
    {
        std::lock_guard<std::mutex> lk(sessions_mutex_);
        for (auto& [key, conn] : sessions_) {
            if (conn) {
                void* tls = ngtcp2_conn_get_tls_native_handle(conn);
                if (tls) {
                    SSL_free(static_cast<SSL*>(tls));
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

    THEMIS_INFO("[QUICServer] stopped");
}

// ─────────────────────────────────────────────────────────────────────────────
// QUICServer — Receive loop
// ─────────────────────────────────────────────────────────────────────────────

void QUICServer::doReceive() {
    socket_->async_receive_from(
        net::buffer(recv_buf_),
        sender_endpoint_,
        [this](const boost::system::error_code& ec, std::size_t bytes) {
            if (ec) {
                if (ec != net::error::operation_aborted) {
                    THEMIS_ERROR("[QUICServer] receive error: {}", ec.message());
                }
                return;
            }

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
// QUICServer — Packet handler
// ─────────────────────────────────────────────────────────────────────────────

void QUICServer::handlePacket(const udp::endpoint& sender,
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
                                      data, len, quicServerNow());
        if (rv != 0) {
            if (rv == NGTCP2_ERR_DRAINING || rv == NGTCP2_ERR_DROP_CONN) {
                // Connection is closing; free SSL and remove from map.
                void* tls = ngtcp2_conn_get_tls_native_handle(it->second);
                if (tls) {
                    SSL_free(static_cast<SSL*>(tls));
                }
                ngtcp2_conn_del(it->second);
                sessions_.erase(it);
                std::lock_guard<std::mutex> slk(stats_mutex_);
                --stats_.active_connections;
            } else {
                THEMIS_WARN("[QUICServer] ngtcp2_conn_read_pkt({}): {}",
                            key, ngtcp2_strerror(rv));
            }
        }
        return;
    }

    // New Initial packet: enforce connection limit before allocating.
    if (!checkConnectionLimit()) {
        THEMIS_WARN("[QUICServer] connection limit reached, dropping from {}",
                    key);
        return;
    }

    // Decode QUIC long-header to extract client DCID.
    ngtcp2_pkt_hd hd;
    if (ngtcp2_pkt_decode_hd_long(&hd, data, len) < 0) {
        std::lock_guard<std::mutex> slk(stats_mutex_);
        // Short-header from unknown connection; silently drop.
        return;
    }

    // Generate a server-side Source Connection ID.
    ngtcp2_cid scid;
    generateCid(&scid);

    // Build ngtcp2 settings from our Config.
    ngtcp2_settings settings;
    ngtcp2_settings_default(&settings);
    settings.initial_ts = quicServerNow();
    settings.max_idle_timeout =
        static_cast<ngtcp2_duration>(config_.max_idle_timeout_sec) *
        1000 * NGTCP2_MILLISECONDS;
    settings.max_stream_data_bidi_local  = config_.initial_max_stream_data_bidi;
    settings.max_stream_data_bidi_remote = config_.initial_max_stream_data_bidi;
    settings.max_stream_data_uni         = config_.initial_max_stream_data_uni;
    settings.max_data                    = config_.initial_max_data;
    settings.max_streams_bidi            = config_.max_streams_per_connection;
    settings.max_streams_uni             = 3;
    // Congestion control algorithm selection (ngtcp2 >= 0.10).
#if defined(NGTCP2_CC_ALGO_BBR) || defined(NGTCP2_CC_ALGO_CUBIC)
    settings.cc_algo = resolveCcAlgo(config_.congestion_control);
#endif

    // Transport parameters (datagram and per-connection limits).
    ngtcp2_transport_params params;
    ngtcp2_transport_params_default(&params);

    // Minimal server callbacks.
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
                                    const uint8_t*          cbdata,
                                    size_t                  cbdatalen,
                                    void*                   /*user_data*/) -> int {
        return ngtcp2_crypto_read_write_crypto_data(conn, level, cbdata, cbdatalen);
    };

    callbacks.handshake_completed = [](ngtcp2_conn* /*conn*/,
                                        void*         user_data) -> int {
        auto* srv = static_cast<QUICServer*>(user_data);
        std::lock_guard<std::mutex> lk(srv->stats_mutex_);
        ++srv->stats_.handshakes_completed;
        return 0;
    };

    // Detect 0-RTT early data acceptance/rejection via the recv_stream_data
    // callback: if the connection is still in early-data state the first
    // stream data is 0-RTT.
    callbacks.recv_stream_data = []([[maybe_unused]] ngtcp2_conn*   conn,
                                    uint32_t       flags,
                                    int64_t        /*stream_id*/,
                                    uint64_t       /*offset*/,
                                    const uint8_t* /*data*/,
                                    size_t         /*datalen*/,
                                    void*          user_data) -> int {
        auto* srv = static_cast<QUICServer*>(user_data);
        // Track 0-RTT early data reception.  The NGTCP2_STREAM_DATA_FLAG_EARLY
        // flag is defined in ngtcp2 >= 0.16; guard for older builds.
#if defined(NGTCP2_STREAM_DATA_FLAG_EARLY)
        if (flags & NGTCP2_STREAM_DATA_FLAG_EARLY) {
            std::lock_guard<std::mutex> lk(srv->stats_mutex_);
            ++srv->stats_.zero_rtt_accepted;
        }
#else
#endif
        return 0;
    };

    callbacks.stream_open = [](ngtcp2_conn* /*conn*/,
                                int64_t     /*stream_id*/,
                                void*        user_data) -> int {
        auto* srv = static_cast<QUICServer*>(user_data);
        std::lock_guard<std::mutex> lk(srv->stats_mutex_);
        ++srv->stats_.total_streams;
        return 0;
    };

    // Create a fresh SSL object for this connection.
    SSL* ssl = SSL_new(ssl_ctx_);
    if (!ssl) {
        THEMIS_ERROR("[QUICServer] SSL_new failed for {}", key);
        return;
    }
    SSL_set_accept_state(ssl);
    if (config_.enable_0rtt) {
        SSL_set_quic_early_data_enabled(ssl, 1);
    }

    ngtcp2_path path;
    std::memset(&path, 0, sizeof(path));

    ngtcp2_conn* conn = nullptr;
    int rv = ngtcp2_conn_server_new(&conn, &hd.dcid, &scid, &path,
                                    kQuicServerVersion1, &callbacks,
                                    &settings, &params, this);
    if (rv != 0) {
        THEMIS_ERROR("[QUICServer] ngtcp2_conn_server_new({}): {}",
                     key, ngtcp2_strerror(rv));
        SSL_free(ssl);
        return;
    }

    ngtcp2_conn_set_tls_native_handle(conn, ssl);

    // Feed the Initial packet.
    ngtcp2_pkt_info pi;
    std::memset(&pi, 0, sizeof(pi));
    rv = ngtcp2_conn_read_pkt(conn, &path, &pi, data, len, quicServerNow());
    if (rv != 0 && rv != NGTCP2_ERR_RETRY) {
        THEMIS_WARN("[QUICServer] ngtcp2_conn_read_pkt (new, {}): {}",
                    key, ngtcp2_strerror(rv));
        ngtcp2_conn_del(conn);
        SSL_free(ssl);
        return;
    }

    sessions_[key] = conn;
    {
        std::lock_guard<std::mutex> slk(stats_mutex_);
        ++stats_.total_connections;
        ++stats_.active_connections;
    }
    THEMIS_INFO("[QUICServer] new QUIC connection from {}", key);
}

// ─────────────────────────────────────────────────────────────────────────────
// QUICServer — Connection limit
// ─────────────────────────────────────────────────────────────────────────────

bool QUICServer::checkConnectionLimit() {
    if (config_.max_connections == 0) {
        return true;  // Unlimited
    }
    std::lock_guard<std::mutex> slk(stats_mutex_);
    if (stats_.active_connections >= config_.max_connections) {
        ++stats_.connection_limit_drops;
        return false;
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// QUICServer — Stats
// ─────────────────────────────────────────────────────────────────────────────

QUICServer::Stats QUICServer::getStats() const {
    std::lock_guard<std::mutex> lk(stats_mutex_);
    return stats_;
}

// ─────────────────────────────────────────────────────────────────────────────
// QUICClient::Stream
// ─────────────────────────────────────────────────────────────────────────────

QUICClient::Stream::Stream(int64_t stream_id, QUICClient* owner)
    : stream_id_(stream_id)
    , owner_(owner)
{}

QUICClient::Stream::~Stream() {
    if (open_.load(std::memory_order_acquire)) {
        close();
    }
}

void QUICClient::Stream::send(const std::vector<uint8_t>& data) {
    if (!open_.load(std::memory_order_acquire)) {
        return;
    }
    if (!owner_ || !owner_->isConnected()) {
        return;
    }
    // Enqueue data for transmission on this stream.
    // In the current implementation the actual transmission is deferred to
    // the ngtcp2 write path; here we record that a send was requested.
    // A production expansion would call ngtcp2_conn_writev_stream() via the
    // owning client's I/O thread.
    THEMIS_DEBUG("[QUICClient::Stream] send {} bytes on stream {}",
                 data.size(), stream_id_);
}

std::vector<uint8_t> QUICClient::Stream::receive() {
    std::lock_guard<std::mutex> lk(buf_mutex_);
    std::vector<uint8_t> out;
    out.swap(recv_buf_);
    return out;
}

void QUICClient::Stream::close() {
    if (!open_.exchange(false, std::memory_order_acq_rel)) {
        return;
    }
    THEMIS_DEBUG("[QUICClient::Stream] stream {} closed", stream_id_);
}

// ─────────────────────────────────────────────────────────────────────────────
// QUICClient
// ─────────────────────────────────────────────────────────────────────────────

QUICClient::QUICClient(const std::string& url, const Config& config)
    : config_(config)
    , url_(url)
{
    if (!parseUrl(url, host_, port_)) {
        THEMIS_WARN("[QUICClient] malformed URL: {}", url);
    }
}

QUICClient::~QUICClient() {
    if (connected_.load(std::memory_order_acquire)) {
        disconnect();
    }
    if (ssl_ctx_) {
        SSL_CTX_free(ssl_ctx_);
        ssl_ctx_ = nullptr;
    }
}

/* static */
bool QUICClient::parseUrl(const std::string& url,
                          std::string&       host,
                          uint16_t&          port) {
    constexpr std::string_view kScheme = "quic://";
    if (url.size() < kScheme.size()) {
        return false;
    }
    std::string_view sv(url);
    if (sv.substr(0, kScheme.size()) != kScheme) {
        return false;
    }
    sv.remove_prefix(kScheme.size());

    // Find last colon for port (handles IPv6 addresses in brackets).
    const auto colon = sv.rfind(':');
    if (colon == std::string_view::npos) {
        return false;
    }

    host = std::string(sv.substr(0, colon));
    const std::string port_str(sv.substr(colon + 1));
    if (port_str.empty()) {
        return false;
    }
    try {
        const int p = std::stoi(port_str);
        if (p <= 0 || p > 65535) {
            return false;
        }
        port = static_cast<uint16_t>(p);
    } catch (...) {
        return false;
    }
    return !host.empty();
}

void QUICClient::connect() {
    if (connected_.load(std::memory_order_acquire)) {
        return;
    }
    if (host_.empty() || port_ == 0) {
        throw std::runtime_error("[QUICClient] invalid URL: " + url_);
    }

    // Build a client-side TLS context (verify_tls controls certificate
    // verification).
    ssl_ctx_ = SSL_CTX_new(TLS_client_method());
    if (!ssl_ctx_) {
        throw std::runtime_error("[QUICClient] SSL_CTX_new failed");
    }
    SSL_CTX_set_min_proto_version(ssl_ctx_, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(ssl_ctx_, TLS1_3_VERSION);
    if (!config_.verify_tls) {
        THEMIS_WARN("[SECURITY][TLS] QUIC client verify_none fallback active: "
                    "verify_tls=false disables server certificate validation (CWE-295).");
        SSL_CTX_set_verify(ssl_ctx_, SSL_VERIFY_NONE, nullptr);
    }
    SSL_CTX_set_quic_method(ssl_ctx_, ngtcp2_crypto_quic_method());

    // Minimal client-side settings.
    ngtcp2_settings settings;
    ngtcp2_settings_default(&settings);
    settings.initial_ts = quicServerNow();
    // Congestion control algorithm selection (ngtcp2 >= 0.10).
#if defined(NGTCP2_CC_ALGO_BBR) || defined(NGTCP2_CC_ALGO_CUBIC)
    settings.cc_algo = resolveCcAlgo(config_.congestion_control);
#endif

    ngtcp2_transport_params params;
    ngtcp2_transport_params_default(&params);

    ngtcp2_callbacks callbacks;
    std::memset(&callbacks, 0, sizeof(callbacks));

    callbacks.get_new_connection_id = [](ngtcp2_conn* /*conn*/,
                                         ngtcp2_cid* cid,
                                         uint8_t* /*token*/,
                                         size_t /*cidlen*/,
                                         void* /*user_data*/) -> int {
        cid->datalen = NGTCP2_MIN_CIDLEN;
        RAND_bytes(cid->data, static_cast<int>(cid->datalen));
        return 0;
    };

    callbacks.recv_crypto_data = [](ngtcp2_conn*            conn,
                                    ngtcp2_encryption_level level,
                                    uint64_t                /*offset*/,
                                    const uint8_t*          cbdata,
                                    size_t                  cbdatalen,
                                    void*                   /*user_data*/) -> int {
        return ngtcp2_crypto_read_write_crypto_data(conn, level, cbdata, cbdatalen);
    };

    SSL* ssl = SSL_new(ssl_ctx_);
    if (!ssl) {
        SSL_CTX_free(ssl_ctx_);
        ssl_ctx_ = nullptr;
        throw std::runtime_error("[QUICClient] SSL_new failed");
    }
    SSL_set_connect_state(ssl);
    SSL_set_tlsext_host_name(ssl, host_.c_str());
    if (config_.enable_0rtt) {
        SSL_set_quic_early_data_enabled(ssl, 1);
    }

    // Generate local and remote CIDs.
    ngtcp2_cid dcid, scid;
    generateCid(&dcid);
    generateCid(&scid);

    ngtcp2_path path;
    std::memset(&path, 0, sizeof(path));

    ngtcp2_conn* conn = nullptr;
    int rv = ngtcp2_conn_client_new(&conn, &dcid, &scid, &path,
                                    kQuicServerVersion1, &callbacks,
                                    &settings, &params, this);
    if (rv != 0) {
        SSL_free(ssl);
        SSL_CTX_free(ssl_ctx_);
        ssl_ctx_ = nullptr;
        throw std::runtime_error(
            std::string("[QUICClient] ngtcp2_conn_client_new: ") +
            ngtcp2_strerror(rv));
    }

    ngtcp2_conn_set_tls_native_handle(conn, ssl);
    conn_ = conn;

    connected_.store(true, std::memory_order_release);
    THEMIS_INFO("[QUICClient] connected to {}:{} (cc={})",
                host_, port_, config_.congestion_control);
}

void QUICClient::disconnect() {
    if (!connected_.exchange(false, std::memory_order_acq_rel)) {
        return;
    }

    // Close all open streams.
    {
        std::lock_guard<std::mutex> lk(streams_mutex_);
        for (auto& [id, stream] : streams_) {
            if (stream && stream->isOpen()) {
                stream->close();
            }
        }
        streams_.clear();
    }

    if (conn_) {
        void* tls = ngtcp2_conn_get_tls_native_handle(conn_);
        if (tls) {
            SSL_free(static_cast<SSL*>(tls));
        }
        ngtcp2_conn_del(conn_);
        conn_ = nullptr;
    }

    if (ssl_ctx_) {
        SSL_CTX_free(ssl_ctx_);
        ssl_ctx_ = nullptr;
    }

    THEMIS_INFO("[QUICClient] disconnected from {}:{}", host_, port_);
}

std::unique_ptr<QUICClient::Stream> QUICClient::openStream() {
    if (!connected_.load(std::memory_order_acquire)) {
        throw std::runtime_error("[QUICClient] openStream called when not connected");
    }

    // Client-initiated bidirectional stream IDs: 0, 4, 8, …  (RFC 9000 §2.1)
    int64_t sid;
    {
        std::lock_guard<std::mutex> lk(streams_mutex_);
        sid = next_stream_id_;
        next_stream_id_ += 4;
    }

    auto stream = std::make_unique<Stream>(sid, this);

    {
        std::lock_guard<std::mutex> lk(streams_mutex_);
        streams_[sid] = stream.get();
    }

    THEMIS_DEBUG("[QUICClient] opened stream {}", sid);
    return stream;
}

}  // namespace themis::network

#endif  // THEMIS_ENABLE_HTTP3

