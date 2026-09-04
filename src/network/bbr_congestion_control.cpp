/**
 * @file bbr_congestion_control.cpp
 * @brief BBRv2 congestion control configuration implementation for ThemisDB.
 * @version 0.1.0
 * @note Maturity: 🟡 BETA
 */

#include "network/bbr_congestion_control.h"
#include "utils/logger.h"

#include <cerrno>
#include <cstring>

// ── Platform-specific headers ─────────────────────────────────────────────────
#ifdef __linux__
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#  include <sys/socket.h>
#  include <unistd.h>
#  include <cstdint>
#  ifndef TCP_CONGESTION
#    define TCP_CONGESTION 13
#  endif
#  ifndef TCP_CC_INFO
#    define TCP_CC_INFO    26
#  endif
#  ifndef TCP_ECN
#    define TCP_ECN        14
#  endif
// BBR congestion-control info structure (kernel 4.9+).
// Not exposed by glibc; defined locally using standard integer types.
struct tcp_bbr_info {
    uint32_t bbr_bw_lo = 0;       ///< Bandwidth estimate lower 32 bits (bytes/s)
    uint32_t bbr_bw_hi;       ///< Bandwidth estimate upper 32 bits (bytes/s)
    uint32_t bbr_min_rtt;     ///< Min-RTT estimate (microseconds)
    uint32_t bbr_pacing_gain; ///< Pacing gain (scaled by 2^8)
    uint32_t bbr_cwnd_gain;   ///< CWND gain (scaled by 2^8)
};
#endif // __linux__

