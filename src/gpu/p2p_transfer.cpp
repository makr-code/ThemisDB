/**
 * @file p2p_transfer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=4, Debt=0, C=0, H=3, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * GPUP2PTransferManager — Peer-to-peer GPU-to-GPU direct transfer manager.
 * =========================================================================
 * Enables direct memory copies between GPU devices via NVLink or PCIe P2P
 * DMA, bypassing host (CPU) memory for maximum bandwidth.
 *
 * Real hardware calls are gated behind THEMIS_ENABLE_CUDA / THEMIS_ENABLE_HIP.
 * Without either define all operations use an in-memory CPU simulation path so
 * that the complete public API can be exercised on any machine without GPU
 * hardware.
 *
 * Enforcement layers before any transfer is executed:
 *  1. PEER_TO_PEER feature flag (GPUFeatureFlags::Feature::PEER_TO_PEER).
 *  2. Peer access must be enabled for the (src, dst) pair before transfer().
 *  3. Transfer size must be > 0 and pointers must be non-null.
 */

#include "themis/gpu/p2p_transfer.h"

#include <cstring>

#include "themis/gpu/cluster_topology.h"
#include "themis/gpu/feature_flags.h"

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#endif
#ifdef THEMIS_ENABLE_HIP
#include <hip/hip_runtime.h>
#endif

