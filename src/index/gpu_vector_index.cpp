#include "index/gpu_vector_index.h"
#include "acceleration/compute_backend.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <unordered_map>

#ifdef THEMIS_ENABLE_VULKAN
#include "llm/lora_framework/vulkan_context.h"
#include "llm/lora_framework/vulkan_buffer.h"
#include "llm/lora_framework/vulkan_pipeline.h"
#endif

// Forward declare Vulkan backend
#ifdef THEMIS_ENABLE_VULKAN
namespace themis {
namespace index {
class VulkanVectorIndexBackend;
}
}
#endif

namespace themis {
namespace index {

// =============================================================================
// GPUVectorIndex::Impl
// =============================================================================

class GPUVectorIndex::Impl {
public:
    Config config;
    int dimension = 0;
    bool initialized = false;
    Backend activeBackend = Backend::CPU;
    
    // Vector storage (CPU-side)
    std::vector<std::string> vectorIds;
    std::vector<std::vector<float>> vectorData;
    std::unordered_map<std::string, size_t> idToIndex;
    
    // Backend implementations
    #ifdef THEMIS_ENABLE_VULKAN
    std::unique_ptr<VulkanVectorIndexBackend> vulkanBackend;
    #endif
    
    // Statistics
    Statistics stats;
    std::chrono::steady_clock::time_point lastQueryTime;
    size_t queryCount = 0;
    double totalQueryTimeMs = 0.0;
    
    Impl(const Config& cfg) : config(cfg) {}
    
    ~Impl() {
        shutdown();
    }
    
    bool initialize(int dim) {
        dimension = dim;
        stats.dimension = dim;
        
        // Determine which backend to use
        Backend requestedBackend = config.backend;
        if (requestedBackend == Backend::AUTO) {
            requestedBackend = selectBestBackend();
        }
        
        // Try to initialize requested backend
        bool backendInitialized = false;
        
        #ifdef THEMIS_ENABLE_VULKAN
        if (requestedBackend == Backend::VULKAN) {
            backendInitialized = initializeVulkanBackend(dim);
            if (backendInitialized) {
                activeBackend = Backend::VULKAN;
                stats.isGPUActive = true;
                std::cout << "GPUVectorIndex: Using Vulkan backend\n";
            }
        }
        #endif
        
        // Fall back to CPU if requested backend failed or not available
        if (!backendInitialized) {
            if (requestedBackend != Backend::CPU && !config.allowCPUFallback) {
                std::cerr << "GPUVectorIndex: Requested backend not available and CPU fallback disabled\n";
                return false;
            }
            
            activeBackend = Backend::CPU;
            stats.isGPUActive = false;
            if (requestedBackend != Backend::CPU) {
                std::cout << "GPUVectorIndex: Falling back to CPU backend\n";
            } else {
                std::cout << "GPUVectorIndex: Using CPU backend\n";
            }
        }
        
        stats.activeBackend = activeBackend;
        initialized = true;
        return true;
    }
    
    void shutdown() {
        #ifdef THEMIS_ENABLE_VULKAN
        if (vulkanBackend) {
            vulkanBackend.reset();
        }
        #endif
        initialized = false;
    }
    
    Backend selectBestBackend() {
        // Try Vulkan first (cross-platform)
        #ifdef THEMIS_ENABLE_VULKAN
        if (isVulkanAvailable()) {
            return Backend::VULKAN;
        }
        #endif
        
        // Fall back to CPU
        return Backend::CPU;
    }
    
    #ifdef THEMIS_ENABLE_VULKAN
    bool isVulkanAvailable() {
        // Check if Vulkan is available by trying to create a context
        try {
            lora::vulkan::VulkanContext testContext;
            return testContext.is_available();
        } catch (...) {
            return false;
        }
    }
    
    bool initializeVulkanBackend(int dim) {
        try {
            vulkanBackend = std::make_unique<VulkanVectorIndexBackend>(config);
            if (!vulkanBackend->initialize(dim)) {
                vulkanBackend.reset();
                return false;
            }
            
            // Upload existing vectors to GPU
            if (!vectorData.empty()) {
                return vulkanBackend->uploadVectors(vectorData);
            }
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "GPUVectorIndex: Vulkan initialization failed: " << e.what() << "\n";
            vulkanBackend.reset();
            return false;
        }
    }
    #endif
    
    bool addVector(const std::string& id, const std::vector<float>& vector) {
        if (!initialized || vector.size() != static_cast<size_t>(dimension)) {
            return false;
        }
        
        // Check if ID already exists
        auto it = idToIndex.find(id);
        if (it != idToIndex.end()) {
            // Update existing vector
            vectorData[it->second] = vector;
        } else {
            // Add new vector
            size_t index = vectorData.size();
            vectorIds.push_back(id);
            vectorData.push_back(vector);
            idToIndex[id] = index;
        }
        
        stats.numVectors = vectorData.size();
        
        // Update GPU backend if active
        #ifdef THEMIS_ENABLE_VULKAN
        if (activeBackend == Backend::VULKAN && vulkanBackend) {
            vulkanBackend->uploadVectors(vectorData);
        }
        #endif
        
        return true;
    }
    
