/**
 * @file p2p_transfer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=8; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=5, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "themis/gpu/device_discovery.h"

namespace themis {
namespace gpu {

/**
 * @brief Peer-to-peer GPU-to-GPU direct transfer manager (NVLink/PCIe).
 *
 * Enables and manages direct memory transfers between GPU devices without
 * routing data through host (CPU) memory.  On NVIDIA hardware with NVLink
 * the transfers use the high-bandwidth NVLink fabric; on other pairs they
 * use PCIe peer-to-peer DMA.  When neither path is available (e.g. CI
 * without CUDA/HIP hardware) all transfers transparently fall back to a
 * CPU simulation so that the full API can be exercised without real GPU
 * hardware.
 *
 * Hardware integration
 * --------------------
 * When `THEMIS_ENABLE_CUDA` is defined:
 *  - `canAccessPeer`      → `cudaDeviceCanAccessPeer`
 *  - `enablePeerAccess`   → `cudaDeviceEnablePeerAccess`
 *  - `disablePeerAccess`  → `cudaDeviceDisablePeerAccess`
 *  - `transfer`           → `cudaMemcpyPeer`
 *
 * When `THEMIS_ENABLE_HIP` is defined (and CUDA is absent):
 *  - `canAccessPeer`      → `hipDeviceCanAccessPeer`
 *  - `enablePeerAccess`   → `hipDeviceEnablePeerAccess`
 *  - `disablePeerAccess`  → `hipDeviceDisablePeerAccess`
 *  - `transfer`           → `hipMemcpyPeer`
 *
 * CPU fallback path: peer access capability returns false; `transfer`
 * succeeds via an in-memory `memcpy` simulation so tests always pass.
 *
 * Feature gate
 * ------------
 * All mutating operations require `GPUFeatureFlags::Feature::PEER_TO_PEER`
 * to be enabled.  Read-only queries (`canAccessPeer`, `isPeerAccessEnabled`,
 * `getStats`) are always permitted.  The feature is enabled by default for
 * ENTERPRISE and HYPERSCALER editions.
 *
 * Thread safety: all public methods are protected by an internal mutex.
 */
class GPUP2PTransferManager {
public:
    // -----------------------------------------------------------------------
    // Operation status
    // -----------------------------------------------------------------------
    enum class Status {
        OK,                         ///< Operation succeeded
        FEATURE_DISABLED,           ///< PEER_TO_PEER feature flag is off
        PEER_ACCESS_NOT_SUPPORTED,  ///< Hardware does not support P2P access
        PEER_ACCESS_ALREADY_ENABLED,///< enablePeerAccess called when already enabled
        PEER_ACCESS_NOT_ENABLED,    ///< transfer called without enablePeerAccess
        INVALID_DEVICE,             ///< src or dst device index out of range
        TRANSFER_FAILED,            ///< cudaMemcpyPeer / hipMemcpyPeer returned error
        OUT_OF_MEMORY,              ///< Insufficient device memory for the transfer
    };

    // -----------------------------------------------------------------------
    // Transfer request descriptor
    // -----------------------------------------------------------------------
    struct TransferRequest {
        int         src_device = 0;     ///< Source GPU device index
        int         dst_device = 0;     ///< Destination GPU device index
        const void* src_ptr    = nullptr;///< Source device pointer (host ptr on CPU fallback)
        void*       dst_ptr    = nullptr;///< Destination device pointer (host ptr on CPU fallback)
        size_t      size_bytes = 0;     ///< Number of bytes to transfer
    };

    // -----------------------------------------------------------------------
    // Transfer result
    // -----------------------------------------------------------------------
    struct TransferResult {
        bool        ok             = false;
        size_t      bytes_transferred = 0; ///< Bytes successfully moved
        std::string error_message;
    };

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------
    struct Stats {
        size_t total_transfers         = 0; ///< transfer() calls that returned OK
        size_t bytes_transferred       = 0; ///< Total bytes transferred across all OK transfers
        size_t nvlink_transfers        = 0; ///< Transfers routed through NVLink
        size_t pcie_transfers          = 0; ///< Transfers routed through PCIe P2P
        size_t cpu_fallback_transfers  = 0; ///< Transfers via CPU simulation (no HW P2P)
        size_t failed_transfers        = 0; ///< transfer() calls that returned an error
        size_t peer_access_enabled_count  = 0; ///< Successful enablePeerAccess() calls
        size_t peer_access_disabled_count = 0; ///< Successful disablePeerAccess() calls
    };

    // -----------------------------------------------------------------------
    // Singleton
    // -----------------------------------------------------------------------
    static GPUP2PTransferManager& GetInstance() {
        static GPUP2PTransferManager inst;
        return inst;
    }

