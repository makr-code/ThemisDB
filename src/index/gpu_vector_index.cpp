#include "index/gpu_vector_index.h"
#include "acceleration/compute_backend.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <unordered_map>

// Include GPU backend headers
#ifdef THEMIS_ENABLE_HIP
#include "acceleration/hip_backend.h"
#endif

#ifdef THEMIS_ENABLE_CUDA
#include "acceleration/cuda_backend.h"
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
    
    // GPU backend
#ifdef THEMIS_ENABLE_HIP
    std::unique_ptr<themis::acceleration::HIPVectorBackend> hipBackend;
#endif
#ifdef THEMIS_ENABLE_CUDA
    std::unique_ptr<themis::acceleration::CUDAVectorBackend> cudaBackend;
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
        
        // Attempt to initialize GPU backend based on config
        Backend requestedBackend = config.backend;
        
        if (requestedBackend == Backend::AUTO) {
            // Auto-detect best available backend
#ifdef THEMIS_ENABLE_HIP
            if (initializeHIP()) {
                activeBackend = Backend::HIP;
                std::cout << "GPUVectorIndex: Auto-selected HIP backend (AMD GPU)\n";
            } else
#endif
#ifdef THEMIS_ENABLE_CUDA  
            if (initializeCUDA()) {
                activeBackend = Backend::CUDA;
                std::cout << "GPUVectorIndex: Auto-selected CUDA backend (NVIDIA GPU)\n";
            } else
#endif
            {
                activeBackend = Backend::CPU;
                std::cout << "GPUVectorIndex: Using CPU backend (no GPU available)\n";
            }
        } else if (requestedBackend == Backend::HIP) {
#ifdef THEMIS_ENABLE_HIP
            if (initializeHIP()) {
                activeBackend = Backend::HIP;
                std::cout << "GPUVectorIndex: Using HIP backend (AMD GPU)\n";
            } else
#endif
            {
                if (config.allowCPUFallback) {
                    activeBackend = Backend::CPU;
                    std::cout << "GPUVectorIndex: HIP unavailable, falling back to CPU\n";
                } else {
                    std::cerr << "GPUVectorIndex: HIP backend requested but not available\n";
                    return false;
                }
            }
        } else if (requestedBackend == Backend::CUDA) {
#ifdef THEMIS_ENABLE_CUDA
            if (initializeCUDA()) {
                activeBackend = Backend::CUDA;
                std::cout << "GPUVectorIndex: Using CUDA backend (NVIDIA GPU)\n";
            } else
#endif
            {
                if (config.allowCPUFallback) {
                    activeBackend = Backend::CPU;
                    std::cout << "GPUVectorIndex: CUDA unavailable, falling back to CPU\n";
                } else {
                    std::cerr << "GPUVectorIndex: CUDA backend requested but not available\n";
                    return false;
                }
            }
        } else {
            // CPU backend explicitly requested
            activeBackend = Backend::CPU;
            std::cout << "GPUVectorIndex: Using CPU backend (explicitly requested)\n";
        }
        
        stats.activeBackend = activeBackend;
        stats.isGPUActive = (activeBackend != Backend::CPU);
        initialized = true;
        return true;
    }
    
    void shutdown() {
        // Shutdown GPU backends
#ifdef THEMIS_ENABLE_HIP
        if (hipBackend) {
            hipBackend->shutdown();
            hipBackend.reset();
        }
#endif
#ifdef THEMIS_ENABLE_CUDA
        if (cudaBackend) {
            cudaBackend->shutdown();
            cudaBackend.reset();
        }
#endif
        initialized = false;
    }
    
    // GPU backend initialization helpers
    bool initializeHIP() {
#ifdef THEMIS_ENABLE_HIP
        try {
            themis::acceleration::HIPVectorBackend::HIPConfig hipConfig;
            hipConfig.deviceId = config.deviceId;
            hipConfig.waveSize = config.waveSize;
            hipConfig.enableRocBLAS = config.enableRocBLAS;
            hipConfig.maxVRAM_MB = config.maxVRAM_MB;
            
            hipBackend = std::make_unique<themis::acceleration::HIPVectorBackend>(hipConfig);
            
            if (hipBackend->isAvailable() && hipBackend->initialize()) {
                return true;
            }
        } catch (const std::exception& e) {
            std::cerr << "HIP initialization failed: " << e.what() << std::endl;
        }
#endif
        return false;
    }
    
    bool initializeCUDA() {
#ifdef THEMIS_ENABLE_CUDA
        try {
            // Similar CUDA initialization would go here
            // For now, return false as CUDA backend is not implemented in this PR
            return false;
        } catch (const std::exception& e) {
            std::cerr << "CUDA initialization failed: " << e.what() << std::endl;
        }
#endif
        return false;
    }
    
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
    
    // GPU-accelerated search (HIP)
    std::vector<SearchResult> searchHIP(const std::vector<float>& query, size_t k) {
#ifdef THEMIS_ENABLE_HIP
        if (!hipBackend || vectorData.empty() || query.size() != static_cast<size_t>(dimension)) {
            return {};
        }
        
        auto startTime = std::chrono::steady_clock::now();
        
        // Flatten vector data for GPU
        std::vector<float> flatVectors(vectorData.size() * dimension);
        for (size_t i = 0; i < vectorData.size(); ++i) {
            std::copy(vectorData[i].begin(), vectorData[i].end(), 
                     flatVectors.begin() + i * dimension);
        }
        
        // Use GPU batch KNN search
        bool useL2 = (config.metric == DistanceMetric::L2);
        auto results = hipBackend->batchKnnSearch(
            query.data(), 1, dimension,
            flatVectors.data(), vectorData.size(),
            k, useL2
        );
        
        if (results.empty()) {
            // GPU failed, fall back to CPU if allowed
            if (config.allowCPUFallback) {
                std::cerr << "HIP search failed, falling back to CPU\n";
                return searchCPU(query, k);
            }
            return {};
        }
        
        // Convert results to SearchResult format
        std::vector<SearchResult> searchResults;
        searchResults.reserve(k);
        for (const auto& [idx, dist] : results[0]) {
            if (idx < vectorIds.size()) {
                searchResults.push_back({vectorIds[idx], dist});
            }
        }
        
        auto endTime = std::chrono::steady_clock::now();
        updateQueryStats(startTime, endTime);
        
        return searchResults;
#else
        // HIP not available, use CPU
        return searchCPU(query, k);
#endif
    }
    
    // Batch search with GPU acceleration
    std::vector<std::vector<SearchResult>> searchBatchHIP(
        const std::vector<std::vector<float>>& queries, size_t k) {
#ifdef THEMIS_ENABLE_HIP
        if (!hipBackend || vectorData.empty() || queries.empty()) {
            return {};
        }
        
        // Flatten queries
        std::vector<float> flatQueries(queries.size() * dimension);
        for (size_t i = 0; i < queries.size(); ++i) {
            if (queries[i].size() != static_cast<size_t>(dimension)) {
                return {}; // Invalid query dimensions
            }
            std::copy(queries[i].begin(), queries[i].end(), 
                     flatQueries.begin() + i * dimension);
        }
        
        // Flatten vector data for GPU
        std::vector<float> flatVectors(vectorData.size() * dimension);
        for (size_t i = 0; i < vectorData.size(); ++i) {
            std::copy(vectorData[i].begin(), vectorData[i].end(), 
                     flatVectors.begin() + i * dimension);
        }
        
        // Use GPU batch KNN search
        bool useL2 = (config.metric == DistanceMetric::L2);
        auto results = hipBackend->batchKnnSearch(
            flatQueries.data(), queries.size(), dimension,
            flatVectors.data(), vectorData.size(),
            k, useL2
        );
        
        if (results.empty()) {
            // GPU failed, fall back to CPU if allowed
            if (config.allowCPUFallback) {
                std::cerr << "HIP batch search failed, falling back to CPU\n";
                std::vector<std::vector<SearchResult>> cpuResults;
                cpuResults.reserve(queries.size());
                for (const auto& query : queries) {
                    cpuResults.push_back(searchCPU(query, k));
                }
                return cpuResults;
            }
            return {};
        }
        
        // Convert results to SearchResult format
        std::vector<std::vector<SearchResult>> searchResults(queries.size());
        for (size_t q = 0; q < queries.size(); ++q) {
            searchResults[q].reserve(k);
            for (const auto& [idx, dist] : results[q]) {
                if (idx < vectorIds.size()) {
                    searchResults[q].push_back({vectorIds[idx], dist});
                }
            }
        }
        
        return searchResults;
#else
        // HIP not available, use CPU
        std::vector<std::vector<SearchResult>> cpuResults;
        cpuResults.reserve(queries.size());
        for (const auto& query : queries) {
            cpuResults.push_back(searchCPU(query, k));
        }
        return cpuResults;
#endif
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
        backends.push_back(Backend::CPU); // Only CPU available in current version
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
    
    // Use appropriate backend
    switch (pImpl->activeBackend) {
        case Backend::HIP:
            return pImpl->searchHIP(query, k);
        case Backend::CUDA:
            // CUDA backend not implemented in this PR
            if (pImpl->config.allowCPUFallback) {
                return pImpl->searchCPU(query, k);
            }
            return {};
        case Backend::CPU:
        case Backend::AUTO:
        default:
            return pImpl->searchCPU(query, k);
    }
}

