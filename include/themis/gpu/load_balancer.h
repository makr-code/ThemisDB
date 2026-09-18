/**
 * @file load_balancer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <mutex>
#include <string>
#include <vector>
#include "themis/gpu/device_discovery.h"
#include "themis/gpu/cluster_topology.h"

namespace themis {
namespace gpu {

/**
 * @brief Multi-GPU work load balancer.
 *
 * Distributes GPU work requests across a list of available devices using
 * configurable strategies.  Tracks per-device allocation load and skips
 * unhealthy devices automatically.
 *
 * Strategies
 * ----------
 * - ROUND_ROBIN   — select devices in sequence, skipping unhealthy ones.
 * - LEAST_LOADED  — select the device with the most free VRAM.
 * - FIRST_HEALTHY — always select the first healthy device (deterministic,
 *                   useful for single-GPU environments).
 *
 * Integration with DeviceDiscovery
 * ---------------------------------
 * `updateDevices()` refreshes the device list from the discovery layer.
 * `markDeviceFailed()` can be called when a runtime error is detected to
 * remove a device from selection until `resetDevice()` is called.
 *
 * Thread safety: all public methods are protected by an internal mutex.
 */
class GPULoadBalancer {
public:
    enum class Strategy {
        ROUND_ROBIN,     ///< Cycle through healthy devices
        LEAST_LOADED,    ///< Pick device with most free VRAM
        FIRST_HEALTHY,   ///< Always pick the first healthy device
        TOPOLOGY_AWARE,  ///< Pick device with highest total NVLink bandwidth (NVLink topology-aware scheduling)
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------
    explicit GPULoadBalancer(Strategy strategy = Strategy::LEAST_LOADED);

    /**
     * @brief Construct and initialise device list immediately.
     */
    GPULoadBalancer(Strategy strategy,
                    const std::vector<DeviceInfo>& devices);

    // -----------------------------------------------------------------------
    // Device management
    // -----------------------------------------------------------------------

    /**
     * @brief Replace the device list (e.g. after re-enumeration).
     * @param[in] devices Input parameter.
     */
    void updateDevices(const std::vector<DeviceInfo>& devices);

    /**
     * @brief Mark a device as failed/unavailable by device index.
     *
     * The device is removed from selection until resetDevice() is called.
     */
    void markDeviceFailed(int device_index, const std::string& reason = "");

    /**
     * @brief Restore a previously failed device to the eligible pool.
     * @param[in] device_index Input parameter.
     */
    void resetDevice(int device_index);

    /**
     * @brief Supply a topology snapshot for TOPOLOGY_AWARE scheduling.
     *
     * When the TOPOLOGY_AWARE strategy is active the balancer picks the
     * device with the highest sum of outgoing NVLink bandwidths according
     * to the provided topology's bandwidth_matrix.  If the topology has no
     * NVLink links (or no topology has been set) the strategy falls back to
     * LEAST_LOADED behaviour.
     *
     * Thread safety: acquires the internal mutex.
     *
     * @param topology  Topology snapshot produced by GPUClusterTopology::detect()
     *                  or assembled manually for testing.
     */
    void setTopology(const GPUClusterTopology& topology);

    // -----------------------------------------------------------------------
    // Work placement
    // -----------------------------------------------------------------------

    /**
     * @brief Select the best device for the next work item.
     *
     * @param required_vram_bytes  Minimum free VRAM needed (0 = any).
     * @return Pointer to selected DeviceInfo (stable reference into the
     *         internal list), or nullptr if no eligible device found.
     */
    const DeviceInfo* selectDevice(uint64_t required_vram_bytes = 0);

    /**
     * @brief Notify the balancer that @p bytes were allocated on
     *        @p device_index so it can update internal load tracking.
     * @param[in] device_index Input parameter.
     * @param[in] bytes Input parameter.
     */
    void recordAllocation(int device_index, uint64_t bytes);

    /**
     * @brief Notify the balancer that @p bytes were freed from
     *        @p device_index.
     * @param[in] device_index Input parameter.
     * @param[in] bytes Input parameter.
     */
    void recordDeallocation(int device_index, uint64_t bytes);

    /**
     * @brief ----------------------------------------------------------------------- Queries -----------------------------------------------------------------------
     * @return Return value.
     */
    size_t totalDevices()   const;
    /**
     * @brief TBD: Describe healthyDevices.
     * @return Return value.
     */
    size_t healthyDevices() const;
    Strategy strategy()     const { return strategy_; }

    /**
     * @brief Per-device load summary for admin/ops endpoints.
     */
    struct DeviceLoad {
        int         index              = 0;
        std::string name;
        std::string backend;
        uint64_t    free_vram_bytes    = 0;
        uint64_t    tracked_alloc_bytes = 0;  ///< Balancer's own allocation tally
        bool        is_healthy         = true;
        std::string failure_reason;
    };

    /**
     * @brief TBD: Describe getDeviceLoads.
     * @return Return value.
     */
    std::vector<DeviceLoad> getDeviceLoads() const;

private:
    Strategy strategy_;
    mutable std::mutex mutex_;

    struct DeviceEntry {
        DeviceInfo info;
        bool       balancer_healthy = true;  ///< false = markDeviceFailed()
        std::string failure_reason;
        uint64_t   tracked_alloc_bytes = 0;
    };

    std::vector<DeviceEntry> devices_;
    size_t round_robin_cursor_ = 0;

    /**
     * @brief Internal helpers — called under mutex_.
     * @param[in] required_vram Input parameter.
     * @return Pointer to the result.
     */
    DeviceEntry* selectRoundRobin(uint64_t required_vram);
    /**
     * @brief TBD: Describe selectLeastLoaded.
     * @param[in] required_vram Input parameter.
     * @return Pointer to the result.
     */
    DeviceEntry* selectLeastLoaded(uint64_t required_vram);
    /**
     * @brief TBD: Describe selectFirstHealthy.
     * @param[in] required_vram Input parameter.
     * @return Pointer to the result.
     */
    DeviceEntry* selectFirstHealthy(uint64_t required_vram);
    /**
     * @brief TBD: Describe selectTopologyAware.
     * @param[in] required_vram Input parameter.
     * @return Pointer to the result.
     */
    DeviceEntry* selectTopologyAware(uint64_t required_vram);
    /**
     * @brief TBD: Describe isEligible.
     * @param[in] e Input parameter.
     * @param[in] required_vram Input parameter.
     * @return True on success.
     */
    bool isEligible(const DeviceEntry& e, uint64_t required_vram) const;

    GPUClusterTopology topology_;
};

} // namespace gpu
} // namespace themis