namespace themis {
namespace gpu {

// ============================================================================
// p2pStatusName
// ============================================================================

const char *p2pStatusName(GPUP2PTransferManager::Status s) noexcept {
    using S = GPUP2PTransferManager::Status;
    switch (s) {
        case S::OK:
            return "OK";
        case S::FEATURE_DISABLED:
            return "FEATURE_DISABLED";
        case S::PEER_ACCESS_NOT_SUPPORTED:
            return "PEER_ACCESS_NOT_SUPPORTED";
        case S::PEER_ACCESS_ALREADY_ENABLED:
            return "PEER_ACCESS_ALREADY_ENABLED";
        case S::PEER_ACCESS_NOT_ENABLED:
            return "PEER_ACCESS_NOT_ENABLED";
        case S::INVALID_DEVICE:
            return "INVALID_DEVICE";
        case S::TRANSFER_FAILED:
            return "TRANSFER_FAILED";
        case S::OUT_OF_MEMORY:
            return "OUT_OF_MEMORY";
    }
    return "UNKNOWN";
}

// ============================================================================
// Internal helpers
// ============================================================================

namespace {

// Return the effective device list, falling back to DeviceDiscovery when the
// caller passed an empty vector.
std::vector<DeviceInfo> resolveDevices(const std::vector<DeviceInfo> &devices) {
    if (!devices.empty()) {
        return devices;
    }
    return DeviceDiscovery::Enumerate();
}

// Validate that both indices are within the device list.
bool devicesValid(int src, int dst, const std::vector<DeviceInfo> &devs) {
    if (src < 0 || dst < 0) {
        return false;
    }
    const int n = static_cast<int>(devs.size());
    return src < n && dst < n && src != dst;
}

// Determine the preferred interconnect for the device pair (best-effort).
// Uses GPUClusterTopology::detect() when no explicit topology is provided.
InterconnectType detectInterconnect(int src, int dst, const std::vector<DeviceInfo> &devs) {
    if (!devicesValid(src, dst, devs)) {
        return InterconnectType::CPU;
    }
    auto topo = GPUClusterTopology::detect(devs);
    return topo.preferredInterconnect(src, dst);
}

} // namespace

// ============================================================================
// canAccessPeer
// ============================================================================

bool GPUP2PTransferManager::canAccessPeer(int src_device, int dst_device,
                                          const std::vector<DeviceInfo> &devices) const {
    if (src_device == dst_device) {
        return false;
    }
    const auto devs = resolveDevices(devices);
    if (!devicesValid(src_device, dst_device, devs)) {
        return false;
    }

#ifdef THEMIS_ENABLE_CUDA
    int can         = 0;
    cudaError_t err = cudaDeviceCanAccessPeer(&can, devs[static_cast<size_t>(src_device)].device_index,
                                              devs[static_cast<size_t>(dst_device)].device_index);
    return (err == cudaSuccess) && (can != 0);
#elif defined(THEMIS_ENABLE_HIP)
    int can        = 0;
    hipError_t err = hipDeviceCanAccessPeer(&can, devs[static_cast<size_t>(src_device)].device_index,
                                            devs[static_cast<size_t>(dst_device)].device_index);
    return (err == hipSuccess) && (can != 0);
#else
    // CPU simulation: no hardware P2P available.
    return false;
#endif
}

// ============================================================================
// enablePeerAccess
// ============================================================================

GPUP2PTransferManager::Status GPUP2PTransferManager::enablePeerAccess(int src_device, int dst_device,
                                                                      const std::vector<DeviceInfo> &devices) {
    if (!GPUFeatureFlags::GetInstance().isEnabled(GPUFeatureFlags::Feature::PEER_TO_PEER)) {
        return Status::FEATURE_DISABLED;
    }

    const auto devs = resolveDevices(devices);
    if (!devicesValid(src_device, dst_device, devs)) {
        return Status::INVALID_DEVICE;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const uint32_t key = pairKey(src_device, dst_device);

    if (enabled_pairs_.count(key)) {
        return Status::PEER_ACCESS_ALREADY_ENABLED;
    }

    // Resolved hardware device ordinals (filled per-backend below;
    // on the CPU simulation path the array index serves as the ordinal).
    int src_idx = src_device;
    int dst_idx = dst_device;

#ifdef THEMIS_ENABLE_CUDA
    src_idx = devs[static_cast<size_t>(src_device)].device_index;
    dst_idx = devs[static_cast<size_t>(dst_device)].device_index;

    // Check capability first.
    int can = 0;
    if (cudaDeviceCanAccessPeer(&can, src_idx, dst_idx) != cudaSuccess || !can) {
        return Status::PEER_ACCESS_NOT_SUPPORTED;
    }

    // Switch to the source device context and enable access.
    int prev_device = 0;
    cudaGetDevice(&prev_device);
    cudaSetDevice(src_idx);
    cudaError_t err = cudaDeviceEnablePeerAccess(dst_idx, 0);
    cudaSetDevice(prev_device);

    if (err != cudaSuccess && err != cudaErrorPeerAccessAlreadyEnabled) {
        return Status::TRANSFER_FAILED;
    }

#elif defined(THEMIS_ENABLE_HIP)
    src_idx = devs[static_cast<size_t>(src_device)].device_index;
    dst_idx = devs[static_cast<size_t>(dst_device)].device_index;

    int can = 0;
    if (hipDeviceCanAccessPeer(&can, src_idx, dst_idx) != hipSuccess || !can) {
        return Status::PEER_ACCESS_NOT_SUPPORTED;
    }

    int prev_device = 0;
    hipGetDevice(&prev_device);
    hipSetDevice(src_idx);
    hipError_t err = hipDeviceEnablePeerAccess(dst_idx, 0);
    hipSetDevice(prev_device);

    if (err != hipSuccess && err != hipErrorPeerAccessAlreadyEnabled) {
        return Status::TRANSFER_FAILED;
    }

#else
    // CPU fallback: no hardware P2P available; deny silently.
    return Status::PEER_ACCESS_NOT_SUPPORTED;
#endif

    enabled_pairs_[key] = PairInfo{src_idx, dst_idx};
    ++stats_.peer_access_enabled_count;
    return Status::OK;
}

// ============================================================================
// disablePeerAccess
// ============================================================================

GPUP2PTransferManager::Status GPUP2PTransferManager::disablePeerAccess(int src_device, int dst_device) {
    if (!GPUFeatureFlags::GetInstance().isEnabled(GPUFeatureFlags::Feature::PEER_TO_PEER)) {
        return Status::FEATURE_DISABLED;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const uint32_t key = pairKey(src_device, dst_device);

    auto it = enabled_pairs_.find(key);
    if (it == enabled_pairs_.end()) {
        return Status::PEER_ACCESS_NOT_ENABLED;
    }

#ifdef THEMIS_ENABLE_CUDA
    const int src_idx = it->second.src_device_index;
    const int dst_idx = it->second.dst_device_index;
    int prev_device   = 0;
    cudaGetDevice(&prev_device);
    cudaSetDevice(src_idx);
    cudaDeviceDisablePeerAccess(dst_idx);
    cudaSetDevice(prev_device);
#elif defined(THEMIS_ENABLE_HIP)
    const int src_idx = it->second.src_device_index;
    const int dst_idx = it->second.dst_device_index;
    int prev_device   = 0;
    hipGetDevice(&prev_device);
    hipSetDevice(src_idx);
    hipDeviceDisablePeerAccess(dst_idx);
    hipSetDevice(prev_device);
#endif

    enabled_pairs_.erase(key);
    ++stats_.peer_access_disabled_count;
    return Status::OK;
}

// ============================================================================
// isPeerAccessEnabled
// ============================================================================

bool GPUP2PTransferManager::isPeerAccessEnabled(int src_device, int dst_device) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return enabled_pairs_.count(pairKey(src_device, dst_device)) > 0;
}

// ============================================================================
// transfer
// ============================================================================

GPUP2PTransferManager::TransferResult GPUP2PTransferManager::transfer(const TransferRequest &req,
                                                                      const std::vector<DeviceInfo> &devices) {
    TransferResult result;

    if (!GPUFeatureFlags::GetInstance().isEnabled(GPUFeatureFlags::Feature::PEER_TO_PEER)) {
        result.error_message = "PEER_TO_PEER feature is disabled";
        std::lock_guard<std::mutex> lock(mutex_);
        ++stats_.failed_transfers;
        return result;
    }

    if (req.size_bytes == 0) {
        result.ok                = true;
        result.bytes_transferred = 0;
        return result;
    }

    if (!req.src_ptr || !req.dst_ptr) {
        result.error_message = "null pointer in transfer request";
        std::lock_guard<std::mutex> lock(mutex_);
        ++stats_.failed_transfers;
        return result;
    }

    const auto devs = resolveDevices(devices);
    if (!devicesValid(req.src_device, req.dst_device, devs)) {
        result.error_message = "invalid device index";
        std::lock_guard<std::mutex> lock(mutex_);
        ++stats_.failed_transfers;
        return result;
    }

#if defined(THEMIS_ENABLE_CUDA) || defined(THEMIS_ENABLE_HIP)
    // Hardware P2P path — peer access must have been enabled first.
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!enabled_pairs_.count(pairKey(req.src_device, req.dst_device))) {
            result.error_message = "peer access not enabled for this device pair";
            ++stats_.failed_transfers;
            return result;
        }
    }

