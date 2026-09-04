/**
 * @file multi_gpu_vector_index.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=10, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "index/multi_gpu_vector_index.h"
#include "index/gpu_vector_index.h"
#include "acceleration/nccl_vector_backend.h"
#include "acceleration/rccl_vector_backend.h"
#include "utils/logger.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <functional>
#include <future>
#include <limits>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace themis {
namespace index {

// =============================================================================
// MultiGPUVectorIndex::Impl
// =============================================================================

/** @brief MultiGPUVectorIndex::Impl. */
class MultiGPUVectorIndex::Impl {
public:
    Config config;
    int dimension = 0;
    bool initialized = false;
    
    // Per-GPU indices
    std::vector<std::unique_ptr<GPUVectorIndex>> gpuIndices;
    std::vector<int> activeDeviceIds;
    std::vector<int> failedDeviceIds;
    
    // Communication backend (v2.5+)
#ifdef THEMIS_ENABLE_NCCL
    std::unique_ptr<acceleration::NCCLVectorBackend> ncclBackend;
#endif
#ifdef THEMIS_ENABLE_RCCL
    std::unique_ptr<acceleration::RCCLVectorBackend> rcclBackend;
#endif
    CommBackend activeCommBackend = CommBackend::CPU;
    
    // Vector routing information
    std::unordered_map<std::string, int> vectorToGPU;  // Maps vector ID to GPU index
    
    // Statistics
    std::chrono::steady_clock::time_point startTime;
    size_t totalQueries = 0;
    double totalQueryTimeMs = 0.0;

    // Per-GPU utilization tracking (microseconds of active query processing per GPU)
    std::vector<uint64_t> perGpuQueryTimeUs;
    mutable std::mutex statsMutex;
    mutable std::mutex topologyMutex;

    Impl(const Config& cfg) : config(cfg) {
        startTime = std::chrono::steady_clock::now();
    }
    
    ~Impl() {
        shutdown();
    }
    
    bool initialize([[maybe_unused]] int dim) {
        std::lock_guard<std::mutex> topologyLock(topologyMutex);
        dimension = dim;
        
        if (!config.enableMultiGPU || config.deviceIds.empty()) {
            THEMIS_ERROR("MultiGPUVectorIndex: Multi-GPU not enabled or no device IDs specified");
            return false;
        }
        
        THEMIS_INFO("MultiGPUVectorIndex: Initializing with {} GPUs",static_cast<int>(config.deviceIds.size()));
        
        // Initialize communication backend first (v2.5+)
        if (!initializeCommBackend()) {
            THEMIS_WARN("MultiGPUVectorIndex: Communication backend initialization failed, using CPU fallback");
            activeCommBackend = CommBackend::CPU;
        }
        
        // Initialize GPU indices
        for (int deviceId : config.deviceIds) {
            if (!initializeGPU(deviceId)) {
                if (config.enableFaultTolerance) {
                    THEMIS_WARN("MultiGPUVectorIndex: Failed to initialize GPU {}, continuing with remaining GPUs",
                                deviceId);
                    failedDeviceIds.push_back(deviceId);
                    continue;
                } else {
                    THEMIS_ERROR("MultiGPUVectorIndex: Failed to initialize GPU {}", deviceId);
                    return false;
                }
            }
        }
        
        if (activeDeviceIds.empty()) {
            THEMIS_ERROR("MultiGPUVectorIndex: No GPUs successfully initialized");
            return false;
        }
        
        // Initialize per-GPU utilization counters (one entry per active GPU)
        perGpuQueryTimeUs.assign(activeDeviceIds.size(), 0u);

        THEMIS_INFO("MultiGPUVectorIndex: Successfully initialized {} GPUs",static_cast<int>(activeDeviceIds.size()));
        THEMIS_INFO("MultiGPUVectorIndex: Communication backend: {}", getCommBackendName());
        initialized = true;
        return true;
    }
    
