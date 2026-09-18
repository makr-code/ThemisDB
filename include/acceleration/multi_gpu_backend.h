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

struct ShardDescriptor {
    int    deviceId  = 0;  ///< GPU device ID for this shard
    size_t startIdx  = 0;  ///< Inclusive start in the flat vector array
    size_t endIdx    = 0;  ///< Exclusive end in the flat vector array

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
        int numDevices = 2;

        std::vector<int> deviceIds;

        int minDevices = 2;

        CommBackend commBackend = CommBackend::AUTO;

        bool enableP2P = true;

        bool enableNVLink = true;

        bool enableXGMI = true;

        size_t commBufferSizeMB = 256;

        bool allowCPUFallback = true;
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

    std::vector<float> computeDistances(
        const float* queries,
        size_t       numQueries,
        size_t       dim,
        const float* vectors,
        size_t       numVectors,
        bool         useL2 = true) override;

    std::vector<std::vector<std::pair<uint32_t, float>>> batchKnnSearch(
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
     * @brief Active Device Count.
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
    static int detectGPUCount() noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace acceleration
} // namespace themis