std::vector<std::vector<GPUVectorIndex::SearchResult>> GPUVectorIndex::searchBatch(
    const std::vector<std::vector<float>>& queries, size_t k) {
    
    if (!pImpl->initialized) {
        return {};
    }
    
    // Use appropriate backend
    switch (pImpl->activeBackend) {
        case Backend::HIP:
            return pImpl->searchBatchHIP(queries, k);
        case Backend::CUDA:
            // CUDA backend not implemented in this PR
            if (pImpl->config.allowCPUFallback) {
                std::vector<std::vector<SearchResult>> results;
                results.reserve(queries.size());
                for (const auto& query : queries) {
                    results.push_back(search(query, k));
                }
                return results;
            }
            return {};
        case Backend::CPU:
        case Backend::AUTO:
        default:
            // CPU fallback
            std::vector<std::vector<SearchResult>> results;
            results.reserve(queries.size());
            for (const auto& query : queries) {
                results.push_back(pImpl->searchCPU(query, k));
            }
            return results;
    }
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
    
    // Don't switch if already using requested backend
    if (pImpl->activeBackend == backend) {
        return true;
    }
    
    Backend oldBackend = pImpl->activeBackend;
    
    // Try to switch to requested backend
    if (backend == Backend::HIP) {
#ifdef THEMIS_ENABLE_HIP
        if (pImpl->initializeHIP()) {
            pImpl->activeBackend = Backend::HIP;
            pImpl->stats.activeBackend = Backend::HIP;
            pImpl->stats.isGPUActive = true;
            std::cout << "Switched to HIP backend\n";
            return true;
        }
#endif
        std::cerr << "Cannot switch to HIP backend (not available)\n";
        return false;
    } else if (backend == Backend::CUDA) {
#ifdef THEMIS_ENABLE_CUDA
        if (pImpl->initializeCUDA()) {
            pImpl->activeBackend = Backend::CUDA;
            pImpl->stats.activeBackend = Backend::CUDA;
            pImpl->stats.isGPUActive = true;
            std::cout << "Switched to CUDA backend\n";
            return true;
        }
#endif
        std::cerr << "Cannot switch to CUDA backend (not available)\n";
        return false;
    } else if (backend == Backend::CPU || backend == Backend::AUTO) {
        // Shutdown GPU backends when switching to CPU
#ifdef THEMIS_ENABLE_HIP
        if (pImpl->hipBackend) {
            pImpl->hipBackend->shutdown();
        }
#endif
#ifdef THEMIS_ENABLE_CUDA
        if (pImpl->cudaBackend) {
            pImpl->cudaBackend->shutdown();
        }
#endif
        pImpl->activeBackend = Backend::CPU;
        pImpl->stats.activeBackend = Backend::CPU;
        pImpl->stats.isGPUActive = false;
        std::cout << "Switched to CPU backend\n";
        return true;
    }
    
    return false;
}

std::vector<GPUVectorIndex::Backend> GPUVectorIndex::getAvailableBackends() const {
    std::vector<Backend> backends;
    
    // CPU is always available
    backends.push_back(Backend::CPU);
    
    // Check for HIP availability
#ifdef THEMIS_ENABLE_HIP
    // Use static method to check availability without needing instance
    if (themis::acceleration::HIPVectorBackend().isAvailable()) {
        backends.push_back(Backend::HIP);
    }
#endif
    
    // Check for CUDA availability
#ifdef THEMIS_ENABLE_CUDA
    // CUDA backend check would go here when implemented
#endif
    
    return backends;
}

} // namespace index
} // namespace themis
