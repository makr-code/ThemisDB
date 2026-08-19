/**
 * @file bbr_congestion_control.h
 * @brief BBRv2 congestion control configuration and diagnostics interface for ThemisDB.
 * @version 0.1.0
 * @note Maturity: 🟡 BETA
 * @note Status: Interface-Stable
 */

// ThemisDB – BBRv2 Congestion Control Configuration Interface
//
// Provides a platform-abstracted API for configuring and observing BBRv2
// congestion control on TCP (and QUIC) connections managed by ThemisDB.
//
// BBRv2 (Bottleneck Bandwidth and Round-Trip Propagation Time, v2) is a
// model-based congestion control algorithm that achieves high throughput and
// low latency without relying on packet loss as the primary congestion signal.
//
// This interface exposes:
//   - Per-connection BBR mode selection (BBR, BBRv2, CUBIC fallback)
//   - Read-only BBR diagnostic state (bandwidth estimate, inflight, cwnd)
//   - Tunable parameters for the probing phases (where the kernel permits)
//   - A diagnostic snapshot suitable for operator telemetry
//
// Platform notes:
//   - Linux ≥ 5.17: BBRv2 available as "bbr2" via TCP_CONGESTION sockopt.
//   - Linux 4.9–5.16: BBRv1 only ("bbr").
//   - All other platforms / non-Linux: read-only diagnostics return zeroes;
//     setCongestionAlgorithm() is a no-op that returns false.
//
// Guarded by THEMIS_ENABLE_BBR for the configurable path; the diagnostic
// structs and enums are always available.

#pragma once

#include <cstdint>
#include <string>

namespace themis {
namespace network {

// =============================================================================
// Congestion algorithm selector
// =============================================================================

/**
 * @brief TCP congestion control algorithm selection.
 */
enum class CongestionAlgorithm {
    BBRv1,   ///< BBR v1 (Linux ≥ 4.9, "bbr")
    BBRv2,   ///< BBR v2 (Linux ≥ 5.17, "bbr2")
    CUBIC,   ///< CUBIC (default Linux TCP congestion algorithm)
    RENO,    ///< TCP Reno (classic)
    SYSTEM,  ///< Use the system/socket default — no override applied
};

// =============================================================================
// BBR-specific tuning parameters
// =============================================================================

/**
 * @brief Tunable BBRv2 parameters for a connection or listener.
 *
 * All fields are optional overrides. The default-constructed value means
 * "use the kernel/system default" for that parameter.
 *
 * @note Not all kernels expose all parameters. Unsupported fields are silently
 *       ignored by @c BbrCongestionController::applyConfig().
 */
struct BbrConfig {
    /// Target congestion window gain during ProbeBW phase (BBRv2 only).
    /// Expressed as a fraction (e.g., 1.25 = 125 % of BDP). 0 = kernel default.
    double probe_bw_cwnd_gain = 0.0;

    /// Target pacing gain during ProbeBW UP phase.  0 = kernel default.
    double probe_bw_pacing_gain = 0.0;

    /// ProbeRTT duration in milliseconds. 0 = kernel default (200 ms).
    uint32_t probe_rtt_duration_ms = 0;

    /// Minimum number of packets in flight during ProbeRTT.  0 = kernel default.
    uint32_t probe_rtt_cwnd_gain = 0;

    /// Enable ECN (Explicit Congestion Notification) support (BBRv2 + ECN).
    bool enable_ecn = false;

    BbrConfig() = default;
};

// =============================================================================
// BBR diagnostic state (read-only snapshot from the kernel)
// =============================================================================

/**
 * @brief Diagnostic snapshot of per-connection BBR/CUBIC congestion state.
 *
 * All fields are read from the kernel via TCP_INFO / TCP_CC_INFO getsockopt.
 * Fields that the kernel does not populate are zero-initialised.
 */
struct CongestionDiagnostics {
    /// Active algorithm name as reported by the kernel (e.g., "bbr2", "cubic").
    std::string algorithm;

    /// BBR bandwidth estimate in bits per second.
    uint64_t bandwidth_bps = 0;

    /// Minimum RTT estimate in microseconds (0 if not measured).
    uint32_t min_rtt_us = 0;

    /// Current smoothed RTT in microseconds.
    uint32_t srtt_us = 0;

    /// Current congestion window in bytes.
    uint32_t cwnd_bytes = 0;

    /// Bytes currently in flight (sent but not yet acknowledged).
    uint32_t inflight_bytes = 0;

    /// Pacing rate in bytes per second (0 if pacing not active).
    uint64_t pacing_rate_bps = 0;

    /// Number of retransmit timeouts (RTO) on this connection.
    uint32_t rto_count = 0;

