/**
 * @file nccl_vector_backend.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=13; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=6, Debt=0, C=0, H=2, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "acceleration/nccl_vector_backend.h"
#include <cfloat>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <numeric>
#include <stdexcept>
#include <cstring>
#include <vector>

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
    std::vector<ncclComm_t> allComms;  // Store all communicators to properly destroy them
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
        // For now, we use ncclCommInitAll for simplicity in single-process multi-GPU setup
        allComms.resize(config.worldSize);
        NCCL_CHECK(ncclCommInitAll(allComms.data(), config.worldSize, config.deviceIds.data()));
        comm = allComms[config.rank];
        
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
        if (initialized) {
            // Destroy all communicators created by ncclCommInitAll
            for (auto& c : allComms) {
                if (c != nullptr) {
                    ncclCommDestroy(c);
                }
            }
            allComms.clear();
            comm = nullptr;
            initialized = false;
        }
    }
    
    bool enableAllP2PAccess() {
        if (config.deviceIds.empty()) {
          return true;
        }
        
        for (size_t i = 0; i <static_cast<int>(config.deviceIds.size()); ++i) {
            for (size_t j = i + 1; j <static_cast<int>(config.deviceIds.size()); ++j) {
                int canAccess = 0;
                cudaDeviceCanAccessPeer(&canAccess, config.deviceIds[i], config.deviceIds[j]);
                if (canAccess) {
                    cudaSetDevice(config.deviceIds[i]);
                    cudaError_t err = cudaDeviceEnablePeerAccess(config.deviceIds[j], 0);
                    if (err == cudaErrorPeerAccessAlreadyEnabled) {
                        // Clear the sticky error state
                        cudaGetLastError();
                    } else if (err != cudaSuccess) {
                        std::cerr << "CUDA error: " << cudaGetErrorString(err)
                                  << " at " << __FILE__ << ":" << __LINE__ << std::endl;
                        return false;
                    }
                    
                    cudaSetDevice(config.deviceIds[j]);
                    err = cudaDeviceEnablePeerAccess(config.deviceIds[i], 0);
                    if (err == cudaErrorPeerAccessAlreadyEnabled) {
                        // Clear the sticky error state
                        cudaGetLastError();
                    } else if (err != cudaSuccess) {
                        std::cerr << "CUDA error: " << cudaGetErrorString(err)
                                  << " at " << __FILE__ << ":" << __LINE__ << std::endl;
                        return false;
                    }
                }
            }
        }
        
        return true;
    }
    
    bool checkNVLinkAvailable() {
        // Simple check: if P2P is available between any two devices, assume NVLink
        if (static_cast<int>(config.deviceIds.size()) < 2) {
          return false;
        }
        
        int canAccess = 0;
        cudaDeviceCanAccessPeer(&canAccess, config.deviceIds[0], config.deviceIds[1]);
        return canAccess != 0;
    }
    
    int countNVLinks() {
        // Simplified: count P2P-capable device pairs
        int count = 0;
        for (size_t i = 0; i <static_cast<int>(config.deviceIds.size()); ++i) {
            for (size_t j = i + 1; j <static_cast<int>(config.deviceIds.size()); ++j) {
                int canAccess = 0;
                cudaDeviceCanAccessPeer(&canAccess, config.deviceIds[i], config.deviceIds[j]);
                if (canAccess) {
                  count++;
                }
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
    
    void recordP2P([[maybe_unused]] size_t bytes) {
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

// Explicitly defined to ensure Impl is complete type when destructed
NCCLVectorBackend::~NCCLVectorBackend() {}

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
    if (!pImpl->initialized) {
      return false;
    }
    
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
    if (!pImpl->initialized) {
      return false;
    }
    
    pImpl->startTiming();
    NCCL_CHECK(ncclBroadcast(buffer, buffer, count, ncclFloat, root, 
                             pImpl->comm, stream));
    pImpl->recordCollective();
    return true;
}

bool NCCLVectorBackend::allGather(const float* sendBuf, float* recvBuf, size_t sendCount,
                                   cudaStream_t stream) {
    if (!pImpl->initialized) {
      return false;
    }
    
    pImpl->startTiming();
    NCCL_CHECK(ncclAllGather(sendBuf, recvBuf, sendCount, ncclFloat, 
                             pImpl->comm, stream));
    pImpl->recordCollective();
    return true;
}

bool NCCLVectorBackend::reduce(const float* sendBuf, float* recvBuf, size_t count,
                                ReductionOp op, int root, cudaStream_t stream) {
    if (!pImpl->initialized) {
      return false;
    }
    
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
    if (!pImpl->initialized) {
      return false;
    }
    
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
    if (!pImpl->initialized) {
      return false;
    }
    
    pImpl->startTiming();
    NCCL_CHECK(ncclSend(buffer, count, ncclFloat, peerRank, pImpl->comm, stream));
    pImpl->recordP2P(count * sizeof(float));
    return true;
}

bool NCCLVectorBackend::p2pRecv(float* buffer, size_t count, int peerRank,
                                 cudaStream_t stream) {
    if (!pImpl->initialized) {
      return false;
    }
    
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
    if (!pImpl->initialized) {
      return false;
    }
    
    // Use a barrier via AllReduce of a dummy value
    float dummy = 0.0f;
    return allReduce(&dummy, &dummy, 1, ReductionOp::SUM, stream);
}

bool NCCLVectorBackend::waitAll() {
    if (!pImpl->initialized) {
      return false;
    }
    CUDA_CHECK(cudaDeviceSynchronize());
    return true;
}

bool NCCLVectorBackend::mergeTopK(const uint32_t* localIndices, const float* localDistances,
                                   size_t localK, uint32_t* globalIndices, float* globalDistances,
                                   size_t k, int root, cudaStream_t stream) {
    if (!pImpl->initialized) {
      return false;
    }
    
    int rank = pImpl->config.rank;
    size_t worldSize = pImpl->config.worldSize;
    
    // Validate that requested k does not exceed the locally available top-k
    if (k > localK) {
        std::cerr << "NCCLVectorBackend::mergeTopK: requested k (" << k
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
        CUDA_CHECK(cudaMemcpy(globalIndices, localIndices, k * sizeof(uint32_t),
                              cudaMemcpyDeviceToDevice));
        CUDA_CHECK(cudaMemcpy(globalDistances, localDistances, k * sizeof(float),
                              cudaMemcpyDeviceToDevice));
        pImpl->recordCollective();
        return true;
    }
    
    // Distributed mergeTopK via ncclAllGather + host-side partial sort + ncclBcast.
    //
    // Strategy:
    //   1. AllGather — every rank sends its localK (indices, distances) to all
    //      other ranks, producing a gathered buffer of worldSize × localK candidates.
    //   2. Host-side merge — copy gathered buffers to host and run std::partial_sort
    //      over all worldSize × localK candidates to select the global top-k.
    //   3. Broadcast — broadcast the merged top-k from `root` to all ranks so that
    //      each rank ends up with identical global results in its output buffers.
    //
    // Performance note: the host-side D2H+sort+H2D+Bcast path incurs latency.
    // For latency-critical paths at small worldSize the overhead is acceptable;
    // a fully device-side bitonic sort is possible as a future optimisation.

    pImpl->startTiming();

    const size_t totalK = static_cast<size_t>(worldSize) * localK;

    // ── Step 1: Allocate gathered device buffers ───────────────────────────────
    uint32_t* d_gathered_indices  = nullptr;
    float*    d_gathered_distances = nullptr;
    CUDA_CHECK(cudaMalloc(&d_gathered_indices,  totalK * sizeof(uint32_t)));
    CUDA_CHECK(cudaMalloc(&d_gathered_distances, totalK * sizeof(float)));

    // ── Step 2: AllGather — collect per-rank localK results ───────────────────
    // Both collectives are issued inside a single NCCL group to allow pipelining.
    ncclGroupStart();
    NCCL_CHECK(ncclAllGather(
        localIndices, d_gathered_indices, localK, ncclUint32,
        pImpl->comm, static_cast<cudaStream_t>(stream)));
    NCCL_CHECK(ncclAllGather(
        localDistances, d_gathered_distances, localK, ncclFloat,
        pImpl->comm, static_cast<cudaStream_t>(stream)));
    ncclGroupEnd();
    CUDA_CHECK(cudaStreamSynchronize(static_cast<cudaStream_t>(stream)));

    // ── Step 3: Host-side merge ────────────────────────────────────────────────
    std::vector<uint32_t> h_indices(totalK);
    std::vector<float>    h_distances(totalK);
    CUDA_CHECK(cudaMemcpy(h_indices.data(),   d_gathered_indices,
                          totalK * sizeof(uint32_t), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(h_distances.data(), d_gathered_distances,
                          totalK * sizeof(float),    cudaMemcpyDeviceToHost));

    {
        cudaError_t e1 = cudaFree(d_gathered_indices);
        cudaError_t e2 = cudaFree(d_gathered_distances);
        if (e1 != cudaSuccess || e2 != cudaSuccess) {
            std::cerr << "CUDA error freeing gathered buffers in mergeTopK" << std::endl;
            return false;
        }
    }

    // Select the global top-k by partial-sort on distance
    std::vector<size_t> order(totalK);
    std::iota(order.begin(), order.end(), 0);
    const size_t select_k = (k < totalK) ? k : totalK;
    std::partial_sort(order.begin(), order.begin() + select_k, order.end(),
                      [&](size_t a, size_t b) {
                          return h_distances[a] < h_distances[b];
                      });

    std::vector<uint32_t> h_global_indices(k, static_cast<uint32_t>(-1));
    std::vector<float>    h_global_distances(k, FLT_MAX);
    for (size_t i = 0; i < select_k; ++i) {
        h_global_indices[i]   = h_indices[order[i]];
        h_global_distances[i] = h_distances[order[i]];
    }

    // Copy merged results to each rank's output device buffers
    CUDA_CHECK(cudaMemcpy(globalIndices,   h_global_indices.data(),
                          k * sizeof(uint32_t), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(globalDistances, h_global_distances.data(),
                          k * sizeof(float),    cudaMemcpyHostToDevice));

    // ── Step 4: Broadcast from root so all ranks have identical output ────────
    // (Technically redundant since AllGather gives every rank the same data and
    //  all ranks ran the same deterministic merge, but honors the `root` API
    //  contract and guards against any non-determinism in partial_sort ties.)
    ncclGroupStart();
    NCCL_CHECK(ncclBcast(globalIndices,   k, ncclUint32, root,
                         pImpl->comm, static_cast<cudaStream_t>(stream)));
    NCCL_CHECK(ncclBcast(globalDistances, k, ncclFloat,  root,
                         pImpl->comm, static_cast<cudaStream_t>(stream)));
    ncclGroupEnd();

    pImpl->recordCollective();
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
    if (static_cast<int>(deviceIds.size()) < 2) {
      return false;
    }
    
    // Check if P2P is available between devices (simplified NVLink check)
    for (size_t i = 0; i < deviceIds.size(); ++i) {
        for (size_t j = i + 1; j < deviceIds.size(); ++j) {
            int canAccess = 0;
            cudaDeviceCanAccessPeer(&canAccess, deviceIds[i], deviceIds[j]);
            if (canAccess) {
              return true;
            }
        }
    }
    
    return false;
}

#else // THEMIS_ENABLE_NCCL

// STUB/SIMULATION NOTE:
// Purpose: Satisfy the linker and allow ThemisDB to be built and run without
//   NCCL (NVIDIA Collective Communications Library).  All multi-GPU collective
//   operations (allReduce, broadcast, allGather, reduce, reduceScatter, p2pSend,
//   p2pRecv, mergeTopK) return false, and isNCCLAvailable() returns false, so
//   callers can gracefully fall back to single-GPU or CPU paths.
// Activation: `THEMIS_ENABLE_NCCL` is not defined at compile time (default for
//   CPU-only, non-CUDA, or single-GPU builds).
// Production Delta: All multi-GPU collective operations are unavailable.
//   Distributed ANN search (mergeTopK across GPUs) is completely disabled.
//   Training workloads that require gradient allReduce will fail silently or
//   must be routed to CPU paths.
// Removal Plan: Install NCCL (e.g., via CUDA toolkit installer or conda) and
//   set `-DTHEMIS_ENABLE_NCCL=1` in CMake.  The full NCCL implementation block
//   (above `#else`) will then be compiled instead.
// Roadmap ref: src/acceleration/FUTURE_ENHANCEMENTS.md §"NCCL/RCCL Activation"

// STUB/SIMULATION NOTE (allReduce bridge):
// Purpose:    Allow injection of a real allReduce implementation for the
//             non-NCCL stub path, enabling integration tests and gradual
//             feature rollout without modifying the production NCCL path.
// Activation: Runtime — when setAllReduceFn() is called with a non-empty fn.
// Production Delta: With no fn injected, allReduce() returns false; with fn
//             injected the provided implementation is called instead.
// Removal Plan: Remove bridge once THEMIS_ENABLE_NCCL is always set in CI/CD.
static std::mutex s_nccl_allreduce_mutex_;
static NCCLVectorBackend::AllReduceFn s_allreduce_fn_;

void NCCLVectorBackend::setAllReduceFn(NCCLVectorBackend::AllReduceFn fn) {
    std::lock_guard<std::mutex> lk(s_nccl_allreduce_mutex_);
    s_allreduce_fn_ = std::move(fn);
}

// Stub implementation when NCCL is not available
// Define empty Impl class to satisfy unique_ptr
/** @brief Define empty Impl class to satisfy unique_ptr. */
class NCCLVectorBackend::Impl {
public:
    Impl() = default;
    ~Impl() = default;
};