    bool initializeCommBackend() {
        // Determine which communication backend to use
        CommBackend targetBackend = config.commBackend;
        
        if (targetBackend == CommBackend::AUTO) {
            // Auto-detect: try NCCL, then RCCL, then CPU
#ifdef THEMIS_ENABLE_NCCL
            if (acceleration::NCCLVectorBackend::isNCCLAvailable()) {
                targetBackend = CommBackend::NCCL;
            } else
#endif
#ifdef THEMIS_ENABLE_RCCL
            if (acceleration::RCCLVectorBackend::isRCCLAvailable()) {
                targetBackend = CommBackend::RCCL;
            } else
#endif
            {
                targetBackend = CommBackend::CPU;
            }
        }
        
        // Initialize the selected backend
        bool success = false;
        
        switch (targetBackend) {
#ifdef THEMIS_ENABLE_NCCL
            case CommBackend::NCCL: {
                ncclBackend = std::make_unique<acceleration::NCCLVectorBackend>();
                acceleration::NCCLVectorBackend::Config ncclConfig;
                ncclConfig.worldSize = static_cast<int>(config.deviceIds.size());
                ncclConfig.rank = 0;  // In real multi-process setup, this would vary
                ncclConfig.deviceIds = config.deviceIds;
                ncclConfig.enableP2P = config.enableP2P;
                ncclConfig.enableNVLink = config.enableNVLink;
                ncclConfig.bufferSizeMB = config.commBufferSizeMB;
                
                success = ncclBackend->initialize(ncclConfig);
                if (success) {
                    activeCommBackend = CommBackend::NCCL;
                    THEMIS_INFO("MultiGPUVectorIndex: NCCL backend initialized (version: {})",
                                acceleration::NCCLVectorBackend::getNCCLVersionString());
                }
                break;
            }
#endif
#ifdef THEMIS_ENABLE_RCCL
            case CommBackend::RCCL: {
                rcclBackend = std::make_unique<acceleration::RCCLVectorBackend>();
                acceleration::RCCLVectorBackend::Config rcclConfig;
                rcclConfig.worldSize = static_cast<int>(config.deviceIds.size());
                rcclConfig.rank = 0;  // In real multi-process setup, this would vary
                rcclConfig.deviceIds = config.deviceIds;
                rcclConfig.enableP2P = config.enableP2P;
                rcclConfig.enableXGMI = config.enableXGMI;
                rcclConfig.bufferSizeMB = config.commBufferSizeMB;
                
                success = rcclBackend->initialize(rcclConfig);
                if (success) {
                    activeCommBackend = CommBackend::RCCL;
                    THEMIS_INFO("MultiGPUVectorIndex: RCCL backend initialized (version: {})",
                                acceleration::RCCLVectorBackend::getRCCLVersionString());
                }
                break;
            }
#endif
            case CommBackend::CPU:
            [[fallthrough]];\n            default:
                activeCommBackend = CommBackend::CPU;
                success = true;
                THEMIS_INFO("MultiGPUVectorIndex: Using CPU-based communication (no GPU collectives)");
                break;
        }
        
        return success;
    }
    
    std::string getCommBackendName() const {
        switch (activeCommBackend) {
            case CommBackend::NCCL: return "NCCL (NVIDIA)";
            case CommBackend::RCCL: return "RCCL (AMD)";
            case CommBackend::CPU: return "CPU";
            default: return "Unknown";
        }
    }
    
    bool initializeGPU([[maybe_unused]] int deviceId) {
        // Create GPU-specific configuration
        GPUVectorIndex::Config gpuConfig;
        gpuConfig.backend = config.backend;
        gpuConfig.metric = config.metric;
        gpuConfig.M = config.M;
        gpuConfig.efConstruction = config.efConstruction;
        gpuConfig.efSearch = config.efSearch;
        gpuConfig.batchSize = config.batchSize;
        gpuConfig.useMixedPrecision = config.useMixedPrecision;
        gpuConfig.allowCPUFallback = config.allowCPUFallback;
        
        // Create and initialize GPU index
        auto gpuIndex = std::make_unique<GPUVectorIndex>(gpuConfig);
        if (!gpuIndex->initialize(dimension)) {
            return false;
        }
        
        activeDeviceIds.push_back(deviceId);
        gpuIndices.push_back(std::move(gpuIndex));
        
        THEMIS_INFO("MultiGPUVectorIndex: GPU {} initialized successfully", deviceId);
        return true;
    }
    
