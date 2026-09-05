/**
 * @file multi_gpu_backend.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "acceleration/multi_gpu_backend.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <vector>

#include "acceleration/cpu_backend.h"
#include "acceleration/nccl_vector_backend.h"
#include "acceleration/rccl_vector_backend.h"

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#endif

#ifdef THEMIS_ENABLE_HIP
#include <hip/hip_runtime.h>
#endif

namespace themis {
namespace acceleration {

// =============================================================================
// Static helper
// =============================================================================

int MultiGPUVectorBackend::detectGPUCount() noexcept {
#ifdef THEMIS_ENABLE_CUDA
    int count = 0;
    if (cudaGetDeviceCount(&count) == cudaSuccess) {
        return count;
    }
    return 0;
#elif defined(THEMIS_ENABLE_HIP)
    int count = 0;
    if (hipGetDeviceCount(&count) == hipSuccess) {
        return count;
    }
    return 0;
#else
    return 0;
#endif
}

// =============================================================================
// MultiGPUVectorBackend::Impl
// =============================================================================

/** @brief MultiGPUVectorBackend::Impl. */
class MultiGPUVectorBackend::Impl {
  public:
    Config config;
    bool initialized = false;

    // Shard descriptors (built at initialize())
    std::vector<ShardDescriptor> shardDescs;

    // Per-shard CPU sub-backends (used for actual compute; replaced by GPU
    // backends in future when CUDA kernels are available in the acceleration layer)
    std::vector<std::unique_ptr<CPUVectorBackend>> subBackends;

    // Active communication backend
    CommBackend activeComm = CommBackend::CPU;

    // Optional NCCL/RCCL backends for collective merge operations
#ifdef THEMIS_ENABLE_NCCL
    std::unique_ptr<NCCLVectorBackend> ncclBackend;
#endif
#ifdef THEMIS_ENABLE_RCCL
    std::unique_ptr<RCCLVectorBackend> rcclBackend;
#endif

    explicit Impl(const Config &cfg) : config(cfg) {}

    ~Impl() {
        shutdown();
    }

    // -------------------------------------------------------------------------

    bool initialize() {
        // Determine actual device IDs to use
        std::vector<int> deviceIds = config.deviceIds;
        if (deviceIds.empty()) {
            for (int i = 0; i < config.numDevices; ++i) {
                deviceIds.push_back(i);
            }
        }

        // Clamp to available GPU count
        int gpuCount = MultiGPUVectorBackend::detectGPUCount();
        if (gpuCount > 0 && deviceIds.size() > static_cast<size_t>(gpuCount)) {
            std::cerr << "MultiGPUVectorBackend: requested " << deviceIds.size() << " devices but only " << gpuCount
                      << " visible; clamping.\n";
            deviceIds.resize(static_cast<size_t>(gpuCount));
        }

        if (deviceIds.empty()) {
            std::cerr << "MultiGPUVectorBackend: no devices available.\n";
            return false;
        }

        // Build shard descriptors (ranges assigned later at search time based on
        // numVectors; here we just record device IDs)
        shardDescs.clear();
        shardDescs.reserve(deviceIds.size());
        for (int devId : deviceIds) {
            ShardDescriptor sd;
            sd.deviceId = devId;
            sd.startIdx = 0;
            sd.endIdx   = 0;
            shardDescs.push_back(sd);
        }

        // Initialise per-shard sub-backends (CPU fallback)
        subBackends.clear();
        subBackends.reserve(shardDescs.size());
        for (size_t i = 0; i < shardDescs.size(); ++i) {
            auto sb = std::make_unique<CPUVectorBackend>();
            if (!sb->initialize()) {
                if (!config.allowCPUFallback) {
                    std::cerr << "MultiGPUVectorBackend: sub-backend init failed for device " << shardDescs[i].deviceId
                              << "\n";
                    return false;
                }
                std::cerr << "MultiGPUVectorBackend: warning — sub-backend init failed "
                             "for device "
                          << shardDescs[i].deviceId << "; retaining CPU fallback.\n";
            }
            subBackends.push_back(std::move(sb));
        }

        // Initialise communication backend
        initCommBackend(deviceIds);

        std::cout << "MultiGPUVectorBackend: initialised with " << shardDescs.size()
                  << " shards (comm=" << commBackendName() << ").\n";
        initialized = true;
        return true;
    }

