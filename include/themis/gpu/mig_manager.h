/**
 * @file mig_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "themis/gpu/device_discovery.h"

namespace themis {
namespace gpu {

/**
 * @brief MIG (Multi-Instance GPU) partition manager for NVIDIA A/H series.
 *
 * Manages the lifecycle of MIG GPU instances on NVIDIA Ampere (A100) and
 * Hopper (H100) GPUs (compute capability major >= 8).  Each physical GPU can
 * be partitioned into up to 7 independent GPU Instances (GIs); each GI
 * provides isolated VRAM and compute slices with hardware-level fault
 * isolation.
 *
 * MIG profiles
 * ------------
 * Profiles describe the fraction of the physical GPU allocated to an instance.
 * Standard profiles follow the "<slices>g.<memory>gb" naming convention:
 *   - "1g.5gb"  — 1/7 GPU, ~5 GiB VRAM  (A100 40 GB)
 *   - "2g.10gb" — 2/7 GPU, ~10 GiB VRAM
 *   - "3g.20gb" — 3/7 GPU, ~20 GiB VRAM
 *   - "4g.20gb" — 4/7 GPU, ~20 GiB VRAM
 *   - "7g.40gb" — full GPU, ~40 GiB VRAM
 *   - "1g.10gb" — 1/7 GPU, ~10 GiB VRAM (A100 80 GB)
 *   - "7g.80gb" — full GPU, ~80 GiB VRAM (A100 80 GB)
 *   - "1g.12gb" — 1/7 GPU, ~12 GiB VRAM (H100)
 *   - "7g.80gb" — full GPU, ~80 GiB VRAM (H100 SXM)
 *
 * Hardware integration
 * --------------------
 * When `THEMIS_ENABLE_CUDA` and `THEMIS_ENABLE_NVML` are defined the manager
 * calls the NVML API (`nvmlDeviceSetMIGMode`, `nvmlDeviceCreateGpuInstance`,
 * `nvmlDeviceGetGpuInstanceById`) to create and destroy real MIG partitions.
 * In the current CPU-simulation build all operations are performed against an
 * in-memory registry so that the entire public API can be tested on any
 * machine without GPU hardware.
 *
 * Feature gate
 * ------------
 * All mutating operations require `GPUFeatureFlags::Feature::MIG_MANAGER` to
 * be enabled.  Read-only queries (getInstances, deviceSupportsMIG) are always
 * permitted.  The feature is enabled by default for ENTERPRISE and
 * HYPERSCALER editions.
 *
 * Thread safety: all public methods are protected by an internal mutex.
 */
class MIGManager {
public:
    // -----------------------------------------------------------------------
    // MIG profile constants
    // -----------------------------------------------------------------------

    /// Well-known MIG profile strings.  Callers may also pass arbitrary
    /// profile strings; only profiles present in kKnownProfiles are accepted.
    static constexpr const char* kProfile_1g_5gb  = "1g.5gb";
    static constexpr const char* kProfile_2g_10gb = "2g.10gb";
    static constexpr const char* kProfile_3g_20gb = "3g.20gb";
    static constexpr const char* kProfile_4g_20gb = "4g.20gb";
    static constexpr const char* kProfile_7g_40gb = "7g.40gb";
    static constexpr const char* kProfile_1g_10gb = "1g.10gb";
    static constexpr const char* kProfile_1g_12gb = "1g.12gb";
    static constexpr const char* kProfile_7g_80gb = "7g.80gb";

    // -----------------------------------------------------------------------
    // MIG instance descriptor
    // -----------------------------------------------------------------------

    /**
     * @brief Describes a single active MIG GPU instance.
     */
    struct MIGInstance {
        std::string instance_id;      ///< Unique identifier, e.g. "dev0_gi0"
        int         device_index = 0; ///< Physical GPU device index (0-based)
        int         gi_id        = 0; ///< GPU Instance ID within the device
        std::string profile;          ///< MIG profile string, e.g. "1g.5gb"
        uint64_t    memory_bytes = 0; ///< VRAM allocated for this instance
        bool        is_active    = true; ///< false if the instance was destroyed
        std::string tenant_id;        ///< Assigned tenant; empty = unassigned
    };

    // -----------------------------------------------------------------------
    // Operation status
    // -----------------------------------------------------------------------
    enum class Status {
        OK,                        ///< Operation succeeded
        MIG_NOT_SUPPORTED,         ///< Device compute major < 8 (not A/H series)
        MIG_FEATURE_DISABLED,      ///< MIG_MANAGER feature flag is off
        INVALID_PROFILE,           ///< Unknown or unsupported MIG profile string
        DEVICE_NOT_FOUND,          ///< Requested device_index not in device list
        PARTITION_LIMIT_EXCEEDED,  ///< Maximum instances for this device reached
        INSTANCE_NOT_FOUND,        ///< instance_id does not exist in the registry
        ALREADY_ASSIGNED,          ///< Instance is already assigned to a tenant
        NOT_ASSIGNED,              ///< Instance has no tenant assignment to clear
    };

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------
    struct Stats {
        size_t total_created    = 0; ///< Total createPartition() calls that returned OK
        size_t total_destroyed  = 0; ///< Total destroyPartition() calls that returned OK
        size_t total_assigned   = 0; ///< Total assignToTenant() calls that returned OK
        size_t total_unassigned = 0; ///< Total unassignFromTenant() calls that returned OK
        size_t active_instances = 0; ///< Current number of live instances
    };