namespace themis {
namespace network {

// =============================================================================
// BbrCongestionController
// =============================================================================

BbrCongestionController::BbrCongestionController(int              socket_fd,
                                                  CongestionAlgorithm algorithm)
    : socket_fd_(socket_fd)
    , active_algorithm_(algorithm)
{
    if (algorithm != CongestionAlgorithm::SYSTEM) {
        setCongestionAlgorithm(algorithm);
    }
}

bool BbrCongestionController::setCongestionAlgorithm(CongestionAlgorithm algorithm) noexcept {
#ifdef __linux__
    const char* name = algorithmToKernelName(algorithm);
    if (name == nullptr) {
        return false;
    }
    if (::setsockopt(socket_fd_, IPPROTO_TCP, TCP_CONGESTION,
                     name, static_cast<socklen_t>(std::strlen(name))) == 0) {
        active_algorithm_ = algorithm;
        return true;
    }
    // If BBRv2 was requested but not available, fall back to BBRv1 then CUBIC.
    if (algorithm == CongestionAlgorithm::BBRv2) {
        if (setCongestionAlgorithm(CongestionAlgorithm::BBRv1)) {
            return true;
        }
        return setCongestionAlgorithm(CongestionAlgorithm::CUBIC);
    }
    return false;
#else
    (void)algorithm;
    return false;
#endif
}

bool BbrCongestionController::applyConfig(const BbrConfig& cfg) noexcept {
#ifdef __linux__
    bool ok = true;

    // ECN: enable via IP_RECVTOS + TCP_ECN setsockopt (kernel convention).
    if (cfg.enable_ecn) {
        const int ecn_val = 1;
        if (::setsockopt(socket_fd_, IPPROTO_TCP, TCP_ECN,
                         &ecn_val, sizeof(ecn_val)) != 0) {
            ok = false;
        }
    }

    // BBRv2 exposes per-connection parameters via setsockopt keys in
    // linux/tcp.h (kernel ≥ 5.17). These are silently ignored on kernels
    // that do not support them.
    // probe_rtt_duration_ms → TCP_BBR_PROBE_RTT_INTERVAL (key 51, not in all kernels)
    if (cfg.probe_rtt_duration_ms > 0) {
        const uint32_t val = cfg.probe_rtt_duration_ms;
#  ifdef TCP_BBR_PROBE_RTT_INTERVAL
        ::setsockopt(socket_fd_, IPPROTO_TCP, TCP_BBR_PROBE_RTT_INTERVAL,
                     &val, sizeof(val));
#  else
        (void)val;
#  endif
    }

    return ok;
#else
    (void)cfg;
    return false;
#endif
}

CongestionDiagnostics BbrCongestionController::getDiagnostics() const noexcept {
    CongestionDiagnostics diag;

#ifdef __linux__
    // Read active algorithm name.
    diag.algorithm = getActiveAlgorithmName();

    // Read TCP_INFO for RTT, cwnd, retransmit, loss info.
    struct tcp_info ti{};
    socklen_t optlen = sizeof(ti);
    if (::getsockopt(socket_fd_, IPPROTO_TCP, TCP_INFO, &ti, &optlen) == 0) {
        diag.srtt_us               = ti.tcpi_rtt;
        diag.cwnd_bytes            = ti.tcpi_snd_cwnd * ti.tcpi_snd_mss;
        diag.inflight_bytes        = ti.tcpi_unacked * ti.tcpi_snd_mss;
        diag.rto_count             = ti.tcpi_total_retrans;
        diag.fast_retransmit_count = ti.tcpi_retrans;
        diag.has_loss              = (ti.tcpi_lost > 0);
    }

    // TCP_CC_INFO provides BBR-specific fields (bandwidth, min_rtt).
#  ifdef TCP_CC_INFO
    union {
        struct tcp_bbr_info bbr;
        char raw[256];
    } cc_info{};
    socklen_t cc_optlen = sizeof(cc_info);
    if (::getsockopt(socket_fd_, IPPROTO_TCP, TCP_CC_INFO,
                     &cc_info, &cc_optlen) == 0) {
        // tcp_bbr_info.bbr_bw is in bytes/sec (64-bit, high and low words).
        const uint64_t bw_lo = cc_info.bbr.bbr_bw_lo;
        const uint64_t bw_hi = cc_info.bbr.bbr_bw_hi;
        diag.bandwidth_bps = ((bw_hi << 32) | bw_lo) * 8;
        diag.min_rtt_us    = cc_info.bbr.bbr_min_rtt;
    }
#  endif // TCP_CC_INFO

    (void)diag.has_ecn_congestion; // ECN flag: would need ECN-specific sockopt
#endif // __linux__

    return diag;
}

std::string BbrCongestionController::getActiveAlgorithmName() const noexcept {
#ifdef __linux__
    char buf[32] = {};
    socklen_t optlen = sizeof(buf) - 1;
    if (::getsockopt(socket_fd_, IPPROTO_TCP, TCP_CONGESTION,
                     buf, &optlen) == 0) {
        return std::string(buf);
    }
#endif
    return {};
}

// static
bool BbrCongestionController::isBBRv2Available() noexcept {
#ifdef __linux__
    const int probe_fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (probe_fd < 0) {
      return false;
    }
    const char* name = "bbr2";
    const bool ok = ::setsockopt(probe_fd, IPPROTO_TCP, TCP_CONGESTION,
                                  name, static_cast<socklen_t>(std::strlen(name))) == 0;
    ::close(probe_fd);
    return ok;
#else
    return false;
#endif
}

// static
bool BbrCongestionController::isBBRv1Available() noexcept {
#ifdef __linux__
    const int probe_fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (probe_fd < 0) {
      return false;
    }
    const char* name = "bbr";
    const bool ok = ::setsockopt(probe_fd, IPPROTO_TCP, TCP_CONGESTION,
                                  name, static_cast<socklen_t>(std::strlen(name))) == 0;
    ::close(probe_fd);
    return ok;
#else
    return false;
#endif
}

// static
const char* BbrCongestionController::algorithmToKernelName(CongestionAlgorithm algorithm) noexcept {
    switch (algorithm) {
        case CongestionAlgorithm::BBRv1:  return "bbr";
        case CongestionAlgorithm::BBRv2:  return "bbr2";
        case CongestionAlgorithm::CUBIC:  return "cubic";
        case CongestionAlgorithm::RENO:   return "reno";
        case CongestionAlgorithm::SYSTEM: return nullptr;
    }
    return nullptr;
}

} // namespace network
} // namespace themis
