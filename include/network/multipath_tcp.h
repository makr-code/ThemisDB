/**
 * @file multipath_tcp.h
 * @brief MPTCP (Multipath TCP) subflow management interface for ThemisDB network module.
 * @version 0.1.0
 * @note Maturity: 🟡 BETA
 * @note Status: Interface-Stable
 */

// ThemisDB – MPTCP subflow management interface
//
// Provides a platform-abstracted API for managing Multipath TCP (MPTCP, RFC 8684)
// subflows. On Linux kernels ≥ 5.6 with MPTCP enabled, this interface wires
// MPTCP sockets via the kernel's IPPROTO_MPTCP protocol family. On platforms
// without MPTCP support (or when THEMIS_ENABLE_MPTCP is not defined), all
// operations degrade gracefully to standard TCP semantics.
//
// Key design goals:
//   - Transparent fallback: absent MPTCP kernel support → single-path TCP.
//   - Per-connection subflow visibility: enumeration, priority, and bandwidth.
//   - Operator-facing diagnostics: subflow state, path RTT, bytes-per-path.
//   - Zero extra copies: subflow management does not touch payload buffers.
//
// Guarded by THEMIS_ENABLE_MPTCP (requires Linux ≥ 5.6 with mptcp enabled).

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace network {

/// @brief MPTCP subflow operational state.
enum class MptcpSubflowState {
    ACTIVE,    ///< Subflow is established and carrying traffic
    BACKUP,    ///< Subflow is established but not currently active (backup path)
    CLOSING,   ///< Subflow teardown in progress
    REMOVED,   ///< Subflow has been removed from the connection
    FAILED,    ///< Subflow experienced a fatal error
};

/// @brief Descriptor for a single MPTCP subflow.
struct MptcpSubflowInfo {
    uint32_t    subflow_id    = 0;        ///< Kernel-assigned subflow identifier
    std::string local_addr;               ///< Local IP address of this subflow
    uint16_t    local_port   = 0;         ///< Local TCP port
    std::string remote_addr;              ///< Remote IP address of this subflow
    uint16_t    remote_port  = 0;         ///< Remote TCP port
    MptcpSubflowState state  = MptcpSubflowState::ACTIVE;
    uint32_t    priority     = 0;         ///< 0 = primary, >0 = backup (lower = higher priority)
    uint64_t    bytes_sent   = 0;         ///< Total bytes sent over this subflow
    uint64_t    bytes_recv   = 0;         ///< Total bytes received over this subflow
    uint32_t    rtt_us       = 0;         ///< Smoothed RTT estimate in microseconds (0 if unknown)
    uint32_t    bandwidth_kbps = 0;       ///< Estimated bandwidth in kbps (0 if unknown)
};

/// @brief Aggregate MPTCP connection statistics.
struct MptcpConnectionStats {
    uint32_t active_subflows   = 0;       ///< Number of subflows in ACTIVE state
    uint32_t total_subflows    = 0;       ///< Total subflows (incl. backup/closing)
    uint64_t total_bytes_sent  = 0;       ///< Bytes sent across all subflows
    uint64_t total_bytes_recv  = 0;       ///< Bytes received across all subflows
    bool     fallback_to_tcp   = false;   ///< True if session fell back to plain TCP
};

/// @brief Configuration for an MPTCP-capable listener or connection.
struct MptcpConfig {
    /// Maximum number of subflows per MPTCP connection (0 = kernel default).
    uint32_t max_subflows = 0;

    /// Enable MPTCP join advertisements (server announces additional paths).
    bool enable_path_manager = true;

    /// Scheduler hint for the kernel path manager ("default", "redundant",
    /// "roundrobin"). Not all kernels honour this field.
    std::string scheduler = "default";

    /// Fail closed to plain TCP when MPTCP is unavailable (rather than error).
    bool fallback_to_tcp = true;

    MptcpConfig() = default;
};

// =============================================================================
// MptcpSubflowManager
// =============================================================================

/**
 * @brief Manages MPTCP subflows for a single connection socket.
 *
 * Wraps a connected MPTCP socket (or a plain TCP socket in fallback mode)
 * and exposes operations to enumerate, prioritise, and observe subflows.
 *
 * Thread safety:
 *   - @c enumerateSubflows(), @c getStats(), and @c isMptcpEnabled() are safe
 *     to call concurrently from multiple threads.
 *   - @c setPriority(), @c removeSubflow(), and @c close() must not be called
 *     concurrently with each other.
 *
 * Platform behaviour:
 *   - Linux ≥ 5.6 with CONFIG_MPTCP: full MPTCP subflow management via
 *     IPPROTO_MPTCP sockets and MPTCP_INFO getsockopt.
 *   - All other platforms: no-op methods; @c isMptcpEnabled() returns false.
 */
class MptcpSubflowManager {
public:
    /**
     * @brief Construct a manager for the given connected socket fd.
     *
     * @param socket_fd  File descriptor of a connected MPTCP or TCP socket.
     *                   The caller retains ownership; close the fd after
     *                   @c MptcpSubflowManager is destroyed.
     * @param config     MPTCP configuration options.
     */
    explicit MptcpSubflowManager(int socket_fd, const MptcpConfig& config = {});

    ~MptcpSubflowManager() = default;

    // Non-copyable, movable.
    MptcpSubflowManager(const MptcpSubflowManager&)            = delete;
    MptcpSubflowManager& operator=(const MptcpSubflowManager&) = delete;
    MptcpSubflowManager(MptcpSubflowManager&&) noexcept = default;
    MptcpSubflowManager& operator=(MptcpSubflowManager&&) noexcept = default;

    // -------------------------------------------------------------------------
    // Capability and state queries
    // -------------------------------------------------------------------------

    /**
     * @brief Return true if the underlying socket uses MPTCP (not plain TCP).
     *
     * Returns false after fallback or on platforms without MPTCP support.
     */
    bool isMptcpEnabled() const noexcept;

    /**
     * @brief Return a snapshot of all subflows for this connection.
     *
     * On platforms without MPTCP or in fallback mode, returns a single
     * synthetic entry representing the plain TCP path.
     */
    std::vector<MptcpSubflowInfo> enumerateSubflows() const;

    /**
     * @brief Return aggregate connection statistics.
     */
    MptcpConnectionStats getStats() const;

    // -------------------------------------------------------------------------
    // Subflow control
    // -------------------------------------------------------------------------

    /**
     * @brief Set the priority (backup flag) for a subflow.
     *
     * @param subflow_id  Subflow to modify (from @c MptcpSubflowInfo::subflow_id).
     * @param priority    0 = primary active, >0 = backup (lower = higher priority).
     * @return true on success; false if subflow_id not found or MPTCP unavailable.
     */
    bool setPriority(uint32_t subflow_id, uint32_t priority);

    /**
     * @brief Request removal of a subflow from the connection.
     *
     * The kernel will complete any in-flight data on the subflow before
     * tearing it down. Returns false if MPTCP is unavailable.
     *
     * @param subflow_id  Subflow to remove.
     * @return true if removal was initiated; false on error.
     */
    bool removeSubflow(uint32_t subflow_id);

    // -------------------------------------------------------------------------
    // Diagnostic hook
    // -------------------------------------------------------------------------

    /**
     * @brief Register a callback invoked whenever a subflow changes state.
     *
     * Callback signature: void(const MptcpSubflowInfo& updated_info)
     * Callbacks are invoked from the calling thread during @c enumerateSubflows()
     * when state changes are detected via diff against the last snapshot.
     *
     * @note No background polling thread is created. State-change detection
     *       happens lazily on the next @c enumerateSubflows() call.
     */
    void setSubflowStateCallback(
        std::function<void(const MptcpSubflowInfo&)> callback);

private:
    int                   socket_fd_;
    MptcpConfig           config_;
    bool                  mptcp_enabled_{false};

    mutable std::vector<MptcpSubflowInfo> last_snapshot_;
    std::function<void(const MptcpSubflowInfo&)> state_cb_;
};

// =============================================================================
// Utility: create an MPTCP-capable server or client socket
// =============================================================================

/**
 * @brief Create a listening TCP (or MPTCP) socket on the given address/port.
 *
 * On Linux ≥ 5.6 with THEMIS_ENABLE_MPTCP defined, the socket is created
 * with IPPROTO_MPTCP. On other platforms (or if MPTCP creation fails and
 * @c config.fallback_to_tcp is true), falls back to IPPROTO_TCP.
 *
 * @param host    Bind address (IPv4 or IPv6).
 * @param port    TCP port to bind.
 * @param config  MPTCP configuration.
 * @return        Open listening socket fd (non-blocking, SO_REUSEADDR set),
 *                or -1 on fatal error (errno set).
 */
int createMptcpListenSocket(const std::string& host,
                            uint16_t           port,
                            const MptcpConfig& config = {}) noexcept;

/**
 * @brief Probe whether the running kernel supports MPTCP.
 *
 * @return true if MPTCP is available (Linux ≥ 5.6, kernel compiled with
 *         CONFIG_MPTCP, and the MPTCP netlink family is responsive).
 */
bool isMptcpKernelSupported() noexcept;

} // namespace network
} // namespace themis