    bool removeVector(const std::string& id) {
        auto it = idToIndex.find(id);
        if (it == idToIndex.end()) {
            return false;
        }
        
        size_t index = it->second;
        
        // Swap with last element and pop (to avoid shifting)
        size_t lastIndex = vectorData.size() - 1;
        if (index != lastIndex) {
            vectorIds[index] = vectorIds[lastIndex];
            vectorData[index] = vectorData[lastIndex];
            idToIndex[vectorIds[index]] = index;
        }
        
        vectorIds.pop_back();
        vectorData.pop_back();
        idToIndex.erase(id);
        
        stats.numVectors = vectorData.size();
        return true;
    }
    
    std::vector<SearchResult> search(const std::vector<float>& query, size_t k) {
        if (!initialized) {
            return {};
        }
        
        // Try GPU backend first if active
        #ifdef THEMIS_ENABLE_VULKAN
        if (activeBackend == Backend::VULKAN && vulkanBackend) {
            auto indices = vulkanBackend->searchIndices(query, k);
            if (!indices.empty()) {
                // Map indices to IDs
                std::vector<SearchResult> results;
                results.reserve(indices.size());
                for (const auto& [distance, index] : indices) {
                    if (index < vectorIds.size()) {
                        results.push_back({vectorIds[index], distance});
                    }
                }
                return results;
            }
            // Fall through to CPU if GPU search fails
            std::cerr << "GPUVectorIndex: Vulkan search failed, falling back to CPU\n";
        }
        #endif
        
        // Use CPU implementation
        return searchCPU(query, k);
    }
    
    std::vector<SearchResult> searchCPU(const std::vector<float>& query, size_t k) {
        if (vectorData.empty() || query.size() != static_cast<size_t>(dimension)) {
            return {};
        }
        
        auto startTime = std::chrono::steady_clock::now();
        
        // Compute distances
        std::vector<std::pair<float, size_t>> distances;
        distances.reserve(vectorData.size());
        
        for (size_t i = 0; i < vectorData.size(); ++i) {
            float dist = computeDistance(query.data(), vectorData[i].data(), dimension);
            distances.emplace_back(dist, i);
        }
        
        // Sort and take top-k
        size_t topK = std::min(k, distances.size());
        std::partial_sort(distances.begin(), distances.begin() + topK, distances.end(),
                         [](const auto& a, const auto& b) { return a.first < b.first; });
        
        std::vector<SearchResult> results;
        results.reserve(topK);
        for (size_t i = 0; i < topK; ++i) {
            results.push_back({vectorIds[distances[i].second], distances[i].first});
        }
        
        auto endTime = std::chrono::steady_clock::now();
        updateQueryStats(startTime, endTime);
        
        return results;
    }
    
    float computeDistance(const float* a, const float* b, int dim) {
        switch (config.metric) {
            case DistanceMetric::L2: {
                float sum = 0.0f;
                for (int i = 0; i < dim; ++i) {
                    float diff = a[i] - b[i];
                    sum += diff * diff;
                }
                return sum;
            }
            case DistanceMetric::COSINE: {
                float dot = 0.0f, normA = 0.0f, normB = 0.0f;
                for (int i = 0; i < dim; ++i) {
                    dot += a[i] * b[i];
                    normA += a[i] * a[i];
                    normB += b[i] * b[i];
                }
                float denominator = std::sqrt(normA * normB);
                if (denominator < 1e-10f) return 1.0f;
                return 1.0f - (dot / denominator);
            }
            case DistanceMetric::INNER_PRODUCT: {
                float dot = 0.0f;
                for (int i = 0; i < dim; ++i) {
                    dot += a[i] * b[i];
                }
                return std::max(0.0f, -dot);
            }
            default:
                return 0.0f;
        }
    }
    
    void updateQueryStats(const std::chrono::steady_clock::time_point& start,
                         const std::chrono::steady_clock::time_point& end) {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double queryTimeMs = duration.count() / 1000.0;
        
        queryCount++;
        totalQueryTimeMs += queryTimeMs;
        stats.avgQueryTimeMs = totalQueryTimeMs / queryCount;
        
        // Calculate throughput (QPS)
        if (queryCount > 1) {
            auto totalDuration = std::chrono::duration_cast<std::chrono::seconds>(end - lastQueryTime);
            if (totalDuration.count() > 0) {
                stats.throughputQPS = queryCount / static_cast<double>(totalDuration.count());
            }
        }
        lastQueryTime = end;
    }
    
