/**
 * @file nccl_vector_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=8; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "acceleration/compute_backend.h"
#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <functional>
#include <mutex>

// Forward declarations - defined differently based on NCCL availability
#ifdef THEMIS_ENABLE_NCCL
struct ncclComm;
typedef ncclComm* ncclComm_t;
struct cudaStream_st;
typedef cudaStream_st* cudaStream_t;
#else
// Stub typedefs for CPU-only builds
typedef void* ncclComm_t;
typedef void* cudaStream_t;
#endif

namespace themis {
namespace acceleration {

/**
 * NCCL Vector Backend for Multi-GPU Communication
 * 
 * Provides collective operations and peer-to-peer transfers for multi-GPU
 * vector indexing using NVIDIA NCCL (NVIDIA Collective Communications Library).
 * 
 * When THEMIS_ENABLE_NCCL is not defined, provides stub implementations
 * that always return false, allowing CPU-only builds to compile and link.
 * 
 * Features (when NCCL is enabled):
 * - AllReduce for distributed distance computations
 * - Broadcast for index synchronization
 * - P2P transfers for direct GPU-to-GPU communication
 * - Multi-GPU top-k result merging
 * 
 * Sources:
 * - Library: NCCL (NVIDIA Collective Communications Library)
 * - Repository: https://github.com/NVIDIA/nccl
 * - License: BSD 3-Clause
 * - Documentation: https://docs.nvidia.com/deeplearning/nccl/
 * 
 * @version v2.5+
 */
class NCCLVectorBackend {
public:
    /**
     * Configuration for NCCL backend
     */
    struct Config {
        int worldSize = 1;          // Total number of GPUs
        int rank = 0;               // Current GPU rank (0 to worldSize-1)
        std::vector<int> deviceIds; // GPU device IDs to use
        bool enableP2P = true;      // Enable peer-to-peer transfers
        bool enableNVLink = true;   // Use NVLink if available
        size_t bufferSizeMB = 256;  // Communication buffer size
    };

    /**
     * Collective operation types
     */
    enum class CollectiveOp {
        ALL_REDUCE,     // Reduce and broadcast result to all GPUs
        BROADCAST,      // Broadcast from one GPU to all
        REDUCE_SCATTER, // Reduce and scatter results
        ALL_GATHER,     // Gather from all GPUs
        REDUCE          // Reduce to single GPU
    };

    /**
     * Reduction operations
     */
    enum class ReductionOp {
        SUM,
        MIN,
        MAX,
        PROD
    };

    // Constructor & Destructor
    NCCLVectorBackend();
    ~NCCLVectorBackend();

    // Initialization
    bool initialize(const Config& config);
    void shutdown();
    bool isInitialized() const;

    // Device management
    int getRank() const;
    int getWorldSize() const;
    std::vector<int> getDeviceIds() const;
    bool isP2PEnabled() const;

    // Collective operations
    /**
     * AllReduce: Reduce values across all GPUs and broadcast result
     * @param sendBuf Input buffer on this GPU
     * @param recvBuf Output buffer on this GPU
     * @param count Number of elements
     * @param op Reduction operation
     * @param stream CUDA stream for async operation
     */
    bool allReduce(const float* sendBuf, float* recvBuf, size_t count,
                   ReductionOp op, cudaStream_t stream = nullptr);

    /**
     * Broadcast: Send data from root GPU to all GPUs
     * @param buffer Buffer to broadcast (input on root, output on others)
     * @param count Number of elements
     * @param root Rank of the root GPU
     * @param stream CUDA stream for async operation
     */
    bool broadcast(float* buffer, size_t count, int root,
                   cudaStream_t stream = nullptr);

    /**
     * AllGather: Gather data from all GPUs
     * @param sendBuf Input buffer on this GPU
     * @param recvBuf Output buffer for all GPU data
     * @param sendCount Number of elements per GPU
     * @param stream CUDA stream for async operation
     */
    bool allGather(const float* sendBuf, float* recvBuf, size_t sendCount,
                   cudaStream_t stream = nullptr);