    void shutdown() {
        subBackends.clear();
        shardDescs.clear();
#ifdef THEMIS_ENABLE_NCCL
        if (ncclBackend) {
            ncclBackend->shutdown();
            ncclBackend.reset();
        }
#endif
#ifdef THEMIS_ENABLE_RCCL
        if (rcclBackend) {
            rcclBackend->shutdown();
            rcclBackend.reset();
        }
#endif
        initialized = false;
    }

    // -------------------------------------------------------------------------
    // Range assignment: split [0, numVectors) evenly across shards
    //
    // Returns a local vector — does NOT mutate shardDescs, so concurrent
    // calls to computeDistances / batchKnnSearch are safe.
    // -------------------------------------------------------------------------

    std::vector<ShardDescriptor> buildRanges(size_t numVectors) const {
        size_t n = shardDescs.size();
        std::vector<ShardDescriptor> ranges(n);

        size_t base      = (n > 0) ? numVectors / n : 0;
        size_t remainder = (n > 0) ? numVectors % n : 0;
        size_t offset    = 0;

        for (size_t i = 0; i < n; ++i) {
            size_t count       = base + (i < remainder ? 1 : 0);
            ranges[i]          = shardDescs[i]; // copy shard descriptor (deviceId + zero-initialised range fields)
            ranges[i].startIdx = offset;
            ranges[i].endIdx   = offset + count;
            offset += count;
        }
        assert(n == 0 || offset == numVectors);
        return ranges;
    }

    // -------------------------------------------------------------------------
    // computeDistances — per-shard distance computation + global concat
    // -------------------------------------------------------------------------

    std::vector<float> computeDistances(const float *queries, size_t numQueries, size_t dim, const float *vectors,
                                        size_t numVectors, bool useL2) {
        if (!initialized || subBackends.empty()) {
            return {};
        }

        // Build per-call ranges without mutating shared state
        const auto ranges = buildRanges(numVectors);

        // Output: [numQueries × numVectors]
        std::vector<float> result(numQueries * numVectors, 0.0f);

        for (size_t s = 0; s < ranges.size(); ++s) {
            const auto &shard = ranges[s];
            size_t shardSize  = shard.numVectors();
            if (shardSize == 0) {
                continue;
            }

            const float *shardVectors = vectors + shard.startIdx * dim;

            auto shardDists
                = subBackends[s]->computeDistances(queries, numQueries, dim, shardVectors, shardSize, useL2);

            // Copy shard results into the global output at the correct column offsets
            for (size_t q = 0; q < numQueries; ++q) {
                for (size_t v = 0; v < shardSize; ++v) {
                    result[q * numVectors + shard.startIdx + v] = shardDists[q * shardSize + v];
                }
            }
        }

        return result;
    }

    // -------------------------------------------------------------------------
    // batchKnnSearch — fan-out to all shards, remap indices, merge top-k
    // -------------------------------------------------------------------------