    std::vector<Backend> getAvailableBackends() {
        std::vector<Backend> backends;
        backends.push_back(Backend::CPU); // Always available
        
        #ifdef THEMIS_ENABLE_VULKAN
        if (isVulkanAvailable()) {
            backends.push_back(Backend::VULKAN);
        }
        #endif
        
        return backends;
    }
};

// =============================================================================
// GPUVectorIndex public interface
// =============================================================================

GPUVectorIndex::GPUVectorIndex(const Config& config)
    : pImpl(std::make_unique<Impl>(config)) {
}

GPUVectorIndex::~GPUVectorIndex() = default;

bool GPUVectorIndex::initialize(int dimension) {
    return pImpl->initialize(dimension);
}

void GPUVectorIndex::shutdown() {
    pImpl->shutdown();
}

bool GPUVectorIndex::addVector(const std::string& id, const std::vector<float>& vector) {
    return pImpl->addVector(id, vector);
}

bool GPUVectorIndex::addVectorBatch(const std::vector<std::string>& ids,
                                   const std::vector<std::vector<float>>& vectors) {
    if (ids.size() != vectors.size()) {
        return false;
    }
    
    for (size_t i = 0; i < ids.size(); ++i) {
        if (!addVector(ids[i], vectors[i])) {
            return false;
        }
    }
    return true;
}

bool GPUVectorIndex::removeVector(const std::string& id) {
    return pImpl->removeVector(id);
}

bool GPUVectorIndex::updateVector(const std::string& id, const std::vector<float>& vector) {
    return pImpl->addVector(id, vector); // Same as add (upsert)
}

std::vector<GPUVectorIndex::SearchResult> GPUVectorIndex::search(
    const std::vector<float>& query, size_t k) {
    
    if (!pImpl->initialized) {
        return {};
    }
    
    return pImpl->search(query, k);
}

std::vector<std::vector<GPUVectorIndex::SearchResult>> GPUVectorIndex::searchBatch(
    const std::vector<std::vector<float>>& queries, size_t k) {
    
    std::vector<std::vector<SearchResult>> results;
    results.reserve(queries.size());
    
    for (const auto& query : queries) {
        results.push_back(search(query, k));
    }
    
    return results;
}

bool GPUVectorIndex::buildIndex() {
    // Index building happens automatically on GPU
    return true;
}

bool GPUVectorIndex::saveIndex(const std::string& path) {
    // TODO: Implement serialization
    (void)path;
    return false;
}

bool GPUVectorIndex::loadIndex(const std::string& path) {
    // TODO: Implement deserialization
    (void)path;
    return false;
}

void GPUVectorIndex::setEfSearch(int ef) {
    pImpl->config.efSearch = ef;
}

void GPUVectorIndex::setBatchSize(int size) {
    pImpl->config.batchSize = size;
}

GPUVectorIndex::Backend GPUVectorIndex::getActiveBackend() const {
    return pImpl->activeBackend;
}

GPUVectorIndex::Statistics GPUVectorIndex::getStatistics() const {
    return pImpl->stats;
}

bool GPUVectorIndex::switchBackend(Backend backend) {
    if (!pImpl->initialized) {
        return false;
    }
    
    // Can't switch if backend is not available
    auto available = getAvailableBackends();
    if (std::find(available.begin(), available.end(), backend) == available.end()) {
        std::cerr << "GPUVectorIndex: Requested backend not available\n";
        return false;
    }
    
    // Already using this backend
    if (pImpl->activeBackend == backend) {
        return true;
    }
    
    // Shutdown current backend
    int dim = pImpl->dimension;
    auto vectors = pImpl->vectorData;
    pImpl->shutdown();
    
    // Switch to new backend
    pImpl->config.backend = backend;
    if (!pImpl->initialize(dim)) {
        return false;
    }
    
    // Restore vectors
    for (size_t i = 0; i < vectors.size(); ++i) {
        pImpl->addVector(pImpl->vectorIds[i], vectors[i]);
    }
    
    return true;
}

std::vector<GPUVectorIndex::Backend> GPUVectorIndex::getAvailableBackends() const {
    return pImpl->getAvailableBackends();
}

// =============================================================================
// Vulkan Backend Implementation
// =============================================================================

#ifdef THEMIS_ENABLE_VULKAN
// Include the Vulkan backend implementation
// The actual implementation is in gpu_vector_index_vulkan.cpp

/**
 * @brief Vulkan backend implementation for GPU vector indexing
 */
class VulkanVectorIndexBackend {
public:
    explicit VulkanVectorIndexBackend(const GPUVectorIndex::Config& config);
    ~VulkanVectorIndexBackend();
    
    bool initialize(int dimension);
    void shutdown();
    bool uploadVectors(const std::vector<std::vector<float>>& vectors);
    std::vector<std::pair<float, size_t>> searchIndices(const std::vector<float>& query, size_t k);
    std::vector<GPUVectorIndex::SearchResult> search(const std::vector<float>& query, size_t k);
    std::vector<std::vector<GPUVectorIndex::SearchResult>> searchBatch(
        const std::vector<std::vector<float>>& queries, size_t k);
    GPUVectorIndex::Statistics getStatistics() const;
    bool isInitialized() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};
#endif

} // namespace index
} // namespace themis