    /**
     * Reduce: Reduce values from all GPUs to root
     * @param sendBuf Input buffer on this GPU
     * @param recvBuf Output buffer (only valid on root)
     * @param count Number of elements
     * @param op Reduction operation
     * @param root Rank of the root GPU
     * @param stream CUDA stream for async operation
     */
    bool reduce(const float* sendBuf, float* recvBuf, size_t count,
                ReductionOp op, int root, cudaStream_t stream = nullptr);

    /**
     * ReduceScatter: Reduce and scatter results across GPUs
     * @param sendBuf Input buffer on this GPU
     * @param recvBuf Output buffer on this GPU
     * @param recvCount Number of elements per GPU
     * @param op Reduction operation
     * @param stream CUDA stream for async operation
     */
    bool reduceScatter(const float* sendBuf, float* recvBuf, size_t recvCount,
                       ReductionOp op, cudaStream_t stream = nullptr);

    // Peer-to-peer operations
    /**
     * P2P Send: Send data to another GPU
     * @param buffer Data to send
     * @param count Number of elements
     * @param peerRank Destination GPU rank
     * @param stream CUDA stream for async operation
     */
    bool p2pSend(const float* buffer, size_t count, int peerRank,
                 cudaStream_t stream = nullptr);

    /**
     * P2P Receive: Receive data from another GPU
     * @param buffer Buffer to receive data
     * @param count Number of elements
     * @param peerRank Source GPU rank
     * @param stream CUDA stream for async operation
     */
    bool p2pRecv(float* buffer, size_t count, int peerRank,
                 cudaStream_t stream = nullptr);

    /**
     * Enable P2P access between two GPUs
     */
    bool enableP2PAccess(int deviceId1, int deviceId2);

    /**
     * Check if P2P is available between two GPUs
     */
    bool canAccessPeer(int deviceId1, int deviceId2);

    // Synchronization
    /**
     * Synchronize all GPUs (barrier)
     */
    bool synchronize(cudaStream_t stream = nullptr);

    /**
     * Wait for all pending operations to complete
     */
    bool waitAll();

    // Multi-GPU vector operations
    /**
     * Distributed top-k merge across GPUs.
     * Each GPU has local top-k results, merged into a global top-k set.
     * 
     * @param localIndices Local result indices.
     * @param localDistances Local result distances.
     * @param localK Number of local results.
     * @param globalIndices Output buffer for global result indices (only valid on root).
     * @param globalDistances Output buffer for global result distances (only valid on root).
     * @param k Final number of results to return.
     * @param root Rank where final results are gathered.
     * @param stream CUDA stream for async operation.
     */
    bool mergeTopK(const uint32_t* localIndices, const float* localDistances,
                   size_t localK, uint32_t* globalIndices, float* globalDistances,
                   size_t k, int root, cudaStream_t stream = nullptr);

    // Statistics and monitoring
    struct Statistics {
        size_t totalBytesSent = 0;
        size_t totalBytesReceived = 0;
        size_t numCollectives = 0;
        size_t numP2PTransfers = 0;
        double avgCollectiveTimeMs = 0.0;
        double avgP2PTimeMs = 0.0;
        bool nvlinkAvailable = false;
        int numNVLinks = 0;
    };

    Statistics getStatistics() const;
    void resetStatistics();

    // Capability detection
    static bool isNCCLAvailable();
    static int getNCCLVersion();
    static std::string getNCCLVersionString();
    static bool checkNVLinkSupport(const std::vector<int>& deviceIds);

#ifndef THEMIS_ENABLE_NCCL
    // -----------------------------------------------------------------------
    // Stub-path injection — active only when THEMIS_ENABLE_NCCL is not defined.
    // -----------------------------------------------------------------------
    using AllReduceFn = std::function<bool(
        const float* send, float* recv, size_t count, ReductionOp op, void* stream)>;

    /// Inject an allReduce implementation for the non-NCCL stub path.
    /// Pass empty fn to restore fail-closed stub default.
    static void setAllReduceFn(AllReduceFn fn);
#endif // !THEMIS_ENABLE_NCCL

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace acceleration
} // namespace themis

