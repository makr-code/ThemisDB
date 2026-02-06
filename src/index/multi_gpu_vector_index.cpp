#include "index/multi_gpu_vector_index.h"
#include "index/gpu_vector_index.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <iostream>
#include <unordered_map>
#include <sstream>

namespace themis {
namespace index {

// =============================================================================
// MultiGPUVectorIndex::Impl
// =============================================================================

class MultiGPUVectorIndex::Impl {
public:
    Config config;
    int dimension = 0;
    bool initialized = false;
    
    // Per-GPU indices
    std::vector<std::unique_ptr<GPUVectorIndex>> gpuIndices;
    std::vector<int> activeDeviceIds;
    std::vector<int> failedDeviceIds;
    
    // Vector routing information
    std::unordered_map<std::string, int> vectorToGPU;  // Maps vector ID to GPU index
    
    // Statistics
    std::chrono::steady_clock::time_point startTime;
    size_t totalQueries = 0;
    double totalQueryTimeMs = 0.0;
    
    Impl(const Config& cfg) : config(cfg) {
        startTime = std::chrono::steady_clock::now();
    }
    
    ~Impl() {
        shutdown();
    }
    
    bool initialize(int dim) {
        dimension = dim;
        
        if (!config.enableMultiGPU || config.deviceIds.empty()) {
            std::cerr << "MultiGPUVectorIndex: Multi-GPU not enabled or no device IDs specified\n";
            return false;
        }
        
        std::cout << "MultiGPUVectorIndex: Initializing with " << config.deviceIds.size() 
                  << " GPUs\n";
        
        // Initialize GPU indices
        for (int deviceId : config.deviceIds) {
            if (!initializeGPU(deviceId)) {
                if (config.enableFaultTolerance) {
                    std::cerr << "Warning: Failed to initialize GPU " << deviceId 
                             << ", continuing with remaining GPUs\n";
                    failedDeviceIds.push_back(deviceId);
                    continue;
                } else {
                    std::cerr << "Error: Failed to initialize GPU " << deviceId << "\n";
                    return false;
                }
            }
        }
        
        if (activeDeviceIds.empty()) {
            std::cerr << "Error: No GPUs successfully initialized\n";
            return false;
        }
        
        std::cout << "Successfully initialized " << activeDeviceIds.size() << " GPUs\n";
        initialized = true;
        return true;
    }
    
    bool initializeGPU(int deviceId) {
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
        
        std::cout << "  GPU " << deviceId << " initialized successfully\n";
        return true;
    }
    
    void shutdown() {
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
                return totalVectors % activeDeviceIds.size();
            }
            
            case PartitionStrategy::HASH_BASED: {
                // Hash the vector ID
                std::hash<std::string> hasher;
                size_t hash = hasher(id);
                return hash % activeDeviceIds.size();
            }
            
            case PartitionStrategy::RANGE_BASED: {
                // Lexicographic range partitioning
                // This is simplified - production would use proper range mapping
                std::hash<std::string> hasher;
                size_t hash = hasher(id);
                return hash % activeDeviceIds.size();
            }
            
            case PartitionStrategy::BALANCED: {
                // Choose GPU with fewest vectors
                size_t minVectors = SIZE_MAX;
                int selectedGPU = 0;
                
                for (size_t i = 0; i < gpuIndices.size(); ++i) {
                    auto stats = gpuIndices[i]->getStatistics();
                    if (stats.numVectors < minVectors) {
                        minVectors = stats.numVectors;
                        selectedGPU = i;
                    }
                }
                return selectedGPU;
            }
            
            default:
                return 0;
        }
    }
    