    // -----------------------------------------------------------------------
    // Singleton
    // -----------------------------------------------------------------------
    static MIGManager& GetInstance() {
        static MIGManager inst;
        return inst;
    }

    // Non-copyable.
    MIGManager(const MIGManager&)            = delete;
    MIGManager& operator=(const MIGManager&) = delete;

    // -----------------------------------------------------------------------
    // Construction / destruction
    // -----------------------------------------------------------------------
    MIGManager() = default;
    ~MIGManager() = default;

    // -----------------------------------------------------------------------
    // Partition lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Create a MIG partition on the specified device.
     *
     * Requirements:
     * - `MIG_MANAGER` feature flag must be enabled.
     * - `device_index` must refer to a healthy GPU with compute major >= 8.
     * - `profile` must be one of the known profile strings.
     * - The device must have capacity for at least one more instance
     *   (max 7 per device).
     *
     * On success `out_instance_id` is populated with a unique identifier
     * that can be used in subsequent calls.
     *
     * @param device_index    Physical GPU device index.
     * @param profile         MIG profile string (e.g. "1g.5gb").
     * @param out_instance_id Receives the new instance ID on success.
     * @return Status::OK or an error code.
     */
    Status createPartition(int device_index,
                           const std::string& profile,
                           std::string& out_instance_id);

    /**
     * @brief Overload that accepts an explicit device list instead of calling
     * DeviceDiscovery::Enumerate().  Useful for unit testing without GPU
     * hardware.
     */
    Status createPartition(int device_index,
                           const std::string& profile,
                           std::string& out_instance_id,
                           const std::vector<DeviceInfo>& devices);

    /**
     * @brief Destroy a MIG partition and release its resources.
     *
     * The partition must not be assigned to a tenant at the time of
     * destruction (call `unassignFromTenant` first).
     *
     * @param instance_id  ID returned by a prior createPartition() call.
     * @return Status::OK or an error code.
     */
    Status destroyPartition(const std::string& instance_id);

    // -----------------------------------------------------------------------
    // Tenant assignment
    // -----------------------------------------------------------------------

    /**
     * @brief Assign a MIG instance to a tenant.
     *
     * Tenants are identified by an arbitrary non-empty string.  An instance
     * can be assigned to at most one tenant at a time.
     *
     * @param instance_id  MIG instance to assign.
     * @param tenant_id    Non-empty tenant identifier.
     * @return Status::OK, INSTANCE_NOT_FOUND, or ALREADY_ASSIGNED.
     */
    Status assignToTenant(const std::string& instance_id,
                          const std::string& tenant_id);

    /**
     * @brief Remove the tenant assignment from a MIG instance.
     *
     * @param instance_id  MIG instance to unassign.
     * @return Status::OK, INSTANCE_NOT_FOUND, or NOT_ASSIGNED.
     */
    Status unassignFromTenant(const std::string& instance_id);

    // -----------------------------------------------------------------------
    // Queries
    // -----------------------------------------------------------------------

    /**
     * @brief Return all active MIG instances across all devices.
     */
    std::vector<MIGInstance> getInstances() const;

    /**
     * @brief Return all active MIG instances on a specific device.
     */
    std::vector<MIGInstance> getInstancesForDevice(int device_index) const;

    /**
     * @brief Return all MIG instances currently assigned to a tenant.
     */
    std::vector<MIGInstance> getInstancesForTenant(
        const std::string& tenant_id) const;

    /**
     * @brief Return the MIG instance descriptor for a given instance ID.
     *
     * Returns false (and leaves @p out unchanged) when the instance does not
     * exist.
     */
    bool getInstance(const std::string& instance_id,
                     MIGInstance& out) const;

    // -----------------------------------------------------------------------
    // Capability queries
    // -----------------------------------------------------------------------

    /**
     * @brief Return true when @p device supports MIG (compute major >= 8).
     *
     * Does NOT check whether MIG mode is currently enabled in the driver.
     */
    static bool deviceSupportsMIG(const DeviceInfo& device) noexcept;

    /**
     * @brief Return true when @p profile is a recognised MIG profile string.
     */
    static bool isKnownProfile(const std::string& profile) noexcept;

    /**
     * @brief Return the VRAM size in bytes associated with @p profile.
     *
     * Returns 0 for unknown profiles.
     */
    static uint64_t profileMemoryBytes(const std::string& profile) noexcept;

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------
    Stats getStats() const;

    /**
     * @brief Reset statistics and remove all instances (for testing).
     */
    void reset();

    /**
     * @brief Build a deterministic instance ID from device index and gi_id.
     *
     * Public so that tests can construct expected IDs without duplicating the
     * naming logic.
     */
    static std::string makeInstanceId(int device_index, int gi_id);

private:
    mutable std::mutex mutex_;

    // Registry: instance_id -> MIGInstance.
    std::unordered_map<std::string, MIGInstance> instances_;

    // Per-device instance count: device_index -> count.
    std::unordered_map<int, int> device_instance_count_;

    // Next GI ID per device: device_index -> next gi_id.
    std::unordered_map<int, int> next_gi_id_;

    // Counters.
    size_t stat_created_    = 0;
    size_t stat_destroyed_  = 0;
    size_t stat_assigned_   = 0;
    size_t stat_unassigned_ = 0;

    static constexpr int kMaxInstancesPerDevice = 7;
};

/**
 * @brief Human-readable name for a MIGManager::Status value.
 */
const char* migStatusName(MIGManager::Status s) noexcept;

} // namespace gpu
} // namespace themis