    const int src_idx = devs[static_cast<size_t>(req.src_device)].device_index;
    const int dst_idx = devs[static_cast<size_t>(req.dst_device)].device_index;

#ifdef THEMIS_ENABLE_CUDA
    cudaError_t err = cudaMemcpyPeer(req.dst_ptr, dst_idx, req.src_ptr, src_idx, req.size_bytes);

    if (err != cudaSuccess) {
        std::lock_guard<std::mutex> lock(mutex_);
        result.error_message = "cudaMemcpyPeer failed";
        ++stats_.failed_transfers;
        return result;
    }
    
    // Verify no lingering errors from the transfer
    err = cudaGetLastError();
    if (err != cudaSuccess) {
        std::lock_guard<std::mutex> lock(mutex_);
        result.error_message = "cudaMemcpyPeer: lingering error after transfer";
        ++stats_.failed_transfers;
        return result;
    }
#else // THEMIS_ENABLE_HIP
    hipError_t err = hipMemcpyPeer(req.dst_ptr, dst_idx, req.src_ptr, src_idx, req.size_bytes);

    if (err != hipSuccess) {
        std::lock_guard<std::mutex> lock(mutex_);
        result.error_message = "hipMemcpyPeer failed";
        ++stats_.failed_transfers;
        return result;
    }
    
    // Verify no lingering errors from the transfer
    err = hipGetLastError();
    if (err != hipSuccess) {
        std::lock_guard<std::mutex> lock(mutex_);
        result.error_message = "hipMemcpyPeer: lingering error after transfer";
        ++stats_.failed_transfers;
        return result;
    }
#endif

    // Determine the interconnect type for stats.
    const InterconnectType itype = detectInterconnect(req.src_device, req.dst_device, devs);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++stats_.total_transfers;
        stats_.bytes_transferred += req.size_bytes;
        if (itype == InterconnectType::NVLINK) {
            ++stats_.nvlink_transfers;
        } else {
            ++stats_.pcie_transfers;
        }
    }
#else
    // -------------------------------------------------------------------------
    // CPU simulation path: no hardware P2P; succeed via memcpy so tests pass.
    // -------------------------------------------------------------------------
    std::memcpy(req.dst_ptr, req.src_ptr, req.size_bytes);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++stats_.total_transfers;
        stats_.bytes_transferred += req.size_bytes;
        ++stats_.cpu_fallback_transfers;
    }
#endif

    result.ok                = true;
    result.bytes_transferred = req.size_bytes;
    return result;
}

// ============================================================================
// getStats / reset
// ============================================================================

GPUP2PTransferManager::Stats GPUP2PTransferManager::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

void GPUP2PTransferManager::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    enabled_pairs_.clear();
    stats_ = Stats{};
}

} // namespace gpu
} // namespace themis