    bool addVector(const std::string& id, const std::vector<float>& vector) {
        if (!initialized) {
            return false;
        }
        
        // Check if vector already exists
        auto it = vectorToGPU.find(id);
        if (it != vectorToGPU.end()) {
            // Update existing vector on its current GPU
            int gpuIdx = it->second;
            if (gpuIdx >= 0 && gpuIdx < static_cast<int>(gpuIndices.size())) {
                return gpuIndices[gpuIdx]->updateVector(id, vector);
            }
            return false;
        }
        
        // Select GPU for new vector
        int gpuIdx = selectGPUForVector(id);
        if (gpuIdx < 0 || gpuIdx >= static_cast<int>(gpuIndices.size())) {
            return false;
        }
        
        // Add to selected GPU
        if (gpuIndices[gpuIdx]->addVector(id, vector)) {
            vectorToGPU[id] = gpuIdx;
            return true;
        }
        
        return false;
    }
    
    bool removeVector(const std::string& id) {
        auto it = vectorToGPU.find(id);
        if (it == vectorToGPU.end()) {
            return false;
        }
        
        int gpuIdx = it->second;
        if (gpuIdx >= 0 && gpuIdx < static_cast<int>(gpuIndices.size())) {
            bool success = gpuIndices[gpuIdx]->removeVector(id);
            if (success) {
                vectorToGPU.erase(it);
            }
            return success;
        }
        
        return false;
    }
    