NCCLVectorBackend::NCCLVectorBackend() : pImpl(std::make_unique<Impl>()) {}

// Explicitly defined to ensure Impl is complete type when destructed
NCCLVectorBackend::~NCCLVectorBackend() {}
bool NCCLVectorBackend::initialize(const Config&) { return false; }
void NCCLVectorBackend::shutdown() {}
bool NCCLVectorBackend::isInitialized() const { return false; }
int NCCLVectorBackend::getRank() const { return 0; }
int NCCLVectorBackend::getWorldSize() const { return 1; }
std::vector<int> NCCLVectorBackend::getDeviceIds() const { return {}; }
bool NCCLVectorBackend::isP2PEnabled() const { return false; }
bool NCCLVectorBackend::allReduce(const float* send, float* recv, size_t count,
                                  ReductionOp op, void* stream) {
    NCCLVectorBackend::AllReduceFn fn;
    {
        std::lock_guard<std::mutex> lk(s_nccl_allreduce_mutex_);
        fn = s_allreduce_fn_;
    }
    if (fn) [[unlikely]] {
        try {
            return fn(send, recv, count, op, stream);
        } catch (const std::exception &) {
            return false;
        } catch (const std::string &) {
            return false;
        } catch (const char *) {
            return false;
        }
    }
    return false;
}
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