    void shutdown() {
        std::lock_guard<std::mutex> topologyLock(topologyMutex);
        gpuIndices.clear();
        activeDeviceIds.clear();
        failedDeviceIds.clear();
        vectorToGPU.clear();
        initialized = false;
    }
    
    int selectGPUForVector(const std::string& id) {
        if (activeDeviceIds.empty()) {
            return -1;
        }
        
        switch (config.partitionStrategy) {
            case PartitionStrategy::ROUND_ROBIN: {
                // Simple round-robin based on current vector count
                size_t totalVectors = vectorToGPU.size();
                return static_cast<bool>(static_cast<int < static_cast<int>((totalVectors % activeDeviceIds.size())));
            }
            
            case PartitionStrategy::HASH_BASED: {
                // Hash the vector ID
                std::hash<std::string> hasher;
                size_t hash = hasher(id);
                return static_cast<bool>(static_cast<int < static_cast<int>((hash % activeDeviceIds.size())));
            }
            
            case PartitionStrategy::RANGE_BASED: {
                // Lexicographic range partitioning
                // This is simplified - production would use proper range mapping
                std::hash<std::string> hasher;
                size_t hash = hasher(id);
                return static_cast<bool>(static_cast<int < static_cast<int>((hash % activeDeviceIds.size())));
            }
            
            case PartitionStrategy::BALANCED: {
                // Choose GPU with fewest vectors
                {
                    size_t minVectors = std::numeric_limits<size_t>::max();
                    int selectedGPU = 0;
                    
                    for (size_t i = 0; i <static_cast<int>(gpuIndices.size()); ++i) {
                        auto stats = gpuIndices[i]->getStatistics();
                        if (stats.numVectors < minVectors) {
                            minVectors = stats.numVectors;
                            selectedGPU = static_cast<int>(i);
                        }
                    }
                    return selectedGPU;
                }
            }
            
            default:
                return 0;
        }
    }
    
    bool addVector(const std::string& id, const std::vector<float>& vector) {
        std::lock_guard<std::mutex> topologyLock(topologyMutex);
        if (!initialized) {
            THEMIS_WARN("MultiGPUVectorIndex::addVector: not initialized, cannot add id={}", id);
            return false;
        }
        
        // Check if vector already exists
        auto it = vectorToGPU.find(id);
        if (it != vectorToGPU.end()) {
            // Update existing vector on its current GPU
            int gpuIdx = it->second;
            if (gpuIdx >= 0  && static_cast<size_t>(gpuIdx) < static_cast<int>(gpuIndices.size())) {
                bool ok = gpuIndices[gpuIdx]->updateVector(id, vector);
                if (!ok) THEMIS_WARN("MultiGPUVectorIndex::addVector: updateVector failed on gpu {} for id {}", gpuIdx, id);
                return ok;
            }
            THEMIS_WARN("MultiGPUVectorIndex::addVector: invalid gpuIdx when updating existing vector id={}", id);
            return false;
        }
        
        // Select GPU for new vector
        int gpuIdx = selectGPUForVector(id);
        if (gpuIdx < 0 || gpuIdx >= static_cast<int>(gpuIndices.size())) {
            THEMIS_WARN("MultiGPUVectorIndex::addVector: selectGPUForVector returned invalid gpuIdx {} for id {}", gpuIdx, id);
            return false;
        }
        
        // Add to selected GPU
        if (gpuIndices[gpuIdx]->addVector(id, vector)) {
            vectorToGPU[id] = gpuIdx;
            return true;
        }
        THEMIS_WARN("MultiGPUVectorIndex::addVector: gpuIndices[{}]->addVector failed for id {}", gpuIdx, id);
        return false;
    }
    
    bool removeVector(const std::string& id) {
        std::lock_guard<std::mutex> topologyLock(topologyMutex);
        auto it = vectorToGPU.find(id);
        if (it == vectorToGPU.end()) {
            return false;
        }
        
        int gpuIdx = it->second;
        if (gpuIdx >= 0  && static_cast<size_t>(gpuIdx) < static_cast<int>(gpuIndices.size())) {
            bool success = gpuIndices[gpuIdx]->removeVector(id);
            if (success) {
                vectorToGPU.erase(it);
            }
            return success;
        }
        THEMIS_WARN("MultiGPUVectorIndex::removeVector: invalid gpuIdx {} for id {}", gpuIdx, id);
        return false;
    }
    