    std::vector<std::vector<std::pair<uint32_t, float>>> batchKnnSearch(const float *queries, size_t numQueries,
                                                                        size_t dim, const float *vectors,
                                                                        size_t numVectors, size_t k, bool useL2) {
        if (!initialized || subBackends.empty()) {
            return std::vector<std::vector<std::pair<uint32_t, float>>>(numQueries);
        }

        // Build per-call ranges without mutating shared state
        const auto ranges = buildRanges(numVectors);

        // Per-query merged result buffer
        std::vector<std::vector<std::pair<uint32_t, float>>> merged(numQueries);

        // Fan out to each shard
        for (size_t s = 0; s < ranges.size(); ++s) {
            const auto &shard = ranges[s];
            size_t shardSize  = shard.numVectors();
            if (shardSize == 0) {
                continue;
            }

            const float *shardVectors = vectors + shard.startIdx * dim;

            // k_local: search for min(k, shardSize) locally
            size_t kLocal = std::min(k, shardSize);

            auto shardResults
                = subBackends[s]->batchKnnSearch(queries, numQueries, dim, shardVectors, shardSize, kLocal, useL2);

            // Remap shard-local indices to global indices
            for (size_t q = 0; q < numQueries; ++q) {
                for (auto &[localIdx, dist] : shardResults[q]) {
                    uint32_t globalIdx = static_cast<uint32_t>(shard.startIdx) + localIdx;
                    merged[q].emplace_back(globalIdx, dist);
                }
            }
        }

        // Host-side top-k merge: partial sort then trim to k
        for (size_t q = 0; q < numQueries; ++q) {
            auto &row = merged[q];
            if (row.size() > k) {
                std::partial_sort(row.begin(), row.begin() + k, row.end(),
                                  [](const std::pair<uint32_t, float> &a, const std::pair<uint32_t, float> &b) {
                                      return a.second < b.second;
                                  });
                row.resize(k);
            } else {
                std::sort(row.begin(), row.end(),
                          [](const std::pair<uint32_t, float> &a, const std::pair<uint32_t, float> &b) {
                              return a.second < b.second;
                          });
            }
        }

        return merged;
    }

    // -------------------------------------------------------------------------
    // Communication backend helpers
    // -------------------------------------------------------------------------

    void initCommBackend(const std::vector<int> &deviceIds) {
        (void)deviceIds;
        CommBackend target = config.commBackend;

        if (target == CommBackend::AUTO) {
#ifdef THEMIS_ENABLE_NCCL
            if (NCCLVectorBackend::isNCCLAvailable()) {
                target = CommBackend::NCCL;
            } else
#endif
#ifdef THEMIS_ENABLE_RCCL
                if (RCCLVectorBackend::isRCCLAvailable()) {
                target = CommBackend::RCCL;
            } else
#endif
            {
                target = CommBackend::CPU;
            }
        }

        bool success = false;

        switch (target) {
#ifdef THEMIS_ENABLE_NCCL
            case CommBackend::NCCL: {
                ncclBackend = std::make_unique<NCCLVectorBackend>();
                NCCLVectorBackend::Config ncclCfg;
                ncclCfg.worldSize    = static_cast<int>(deviceIds.size());
                ncclCfg.rank         = 0;
                ncclCfg.deviceIds    = deviceIds;
                ncclCfg.enableP2P    = config.enableP2P;
                ncclCfg.enableNVLink = config.enableNVLink;
                ncclCfg.bufferSizeMB = config.commBufferSizeMB;

                if (ncclBackend->initialize(ncclCfg)) {
                    activeComm = CommBackend::NCCL;
                    success    = true;
                } else {
                    std::cerr << "MultiGPUVectorBackend: NCCL init failed, "
                                 "falling back to CPU merge.\n";
                    ncclBackend.reset();
                }
                break;
            }
#endif
#ifdef THEMIS_ENABLE_RCCL
            case CommBackend::RCCL: {
                rcclBackend = std::make_unique<RCCLVectorBackend>();
                RCCLVectorBackend::Config rcclCfg;
                rcclCfg.worldSize    = static_cast<int>(deviceIds.size());
                rcclCfg.rank         = 0;
                rcclCfg.deviceIds    = deviceIds;
                rcclCfg.enableP2P    = config.enableP2P;
                rcclCfg.enableXGMI   = config.enableXGMI;
                rcclCfg.bufferSizeMB = config.commBufferSizeMB;

                if (rcclBackend->initialize(rcclCfg)) {
                    activeComm = CommBackend::RCCL;
                    success    = true;
                } else {
                    std::cerr << "MultiGPUVectorBackend: RCCL init failed, "
                                 "falling back to CPU merge.\n";
                    rcclBackend.reset();
                }
                break;
            }
#endif
            case CommBackend::CPU:
            [[fallthrough]];
            default:
                // unused when NCCL/RCCL are not compiled in
                activeComm = CommBackend::CPU;
                success    = true;
                break;
        }

        if (!success) {
            activeComm = CommBackend::CPU;
        }
    }