    /// Number of fast retransmits.
    uint32_t fast_retransmit_count = 0;

    /// True if the connection has suffered any packet loss event.
    bool has_loss = false;

    /// True if ECN-capable and at least one CE mark was received.
    bool has_ecn_congestion = false;
};

// =============================================================================
// BbrCongestionController
// =============================================================================

/**
 * @brief Per-connection BBRv2 congestion control configurator and diagnostics.
 *
 * Binds to an existing connected TCP socket (identified by file descriptor)
 * and provides:
 *   - Algorithm selection via TCP_CONGESTION sockopt.
 *   - Optional BBR parameter tuning (where the kernel supports it).
 *   - Read-only diagnostic snapshots via TCP_INFO / TCP_CC_INFO.
 *
 * Thread safety:
 *   - @c getDiagnostics() is safe to call concurrently.
 *   - @c applyConfig() and @c setCongestionAlgorithm() must not be called
 *     concurrently with each other on the same instance.
 *
 * Usage example:
 * @code
 *   BbrCongestionController bcc(socket_fd, CongestionAlgorithm::BBRv2);
 *   bcc.applyConfig({});                         // kernel defaults
 *   auto diag = bcc.getDiagnostics();
 *   LOG_INFO("bbr bandwidth estimate: {} Mbps",
 *            diag.bandwidth_bps / 1'000'000.0);
 * @endcode
 */
class BbrCongestionController {
public:
    /**
     * @brief Attach to an existing connected socket and optionally set
     *        the congestion algorithm immediately.
     *
     * @param socket_fd  Connected TCP socket fd. Caller retains ownership.
     * @param algorithm  Algorithm to activate (SYSTEM = no change on construction).
     *
     * @note On platforms that do not support BBRv2, the algorithm silently
     *       falls back to CUBIC if @c setCongestionAlgorithm() is called with
     *       CongestionAlgorithm::BBRv2.
     */
    explicit BbrCongestionController(int              socket_fd,
                                     CongestionAlgorithm algorithm = CongestionAlgorithm::SYSTEM);

    ~BbrCongestionController() = default;

    // Non-copyable, movable.
    BbrCongestionController(const BbrCongestionController&)            = delete;
    BbrCongestionController& operator=(const BbrCongestionController&) = delete;
    BbrCongestionController(BbrCongestionController&&) noexcept = default;
    BbrCongestionController& operator=(BbrCongestionController&&) noexcept = default;

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    /**
     * @brief Set the TCP congestion control algorithm for the bound socket.
     *
     * @param algorithm  Algorithm to activate.
     * @return true on success; false on error (errno set) or if the algorithm
     *         is not supported by the kernel.
     */
    bool setCongestionAlgorithm(CongestionAlgorithm algorithm) noexcept;

    /**
     * @brief Apply BBR-specific tuning parameters to the bound socket.
     *
     * No-op if the active algorithm is not BBR/BBRv2 or the kernel does not
     * expose the requested setsockopt keys.
     *
     * @param cfg  Parameter overrides. Default-constructed = kernel defaults.
     * @return true if all non-zero parameters were accepted by the kernel.
     */
    bool applyConfig(const BbrConfig& cfg) noexcept;

    // -------------------------------------------------------------------------
    // Diagnostics
    // -------------------------------------------------------------------------

    /**
     * @brief Return a current diagnostic snapshot for the bound socket.
     *
     * Reads TCP_INFO and TCP_CC_INFO (where available). Fields not populated
     * by the kernel are zero-initialised.
     */
    CongestionDiagnostics getDiagnostics() const noexcept;

    /**
     * @brief Return the currently active algorithm name.
     *
     * Reads the TCP_CONGESTION socket option.
     * Returns an empty string on non-Linux platforms.
     */
    std::string getActiveAlgorithmName() const noexcept;

    // -------------------------------------------------------------------------
    // Capability queries (static)
    // -------------------------------------------------------------------------

    /**
     * @brief Return true if BBRv2 is available on the running system.
     *
     * Performs a lightweight probe via a temporary socket and TCP_CONGESTION.
     */
    static bool isBBRv2Available() noexcept;

    /**
     * @brief Return true if BBRv1 is available on the running system.
     */
    static bool isBBRv1Available() noexcept;

    /**
     * @brief Convert a @c CongestionAlgorithm to its kernel name string.
     *
     * Returns the TCP_CONGESTION string for use in setsockopt (e.g., "bbr2").
     */
    static const char* algorithmToKernelName(CongestionAlgorithm algorithm) noexcept;

private:
    int                 socket_fd_{-1};
    CongestionAlgorithm active_algorithm_{CongestionAlgorithm::SYSTEM};
};

} // namespace network
} // namespace themis