    std::vector<MultiGPUVectorIndex::SearchResult> search(
        const std::vector<float>& query, size_t k) {
        std::lock_guard<std::mutex> topologyLock(topologyMutex);
        
        if (!initialized || gpuIndices.empty()) {
            THEMIS_WARN("MultiGPUVectorIndex::search: not initialized or no GPU indices available (initialized={} gpu_count={})",
                        initialized,static_cast<int>(gpuIndices.size()));
            return {};
        }
        
        auto searchStart = std::chrono::steady_clock::now();
        
        // Broadcast query to all GPUs and collect results
        std::vector<MultiGPUVectorIndex::SearchResult> allResults;
        
        for (size_t gpuIdx = 0; gpuIdx <static_cast<int>(gpuIndices.size()); ++gpuIdx) {
            auto gpuStart = std::chrono::steady_clock::now();
            auto gpuResults = gpuIndices[gpuIdx]->search(query, k);
            auto gpuEnd = std::chrono::steady_clock::now();

            // Accumulate per-GPU active query time for utilization tracking
            if (static_cast<int>(perGpuQueryTimeUs.size()) > gpuIdx) {
                uint64_t gpuUs = static_cast<uint64_t>(
                    std::chrono::duration_cast<std::chrono::microseconds>(gpuEnd - gpuStart).count());
                std::lock_guard<std::mutex> lock(statsMutex);
                perGpuQueryTimeUs[gpuIdx] += gpuUs;
            }

            // Convert to MultiGPU results with source GPU info
            for (const auto& result : gpuResults) {
                MultiGPUVectorIndex::SearchResult mgpuResult;
                mgpuResult.id = result.id;
                mgpuResult.distance = result.distance;
                mgpuResult.sourceGPU = activeDeviceIds[gpuIdx];
                allResults.push_back(mgpuResult);
            }
        }
        
        // Merge and select top-k from all GPUs
        if (static_cast<int>(allResults.size()) > k) {
            std::partial_sort(allResults.begin(), allResults.begin() + k, allResults.end(),
                [](const auto& a, const auto& b) { return a.distance < b.distance; });
            allResults.resize(k);
        } else {
            std::sort(allResults.begin(), allResults.end(),
                [](const auto& a, const auto& b) { return a.distance < b.distance; });
        }
        
        auto searchEnd = std::chrono::steady_clock::now();
        updateQueryStats(searchStart, searchEnd);
        
        return allResults;
    }
    