    std::string commBackendName() const {
        switch (activeComm) {
            case CommBackend::NCCL:
                return "NCCL";
            case CommBackend::RCCL:
                return "RCCL";
            default:
                return "CPU";
        }
    }
};

// =============================================================================
// MultiGPUVectorBackend — public interface
// =============================================================================

MultiGPUVectorBackend::MultiGPUVectorBackend() : MultiGPUVectorBackend(Config{}) {}

MultiGPUVectorBackend::MultiGPUVectorBackend(const Config &config) : pImpl_(std::make_unique<Impl>(config)) {}

MultiGPUVectorBackend::~MultiGPUVectorBackend() = default;

bool MultiGPUVectorBackend::isAvailable() const noexcept {
    return detectGPUCount() >= pImpl_->config.minDevices;
}

BackendCapabilities MultiGPUVectorBackend::getCapabilities() const {
    BackendCapabilities caps;
    caps.supportsVectorOps       = true;
    caps.supportsGraphOps        = false;
    caps.supportsGeoOps          = false;
    caps.supportsBatchProcessing = true;
    caps.supportsAsync           = false;
    caps.supportedPrecisions     = PrecisionMode::FP32;
    caps.supportedMetrics
        = metricBit(DistanceMetric::L2) | metricBit(DistanceMetric::COSINE) | metricBit(DistanceMetric::INNER_PRODUCT);
    caps.deviceName = "Multi-GPU (" + std::to_string(pImpl_->shardDescs.size()) + " shards)";
    return caps;
}

bool MultiGPUVectorBackend::initialize() {
    if (pImpl_->initialize()) {
        clearError();
        return true;
    }
    setError(ErrorContext(AccelerationErrorCode::ContextCreationFailed, name(),
                          "Failed to initialise multi-GPU sharding backend"));
    return false;
}

void MultiGPUVectorBackend::shutdown() {
    pImpl_->shutdown();
}

std::vector<float> MultiGPUVectorBackend::computeDistances(const float *queries, size_t numQueries, size_t dim,
                                                           const float *vectors, size_t numVectors, bool useL2) {
    return pImpl_->computeDistances(queries, numQueries, dim, vectors, numVectors, useL2);
}

std::vector<std::vector<std::pair<uint32_t, float>>>
MultiGPUVectorBackend::batchKnnSearch(const float *queries, size_t numQueries, size_t dim, const float *vectors,
                                      size_t numVectors, size_t k, bool useL2) {
    return pImpl_->batchKnnSearch(queries, numQueries, dim, vectors, numVectors, k, useL2);
}

const std::vector<ShardDescriptor> &MultiGPUVectorBackend::shards() const noexcept {
    return pImpl_->shardDescs;
}

int MultiGPUVectorBackend::activeDeviceCount() const noexcept {
    return static_cast<int>(pImpl_->shardDescs.size());
}

MultiGPUVectorBackend::CommBackend MultiGPUVectorBackend::activeCommBackend() const noexcept {
    return pImpl_->activeComm;
}

bool MultiGPUVectorBackend::isCollectiveOpsAvailable() const noexcept {
    return pImpl_->activeComm == CommBackend::NCCL || pImpl_->activeComm == CommBackend::RCCL;
}

} // namespace acceleration
} // namespace themis
