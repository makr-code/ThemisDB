/**
 * @file rccl_vector_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class RCCLVectorBackend {
public:
    struct Config {
        int worldSize = 1;          // Total number of GPUs
        int rank = 0;               // Current GPU rank (0 to worldSize-1)
        std::vector<int> deviceIds; // GPU device IDs to use
        bool enableP2P = true;      // Enable peer-to-peer transfers
        bool enableXGMI = true;     // Use AMD Infinity Fabric if available
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

    using AllReduceFn = std::function<bool(const float* sendBuf,
                                           float* recvBuf, size_t count,
                                           ReductionOp op, hipStream_t stream)>;
    // Constructor & Destructor
    RCCLVectorBackend();
    ~RCCLVectorBackend();

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
                   ReductionOp op, hipStream_t stream = nullptr);

    bool broadcast(float* buffer, size_t count, int root,
                   hipStream_t stream = nullptr);

    bool allGather(const float* sendBuf, float* recvBuf, size_t sendCount,
                   hipStream_t stream = nullptr);

    bool reduce(const float* sendBuf, float* recvBuf, size_t count,
                ReductionOp op, int root, hipStream_t stream = nullptr);

    bool reduceScatter(const float* sendBuf, float* recvBuf, size_t recvCount,
                       ReductionOp op, hipStream_t stream = nullptr);

    // Peer-to-peer operations
    bool p2pSend(const float* buffer, size_t count, int peerRank,
                 hipStream_t stream = nullptr);

    bool p2pRecv(float* buffer, size_t count, int peerRank,
                 hipStream_t stream = nullptr);

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
    bool synchronize(hipStream_t stream = nullptr);

    /**
     * @brief Wait All.
     * @return True when the operation succeeds.
     */
    bool waitAll();

    // Multi-GPU vector operations
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
     * @brief Is RCCLAvailable.
     * @return True when the operation succeeds.
     */
    static bool isRCCLAvailable();
    /**
     * @brief Get RCCLVersion.
     * @return Return value.
     */
    static int getRCCLVersion();
    /**
     * @brief Get RCCLVersion String.
     * @return Return value.
     */
    static std::string getRCCLVersionString();
    /**
     * @brief Check XGMISupport.
     * @param[in] deviceIds Input parameter.
     * @return True when the operation succeeds.
     */
    static bool checkXGMISupport(const std::vector<int>& deviceIds);

#ifndef THEMIS_ENABLE_RCCL
    /**
     * @brief Set All Reduce Fn.
     * @param[in] fn Input parameter.
     */
    static void setAllReduceFn(AllReduceFn fn);
#endif // !THEMIS_ENABLE_RCCL

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace acceleration
} // namespace themis

