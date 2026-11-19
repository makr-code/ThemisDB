/**
 * @file multi_gpu_backend.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "acceleration/multi_gpu_backend.h"

#include <algorithm>
#include <cassert>
#include <future>
#include <iostream>
#include <limits>
#include <mutex>
#include <numeric>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include "acceleration/cpu_backend.h"
#include "acceleration/cuda_backend.h"
#include "acceleration/graphics_backends.h"
#include "acceleration/hip_backend.h"
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

namespace {

std::vector<int> availableDeviceIds(MultiGPUDeviceBackend backend) {
    std::vector<int> deviceIds;
    switch (backend) {
        case MultiGPUDeviceBackend::CUDA:
#ifdef THEMIS_ENABLE_CUDA
            {
                int count = 0;
                if (cudaGetDeviceCount(&count) == cudaSuccess) {
                    for (int deviceId = 0; deviceId < count; ++deviceId) {
                        deviceIds.push_back(deviceId);
                    }
                }
            }
#else
            break;
#endif
            break;
        case MultiGPUDeviceBackend::HIP:
#ifdef THEMIS_ENABLE_HIP
            {
                int count = 0;
                if (hipGetDeviceCount(&count) == hipSuccess) {
                    for (int deviceId = 0; deviceId < count; ++deviceId) {
                        deviceIds.push_back(deviceId);
                    }
                }
            }
#endif
            break;
        case MultiGPUDeviceBackend::VULKAN:
            deviceIds = VulkanVectorBackend::availableDeviceIndices();
            break;
        case MultiGPUDeviceBackend::AUTO:
        case MultiGPUDeviceBackend::CPU:
            break;
    }
    return deviceIds;
}

MultiGPUDeviceBackend resolveAutoBackend(size_t minimumDeviceCount = 1) {
    const auto requiredDevices = std::max<size_t>(1, minimumDeviceCount);
    MultiGPUDeviceBackend bestBackend = MultiGPUDeviceBackend::CPU;
    size_t bestDeviceCount = 0;
    for (const auto backend : {MultiGPUDeviceBackend::CUDA,
                               MultiGPUDeviceBackend::HIP,
                               MultiGPUDeviceBackend::VULKAN}) {
        const auto deviceCount = availableDeviceIds(backend).size();
        if (deviceCount >= requiredDevices) {
            return backend;
        }
        if (deviceCount > bestDeviceCount) {
            bestDeviceCount = deviceCount;
            bestBackend = backend;
        }
    }
    return bestBackend;
}

const char* backendName(MultiGPUDeviceBackend backend) noexcept {
    switch (backend) {
        case MultiGPUDeviceBackend::AUTO: return "AUTO";
        case MultiGPUDeviceBackend::CPU: return "CPU";
        case MultiGPUDeviceBackend::CUDA: return "CUDA";
        case MultiGPUDeviceBackend::HIP: return "HIP";
        case MultiGPUDeviceBackend::VULKAN: return "Vulkan";
    }
    return "Unknown";
}

std::unique_ptr<IVectorBackend> createDeviceBackend(MultiGPUDeviceBackend backend, int deviceId) {
    switch (backend) {
        case MultiGPUDeviceBackend::CPU:
            return std::make_unique<CPUVectorBackend>();
        case MultiGPUDeviceBackend::CUDA:
#ifdef THEMIS_ENABLE_CUDA
            return std::make_unique<CUDAVectorBackend>(deviceId);
#else
            (void)deviceId;
            return nullptr;
#endif
        case MultiGPUDeviceBackend::HIP: {
#ifdef THEMIS_ENABLE_HIP
            HIPVectorBackend::HIPConfig config;
            config.deviceId = deviceId;
            return std::make_unique<HIPVectorBackend>(config);
#else
            (void)deviceId;
            return nullptr;
#endif
        }
        case MultiGPUDeviceBackend::VULKAN:
            return std::make_unique<VulkanVectorBackend>(deviceId);
        case MultiGPUDeviceBackend::AUTO:
            break;
    }
    return nullptr;
}

} // namespace

int MultiGPUVectorBackend::detectDeviceCount(MultiGPUDeviceBackend backend) noexcept {
    try {
        if (backend == MultiGPUDeviceBackend::AUTO) {
            backend = resolveAutoBackend();
        }
        return static_cast<int>(availableDeviceIds(backend).size());
    } catch (...) {
        return 0;
    }
}

int MultiGPUVectorBackend::detectGPUCount() noexcept {
    return std::max({detectDeviceCount(MultiGPUDeviceBackend::CUDA),
                     detectDeviceCount(MultiGPUDeviceBackend::HIP),
                     detectDeviceCount(MultiGPUDeviceBackend::VULKAN)});
}

// =============================================================================
// MultiGPUVectorBackend::Impl
// =============================================================================

class MultiGPUVectorBackend::Impl {
  public:
    Config config;
    bool initialized = false;

    // Shard descriptors (built at initialize())
    std::vector<ShardDescriptor> shardDescs;

    // Each shard owns its backend and serializes access to backend-local streams.
    std::vector<std::unique_ptr<IVectorBackend>> subBackends;
    std::vector<std::unique_ptr<std::mutex>> shardMutexes;
    MultiGPUDeviceBackend resolvedBackend = MultiGPUDeviceBackend::CPU;

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

    /**
     * @brief Initialize.
     * @return True when the operation succeeds.
     * @details Calls: empty(), push_back(), MultiGPUVectorBackend::detectGPUCount(), size(), resize(), clear(), reserve(), std::move().
     */
    bool initialize() {
        shutdown();
        if (config.numDevices <= 0 || config.minDevices < 0) {
            return false;
        }

        const size_t requestedDeviceCount = config.deviceIds.empty()
            ? static_cast<size_t>(config.numDevices)
            : config.deviceIds.size();
        resolvedBackend = config.deviceBackend == MultiGPUDeviceBackend::AUTO
            ? resolveAutoBackend(requestedDeviceCount)
            : config.deviceBackend;
        const auto availableIds = availableDeviceIds(resolvedBackend);

        std::vector<int> requestedIds = config.deviceIds;
        if (requestedIds.empty()) {
            requestedIds.reserve(static_cast<size_t>(config.numDevices));
            for (int index = 0; index < config.numDevices; ++index) {
                requestedIds.push_back(index < static_cast<int>(availableIds.size())
                    ? availableIds[static_cast<size_t>(index)]
                    : index);
            }
        }
        if (requestedIds.empty()) {
            return false;
        }

        std::unordered_set<int> uniqueIds;
        uniqueIds.reserve(requestedIds.size());
        for (const int deviceId : requestedIds) {
            if (deviceId < 0 || !uniqueIds.insert(deviceId).second) {
                return false;
            }
        }

        shardDescs.reserve(requestedIds.size());
        subBackends.reserve(requestedIds.size());
        shardMutexes.reserve(requestedIds.size());
        std::vector<int> activeAcceleratorIds;
        for (const int deviceId : requestedIds) {
            const bool hasAccelerator = resolvedBackend != MultiGPUDeviceBackend::CPU &&
                std::find(availableIds.begin(), availableIds.end(), deviceId) != availableIds.end();
            if (!hasAccelerator && resolvedBackend != MultiGPUDeviceBackend::CPU &&
                !config.allowCPUFallback) {
                shutdown();
                return false;
            }

            auto shardBackend = hasAccelerator ? resolvedBackend : MultiGPUDeviceBackend::CPU;
            auto backend = createDeviceBackend(shardBackend, deviceId);
            bool cpuFallback =
                (config.deviceBackend == MultiGPUDeviceBackend::AUTO &&
                 resolvedBackend == MultiGPUDeviceBackend::CPU) ||
                (resolvedBackend != MultiGPUDeviceBackend::CPU && !hasAccelerator);
            const bool backendInitialized = backend && backend->initialize();
            if (!backendInitialized) {
                if (!config.allowCPUFallback || shardBackend == MultiGPUDeviceBackend::CPU) {
                    shutdown();
                    return false;
                }
                if (backend) {
                    backend->shutdown();
                }
                shardBackend = MultiGPUDeviceBackend::CPU;
                cpuFallback = true;
                backend = createDeviceBackend(shardBackend, deviceId);
                if (!backend || !backend->initialize()) {
                    shutdown();
                    return false;
                }
            }

            ShardDescriptor descriptor;
            descriptor.deviceId = deviceId;
            descriptor.backend = shardBackend;
            descriptor.cpuFallback = cpuFallback;
            shardDescs.push_back(descriptor);
            subBackends.push_back(std::move(backend));
            shardMutexes.push_back(std::make_unique<std::mutex>());
            if (shardBackend == resolvedBackend && shardBackend != MultiGPUDeviceBackend::CPU) {
                activeAcceleratorIds.push_back(deviceId);
            }
        }

        initCommBackend(activeAcceleratorIds);
        initialized = true;
        const auto countBackend = [this](MultiGPUDeviceBackend backend) {
            return std::count_if(shardDescs.begin(), shardDescs.end(),
                [backend](const ShardDescriptor& shard) { return shard.backend == backend; });
        };
        const auto fallbackCount = std::count_if(shardDescs.begin(), shardDescs.end(),
            [](const ShardDescriptor& shard) { return shard.cpuFallback; });
        std::cout << "MultiGPUVectorBackend: initialized " << shardDescs.size()
                  << " shards (selected=" << backendName(resolvedBackend)
                  << ", CUDA=" << countBackend(MultiGPUDeviceBackend::CUDA)
                  << ", HIP=" << countBackend(MultiGPUDeviceBackend::HIP)
                  << ", Vulkan=" << countBackend(MultiGPUDeviceBackend::VULKAN)
                  << ", CPU=" << countBackend(MultiGPUDeviceBackend::CPU)
                  << ", fallbacks=" << fallbackCount
                  << ", comm=" << commBackendName() << ").\n";
        return true;
    }

    /**
     * @brief Shutdown.
     * @details Calls: clear(), reset().
     */
    void shutdown() {
        for (auto& backend : subBackends) {
            if (backend) {
                backend->shutdown();
            }
        }
        subBackends.clear();
        shardMutexes.clear();
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
        /**
         * @brief Ranges.
         * @param[in] n Input parameter.
         * @return Return value.
         */
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

    std::vector<float> computeShard(size_t shardIndex, const ShardDescriptor& shard,
                                    const float* queries, size_t numQueries, size_t dim,
                                    const float* vectors, bool useL2) {
        const auto shardSize = shard.numVectors();
        const auto* shardVectors = vectors + shard.startIdx * dim;
        std::vector<float> distances;
        try {
            std::lock_guard<std::mutex> lock(*shardMutexes[shardIndex]);
            distances = subBackends[shardIndex]->computeDistances(
                queries, numQueries, dim, shardVectors, shardSize, useL2);
        } catch (const std::exception& error) {
            std::cerr << "MultiGPUVectorBackend: shard " << shard.deviceId
                      << " computeDistances failed: " << error.what() << '\n';
        }
        if (distances.size() == numQueries * shardSize) {
            return distances;
        }
        if (!config.allowCPUFallback || shard.backend == MultiGPUDeviceBackend::CPU) {
            return {};
        }

        CPUVectorBackend fallback;
        if (!fallback.initialize()) {
            return {};
        }
        std::cerr << "MultiGPUVectorBackend: shard " << shard.deviceId
                  << " execution failed; using CPU fallback.\n";
        return fallback.computeDistances(queries, numQueries, dim, shardVectors, shardSize, useL2);
    }

    std::vector<std::vector<std::pair<uint32_t, float>>> searchShard(
        size_t shardIndex, const ShardDescriptor& shard,
        const float* queries, size_t numQueries, size_t dim, const float* vectors,
        size_t localK, bool useL2) {
        const auto shardSize = shard.numVectors();
        const auto* shardVectors = vectors + shard.startIdx * dim;
        std::vector<std::vector<std::pair<uint32_t, float>>> results;
        try {
            std::lock_guard<std::mutex> lock(*shardMutexes[shardIndex]);
            results = subBackends[shardIndex]->batchKnnSearch(
                queries, numQueries, dim, shardVectors, shardSize, localK, useL2);
        } catch (const std::exception& error) {
            std::cerr << "MultiGPUVectorBackend: shard " << shard.deviceId
                      << " batchKnnSearch failed: " << error.what() << '\n';
        }
        if (results.size() == numQueries) {
            return results;
        }
        if (!config.allowCPUFallback || shard.backend == MultiGPUDeviceBackend::CPU) {
            return {};
        }

        CPUVectorBackend fallback;
        if (!fallback.initialize()) {
            return {};
        }
        std::cerr << "MultiGPUVectorBackend: shard " << shard.deviceId
                  << " search failed; using CPU fallback.\n";
        return fallback.batchKnnSearch(
            queries, numQueries, dim, shardVectors, shardSize, localK, useL2);
    }

    /**
     * @brief ------------------------------------------------------------------------- computeDistances — per-shard distance computation + global concat -------------------------------------------------------------------------
     * @param[in] queries Input parameter.
     * @param[in] numQueries Input parameter.
     * @param[in] dim Input parameter.
     * @param[in] vectors Input parameter.
     * @param[in] numVectors Input parameter.
     * @param[in] useL2 Input parameter.
     * @return Return value.
     * @details Calls: empty(), buildRanges(), result(), size(), numVectors().
     */

    std::vector<float> computeDistances(const float *queries, size_t numQueries, size_t dim, const float *vectors,
                                        size_t numVectors, bool useL2) {
        if (!initialized || subBackends.empty()) {
            return {};
        }
        if (numQueries == 0 || numVectors == 0 || dim == 0 || queries == nullptr || vectors == nullptr) {
            return {};
        }

        // Build per-call ranges without mutating shared state
        const auto ranges = buildRanges(numVectors);

        // Output: [numQueries × numVectors]
        std::vector<float> result(numQueries * numVectors, 0.0f);

        std::vector<std::future<std::vector<float>>> shardFutures(ranges.size());
        for (size_t shardIndex = 0; shardIndex < ranges.size(); ++shardIndex) {
            if (ranges[shardIndex].numVectors() == 0) {
                continue;
            }
            shardFutures[shardIndex] = std::async(std::launch::async, [this, shardIndex, &ranges,
                queries, numQueries, dim, vectors, useL2]() {
                return computeShard(shardIndex, ranges[shardIndex], queries, numQueries,
                                    dim, vectors, useL2);
            });
        }

        for (size_t shardIndex = 0; shardIndex < ranges.size(); ++shardIndex) {
            const auto& shard = ranges[shardIndex];
            const size_t shardSize = shard.numVectors();
            if (shardSize == 0) {
                continue;
            }

            std::vector<float> shardDists;
            try {
                shardDists = shardFutures[shardIndex].get();
            } catch (const std::exception& error) {
                std::cerr << "MultiGPUVectorBackend: shard " << shard.deviceId
                          << " worker failed: " << error.what() << '\n';
                return {};
            }
            if (shardDists.size() != numQueries * shardSize) {
                return {};
            }

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
        if (numQueries == 0) {
            return merged;
        }
        if (queries == nullptr || vectors == nullptr || dim == 0 || numVectors == 0 || k == 0) {
            return merged;
        }

        std::vector<std::future<std::vector<std::vector<std::pair<uint32_t, float>>>>> shardFutures(ranges.size());
        for (size_t shardIndex = 0; shardIndex < ranges.size(); ++shardIndex) {
            const size_t shardSize = ranges[shardIndex].numVectors();
            if (shardSize == 0) {
                continue;
            }
            const size_t localK = std::min(k, shardSize);
            shardFutures[shardIndex] = std::async(std::launch::async, [this, shardIndex, &ranges,
                queries, numQueries, dim, vectors, localK, useL2]() {
                return searchShard(shardIndex, ranges[shardIndex], queries, numQueries,
                                   dim, vectors, localK, useL2);
            });
        }

        for (size_t shardIndex = 0; shardIndex < ranges.size(); ++shardIndex) {
            const auto& shard = ranges[shardIndex];
            const size_t shardSize = shard.numVectors();
            if (shardSize == 0) {
                continue;
            }
            std::vector<std::vector<std::pair<uint32_t, float>>> shardResults;
            try {
                shardResults = shardFutures[shardIndex].get();
            } catch (const std::exception& error) {
                std::cerr << "MultiGPUVectorBackend: shard " << shard.deviceId
                          << " worker failed: " << error.what() << '\n';
                return std::vector<std::vector<std::pair<uint32_t, float>>>(numQueries);
            }
            if (shardResults.size() != numQueries) {
                return std::vector<std::vector<std::pair<uint32_t, float>>>(numQueries);
            }

            // Remap shard-local indices to global indices
            for (size_t q = 0; q < numQueries; ++q) {
                for (auto &[localIdx, dist] : shardResults[q]) {
                    if (localIdx >= shardSize) {
                        return std::vector<std::vector<std::pair<uint32_t, float>>>(numQueries);
                    }
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

    /**
     * @brief Init Comm Backend.
     * @param[in] deviceIds Input parameter.
     * @details Calls: NCCLVectorBackend::isNCCLAvailable(), RCCLVectorBackend::isRCCLAvailable(), size(), initialize(), reset().
     */
    void initCommBackend(const std::vector<int> &deviceIds) {
        activeComm = CommBackend::CPU;
        if (deviceIds.size() < 2 ||
            (resolvedBackend != MultiGPUDeviceBackend::CUDA &&
             resolvedBackend != MultiGPUDeviceBackend::HIP)) {
            return;
        }
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
                ncclCfg.worldSize    = deviceIds.size();
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
                rcclCfg.worldSize    = deviceIds.size();
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
    const auto requestedBackend = pImpl_->config.deviceBackend;
    if (requestedBackend == MultiGPUDeviceBackend::CPU) {
        return pImpl_->config.numDevices >= pImpl_->config.minDevices;
    }
    const auto requiredDevices = pImpl_->config.minDevices > 0
        ? static_cast<size_t>(pImpl_->config.minDevices)
        : size_t{1};
    const auto backend = requestedBackend == MultiGPUDeviceBackend::AUTO
        ? resolveAutoBackend(requiredDevices)
        : requestedBackend;
    if (backend == MultiGPUDeviceBackend::CPU) {
        return pImpl_->config.minDevices == 0;
    }
    return detectDeviceCount(backend) >= pImpl_->config.minDevices;
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
    const auto backend = pImpl_->initialized ? pImpl_->resolvedBackend
        : (pImpl_->config.deviceBackend == MultiGPUDeviceBackend::AUTO
            ? resolveAutoBackend() : pImpl_->config.deviceBackend);
    const auto cpuFallbackCount = static_cast<size_t>(std::count_if(
        pImpl_->shardDescs.begin(), pImpl_->shardDescs.end(),
        [](const ShardDescriptor& shard) { return shard.cpuFallback; }));
    caps.deviceName = "Multi-device " + std::string(backendName(backend)) +
        " (" + std::to_string(pImpl_->shardDescs.size()) + " shards";
    if (cpuFallbackCount > 0) {
        caps.deviceName += ", " + std::to_string(cpuFallbackCount) + " CPU fallbacks";
    }
    caps.deviceName += ")";
    return caps;
}

/**
 * @brief Initialize.
 * @return True when the operation succeeds.
 * @details Calls: clearError(), setError(), ErrorContext(), name().
 */
bool MultiGPUVectorBackend::initialize() {
    if (pImpl_->initialize()) {
        clearError();
        return true;
    }
    setError(ErrorContext(AccelerationErrorCode::ContextCreationFailed, name(),
                          "Failed to initialise multi-GPU sharding backend"));
    return false;
}

/**
 * @brief Shutdown.
 * @details Implements shutdown without additional internal calls.
 */
void MultiGPUVectorBackend::shutdown() {
    pImpl_->shutdown();
}

/**
 * @brief Compute Distances.
 * @param[in] queries Input parameter.
 * @param[in] numQueries Input parameter.
 * @param[in] dim Input parameter.
 * @param[in] vectors Input parameter.
 * @param[in] numVectors Input parameter.
 * @param[in] useL2 Input parameter.
 * @return Return value.
 * @details Implements computeDistances without additional internal calls.
 */
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
