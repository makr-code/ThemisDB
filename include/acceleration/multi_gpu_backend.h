/**
 * @file multi_gpu_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * ShardDescriptor — metadata for a contiguous slice of vectors assigned to one GPU.
 *
 * Range-based sharding: the flat vector array [0, numVectors) is partitioned
 * into N contiguous ranges; shard i owns [startIdx, endIdx).
 */
struct ShardDescriptor {
    int    deviceId  = 0;  ///< GPU device ID for this shard
    size_t startIdx  = 0;  ///< Inclusive start in the flat vector array
    size_t endIdx    = 0;  ///< Exclusive end in the flat vector array

    /// Number of vectors in this shard.
    size_t numVectors() const noexcept { return endIdx - startIdx; }
};

/**
 * MultiGPUVectorBackend — distributes vector operations across N GPUs via
 * range-based sharding and optional NCCL/RCCL collective operations.
 *
 * **Sharding strategy**: the flat vector array is divided into N contiguous
 * ranges (one per device).  Each shard is processed by a per-device
 * sub-backend.  Query fan-out and top-k merge happen on the host.
 *
 * **Communication backends**:
 * - When THEMIS_ENABLE_NCCL is defined and NCCL ≥ 2.x is present, cross-GPU
 *   transfers use ncclGroupStart / ncclGroupEnd collective calls.
 * - When THEMIS_ENABLE_RCCL is defined and RCCL is present, the AMD path is
 *   used instead.
 * - Otherwise, host-based (CPU) merge is used transparently.
 *
 * **Graceful degradation**: if NCCL/RCCL init fails, or if the requested
 * device count is greater than the number of visible GPUs, the backend
 * reduces its shard count and logs a warning.  If allowCPUFallback is true,
 * sub-backends that fail to initialise are replaced by the CPU backend.
 *
 * **Availability**: isAvailable() returns true only when
 * detectGPUCount() >= Config::minDevices.  BackendRegistry::autoDetect()
 * conditionally registers this backend so it is offered by getBestVectorBackend()
 * ahead of single-GPU CUDA in the fallback chain.
 *
 * @see FUTURE_ENHANCEMENTS.md — "Multi-GPU Sharding for Large Embedding Datasets"
 * @version v1.9.0 (Target)
 */
class MultiGPUVectorBackend : public IVectorBackend {
public:
    /**
     * Communication backend selection for collective merge operations.
     */
    enum class CommBackend {
        AUTO,  ///< Auto-detect: NCCL → RCCL → CPU
        NCCL,  ///< Explicit NCCL (NVIDIA Collective Communications Library)
        RCCL,  ///< Explicit RCCL (AMD Collective Communications Library)
        CPU    ///< Host-based merge, no GPU collectives
    };

    /**
     * Configuration for the multi-GPU sharding backend.
     */
    struct Config {
        /// Number of GPU shards desired.  Clamped to detectGPUCount() at runtime.
        int numDevices = 2;

        /// Explicit GPU device IDs.  When empty, IDs 0 .. numDevices-1 are used.
        std::vector<int> deviceIds;

        /// Minimum GPUs required for isAvailable() to return true.
        int minDevices = 2;

        /// Communication backend for collective merge operations.
        CommBackend commBackend = CommBackend::AUTO;

        /// Enable peer-to-peer GPU transfers when available.
        bool enableP2P = true;

        /// Use NVLink if detected (NCCL path).
        bool enableNVLink = true;

        /// Use AMD Infinity Fabric / XGMI if detected (RCCL path).
        bool enableXGMI = true;

        /// Communication buffer size in megabytes.
        size_t commBufferSizeMB = 256;

        /// Fall back to CPU sub-backend when a GPU device cannot be initialised.
        bool allowCPUFallback = true;
    };

    MultiGPUVectorBackend();
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
     * Compute pairwise distances from each query to each vector.
     *
     * The returned flat array has layout [query][vector], i.e.
     * result[q * numVectors + v] is the distance from query q to vector v.
     *
     * The computation is sharded: each shard processes its contiguous slice
     * of vectors; results are concatenated and re-indexed to global positions.
     */
    std::vector<float> computeDistances(
        const float* queries,
        size_t       numQueries,
        size_t       dim,
        const float* vectors,
        size_t       numVectors,
        bool         useL2 = true) override;

    /**
     * Batched k-nearest-neighbour search across all shards.
     *
     * Each query is broadcast to every shard; per-shard top-k results are
     * collected, shard-local indices are remapped to global vector positions,
     * and a final host-side merge produces the global top-k.
     *
     * Returns one vector<pair<globalIndex, distance>> per query.
     */
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

    /// Shard descriptors built at initialise() time.
    const std::vector<ShardDescriptor>& shards() const noexcept;

    /// Number of active device shards.
    int activeDeviceCount() const noexcept;

    /// Active communication backend selected at initialise() time.
    CommBackend activeCommBackend() const noexcept;

    /// True when NCCL or RCCL collectives are in use.
    bool isCollectiveOpsAvailable() const noexcept;

    // -------------------------------------------------------------------------
    // Static helpers
    // -------------------------------------------------------------------------

    /**
     * Returns the number of CUDA/HIP GPUs visible to the process.
     * Returns 0 on non-GPU builds or when no GPU is present.
     */
    static int detectGPUCount() noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace acceleration
} // namespace themis