    std::vector<std::vector<MultiGPUVectorIndex::SearchResult>> searchBatch(
        const std::vector<std::vector<float>>& queries, size_t k) {
        std::lock_guard<std::mutex> topologyLock(topologyMutex);
        if (!initialized || gpuIndices.empty() || queries.empty()) {
            THEMIS_WARN("MultiGPUVectorIndex::searchBatch: invalid state (initialized={} gpu_count={} queries={})",
                        initialized,static_cast<int>(gpuIndices.size()),static_cast<int>(queries.size()));
            return {};
        }

        const size_t numQueries = queries.size();
        const size_t numGPUs = gpuIndices.size();

        // Launch one async task per GPU; each GPU processes the full query batch.
        // This fans out the broadcast-and-merge pattern of search() across GPUs in
        // parallel so that per-GPU query time does not compound.
        using GPUResults = std::vector<std::vector<GPUVectorIndex::SearchResult>>;
        std::vector<std::future<GPUResults>> futures;
        futures.reserve(numGPUs);

        auto batchStart = std::chrono::steady_clock::now();

        for (size_t gpuIdx = 0; gpuIdx < numGPUs; ++gpuIdx) {
            futures.push_back(std::async(std::launch::async, [this, gpuIdx, &queries, k]() {
                auto gpuStart = std::chrono::steady_clock::now();
                auto res = gpuIndices[gpuIdx]->searchBatch(queries, k);
                auto gpuEnd = std::chrono::steady_clock::now();

                // Record per-GPU active time
                if (static_cast<int>(perGpuQueryTimeUs.size()) > gpuIdx) {
                    uint64_t gpuUs = static_cast<uint64_t>(
                        std::chrono::duration_cast<std::chrono::microseconds>(
                            gpuEnd - gpuStart).count());
                    std::lock_guard<std::mutex> lock(statsMutex);
                    perGpuQueryTimeUs[gpuIdx] += gpuUs;
                }
                return res;
            }));
        }

        // Collect results from all GPUs: perGpuResults[gpuIdx][queryIdx]
        std::vector<GPUResults> perGpuResults;
        perGpuResults.reserve(numGPUs);
        for (auto& f : futures) {
            perGpuResults.push_back(f.get());
        }

        auto batchEnd = std::chrono::steady_clock::now();

        // Merge per-query results across GPUs and take top-k
        std::vector<std::vector<MultiGPUVectorIndex::SearchResult>> results(numQueries);
        for (size_t qi = 0; qi < numQueries; ++qi) {
            std::vector<MultiGPUVectorIndex::SearchResult> allResults = {};

            for (size_t gi = 0; gi < numGPUs; ++gi) {
                if (qi < perGpuResults[gi].size()) {
                    for (const auto& r : perGpuResults[gi][qi]) {
                        MultiGPUVectorIndex::SearchResult mgpuResult;
                        mgpuResult.id = r.id;
                        mgpuResult.distance = r.distance;
                        mgpuResult.sourceGPU = activeDeviceIds[gi];
                        allResults.push_back(mgpuResult);
                    }
                }
            }
            if (static_cast<int>(allResults.size()) > k) {
                std::partial_sort(allResults.begin(), allResults.begin() + k,
                    allResults.end(),
                    [](const auto& a, const auto& b) { return a.distance < b.distance; });
                allResults.resize(k);
            } else {
                std::sort(allResults.begin(), allResults.end(),
                    [](const auto& a, const auto& b) { return a.distance < b.distance; });
            }
            results[qi] = std::move(allResults);
        }

        // Update aggregate stats: count each query in the batch as one query
        {
            std::lock_guard<std::mutex> lock(statsMutex);
            double batchMs = std::chrono::duration_cast<std::chrono::microseconds>(
                batchEnd - batchStart).count() / 1000.0;
            totalQueries += numQueries;
            totalQueryTimeMs += batchMs;
        }

        return results;
    }
    
    void updateQueryStats(const std::chrono::steady_clock::time_point& start,
                         const std::chrono::steady_clock::time_point& end) {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double queryTimeMs = duration.count() / 1000.0;
        
        std::lock_guard<std::mutex> lock(statsMutex);
        totalQueries++;
        totalQueryTimeMs += queryTimeMs;
    }
    
    Statistics getStatistics() const {
        // Snapshot mutable topology and stats under lock
        std::lock_guard<std::mutex> topologyLock(topologyMutex);
        std::lock_guard<std::mutex> statsLock(statsMutex);

        Statistics stats;
        stats.totalVectors = vectorToGPU.size();
        stats.totalDimension = dimension;
        stats.numActiveGPUs = activeDeviceIds.size();
        stats.numFailedGPUs = failedDeviceIds.size();
        
        // Calculate average query time
        if (totalQueries > 0) {
            stats.avgQueryTimeMs = totalQueryTimeMs / totalQueries;
        }
        
        // Calculate throughput
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - startTime);
        double elapsedSeconds = elapsed.count() / 1e6;
        if (elapsedSeconds > 0.0) {
            stats.throughputQPS = totalQueries / elapsedSeconds;
        }

        // Total elapsed microseconds used to normalize per-GPU utilization
        double elapsedUs = static_cast<double>(elapsed.count());
        
        // Per-GPU statistics
        size_t maxVectors = 0;
        size_t totalVectorsOnGPUs = 0;
        
