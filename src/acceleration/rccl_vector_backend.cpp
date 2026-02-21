/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rccl_vector_backend.cpp                            ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   81.0/100                                       ║
    • Total Lines:     527                                            ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "acceleration/rccl_vector_backend.h"
#include <iostream>
#include <chrono>
#include <stdexcept>
#include <cstring>

#ifdef THEMIS_ENABLE_RCCL
#include <rccl/rccl.h>
#include <hip/hip_runtime.h>

// RCCL error checking macro
#define RCCL_CHECK(cmd) \
    do { \
        rcclResult_t r = cmd; \
        if (r != rcclSuccess) { \
            std::cerr << "RCCL error: " << rcclGetErrorString(r) \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

// HIP error checking macro
#define HIP_CHECK(cmd) \
    do { \
        hipError_t e = cmd; \
        if (e != hipSuccess) { \
            std::cerr << "HIP error: " << hipGetErrorString(e) \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

#endif

namespace themis {
namespace acceleration {

#ifdef THEMIS_ENABLE_RCCL

// =============================================================================
// RCCLVectorBackend::Impl
// =============================================================================

class RCCLVectorBackend::Impl {
public:
    Config config;
    rcclComm_t comm = nullptr;
    std::vector<rcclComm_t> allComms;  // Store all communicators to properly destroy them
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
            std::cerr << "RCCL: Invalid world size: " << config.worldSize << std::endl;
            return false;
        }
        
        if (config.rank < 0 || config.rank >= config.worldSize) {
            std::cerr << "RCCL: Invalid rank: " << config.rank << std::endl;
            return false;
        }
        
        std::cout << "RCCL: Initializing with rank " << config.rank 
                  << " of " << config.worldSize << std::endl;
        
        // Set device for this rank
        if (!config.deviceIds.empty() && config.rank < static_cast<int>(config.deviceIds.size())) {
            HIP_CHECK(hipSetDevice(config.deviceIds[config.rank]));
        } else {
            HIP_CHECK(hipSetDevice(config.rank));
        }
        
        // Initialize RCCL communicator
        // Note: In production, this would use rcclCommInitRank with a unique ID
        // For now, we use rcclCommInitAll for simplicity in single-process multi-GPU setup
        allComms.resize(config.worldSize);
        RCCL_CHECK(rcclCommInitAll(allComms.data(), config.worldSize, config.deviceIds.data()));
        comm = allComms[config.rank];
        
        // Enable P2P access if requested
        if (config.enableP2P) {
            enableAllP2PAccess();
        }
        
        // Check for XGMI (AMD Infinity Fabric) support
        if (config.enableXGMI) {
            stats.xgmiAvailable = checkXGMIAvailable();
            stats.numXGMILinks = countXGMILinks();
        }
        
        initialized = true;
        std::cout << "RCCL: Initialization successful" << std::endl;
        return true;
    }
    
    void shutdown() {
        if (initialized) {
            // Destroy all communicators created by rcclCommInitAll
            for (auto& c : allComms) {
                if (c != nullptr) {
                    rcclCommDestroy(c);
                }
            }
            allComms.clear();
            comm = nullptr;
            initialized = false;
        }
    }
    
    bool enableAllP2PAccess() {
        if (config.deviceIds.empty()) return true;
        
        for (size_t i = 0; i < config.deviceIds.size(); ++i) {
            for (size_t j = i + 1; j < config.deviceIds.size(); ++j) {
                int canAccess = 0;
                hipDeviceCanAccessPeer(&canAccess, config.deviceIds[i], config.deviceIds[j]);
                if (canAccess) {
                    hipSetDevice(config.deviceIds[i]);
                    hipError_t err = hipDeviceEnablePeerAccess(config.deviceIds[j], 0);
                    if (err == hipErrorPeerAccessAlreadyEnabled) {
                        // Clear the sticky error state
                        (void)hipGetLastError();
                    } else if (err != hipSuccess) {
                        std::cerr << "HIP error: " << hipGetErrorString(err)
                                  << " at " << __FILE__ << ":" << __LINE__ << std::endl;
                        return false;
                    }
                    
                    hipSetDevice(config.deviceIds[j]);
                    err = hipDeviceEnablePeerAccess(config.deviceIds[i], 0);
                    if (err == hipErrorPeerAccessAlreadyEnabled) {
                        // Clear the sticky error state
                        (void)hipGetLastError();
                    } else if (err != hipSuccess) {
                        std::cerr << "HIP error: " << hipGetErrorString(err)
                                  << " at " << __FILE__ << ":" << __LINE__ << std::endl;
                        return false;
                    }
                }
            }
        }
        
        return true;
    }
    
    bool checkXGMIAvailable() {
        // Simple check: if P2P is available between any two devices, assume XGMI
        if (config.deviceIds.size() < 2) return false;
        
        int canAccess = 0;
        hipDeviceCanAccessPeer(&canAccess, config.deviceIds[0], config.deviceIds[1]);
        return canAccess != 0;
    }
    
    int countXGMILinks() {
        // Simplified: count P2P-capable device pairs
        int count = 0;
        for (size_t i = 0; i < config.deviceIds.size(); ++i) {
            for (size_t j = i + 1; j < config.deviceIds.size(); ++j) {
                int canAccess = 0;
                hipDeviceCanAccessPeer(&canAccess, config.deviceIds[i], config.deviceIds[j]);
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
// RCCLVectorBackend Public API
// =============================================================================

RCCLVectorBackend::RCCLVectorBackend() : pImpl(std::make_unique<Impl>()) {}

// Explicitly defined to ensure Impl is complete type when destructed
RCCLVectorBackend::~RCCLVectorBackend() {}

bool RCCLVectorBackend::initialize(const Config& config) {
    return pImpl->initialize(config);
}

void RCCLVectorBackend::shutdown() {
    pImpl->shutdown();
}

bool RCCLVectorBackend::isInitialized() const {
    return pImpl->initialized;
}

int RCCLVectorBackend::getRank() const {
    return pImpl->config.rank;
}

int RCCLVectorBackend::getWorldSize() const {
    return pImpl->config.worldSize;
}

std::vector<int> RCCLVectorBackend::getDeviceIds() const {
    return pImpl->config.deviceIds;
}

bool RCCLVectorBackend::isP2PEnabled() const {
    return pImpl->config.enableP2P;
}

bool RCCLVectorBackend::allReduce(const float* sendBuf, float* recvBuf, size_t count,
                                   ReductionOp op, hipStream_t stream) {
    if (!pImpl->initialized) return false;
    
    pImpl->startTiming();
    
    rcclRedOp_t rcclOp;
    switch (op) {
        case ReductionOp::SUM:  rcclOp = rcclSum;  break;
        case ReductionOp::MIN:  rcclOp = rcclMin;  break;
        case ReductionOp::MAX:  rcclOp = rcclMax;  break;
        case ReductionOp::PROD: rcclOp = rcclProd; break;
        default: return false;
    }
    
    RCCL_CHECK(rcclAllReduce(sendBuf, recvBuf, count, rcclFloat, rcclOp, 
                             pImpl->comm, stream));
    
    pImpl->recordCollective();
    return true;
}

bool RCCLVectorBackend::broadcast(float* buffer, size_t count, int root,
                                   hipStream_t stream) {
    if (!pImpl->initialized) return false;
    
    pImpl->startTiming();
    RCCL_CHECK(rcclBroadcast(buffer, buffer, count, rcclFloat, root, 
                             pImpl->comm, stream));
    pImpl->recordCollective();
    return true;
}

bool RCCLVectorBackend::allGather(const float* sendBuf, float* recvBuf, size_t sendCount,
                                   hipStream_t stream) {
    if (!pImpl->initialized) return false;
    
    pImpl->startTiming();
    RCCL_CHECK(rcclAllGather(sendBuf, recvBuf, sendCount, rcclFloat, 
                             pImpl->comm, stream));
    pImpl->recordCollective();
    return true;
}

bool RCCLVectorBackend::reduce(const float* sendBuf, float* recvBuf, size_t count,
                                ReductionOp op, int root, hipStream_t stream) {
    if (!pImpl->initialized) return false;
    
    pImpl->startTiming();
    
    rcclRedOp_t rcclOp;
    switch (op) {
        case ReductionOp::SUM:  rcclOp = rcclSum;  break;
        case ReductionOp::MIN:  rcclOp = rcclMin;  break;
        case ReductionOp::MAX:  rcclOp = rcclMax;  break;
        case ReductionOp::PROD: rcclOp = rcclProd; break;
        default: return false;
    }
    
    RCCL_CHECK(rcclReduce(sendBuf, recvBuf, count, rcclFloat, rcclOp, root,
                          pImpl->comm, stream));
    pImpl->recordCollective();
    return true;
}

bool RCCLVectorBackend::reduceScatter(const float* sendBuf, float* recvBuf, size_t recvCount,
                                       ReductionOp op, hipStream_t stream) {
    if (!pImpl->initialized) return false;
    
    pImpl->startTiming();
    
    rcclRedOp_t rcclOp;
    switch (op) {
        case ReductionOp::SUM:  rcclOp = rcclSum;  break;
        case ReductionOp::MIN:  rcclOp = rcclMin;  break;
        case ReductionOp::MAX:  rcclOp = rcclMax;  break;
        case ReductionOp::PROD: rcclOp = rcclProd; break;
        default: return false;
    }
    
    RCCL_CHECK(rcclReduceScatter(sendBuf, recvBuf, recvCount, rcclFloat, rcclOp,
                                 pImpl->comm, stream));
    pImpl->recordCollective();
    return true;
}

bool RCCLVectorBackend::p2pSend(const float* buffer, size_t count, int peerRank,
                                 hipStream_t stream) {
    if (!pImpl->initialized) return false;
    
    pImpl->startTiming();
    RCCL_CHECK(rcclSend(buffer, count, rcclFloat, peerRank, pImpl->comm, stream));
    pImpl->recordP2P(count * sizeof(float));
    return true;
}

bool RCCLVectorBackend::p2pRecv(float* buffer, size_t count, int peerRank,
                                 hipStream_t stream) {
    if (!pImpl->initialized) return false;
    
    pImpl->startTiming();
    RCCL_CHECK(rcclRecv(buffer, count, rcclFloat, peerRank, pImpl->comm, stream));
    pImpl->stats.totalBytesReceived += count * sizeof(float);
    return true;
}

bool RCCLVectorBackend::enableP2PAccess(int deviceId1, int deviceId2) {
    int canAccess = 0;
    HIP_CHECK(hipDeviceCanAccessPeer(&canAccess, deviceId1, deviceId2));
    
    if (canAccess) {
        HIP_CHECK(hipSetDevice(deviceId1));
        hipError_t err = hipDeviceEnablePeerAccess(deviceId2, 0);
        if (err != hipSuccess && err != hipErrorPeerAccessAlreadyEnabled) {
            return false;
        }
    }
    
    return canAccess != 0;
}

bool RCCLVectorBackend::canAccessPeer(int deviceId1, int deviceId2) {
    int canAccess = 0;
    hipDeviceCanAccessPeer(&canAccess, deviceId1, deviceId2);
    return canAccess != 0;
}

bool RCCLVectorBackend::synchronize(hipStream_t stream) {
    if (!pImpl->initialized) return false;
    
    // Use a barrier via AllReduce of a dummy value
    float dummy = 0.0f;
    return allReduce(&dummy, &dummy, 1, ReductionOp::SUM, stream);
}

bool RCCLVectorBackend::waitAll() {
    if (!pImpl->initialized) return false;
    HIP_CHECK(hipDeviceSynchronize());
    return true;
}

bool RCCLVectorBackend::mergeTopK(const uint32_t* localIndices, const float* localDistances,
                                   size_t localK, uint32_t* globalIndices, float* globalDistances,
                                   size_t k, int root, hipStream_t stream) {
    if (!pImpl->initialized) return false;
    
    int rank = pImpl->config.rank;
    size_t worldSize = pImpl->config.worldSize;
    
    // Validate that requested k does not exceed the locally available top-k
    if (k > localK) {
        std::cerr << "RCCLVectorBackend::mergeTopK: requested k (" << k
                  << ") exceeds localK (" << localK << ")." << std::endl;
        return false;
    }
    
    // Single-process case: safe and meaningful to just copy local results
    if (worldSize == 1) {
        if (k == 0) {
            // Nothing to do, but this is a valid no-op
            return true;
        }
        
        pImpl->startTiming();
        // Copy local top-k to global buffers on the single rank
        HIP_CHECK(hipMemcpy(globalIndices, localIndices, k * sizeof(uint32_t),
                            hipMemcpyDeviceToDevice));
        HIP_CHECK(hipMemcpy(globalDistances, localDistances, k * sizeof(float),
                            hipMemcpyDeviceToDevice));
        pImpl->recordCollective();
        return true;
    }
    
    // Distributed mergeTopK is not yet implemented. Avoid claiming success.
    std::cerr << "RCCLVectorBackend::mergeTopK: distributed merge (worldSize = "
              << worldSize << ") is not implemented yet." << std::endl;
    (void)root;   // suppress unused parameter warning until implemented
    (void)stream; // suppress unused parameter warning until implemented
    return false;
}

RCCLVectorBackend::Statistics RCCLVectorBackend::getStatistics() const {
    return pImpl->stats;
}

void RCCLVectorBackend::resetStatistics() {
    pImpl->stats = Statistics{};
}

bool RCCLVectorBackend::isRCCLAvailable() {
    // Check if RCCL library is available
    int deviceCount = 0;
    hipError_t err = hipGetDeviceCount(&deviceCount);
    return (err == hipSuccess && deviceCount > 0);
}

int RCCLVectorBackend::getRCCLVersion() {
    int version = 0;
    rcclGetVersion(&version);
    return version;
}

std::string RCCLVectorBackend::getRCCLVersionString() {
    int version = getRCCLVersion();
    int major = version / 10000;
    int minor = (version % 10000) / 100;
    int patch = version % 100;
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
}

bool RCCLVectorBackend::checkXGMISupport(const std::vector<int>& deviceIds) {
    if (deviceIds.size() < 2) return false;
    
    // Check if P2P is available between devices (simplified XGMI check)
    for (size_t i = 0; i < deviceIds.size(); ++i) {
        for (size_t j = i + 1; j < deviceIds.size(); ++j) {
            int canAccess = 0;
            hipDeviceCanAccessPeer(&canAccess, deviceIds[i], deviceIds[j]);
            if (canAccess) return true;
        }
    }
    
    return false;
}

#else // THEMIS_ENABLE_RCCL

// Stub implementation when RCCL is not available
// Define empty Impl class to satisfy unique_ptr
class RCCLVectorBackend::Impl {
public:
    Impl() = default;
    ~Impl() = default;
};

RCCLVectorBackend::RCCLVectorBackend() : pImpl(std::make_unique<Impl>()) {}

// Explicitly defined to ensure Impl is complete type when destructed
RCCLVectorBackend::~RCCLVectorBackend() {}
bool RCCLVectorBackend::initialize(const Config&) { return false; }
void RCCLVectorBackend::shutdown() {}
bool RCCLVectorBackend::isInitialized() const { return false; }
int RCCLVectorBackend::getRank() const { return 0; }
int RCCLVectorBackend::getWorldSize() const { return 1; }
std::vector<int> RCCLVectorBackend::getDeviceIds() const { return {}; }
bool RCCLVectorBackend::isP2PEnabled() const { return false; }
bool RCCLVectorBackend::allReduce(const float*, float*, size_t, ReductionOp, void*) { return false; }
bool RCCLVectorBackend::broadcast(float*, size_t, int, void*) { return false; }
bool RCCLVectorBackend::allGather(const float*, float*, size_t, void*) { return false; }
bool RCCLVectorBackend::reduce(const float*, float*, size_t, ReductionOp, int, void*) { return false; }
bool RCCLVectorBackend::reduceScatter(const float*, float*, size_t, ReductionOp, void*) { return false; }
bool RCCLVectorBackend::p2pSend(const float*, size_t, int, void*) { return false; }
bool RCCLVectorBackend::p2pRecv(float*, size_t, int, void*) { return false; }
bool RCCLVectorBackend::enableP2PAccess(int, int) { return false; }
bool RCCLVectorBackend::canAccessPeer(int, int) { return false; }
bool RCCLVectorBackend::synchronize(void*) { return false; }
bool RCCLVectorBackend::waitAll() { return false; }
bool RCCLVectorBackend::mergeTopK(const uint32_t*, const float*, size_t, uint32_t*, float*, size_t, int, void*) { return false; }
RCCLVectorBackend::Statistics RCCLVectorBackend::getStatistics() const { return Statistics{}; }
void RCCLVectorBackend::resetStatistics() {}
bool RCCLVectorBackend::isRCCLAvailable() { return false; }
int RCCLVectorBackend::getRCCLVersion() { return 0; }
std::string RCCLVectorBackend::getRCCLVersionString() { return "Not available"; }
bool RCCLVectorBackend::checkXGMISupport(const std::vector<int>&) { return false; }

#endif // THEMIS_ENABLE_RCCL

} // namespace acceleration
} // namespace themis
