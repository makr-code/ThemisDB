/**
 * @file nccl_vector_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
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

class NCCLVectorBackend {
public:
    struct Config {
        int worldSize = 1;          // Total number of GPUs
        int rank = 0;               // Current GPU rank (0 to worldSize-1)
        std::vector<int> deviceIds; // GPU device IDs to use
        bool enableP2P = true;      // Enable peer-to-peer transfers
        bool enableNVLink = true;   // Use NVLink if available
        size_t bufferSizeMB = 256;  // Communication buffer size
    };

    enum class CollectiveOp {
        ALL_REDUCE,     // Reduce and broadcast result to all GPUs
        BROADCAST,      // Broadcast from one GPU to all
        REDUCE_SCATTER, // Reduce and scatter results
        ALL_GATHER,     // Gather from all GPUs
        REDUCE          // Reduce to single GPU
    };

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
    /**
     * @brief Initialize.
     * @param[in] config Input parameter.
     * @return True when the operation succeeds.
     */
    bool initialize(const Config& config);
    /**
     * @brief Shutdown.
     */
    void shutdown();
    /**
     * @brief Is Initialized.
     * @return True when the operation succeeds.
     */
    bool isInitialized() const;

    // Device management
    /**
     * @brief Get Rank.
     * @return Return value.
     */
    int getRank() const;
    /**
     * @brief Get World Size.
     * @return Return value.
     */
    int getWorldSize() const;
    /**
     * @brief Get Device Ids.
     * @return Return value.
     */
    std::vector<int> getDeviceIds() const;
    /**
     * @brief Is P2 PEnabled.
     * @return True when the operation succeeds.
     */
    bool isP2PEnabled() const;

    // Collective operations
    bool allReduce(const float* sendBuf, float* recvBuf, size_t count,
                   ReductionOp op, cudaStream_t stream = nullptr);

    bool broadcast(float* buffer, size_t count, int root,
                   cudaStream_t stream = nullptr);

    bool allGather(const float* sendBuf, float* recvBuf, size_t sendCount,
                   cudaStream_t stream = nullptr);

    bool reduce(const float* sendBuf, float* recvBuf, size_t count,
                ReductionOp op, int root, cudaStream_t stream = nullptr);

    bool reduceScatter(const float* sendBuf, float* recvBuf, size_t recvCount,
                       ReductionOp op, cudaStream_t stream = nullptr);

    // Peer-to-peer operations
    bool p2pSend(const float* buffer, size_t count, int peerRank,
                 cudaStream_t stream = nullptr);

    bool p2pRecv(float* buffer, size_t count, int peerRank,
                 cudaStream_t stream = nullptr);

    /**
     * @brief Enable P2 PAccess.
     * @param[in] deviceId1 Input parameter.
     * @param[in] deviceId2 Input parameter.
     * @return True when the operation succeeds.
     */
    bool enableP2PAccess(int deviceId1, int deviceId2);

    /**
     * @brief Can Access Peer.
     * @param[in] deviceId1 Input parameter.
     * @param[in] deviceId2 Input parameter.
     * @return True when the operation succeeds.
     */
    bool canAccessPeer(int deviceId1, int deviceId2);

    // Synchronization
    bool synchronize(cudaStream_t stream = nullptr);

    /**
     * @brief Wait All.
     * @return True when the operation succeeds.
     */
    bool waitAll();

    // Multi-GPU vector operations
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

    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    Statistics getStatistics() const;
    /**
     * @brief Reset Statistics.
     */
    void resetStatistics();

    // Capability detection
    /**
     * @brief Is NCCLAvailable.
     * @return True when the operation succeeds.
     */
    static bool isNCCLAvailable();
    /**
     * @brief Get NCCLVersion.
     * @return Return value.
     */
    static int getNCCLVersion();
    /**
     * @brief Get NCCLVersion String.
     * @return Return value.
     */
    static std::string getNCCLVersionString();
    /**
     * @brief Check NVLink Support.
     * @param[in] deviceIds Input parameter.
     * @return True when the operation succeeds.
     */
    static bool checkNVLinkSupport(const std::vector<int>& deviceIds);

#ifndef THEMIS_ENABLE_NCCL
    // -----------------------------------------------------------------------
    // Stub-path injection — active only when THEMIS_ENABLE_NCCL is not defined.
    // -----------------------------------------------------------------------
    using AllReduceFn = std::function<bool(
        const float* send, float* recv, size_t count, ReductionOp op, void* stream)>;

    /**
     * @brief Set All Reduce Fn.
     * @param[in] fn Input parameter.
     */
    static void setAllReduceFn(AllReduceFn fn);
#endif // !THEMIS_ENABLE_NCCL

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace acceleration
} // namespace themis

