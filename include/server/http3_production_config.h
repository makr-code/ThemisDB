/**
 * @file http3_production_config.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>
#include <string>
#include <chrono>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <optional>

namespace themis {
namespace server {

// ============================================================================
// Congestion Control Configuration
// ============================================================================

/**
 * @brief QUIC congestion control algorithm selection.
 *
 * Maps to ngtcp2 CC algorithm values.  BBR is recommended for production
 * because it achieves higher throughput on lossy/high-BDP paths and avoids
 * the buffer-bloat that CUBIC can cause on LTE/mobile networks.
 */
enum class Http3CongestionAlgorithm : int {
    Cubic = 0, ///< CUBIC (RFC 8312) – ngtcp2 default
    Reno  = 1, ///< New Reno – conservative, good for very stable networks
    Bbr   = 2, ///< BBR v1 – better for high-BDP and lossy mobile networks
};

// ============================================================================
// Production Configuration
// ============================================================================

/**
 * @brief Consolidated production-readiness settings for HTTP/3.
 *
 * Passed to Http3Handler at construction time.  The defaults are tuned for
 * server workloads; adjust for your deployment.
 */
struct Http3ProductionConfig {
    // ---- Congestion control ------------------------------------------------
    Http3CongestionAlgorithm cc_algorithm = Http3CongestionAlgorithm::Bbr;

    // ---- 0-RTT / session resumption ----------------------------------------
    bool enable_0rtt        = true;  ///< Allow early-data on resumed connections
    uint32_t session_ticket_lifetime_secs = 86400; ///< TLS session ticket TTL (24h)

    // ---- Connection migration ----------------------------------------------
    bool enable_migration         = true;  ///< Allow IP/port changes mid-connection
    bool strict_path_validation   = true;  ///< Require path-challenge before migration

    // ---- Fallback to HTTP/2 on QUIC failure --------------------------------
    bool    enable_http2_fallback         = true;
    uint32_t fallback_failure_threshold   = 3;   ///< Failures before marking fallback
    uint32_t fallback_recovery_secs       = 300; ///< Seconds before re-enabling QUIC

    // ---- Flow-control / transport tuning -----------------------------------
    uint64_t initial_max_data               = 10ULL * 1024 * 1024; ///< 10 MB
    uint64_t initial_max_stream_data_bidi   = 1024 * 1024;          ///< 1 MB per stream
    uint64_t initial_max_stream_data_uni    = 512  * 1024;          ///< 512 KB uni
    uint64_t initial_max_streams_bidi       = 200;
    uint64_t initial_max_streams_uni        = 7;

    // ---- Metrics -----------------------------------------------------------
    bool enable_performance_metrics = true;

    Http3ProductionConfig() = default;
};

// ============================================================================
// Per-Connection Performance Metrics
// ============================================================================

/**
 * @brief Lightweight metrics recorded per HTTP/3 connection.
 *
 * Useful for comparing latency / throughput against HTTP/2 baselines.
 * All durations are in microseconds for sub-millisecond resolution.
 */
struct Http3ConnectionMetrics {
    // Handshake timing
    int64_t  handshake_start_us   = 0; ///< Absolute µs since epoch at connect
    int64_t  handshake_end_us     = 0; ///< Absolute µs since epoch at handshake done
    bool     zero_rtt_used        = false; ///< Connection resumed with 0-RTT early data

    // Traffic
    std::atomic<uint64_t> bytes_sent     {0};
    std::atomic<uint64_t> bytes_received {0};
    std::atomic<uint64_t> requests_total {0};

    // Cumulative request latency (microseconds)
    std::atomic<uint64_t> request_latency_total_us {0};

    // Path migration events
    std::atomic<uint32_t> migration_count {0};

    Http3ConnectionMetrics() = default;

    // Non-copyable due to atomics; provide an explicit snapshot for logging
    struct Snapshot {
        int64_t  handshake_start_us;
        int64_t  handshake_end_us;
        int64_t  handshake_duration_us;
        bool     zero_rtt_used;
        uint64_t bytes_sent;
        uint64_t bytes_received;
        uint64_t requests_total;
        uint64_t avg_request_latency_us;
        uint32_t migration_count;
    };

    Snapshot snapshot() const {
        Snapshot s{};
        s.handshake_start_us      = handshake_start_us;
        s.handshake_end_us        = handshake_end_us;
        s.handshake_duration_us   = handshake_end_us - handshake_start_us;
        s.zero_rtt_used           = zero_rtt_used;
        s.bytes_sent              = bytes_sent.load(std::memory_order_relaxed);
        s.bytes_received          = bytes_received.load(std::memory_order_relaxed);
        s.requests_total          = requests_total.load(std::memory_order_relaxed);
        uint64_t n  = s.requests_total;
        uint64_t lt = request_latency_total_us.load(std::memory_order_relaxed);
        s.avg_request_latency_us  = (n > 0) ? (lt / n) : 0;
        s.migration_count         = migration_count.load(std::memory_order_relaxed);
        return s;
    }
};

// ============================================================================
// HTTP/2 Fallback Manager
// ============================================================================

/**
 * @brief Tracks per-client QUIC failure counts and manages HTTP/2 fallback.
 *
 * When a client's QUIC failure count exceeds the configured threshold the
 * manager marks that client as "use HTTP/2" for a configurable recovery window.
 * After the window expires the client is re-allowed to attempt QUIC.
 *
 * Thread-safe.  Designed for use in Http3Handler and HttpServer.
 */
class Http3FallbackManager {
public:
    explicit Http3FallbackManager(const Http3ProductionConfig& cfg);

    /**
     * @brief Record a QUIC connection/handshake failure for the given client IP.
     */
    void recordQuicFailure(const std::string& client_ip);

    /**
     * @brief Record a successful QUIC connection, resetting the failure counter.
     */
    void recordQuicSuccess(const std::string& client_ip);

    /**
     * @brief Return true if this client should use HTTP/2 instead of QUIC.
     */
    bool shouldFallbackToHttp2(const std::string& client_ip) const;

    /**
     * @brief Generate the Alt-Svc header value announcing HTTP/3 support.
     *
     * Returns an empty string when @p client_ip is in fallback mode so the
     * server can suppress the header and the client stops attempting QUIC.
     *
     * @param h3_port   UDP port on which HTTP/3 listens.
     * @param client_ip Source IP of the requesting client.
     */
    std::string altSvcValue(uint16_t h3_port,
                            const std::string& client_ip) const;

    /**
     * @brief Sweep expired fallback entries (call periodically).
     */
    void purgeExpired();

    /**
     * @brief Total number of clients currently in fallback mode.
     */
    size_t fallbackClientCount() const;

private:
    struct ClientState {
        uint32_t failure_count = 0;
        std::chrono::steady_clock::time_point fallback_until;
        bool in_fallback = false;
    };

    Http3ProductionConfig cfg_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ClientState> clients_;
};

} // namespace server
} // namespace themis