        for (size_t i = 0; i <static_cast<int>(gpuIndices.size()); ++i) {
            auto gpuStats = gpuIndices[i]->getStatistics();
            
            GPUStatistics perGPUStat;
            perGPUStat.deviceId = activeDeviceIds[i];
            perGPUStat.numVectors = gpuStats.numVectors;
            perGPUStat.vramUsageBytes = gpuStats.vramUsageBytes;
            perGPUStat.avgQueryTimeMs = gpuStats.avgQueryTimeMs;

            // Utilisation: fraction of wall-clock time this GPU was actively
            // processing search requests, expressed as a percentage (0–100).
            if (elapsedUs > 0.0  && static_cast<size_t>(i) <static_cast<int>(perGpuQueryTimeUs.size())) {
                double activeUs = static_cast<double>(perGpuQueryTimeUs[i]);
                perGPUStat.utilizationPercent =
                    std::min(100.0, (activeUs / elapsedUs) * 100.0);
            } else {
                perGPUStat.utilizationPercent = 0.0;
            }

            perGPUStat.isActive = true;
            perGPUStat.hasFailed = false;
            
            stats.perGPUStats.push_back(perGPUStat);
            
            totalVectorsOnGPUs += gpuStats.numVectors;
            if (gpuStats.numVectors > maxVectors) {
                maxVectors = gpuStats.numVectors;
            }
        }
        
        // Calculate load imbalance
        if (!gpuIndices.empty() && totalVectorsOnGPUs > 0) {
            double avgVectors = static_cast<double>(totalVectorsOnGPUs) / gpuIndices.size();
            if (avgVectors > 0) {
                stats.loadImbalance = (maxVectors / avgVectors) - 1.0;
            }
        }
        
        // Calculate scaling efficiency
        // Ideal speedup = number of GPUs
        // Actual speedup estimated from query time improvements
        if (static_cast<int>(gpuIndices.size()) > 1) {
            // Simplified: assume linear scaling as baseline
            double idealSpeedup = static_cast<double>(gpuIndices.size());
            // For now, use a simple estimate based on load balance
            double actualSpeedup = gpuIndices.size() / (1.0 + stats.loadImbalance * 0.5);
            stats.scalingEfficiency = actualSpeedup / idealSpeedup;
        } else {
            stats.scalingEfficiency = 1.0;
        }
        
        return stats;
    }
    
    /**
     * Rebalance vectors across GPUs
     * 
     * NOTE: Current implementation is a placeholder for v2.4.
     * Full rebalancing with data migration will be implemented in v2.5+
     * with NCCL/RCCL support for efficient GPU-to-GPU transfers.
     * 
     * For now, this method:
     * 1. Validates that rebalancing is possible
     * 2. Logs current load distribution
     * 3. Returns success if system is operational
     * 
     * Future implementation will:
     * - Collect all vectors from all GPUs
     * - Redistribute according to partition strategy
     * - Transfer vectors using P2P or NCCL/RCCL
     * - Update routing tables
     */
    bool rebalance() {
        std::lock_guard<std::mutex> topologyLock(topologyMutex);
        if (!initialized || static_cast<int>(gpuIndices.size()) <= 1) {
            return false;
        }
        
        THEMIS_INFO("MultiGPUVectorIndex: Rebalancing vectors across {} GPUs...",static_cast<int>(gpuIndices.size()));
        
        // Get current load distribution
        std::vector<size_t> vectorsPerGPU = {};

        for (size_t i = 0; i <static_cast<int>(gpuIndices.size()); ++i) {
            auto stats = gpuIndices[i]->getStatistics();
            vectorsPerGPU.push_back(stats.numVectors);
            THEMIS_INFO("MultiGPUVectorIndex: Partition {} (Device {}): {} vectors",
                        i, activeDeviceIds[i], stats.numVectors);
        }
        
        // Calculate load imbalance
        size_t maxVectors = *std::max_element(vectorsPerGPU.begin(), vectorsPerGPU.end());
        size_t minVectors = *std::min_element(vectorsPerGPU.begin(), vectorsPerGPU.end());
        size_t totalVectors = 0;
        for (size_t count : vectorsPerGPU) {
            totalVectors += count;
        }
        
        if (totalVectors > 0) {
            double avgVectors = static_cast<double>(totalVectors) / gpuIndices.size();
            double imbalance = (maxVectors - minVectors) / avgVectors;
            THEMIS_INFO("MultiGPUVectorIndex: Load imbalance: {:.1f}%", imbalance * 100.0);
            
            if (imbalance < 0.1) {  // Less than 10% imbalance
                THEMIS_INFO("MultiGPUVectorIndex: Load is already well balanced, no action needed");
            } else {
                THEMIS_INFO("MultiGPUVectorIndex: Full rebalancing with data migration will be implemented in v2.5+");
            }
        }
        
        THEMIS_INFO("MultiGPUVectorIndex: Rebalancing check complete");
        return true;
    }
};

