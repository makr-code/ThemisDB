/**
 * @file rccl_vector_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=7; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "acceleration/compute_backend.h"
#include <functional>
#include <memory>
#include <vector>
#include <string>
#include <cstdint>

// Forward declarations - defined differently based on RCCL availability
#ifdef THEMIS_ENABLE_RCCL
struct rcclComm;
typedef rcclComm* rcclComm_t;
struct ihipStream_t;
typedef ihipStream_t* hipStream_t;
#else
// Stub typedefs for CPU-only builds
typedef void* rcclComm_t;
typedef void* hipStream_t;
#endif

namespace themis {
namespace acceleration {

/**
 * RCCL Vector Backend for Multi-GPU Communication on AMD GPUs
 * 
 * Provides collective operations and peer-to-peer transfers for multi-GPU
 * vector indexing using AMD RCCL (ROCm Communication Collectives Library).
 * 
 * When THEMIS_ENABLE_RCCL is not defined, provides stub implementations
 * that always return false, allowing CPU-only builds to compile and link.
 * 
 * Features (when RCCL is enabled):
 * - AllReduce for distributed distance computations
 * - Broadcast for index synchronization
 * - P2P transfers for direct GPU-to-GPU communication
 * - Multi-GPU top-k result merging
 * - AMD Infinity Fabric support
 * 
 * Sources:
 * - Library: RCCL (ROCm Communication Collectives Library)
 * - Repository: https://github.com/ROCmSoftwarePlatform/rccl
 * - License: BSD 3-Clause
 * - Documentation: https://rocm.docs.amd.com/projects/rccl/
 * 
 * @version v2.5+
 */
class RCCLVectorBackend {
public:
    /**
     * Configuration for RCCL backend
     */
    struct Config {
        int worldSize = 1;          // Total number of GPUs
        int rank = 0;               // Current GPU rank (0 to worldSize-1)
        std::vector<int> deviceIds; // GPU device IDs to use
        bool enableP2P = true;      // Enable peer-to-peer transfers
        bool enableXGMI = true;     // Use AMD Infinity Fabric if available
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

    using AllReduceFn = std::function<bool(const float* sendBuf,
                                           float* recvBuf, size_t count,
                                           ReductionOp op, hipStream_t stream)>;
    // Constructor & Destructor
    RCCLVectorBackend();
    ~RCCLVectorBackend();

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
     * @param stream HIP stream for async operation
     */
    bool allReduce(const float* sendBuf, float* recvBuf, size_t count,
                   ReductionOp op, hipStream_t stream = nullptr);

    /**
     * Broadcast: Send data from root GPU to all GPUs
     * @param buffer Buffer to broadcast (input on root, output on others)
     * @param count Number of elements
     * @param root Rank of the root GPU
     * @param stream HIP stream for async operation
     */
    bool broadcast(float* buffer, size_t count, int root,
                   hipStream_t stream = nullptr);

    /**
     * AllGather: Gather data from all GPUs
     * @param sendBuf Input buffer on this GPU
     * @param recvBuf Output buffer for all GPU data
     * @param sendCount Number of elements per GPU
     * @param stream HIP stream for async operation
     */
    bool allGather(const float* sendBuf, float* recvBuf, size_t sendCount,
                   hipStream_t stream = nullptr);

    /**
     * Reduce: Reduce values from all GPUs to root
     * @param sendBuf Input buffer on this GPU
     * @param recvBuf Output buffer (only valid on root)
     * @param count Number of elements
     * @param op Reduction operation
     * @param root Rank of the root GPU
     * @param stream HIP stream for async operation
     */
    bool reduce(const float* sendBuf, float* recvBuf, size_t count,
                ReductionOp op, int root, hipStream_t stream = nullptr);

    /**
     * ReduceScatter: Reduce and scatter results across GPUs
     * @param sendBuf Input buffer on this GPU
     * @param recvBuf Output buffer on this GPU
     * @param recvCount Number of elements per GPU
     * @param op Reduction operation
     * @param stream HIP stream for async operation
     */
    bool reduceScatter(const float* sendBuf, float* recvBuf, size_t recvCount,
                       ReductionOp op, hipStream_t stream = nullptr);

    // Peer-to-peer operations
    /**
     * P2P Send: Send data to another GPU
     * @param buffer Data to send
     * @param count Number of elements
     * @param peerRank Destination GPU rank
     * @param stream HIP stream for async operation
     */
    bool p2pSend(const float* buffer, size_t count, int peerRank,
                 hipStream_t stream = nullptr);

    /**
     * P2P Receive: Receive data from another GPU
     * @param buffer Buffer to receive data
     * @param count Number of elements
     * @param peerRank Source GPU rank
     * @param stream HIP stream for async operation
     */
    bool p2pRecv(float* buffer, size_t count, int peerRank,
                 hipStream_t stream = nullptr);

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
    bool synchronize(hipStream_t stream = nullptr);

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
     * @param stream HIP stream for async operation.
     */
    bool mergeTopK(const uint32_t* localIndices, const float* localDistances,
                   size_t localK, uint32_t* globalIndices, float* globalDistances,
                   size_t k, int root, hipStream_t stream = nullptr);

    // Statistics and monitoring
    struct Statistics {
        size_t totalBytesSent = 0;
        size_t totalBytesReceived = 0;
        size_t numCollectives = 0;
        size_t numP2PTransfers = 0;
        double avgCollectiveTimeMs = 0.0;
        double avgP2PTimeMs = 0.0;
        bool xgmiAvailable = false;     // AMD Infinity Fabric
        int numXGMILinks = 0;
    };

    Statistics getStatistics() const;
    void resetStatistics();

    // Capability detection
    static bool isRCCLAvailable();
    static int getRCCLVersion();
    static std::string getRCCLVersionString();
    static bool checkXGMISupport(const std::vector<int>& deviceIds);

#ifndef THEMIS_ENABLE_RCCL
    /// Inject an allReduce implementation for the non-RCCL stub path.
    /// Pass empty fn to restore fail-closed stub default.
    static void setAllReduceFn(AllReduceFn fn);
#endif // !THEMIS_ENABLE_RCCL

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace acceleration
} // namespace themis

