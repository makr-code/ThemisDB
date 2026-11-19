/**
 * @file multi_gpu_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "acceleration/compute_backend.h"
#include <memory>
#include <vector>
#include <string>
#include <cstdint>

namespace themis {
namespace acceleration {

/** @brief Execution backend selected for each multi-device shard. */
enum class MultiGPUDeviceBackend : uint8_t {
    AUTO,
    CPU,
    CUDA,
    HIP,
    VULKAN
};

struct ShardDescriptor {
    int    deviceId  = 0;  ///< Backend-native device ID; Vulkan uses the physical-device index
    size_t startIdx  = 0;  ///< Inclusive start in the flat vector array
    size_t endIdx    = 0;  ///< Exclusive end in the flat vector array
    MultiGPUDeviceBackend backend = MultiGPUDeviceBackend::CPU; ///< Backend executing this shard
    bool cpuFallback = false; ///< True when accelerator initialization fell back to CPU

    size_t numVectors() const noexcept { return endIdx - startIdx; }
};

class MultiGPUVectorBackend : public IVectorBackend {
public:
    enum class CommBackend {
        AUTO,  ///< Auto-detect: NCCL → RCCL → CPU
        NCCL,  ///< Explicit NCCL (NVIDIA Collective Communications Library)
        RCCL,  ///< Explicit RCCL (AMD Collective Communications Library)
        CPU    ///< Host-based merge, no GPU collectives
    };

    struct Config {
        /**
         * @brief Backend used to execute each shard.
         * AUTO selects CUDA, then HIP, then Vulkan; CPU is selected when no
         * accelerator is available. CPU fallback on per-device initialization
         * failures is controlled separately by allowCPUFallback.
         * @note AUTO is deterministic and selects one backend family per backend instance;
         *       it does not mix CUDA, HIP, and Vulkan devices in one instance.
         */
        MultiGPUDeviceBackend deviceBackend = MultiGPUDeviceBackend::AUTO;
        int numDevices = 2; ///< Shard count used when deviceIds is empty.

        std::vector<int> deviceIds; ///< Unique non-negative backend-native device IDs.

        int minDevices = 2; ///< Minimum detected devices required by isAvailable().

        CommBackend commBackend = CommBackend::AUTO;

        bool enableP2P = true;

        bool enableNVLink = true;

        bool enableXGMI = true;

        size_t commBufferSizeMB = 256;

        bool allowCPUFallback = true; ///< Permit per-shard CPU fallback on missing devices or runtime failure.
    };

    MultiGPUVectorBackend();
    /**
     * @brief Multi GPUVector Backend.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit MultiGPUVectorBackend(const Config& config);
    ~MultiGPUVectorBackend() override;

    // -------------------------------------------------------------------------
    // IComputeBackend interface
    // -------------------------------------------------------------------------

    const char*         name()        const noexcept override { return "MultiGPU"; }
    BackendType         type()        const noexcept override { return BackendType::MULTI_GPU; }
    bool                isAvailable() const noexcept override;
    BackendCapabilities getCapabilities() const override;
    bool                initialize() override;
    void                shutdown() override;

    // -------------------------------------------------------------------------
    // IVectorBackend interface
    // -------------------------------------------------------------------------

    /**
     * @brief Compute a distance matrix across the configured vector shards.
     * @param queries Row-major query vectors; must be non-null when numQueries is positive.
     * @param numQueries Number of query vectors.
     * @param dim Vector dimension; must be positive.
     * @param vectors Row-major corpus vectors; must be non-null when numVectors is positive.
     * @param numVectors Number of corpus vectors distributed over contiguous shard ranges.
     * @param useL2 Select L2 distance; false selects cosine distance.
     * @return Flattened `[numQueries * numVectors]` distances in original corpus order,
     *         or an empty vector if initialization, validation, or a required shard fails.
     * @note Shards execute concurrently and merge by their original corpus offsets.
     *       A failed accelerator shard is retried on CPU only when allowCPUFallback is true;
     *       fallback use is visible through shards().
     * @note Concurrent compute calls are supported; shutdown() must not race with an operation.
     */
    [[nodiscard]] std::vector<float> computeDistances(
        const float* queries,
        size_t       numQueries,
        size_t       dim,
        const float* vectors,
        size_t       numVectors,
        bool         useL2 = true) override;

    /**
     * @brief Search each vector shard and merge local top-k results.
     * @param queries Row-major query vectors; must be non-null when numQueries is positive.
     * @param numQueries Number of query vectors.
     * @param dim Vector dimension; must be positive.
     * @param vectors Row-major corpus vectors distributed over contiguous shard ranges.
     * @param numVectors Number of corpus vectors.
     * @param k Maximum neighbors per query; must be positive.
     * @param useL2 Select L2 distance; false selects cosine distance.
     * @return One distance-sorted result row per query with global corpus indices;
     *         rows are empty when initialization or a required shard fails.
     * @note Shards execute concurrently. A failed accelerator shard is retried on CPU
     *       only when allowCPUFallback is true; fallback use is visible through shards().
     * @note Concurrent calls are supported; shutdown() must not race with an operation.
     */
    [[nodiscard]] std::vector<std::vector<std::pair<uint32_t, float>>> batchKnnSearch(
        const float* queries,
        size_t       numQueries,
        size_t       dim,
        const float* vectors,
        size_t       numVectors,
        size_t       k,
        bool         useL2 = true) override;

    // -------------------------------------------------------------------------
    // Multi-GPU specific accessors
    // -------------------------------------------------------------------------

    /**
     * @brief Shards.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    const std::vector<ShardDescriptor>& shards() const noexcept;

    /**
    * @brief Active shard count, including shards currently using CPU fallback.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    int activeDeviceCount() const noexcept;

    /**
     * @brief Active Comm Backend.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    CommBackend activeCommBackend() const noexcept;

    /**
     * @brief Is Collective Ops Available.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool isCollectiveOpsAvailable() const noexcept;

    // -------------------------------------------------------------------------
    // Static helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Detect GPUCount.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    /**
     * @brief Detect accelerator devices available to AUTO selection.
     * @return Maximum available CUDA, HIP, or Vulkan compute-device count.
     */
    [[nodiscard]] static int detectGPUCount() noexcept;

    /**
     * @brief Detect devices for one selected backend.
     * @param backend Backend to enumerate; AUTO returns the first available
     *        backend count in the documented preference order, CPU returns 0.
     * @return Number of usable compute devices, or zero when unavailable.
     */
    [[nodiscard]] static int detectDeviceCount(MultiGPUDeviceBackend backend) noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace acceleration
} // namespace themis

