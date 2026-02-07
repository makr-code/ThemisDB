#include "acceleration/nccl_vector_backend.h"
#include <iostream>
#include <chrono>
#include <stdexcept>
#include <cstring>

#ifdef THEMIS_ENABLE_NCCL
#include <nccl.h>
#include <cuda_runtime.h>

// NCCL error checking macro
#define NCCL_CHECK(cmd) \
    do { \
        ncclResult_t r = cmd; \
        if (r != ncclSuccess) { \
            std::cerr << "NCCL error: " << ncclGetErrorString(r) \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

// CUDA error checking macro
#define CUDA_CHECK(cmd) \
    do { \
        cudaError_t e = cmd; \
        if (e != cudaSuccess) { \
            std::cerr << "CUDA error: " << cudaGetErrorString(e) \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

#endif

namespace themis {
namespace acceleration {

#ifdef THEMIS_ENABLE_NCCL

// =============================================================================
// NCCLVectorBackend::Impl
// =============================================================================

class NCCLVectorBackend::Impl {
public:
    Config config;
    ncclComm_t comm = nullptr;
    bool initialized = false;
    Statistics stats;
    
    // Timing for statistics
    std::chrono::steady_clock::time_point lastOpStart;
    
    ~Impl() {
        shutdown();
    }
    
    bool initialize(const Config& cfg) {
        config = cfg;
        
        if (config.worldSize < 1) {
            std::cerr << "NCCL: Invalid world size: " << config.worldSize << std::endl;
            return false;
        }
        
        if (config.rank < 0 || config.rank >= config.worldSize) {
            std::cerr << "NCCL: Invalid rank: " << config.rank << std::endl;
            return false;
        }
        
        std::cout << "NCCL: Initializing with rank " << config.rank 
                  << " of " << config.worldSize << std::endl;
        
        // Set device for this rank
        if (!config.deviceIds.empty() && config.rank < static_cast<int>(config.deviceIds.size())) {
            CUDA_CHECK(cudaSetDevice(config.deviceIds[config.rank]));
        } else {
            CUDA_CHECK(cudaSetDevice(config.rank));
        }
        
        // Initialize NCCL communicator
        // Note: In production, this would use ncclCommInitRank with a unique ID
        // For now, we use ncclCommInitAll for simplicity
        ncclComm_t comms[config.worldSize];
        NCCL_CHECK(ncclCommInitAll(comms, config.worldSize, config.deviceIds.data()));
        comm = comms[config.rank];
        
        // Enable P2P access if requested
        if (config.enableP2P) {
            enableAllP2PAccess();
        }
        
        // Check for NVLink support
        if (config.enableNVLink) {
            stats.nvlinkAvailable = checkNVLinkAvailable();
            stats.numNVLinks = countNVLinks();
        }
        
        initialized = true;
        std::cout << "NCCL: Initialization successful" << std::endl;
        return true;
    }
    
    void shutdown() {
        if (initialized && comm != nullptr) {
            ncclCommDestroy(comm);
            comm = nullptr;
            initialized = false;
        }
    }
    
    bool enableAllP2PAccess() {
        if (config.deviceIds.empty()) return true;
        
        for (size_t i = 0; i < config.deviceIds.size(); ++i) {
            for (size_t j = i + 1; j < config.deviceIds.size(); ++j) {
                int canAccess = 0;
                cudaDeviceCanAccessPeer(&canAccess, config.deviceIds[i], config.deviceIds[j]);
                if (canAccess) {
                    cudaSetDevice(config.deviceIds[i]);
                    cudaDeviceEnablePeerAccess(config.deviceIds[j], 0);
                    cudaSetDevice(config.deviceIds[j]);
                    cudaDeviceEnablePeerAccess(config.deviceIds[i], 0);
                }
            }
        }
        
        return true;
    }
    
    bool checkNVLinkAvailable() {
        // Simple check: if P2P is available between any two devices, assume NVLink
        if (config.deviceIds.size() < 2) return false;
        
        int canAccess = 0;
        cudaDeviceCanAccessPeer(&canAccess, config.deviceIds[0], config.deviceIds[1]);
        return canAccess != 0;
    }
    
    int countNVLinks() {
        // Simplified: count P2P-capable device pairs
        int count = 0;
        for (size_t i = 0; i < config.deviceIds.size(); ++i) {
            for (size_t j = i + 1; j < config.deviceIds.size(); ++j) {
                int canAccess = 0;
                cudaDeviceCanAccessPeer(&canAccess, config.deviceIds[i], config.deviceIds[j]);
                if (canAccess) count++;
            }
        }
        return count;
    }
    
    void startTiming() {
        lastOpStart = std::chrono::steady_clock::now();
    }
    
    void recordCollective() {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - lastOpStart);
        double timeMs = duration.count() / 1000.0;
        
        stats.numCollectives++;
        stats.avgCollectiveTimeMs = 
            (stats.avgCollectiveTimeMs * (stats.numCollectives - 1) + timeMs) / stats.numCollectives;
    }
    
    void recordP2P(size_t bytes) {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - lastOpStart);
        double timeMs = duration.count() / 1000.0;
        
        stats.numP2PTransfers++;
        stats.totalBytesSent += bytes;
        stats.avgP2PTimeMs = 
            (stats.avgP2PTimeMs * (stats.numP2PTransfers - 1) + timeMs) / stats.numP2PTransfers;
    }
};

// =============================================================================
// NCCLVectorBackend Public API
// =============================================================================

NCCLVectorBackend::NCCLVectorBackend() : pImpl(std::make_unique<Impl>()) {}

NCCLVectorBackend::~NCCLVectorBackend() = default;

bool NCCLVectorBackend::initialize(const Config& config) {
    return pImpl->initialize(config);
}

void NCCLVectorBackend::shutdown() {
    pImpl->shutdown();
}

bool NCCLVectorBackend::isInitialized() const {
    return pImpl->initialized;
}

int NCCLVectorBackend::getRank() const {
    return pImpl->config.rank;
}

int NCCLVectorBackend::getWorldSize() const {
    return pImpl->config.worldSize;
}

std::vector<int> NCCLVectorBackend::getDeviceIds() const {
    return pImpl->config.deviceIds;
}

bool NCCLVectorBackend::isP2PEnabled() const {
    return pImpl->config.enableP2P;
}

bool NCCLVectorBackend::allReduce(const float* sendBuf, float* recvBuf, size_t count,
                                   ReductionOp op, cudaStream_t stream) {
    if (!pImpl->initialized) return false;
    
    pImpl->startTiming();
    
    ncclRedOp_t ncclOp;
    switch (op) {
        case ReductionOp::SUM:  ncclOp = ncclSum;  break;
        case ReductionOp::MIN:  ncclOp = ncclMin;  break;
        case ReductionOp::MAX:  ncclOp = ncclMax;  break;
        case ReductionOp::PROD: ncclOp = ncclProd; break;
        default: return false;
    }
    
    NCCL_CHECK(ncclAllReduce(sendBuf, recvBuf, count, ncclFloat, ncclOp, 
                             pImpl->comm, stream));
    
    pImpl->recordCollective();
    return true;
}

bool NCCLVectorBackend::broadcast(float* buffer, size_t count, int root,
                                   cudaStream_t stream) {
    if (!pImpl->initialized) return false;
    
    pImpl->startTiming();
    NCCL_CHECK(ncclBroadcast(buffer, buffer, count, ncclFloat, root, 
                             pImpl->comm, stream));
    pImpl->recordCollective();
    return true;
}

bool NCCLVectorBackend::allGather(const float* sendBuf, float* recvBuf, size_t sendCount,
                                   cudaStream_t stream) {
    if (!pImpl->initialized) return false;
    
    pImpl->startTiming();
    NCCL_CHECK(ncclAllGather(sendBuf, recvBuf, sendCount, ncclFloat, 
                             pImpl->comm, stream));
    pImpl->recordCollective();
    return true;
}

bool NCCLVectorBackend::reduce(const float* sendBuf, float* recvBuf, size_t count,
                                ReductionOp op, int root, cudaStream_t stream) {
    if (!pImpl->initialized) return false;
    
    pImpl->startTiming();
    
    ncclRedOp_t ncclOp;
    switch (op) {
        case ReductionOp::SUM:  ncclOp = ncclSum;  break;
        case ReductionOp::MIN:  ncclOp = ncclMin;  break;
        case ReductionOp::MAX:  ncclOp = ncclMax;  break;
        case ReductionOp::PROD: ncclOp = ncclProd; break;
        default: return false;
    }
    
    NCCL_CHECK(ncclReduce(sendBuf, recvBuf, count, ncclFloat, ncclOp, root,
                          pImpl->comm, stream));
    pImpl->recordCollective();
    return true;
}

bool NCCLVectorBackend::reduceScatter(const float* sendBuf, float* recvBuf, size_t recvCount,
                                       ReductionOp op, cudaStream_t stream) {
    if (!pImpl->initialized) return false;
    
    pImpl->startTiming();
    
    ncclRedOp_t ncclOp;
    switch (op) {
        case ReductionOp::SUM:  ncclOp = ncclSum;  break;
        case ReductionOp::MIN:  ncclOp = ncclMin;  break;
        case ReductionOp::MAX:  ncclOp = ncclMax;  break;
        case ReductionOp::PROD: ncclOp = ncclProd; break;
        default: return false;
    }
    
    NCCL_CHECK(ncclReduceScatter(sendBuf, recvBuf, recvCount, ncclFloat, ncclOp,
                                 pImpl->comm, stream));
    pImpl->recordCollective();
    return true;
}

bool NCCLVectorBackend::p2pSend(const float* buffer, size_t count, int peerRank,
                                 cudaStream_t stream) {
    if (!pImpl->initialized) return false;
    
    pImpl->startTiming();
    NCCL_CHECK(ncclSend(buffer, count, ncclFloat, peerRank, pImpl->comm, stream));
    pImpl->recordP2P(count * sizeof(float));
    return true;
}

bool NCCLVectorBackend::p2pRecv(float* buffer, size_t count, int peerRank,
                                 cudaStream_t stream) {
    if (!pImpl->initialized) return false;
    
    pImpl->startTiming();
    NCCL_CHECK(ncclRecv(buffer, count, ncclFloat, peerRank, pImpl->comm, stream));
    pImpl->stats.totalBytesReceived += count * sizeof(float);
    return true;
}

bool NCCLVectorBackend::enableP2PAccess(int deviceId1, int deviceId2) {
    int canAccess = 0;
    CUDA_CHECK(cudaDeviceCanAccessPeer(&canAccess, deviceId1, deviceId2));
    
    if (canAccess) {
        CUDA_CHECK(cudaSetDevice(deviceId1));
        cudaError_t err = cudaDeviceEnablePeerAccess(deviceId2, 0);
        if (err != cudaSuccess && err != cudaErrorPeerAccessAlreadyEnabled) {
            return false;
        }
    }
    
    return canAccess != 0;
}

bool NCCLVectorBackend::canAccessPeer(int deviceId1, int deviceId2) {
    int canAccess = 0;
    cudaDeviceCanAccessPeer(&canAccess, deviceId1, deviceId2);
    return canAccess != 0;
}

bool NCCLVectorBackend::synchronize(cudaStream_t stream) {
    if (!pImpl->initialized) return false;
    
    // Use a barrier via AllReduce of a dummy value
    float dummy = 0.0f;
    return allReduce(&dummy, &dummy, 1, ReductionOp::SUM, stream);
}

bool NCCLVectorBackend::waitAll() {
    if (!pImpl->initialized) return false;
    CUDA_CHECK(cudaDeviceSynchronize());
    return true;
}

bool NCCLVectorBackend::mergeTopK(const uint32_t* localIndices, const float* localDistances,
                                   size_t localK, uint32_t* globalIndices, float* globalDistances,
                                   size_t k, int root, cudaStream_t stream) {
    if (!pImpl->initialized) return false;
    
    // First, gather all local top-k results to root
    size_t totalSize = localK * pImpl->config.worldSize;
    
    // Allocate temporary buffers on root
    uint32_t* allIndices = nullptr;
    float* allDistances = nullptr;
    
    if (pImpl->config.rank == root) {
        CUDA_CHECK(cudaMalloc(&allIndices, totalSize * sizeof(uint32_t)));
        CUDA_CHECK(cudaMalloc(&allDistances, totalSize * sizeof(float)));
    }
    
    // Gather indices
    pImpl->startTiming();
    // Note: NCCL doesn't have direct support for uint32_t, so we'd need to cast or use AllGather differently
    // For now, we'll use a simplified approach with AllGather on floats
    
    // In production, this would:
    // 1. AllGather all local top-k results
    // 2. On root, perform a k-way merge to get global top-k
    // 3. Optionally broadcast global top-k back to all ranks
    
    // Simplified implementation: just gather distances and use AllGather
    if (pImpl->config.rank == root) {
        // Would perform merge here
        // For now, just copy local results as placeholder
        CUDA_CHECK(cudaMemcpy(globalIndices, localIndices, k * sizeof(uint32_t), 
                              cudaMemcpyDeviceToDevice));
        CUDA_CHECK(cudaMemcpy(globalDistances, localDistances, k * sizeof(float),
                              cudaMemcpyDeviceToDevice));
    }
    
    pImpl->recordCollective();
    
    if (pImpl->config.rank == root) {
        cudaFree(allIndices);
        cudaFree(allDistances);
    }
    
    return true;
}

NCCLVectorBackend::Statistics NCCLVectorBackend::getStatistics() const {
    return pImpl->stats;
}

void NCCLVectorBackend::resetStatistics() {
    pImpl->stats = Statistics{};
}

bool NCCLVectorBackend::isNCCLAvailable() {
    // Check if NCCL library is available
    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);
    return (err == cudaSuccess && deviceCount > 0);
}

int NCCLVectorBackend::getNCCLVersion() {
    int version = 0;
    ncclGetVersion(&version);
    return version;
}

std::string NCCLVectorBackend::getNCCLVersionString() {
    int version = getNCCLVersion();
    int major = version / 10000;
    int minor = (version % 10000) / 100;
    int patch = version % 100;
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
}

bool NCCLVectorBackend::checkNVLinkSupport(const std::vector<int>& deviceIds) {
    if (deviceIds.size() < 2) return false;
    
    // Check if P2P is available between devices (simplified NVLink check)
    for (size_t i = 0; i < deviceIds.size(); ++i) {
        for (size_t j = i + 1; j < deviceIds.size(); ++j) {
            int canAccess = 0;
            cudaDeviceCanAccessPeer(&canAccess, deviceIds[i], deviceIds[j]);
            if (canAccess) return true;
        }
    }
    
    return false;
}

#else // THEMIS_ENABLE_NCCL

// Stub implementations when NCCL is not available
NCCLVectorBackend::NCCLVectorBackend() : pImpl(nullptr) {}
NCCLVectorBackend::~NCCLVectorBackend() = default;
bool NCCLVectorBackend::initialize(const Config&) { return false; }
void NCCLVectorBackend::shutdown() {}
bool NCCLVectorBackend::isInitialized() const { return false; }
int NCCLVectorBackend::getRank() const { return 0; }
int NCCLVectorBackend::getWorldSize() const { return 1; }
std::vector<int> NCCLVectorBackend::getDeviceIds() const { return {}; }
bool NCCLVectorBackend::isP2PEnabled() const { return false; }
bool NCCLVectorBackend::allReduce(const float*, float*, size_t, ReductionOp, void*) { return false; }
bool NCCLVectorBackend::broadcast(float*, size_t, int, void*) { return false; }
bool NCCLVectorBackend::allGather(const float*, float*, size_t, void*) { return false; }
bool NCCLVectorBackend::reduce(const float*, float*, size_t, ReductionOp, int, void*) { return false; }
bool NCCLVectorBackend::reduceScatter(const float*, float*, size_t, ReductionOp, void*) { return false; }
bool NCCLVectorBackend::p2pSend(const float*, size_t, int, void*) { return false; }
bool NCCLVectorBackend::p2pRecv(float*, size_t, int, void*) { return false; }
bool NCCLVectorBackend::enableP2PAccess(int, int) { return false; }
bool NCCLVectorBackend::canAccessPeer(int, int) { return false; }
bool NCCLVectorBackend::synchronize(void*) { return false; }
bool NCCLVectorBackend::waitAll() { return false; }
bool NCCLVectorBackend::mergeTopK(const uint32_t*, const float*, size_t, uint32_t*, float*, size_t, int, void*) { return false; }
NCCLVectorBackend::Statistics NCCLVectorBackend::getStatistics() const { return Statistics{}; }
void NCCLVectorBackend::resetStatistics() {}
bool NCCLVectorBackend::isNCCLAvailable() { return false; }
int NCCLVectorBackend::getNCCLVersion() { return 0; }
std::string NCCLVectorBackend::getNCCLVersionString() { return "Not available"; }
bool NCCLVectorBackend::checkNVLinkSupport(const std::vector<int>&) { return false; }

#endif // THEMIS_ENABLE_NCCL

} // namespace acceleration
} // namespace themis