    // Non-copyable.
    GPUP2PTransferManager(const GPUP2PTransferManager&)            = delete;
    GPUP2PTransferManager& operator=(const GPUP2PTransferManager&) = delete;

    GPUP2PTransferManager()  = default;
    ~GPUP2PTransferManager() = default;

    // -----------------------------------------------------------------------
    // Peer access lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Check whether direct P2P access is possible between two devices.
     *
     * Does NOT require the PEER_TO_PEER feature flag; always callable.
     *
     * On CUDA: calls `cudaDeviceCanAccessPeer`.
     * On HIP:  calls `hipDeviceCanAccessPeer`.
     * CPU fallback: returns false (no hardware P2P available).
     *
     * @param src_device  Source GPU device index.
     * @param dst_device  Destination GPU device index.
     * @param devices     Device list from `DeviceDiscovery::Enumerate()`.
     *                    Pass an empty vector to use the live driver query.
     * @return true when direct P2P access is supported.
     */
    bool canAccessPeer(int src_device, int dst_device,
                       const std::vector<DeviceInfo>& devices = {}) const;

    /**
     * @brief Enable direct peer access from src_device to dst_device.
     *
     * Requires `PEER_TO_PEER` feature flag.  Peer access must be supported
     * (see `canAccessPeer`) and must not have been enabled already for this
     * pair.  On the CPU simulation path this always returns
     * PEER_ACCESS_NOT_SUPPORTED.
     *
     * @param src_device  Source GPU device index.
     * @param dst_device  Destination GPU device index.
     * @param devices     Device list for capability checks.
     * @return Status::OK or an error code.
     */
    Status enablePeerAccess(int src_device, int dst_device,
                            const std::vector<DeviceInfo>& devices = {});

    /**
     * @brief Disable direct peer access from src_device to dst_device.
     *
     * Requires `PEER_TO_PEER` feature flag.  Peer access must currently
     * be enabled for this pair.
     *
     * @param src_device  Source GPU device index.
     * @param dst_device  Destination GPU device index.
     * @return Status::OK or an error code.
     */
    Status disablePeerAccess(int src_device, int dst_device);

    /**
     * @brief Return true when peer access has been enabled for the pair
     *        (src_device → dst_device).
     */
    bool isPeerAccessEnabled(int src_device, int dst_device) const;

    // -----------------------------------------------------------------------
    // Transfer
    // -----------------------------------------------------------------------

    /**
     * @brief Perform a direct GPU-to-GPU memory transfer.
     *
     * Requires `PEER_TO_PEER` feature flag.
     *
     * Transfer routing (best available path, in priority order):
     *  1. NVLink  — when a NVLink link is detected between the two devices.
     *  2. PCIe P2P — when peer access is enabled and NVLink is not present.
     *  3. CPU fallback — when no hardware P2P is available; the operation
     *     succeeds via a host-side `memcpy` simulation so that tests can
     *     exercise the full transfer pipeline without GPU hardware.
     *
     * On CUDA the actual transfer is `cudaMemcpyPeer(dst, dst_dev, src,
     * src_dev, size)`.  On HIP it is `hipMemcpyPeer`.
     *
     * @param request  Describes source, destination, pointers, and size.
     * @param devices  Device list for topology decisions.  Pass an empty
     *                 vector to use `DeviceDiscovery::Enumerate()`.
     * @return TransferResult with ok==true on success.
     */
    TransferResult transfer(const TransferRequest& request,
                            const std::vector<DeviceInfo>& devices = {});

    // -----------------------------------------------------------------------
    // Statistics / reset
    // -----------------------------------------------------------------------

    /**
     * @brief Return current transfer statistics.
     */
    Stats getStats() const;

    /**
     * @brief Reset statistics and remove all enabled peer-access records.
     */
    void reset();

private:
    mutable std::mutex mutex_;

    // Enabled peer-access pairs: key = pairKey(src, dst),
    // value = {src_device_index, dst_device_index} (hardware ordinals, not
    // array indices) stored at enablePeerAccess time so that disablePeerAccess
    // can issue the correct cudaSetDevice / hipSetDevice call without needing
    // the device list again.
    struct PairInfo {
        int src_device_index = 0;  ///< Hardware device ordinal for the source
        int dst_device_index = 0;  ///< Hardware device ordinal for the destination
    };
    std::unordered_map<uint32_t, PairInfo> enabled_pairs_;

    Stats stats_;

    static uint32_t pairKey(int src, int dst) noexcept {
        return (static_cast<uint32_t>(src) << 16) |
                static_cast<uint32_t>(dst & 0xFFFF);
    }
};

/**
 * @brief Human-readable name for a GPUP2PTransferManager::Status value.
 */
const char* p2pStatusName(GPUP2PTransferManager::Status s) noexcept;

} // namespace gpu
} // namespace themis