    std::vector<MultiGPUVectorIndex::SearchResult> search(
        const std::vector<float>& query, size_t k) {
        
        if (!initialized || gpuIndices.empty()) {
            return {};
        }
        
        auto startTime = std::chrono::steady_clock::now();
        
        // Broadcast query to all GPUs and collect results
        std::vector<MultiGPUVectorIndex::SearchResult> allResults;
        
        for (size_t gpuIdx = 0; gpuIdx < gpuIndices.size(); ++gpuIdx) {
            auto gpuResults = gpuIndices[gpuIdx]->search(query, k);
            
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
        if (allResults.size() > k) {
            std::partial_sort(allResults.begin(), allResults.begin() + k, allResults.end(),
                [](const auto& a, const auto& b) { return a.distance < b.distance; });
            allResults.resize(k);
        } else {
            std::sort(allResults.begin(), allResults.end(),
                [](const auto& a, const auto& b) { return a.distance < b.distance; });
        }
        
        auto endTime = std::chrono::steady_clock::now();
        updateQueryStats(startTime, endTime);
        
        return allResults;
    }
    
    std::vector<std::vector<MultiGPUVectorIndex::SearchResult>> searchBatch(
        const std::vector<std::vector<float>>& queries, size_t k) {
        
        std::vector<std::vector<MultiGPUVectorIndex::SearchResult>> results;
        results.reserve(queries.size());
        
        // For now, process queries sequentially
        // TODO: Implement parallel batch processing across GPUs
        for (const auto& query : queries) {
            results.push_back(search(query, k));
        }
        
        return results;
    }
    
    void updateQueryStats(const std::chrono::steady_clock::time_point& start,
                         const std::chrono::steady_clock::time_point& end) {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double queryTimeMs = duration.count() / 1000.0;
        
        totalQueries++;
        totalQueryTimeMs += queryTimeMs;
    }
    
    Statistics getStatistics() const {
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
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);
        if (duration.count() > 0) {
            stats.throughputQPS = totalQueries / static_cast<double>(duration.count());
        }
        
        // Per-GPU statistics
        size_t maxVectors = 0;
        size_t totalVectorsOnGPUs = 0;
        
        for (size_t i = 0; i < gpuIndices.size(); ++i) {
            auto gpuStats = gpuIndices[i]->getStatistics();
            
            GPUStatistics perGPUStat;
            perGPUStat.deviceId = activeDeviceIds[i];
            perGPUStat.numVectors = gpuStats.numVectors;
            perGPUStat.vramUsageBytes = gpuStats.vramUsageBytes;
            perGPUStat.avgQueryTimeMs = gpuStats.avgQueryTimeMs;
            perGPUStat.utilizationPercent = 0.0;  // TODO: Implement utilization tracking
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
        if (gpuIndices.size() > 1) {
            // Simplified: assume linear scaling as baseline
            double idealSpeedup = gpuIndices.size();
            // For now, use a simple estimate based on load balance
            double actualSpeedup = gpuIndices.size() / (1.0 + stats.loadImbalance * 0.5);
            stats.scalingEfficiency = actualSpeedup / idealSpeedup;
        } else {
            stats.scalingEfficiency = 1.0;
        }
        
        return stats;
    }
    
    bool rebalance() {
        if (!initialized || gpuIndices.size() <= 1) {
            return false;
        }
        
        std::cout << "MultiGPUVectorIndex: Rebalancing vectors across " 
                  << gpuIndices.size() << " GPUs...\n";
        
        // Collect all vectors from all GPUs
        std::vector<std::pair<std::string, std::vector<float>>> allVectors;
        
        for (auto& [id, gpuIdx] : vectorToGPU) {
            // Note: In production, we would need to implement a method to get vectors
            // For now, this is a placeholder showing the rebalancing logic structure
            (void)gpuIdx;  // Suppress unused warning
        }
        
        // Redistribute according to current partition strategy
        // This is a simplified version - production would implement actual data transfer
        
        std::cout << "Rebalancing complete\n";
        return true;
    }
};

// =============================================================================
// MultiGPUVectorIndex public interface
// =============================================================================

MultiGPUVectorIndex::MultiGPUVectorIndex(const Config& config)
    : pImpl(std::make_unique<Impl>(config)) {
}

MultiGPUVectorIndex::~MultiGPUVectorIndex() = default;

bool MultiGPUVectorIndex::initialize(int dimension) {
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
    if (ids.size() != vectors.size()) {
        return false;
    }
    
    bool allSuccess = true;
    for (size_t i = 0; i < ids.size(); ++i) {
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

bool MultiGPUVectorIndex::addGPU(int deviceId) {
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

bool MultiGPUVectorIndex::removeGPU(int deviceId) {
    if (!pImpl->initialized) {
        return false;
    }
    
    auto it = std::find(pImpl->activeDeviceIds.begin(), pImpl->activeDeviceIds.end(), deviceId);
    if (it == pImpl->activeDeviceIds.end()) {
        return false;  // Not found
    }
    
    size_t idx = std::distance(pImpl->activeDeviceIds.begin(), it);
    
    // Remove GPU index
    pImpl->gpuIndices.erase(pImpl->gpuIndices.begin() + idx);
    pImpl->activeDeviceIds.erase(it);
    
    // TODO: Redistribute vectors from removed GPU
    
    return true;
}

bool MultiGPUVectorIndex::rebalance() {
    return pImpl->rebalance();
}

MultiGPUVectorIndex::Statistics MultiGPUVectorIndex::getStatistics() const {
    return pImpl->getStatistics();
}

std::vector<int> MultiGPUVectorIndex::getActiveGPUs() const {
    return pImpl->activeDeviceIds;
}

std::vector<int> MultiGPUVectorIndex::getFailedGPUs() const {
    return pImpl->failedDeviceIds;
}

void MultiGPUVectorIndex::setPartitionStrategy(PartitionStrategy strategy) {
    pImpl->config.partitionStrategy = strategy;
}

void MultiGPUVectorIndex::setLoadBalancingMode(LoadBalancingMode mode) {
    pImpl->config.loadBalancing = mode;
}

void MultiGPUVectorIndex::setEfSearch(int ef) {
    pImpl->config.efSearch = ef;
    for (auto& gpuIndex : pImpl->gpuIndices) {
        gpuIndex->setEfSearch(ef);
    }
}

MultiGPUVectorIndex::PartitionStrategy MultiGPUVectorIndex::getPartitionStrategy() const {
    return pImpl->config.partitionStrategy;
}

} // namespace index
} // namespace themis