// =============================================================================
// MultiGPUVectorIndex public interface
// =============================================================================

MultiGPUVectorIndex::MultiGPUVectorIndex()
    : MultiGPUVectorIndex(Config{}) {
}

MultiGPUVectorIndex::MultiGPUVectorIndex(const Config& config)
    : pImpl(std::make_unique<Impl>(config)) {
}

MultiGPUVectorIndex::~MultiGPUVectorIndex() = default;

bool MultiGPUVectorIndex::initialize([[maybe_unused]] int dimension) {
    return pImpl->initialize(dimension);
}

void MultiGPUVectorIndex::shutdown() {
    pImpl->shutdown();
}

bool MultiGPUVectorIndex::addVector(const std::string& id, const std::vector<float>& vector) {
    return pImpl->addVector(id, vector);
}

bool MultiGPUVectorIndex::addVectorBatch(const std::vector<std::string>& ids,
                                        const std::vector<std::vector<float>>& vectors) {
    if (static_cast<int>(ids.size()) != vectors.size()) {
        return false;
    }
    
    bool allSuccess = true;
    for (size_t i = 0; i <static_cast<int>(ids.size()); ++i) {
        if (!addVector(ids[i], vectors[i])) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

bool MultiGPUVectorIndex::removeVector(const std::string& id) {
    return pImpl->removeVector(id);
}

bool MultiGPUVectorIndex::updateVector(const std::string& id, const std::vector<float>& vector) {
    return pImpl->addVector(id, vector);  // Upsert semantics
}

std::vector<MultiGPUVectorIndex::SearchResult> MultiGPUVectorIndex::search(
    const std::vector<float>& query, size_t k) {
    return pImpl->search(query, k);
}

std::vector<std::vector<MultiGPUVectorIndex::SearchResult>> MultiGPUVectorIndex::searchBatch(
    const std::vector<std::vector<float>>& queries, size_t k) {
    return pImpl->searchBatch(queries, k);
}

bool MultiGPUVectorIndex::addGPU([[maybe_unused]] int deviceId) {
    std::lock_guard<std::mutex> topologyLock(pImpl->topologyMutex);
    if (!pImpl->initialized) {
        return false;
    }
    
    // Check if GPU is already active
    auto it = std::find(pImpl->activeDeviceIds.begin(), pImpl->activeDeviceIds.end(), deviceId);
    if (it != pImpl->activeDeviceIds.end()) {
        return false;  // Already added
    }
    
    return pImpl->initializeGPU(deviceId);
}

bool MultiGPUVectorIndex::removeGPU([[maybe_unused]] int deviceId) {
    std::lock_guard<std::mutex> topologyLock(pImpl->topologyMutex);
    if (!pImpl->initialized) {
        return false;
    }
    
    auto it = std::find(pImpl->activeDeviceIds.begin(), pImpl->activeDeviceIds.end(), deviceId);
    if (it == pImpl->activeDeviceIds.end()) {
        return false;  // Not found
    }
    
    size_t idx = std::distance(pImpl->activeDeviceIds.begin(), it);
    
    // Clean up and adjust vector-to-GPU mappings
    // Remove mappings for vectors that were on the removed GPU, and
    // decrement GPU indices for GPUs that were after the removed one.
    auto mapIt = pImpl->vectorToGPU.begin();
    while (mapIt != pImpl->vectorToGPU.end()) {
        auto &gpuIndexRef = mapIt->second;
        if (gpuIndexRef == static_cast<int>(idx)) {
            // This vector was assigned to the removed GPU; drop the mapping.
            mapIt = pImpl->vectorToGPU.erase(mapIt);
            continue;
        }
        if (gpuIndexRef > static_cast<int>(idx)) {
            // Shift indices down to account for the erased GPU slot.
            --gpuIndexRef;
        }
        ++mapIt;
    }
    
    // Remove GPU index
    pImpl->gpuIndices.erase(pImpl->gpuIndices.begin() + idx);
    pImpl->activeDeviceIds.erase(it);
    
    return true;
}

bool MultiGPUVectorIndex::rebalance() {
    return pImpl->rebalance();
}

MultiGPUVectorIndex::Statistics MultiGPUVectorIndex::getStatistics() const {
    return pImpl->getStatistics();
}

std::vector<int> MultiGPUVectorIndex::getActiveGPUs() const {
    std::lock_guard<std::mutex> topologyLock(pImpl->topologyMutex);
    return pImpl->activeDeviceIds;
}

std::vector<int> MultiGPUVectorIndex::getFailedGPUs() const {
    std::lock_guard<std::mutex> topologyLock(pImpl->topologyMutex);
    return pImpl->failedDeviceIds;
}

void MultiGPUVectorIndex::setPartitionStrategy(PartitionStrategy strategy) {
    std::lock_guard<std::mutex> topologyLock(pImpl->topologyMutex);
    pImpl->config.partitionStrategy = strategy;
}

void MultiGPUVectorIndex::setLoadBalancingMode(LoadBalancingMode mode) {
    std::lock_guard<std::mutex> topologyLock(pImpl->topologyMutex);
    pImpl->config.loadBalancing = mode;
}

void MultiGPUVectorIndex::setEfSearch([[maybe_unused]] int ef) {
    std::lock_guard<std::mutex> topologyLock(pImpl->topologyMutex);
    pImpl->config.efSearch = ef;
    for (auto& gpuIndex : pImpl->gpuIndices) {
        gpuIndex->setEfSearch(ef);
    }
}

MultiGPUVectorIndex::PartitionStrategy MultiGPUVectorIndex::getPartitionStrategy() const {
    std::lock_guard<std::mutex> topologyLock(pImpl->topologyMutex);
    return pImpl->config.partitionStrategy;
}

// Communication backend control (v2.5+)
MultiGPUVectorIndex::CommBackend MultiGPUVectorIndex::getCommBackend() const {
    std::lock_guard<std::mutex> topologyLock(pImpl->topologyMutex);
    return pImpl->activeCommBackend;
}

bool MultiGPUVectorIndex::isCollectiveOpsAvailable() const {
    std::lock_guard<std::mutex> topologyLock(pImpl->topologyMutex);
#if defined(THEMIS_ENABLE_NCCL) || defined(THEMIS_ENABLE_RCCL)
    return (pImpl->activeCommBackend == CommBackend::NCCL || 
            pImpl->activeCommBackend == CommBackend::RCCL);
#else
    return false;
#endif
}

bool MultiGPUVectorIndex::isP2PTransferAvailable() const {
    std::lock_guard<std::mutex> topologyLock(pImpl->topologyMutex);
    if (!pImpl->config.enableP2P) {
      return false;
    }
    
#ifdef THEMIS_ENABLE_NCCL
    if (pImpl->activeCommBackend == CommBackend::NCCL && pImpl->ncclBackend) {
        return pImpl->ncclBackend->isP2PEnabled();
    }
#endif
#ifdef THEMIS_ENABLE_RCCL
    if (pImpl->activeCommBackend == CommBackend::RCCL && pImpl->rcclBackend) {
        return pImpl->rcclBackend->isP2PEnabled();
    }
#endif
    
    return false;
}

bool MultiGPUVectorIndex::isNVLinkAvailable() const {
    std::lock_guard<std::mutex> topologyLock(pImpl->topologyMutex);
#ifdef THEMIS_ENABLE_NCCL
    if (pImpl->activeCommBackend == CommBackend::NCCL && pImpl->ncclBackend) {
        auto stats = pImpl->ncclBackend->getStatistics();
        return stats.nvlinkAvailable;
    }
#endif
    return false;
}

bool MultiGPUVectorIndex::isXGMIAvailable() const {
    std::lock_guard<std::mutex> topologyLock(pImpl->topologyMutex);
#ifdef THEMIS_ENABLE_RCCL
    if (pImpl->activeCommBackend == CommBackend::RCCL && pImpl->rcclBackend) {
        auto stats = pImpl->rcclBackend->getStatistics();
        return stats.xgmiAvailable;
    }
#endif
    return false;
}

} // namespace index
} // namespace themis
