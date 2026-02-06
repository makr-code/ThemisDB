#include "index/gpu_vector_index.h"
#include "acceleration/compute_backend.h"
#include "acceleration/cuda_backend.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <unordered_map>
#include <memory>

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
    
    // GPU backend (CUDA)
    std::unique_ptr<acceleration::CUDAVectorBackend> cudaBackend;
    
    // Cached flattened vector data for GPU (performance optimization)
    std::vector<float> flatVectorCache;
    bool flatVectorCacheDirty = true;
    
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
        
        // Try CUDA backend if requested or AUTO
        if (requestedBackend == Backend::CUDA || requestedBackend == Backend::AUTO) {
            cudaBackend = std::make_unique<acceleration::CUDAVectorBackend>();
            if (cudaBackend->isAvailable() && cudaBackend->initialize()) {
                activeBackend = Backend::CUDA;
                stats.activeBackend = Backend::CUDA;
                stats.isGPUActive = true;
                
                auto caps = cudaBackend->getCapabilities();
                std::cout << "GPUVectorIndex: Using CUDA backend\n";
                std::cout << "  Device: " << caps.deviceName << "\n";
                std::cout << "  Memory: " << (caps.maxMemoryBytes / (1024*1024*1024)) << " GB\n";
                
                initialized = true;
                return true;
            } else {
                // CUDA not available
                cudaBackend.reset();
                if (requestedBackend == Backend::CUDA) {
                    // User explicitly requested CUDA but it's not available
                    if (config.allowCPUFallback) {
                        std::cout << "GPUVectorIndex: CUDA not available, falling back to CPU\n";
                    } else {
                        std::cerr << "GPUVectorIndex: CUDA not available and CPU fallback disabled\n";
                        return false;
                    }
                }
            }
        }
        
        // Fall back to CPU
        activeBackend = Backend::CPU;
        stats.activeBackend = Backend::CPU;
        stats.isGPUActive = false;
        
        if (requestedBackend == Backend::AUTO) {
            std::cout << "GPUVectorIndex: Using CPU backend (no GPU available)\n";
        } else {
            std::cout << "GPUVectorIndex: Using CPU backend\n";
        }
        
        initialized = true;
        return true;
    }
    
    void shutdown() {
        if (cudaBackend) {
            cudaBackend->shutdown();
            cudaBackend.reset();
        }
        initialized = false;
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
        flatVectorCacheDirty = true;  // Mark cache as dirty
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
        flatVectorCacheDirty = true;  // Mark cache as dirty
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
    
    std::vector<SearchResult> searchGPU(const std::vector<float>& query, size_t k) {
        if (!cudaBackend || vectorData.empty() || query.size() != static_cast<size_t>(dimension)) {
            return {};
        }
        
        // CUDA backend only supports L2 and COSINE metrics
        // Fall back to CPU for INNER_PRODUCT
        if (config.metric == DistanceMetric::INNER_PRODUCT) {
            return searchCPU(query, k);
        }
        
        auto startTime = std::chrono::steady_clock::now();
        
        // Update flattened vector cache if dirty (performance optimization)
        if (flatVectorCacheDirty) {
            flatVectorCache.clear();
            flatVectorCache.reserve(vectorData.size() * dimension);
            for (const auto& vec : vectorData) {
                flatVectorCache.insert(flatVectorCache.end(), vec.begin(), vec.end());
            }
            flatVectorCacheDirty = false;
        }
        
        // Clamp k to number of vectors to prevent out-of-bounds access
        const size_t effectiveK = std::min(k, vectorData.size());
        
        // Use CUDA backend for batch KNN search
        bool useL2 = (config.metric == DistanceMetric::L2);
        auto gpuResults = cudaBackend->batchKnnSearch(
            query.data(),
            1,  // Single query
            dimension,
            flatVectorCache.data(),
            vectorData.size(),
            effectiveK,
            useL2
        );
        
        std::vector<SearchResult> results;
        if (!gpuResults.empty() && !gpuResults[0].empty()) {
            results.reserve(gpuResults[0].size());
            for (const auto& [idx, dist] : gpuResults[0]) {
                if (idx < vectorIds.size()) {
                    results.push_back({vectorIds[idx], dist});
                }
            }
        }
        
        auto endTime = std::chrono::steady_clock::now();
        updateQueryStats(startTime, endTime);
        
        return results;
    }
    
    std::vector<std::vector<SearchResult>> searchBatchGPU(
        const std::vector<std::vector<float>>& queries, size_t k) {
        
        if (!cudaBackend || vectorData.empty() || queries.empty()) {
            return {};
        }
        
        // Check if any query uses INNER_PRODUCT (not supported by CUDA)
        if (config.metric == DistanceMetric::INNER_PRODUCT) {
            // Fall back to CPU for all queries
            std::vector<std::vector<SearchResult>> results;
            results.reserve(queries.size());
            for (const auto& query : queries) {
                results.push_back(searchCPU(query, k));
            }
            return results;
        }
        
        auto startTime = std::chrono::steady_clock::now();
        
        // Update flattened vector cache if dirty
        if (flatVectorCacheDirty) {
            flatVectorCache.clear();
            flatVectorCache.reserve(vectorData.size() * dimension);
            for (const auto& vec : vectorData) {
                flatVectorCache.insert(flatVectorCache.end(), vec.begin(), vec.end());
            }
            flatVectorCacheDirty = false;
        }
        
        // Flatten query vectors for GPU transfer
        std::vector<float> flatQueries;
        flatQueries.reserve(queries.size() * dimension);
        for (const auto& query : queries) {
            if (query.size() != static_cast<size_t>(dimension)) {
                // Skip invalid queries or fall back to CPU for all
                std::vector<std::vector<SearchResult>> results;
                results.reserve(queries.size());
                for (const auto& q : queries) {
                    results.push_back(searchCPU(q, k));
                }
                return results;
            }
            flatQueries.insert(flatQueries.end(), query.begin(), query.end());
        }
        
        // Clamp k to number of vectors
        const size_t effectiveK = std::min(k, vectorData.size());
        
        // Use CUDA backend for true batch KNN search
        bool useL2 = (config.metric == DistanceMetric::L2);
        auto gpuResults = cudaBackend->batchKnnSearch(
            flatQueries.data(),
            queries.size(),  // Multiple queries
            dimension,
            flatVectorCache.data(),
            vectorData.size(),
            effectiveK,
            useL2
        );
        
        // Convert GPU results to SearchResult format
        std::vector<std::vector<SearchResult>> results;
        results.reserve(gpuResults.size());
        
        for (const auto& queryResults : gpuResults) {
            std::vector<SearchResult> batch;
            batch.reserve(queryResults.size());
            for (const auto& [idx, dist] : queryResults) {
                if (idx < vectorIds.size()) {
                    batch.push_back({vectorIds[idx], dist});
                }
            }
            results.push_back(std::move(batch));
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
        backends.push_back(Backend::CPU); // CPU always available
        
        // Check if CUDA is available
        auto cudaTest = std::make_unique<acceleration::CUDAVectorBackend>();
        if (cudaTest->isAvailable()) {
            backends.push_back(Backend::CUDA);
        }
        
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
    
    // Use GPU backend if active, otherwise fall back to CPU
    if (pImpl->activeBackend == Backend::CUDA && pImpl->cudaBackend) {
        return pImpl->searchGPU(query, k);
    }
    
    return pImpl->searchCPU(query, k);
}

std::vector<std::vector<GPUVectorIndex::SearchResult>> GPUVectorIndex::searchBatch(
    const std::vector<std::vector<float>>& queries, size_t k) {
    
    if (!pImpl->initialized || queries.empty()) {
        return {};
    }
    
    // Use GPU batch search if CUDA backend is active
    if (pImpl->activeBackend == Backend::CUDA && pImpl->cudaBackend) {
        return pImpl->searchBatchGPU(queries, k);
    }
    
    // Fall back to CPU: process queries sequentially
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
    if (backend == Backend::CPU) {
        pImpl->activeBackend = Backend::CPU;
        pImpl->stats.activeBackend = Backend::CPU;
        pImpl->stats.isGPUActive = false;
        return true;
    }
    
    if (backend == Backend::AUTO) {
        // Re-run backend selection: prefer CUDA if available, otherwise fall back to CPU
        if (!pImpl->cudaBackend) {
            pImpl->cudaBackend = std::make_unique<acceleration::CUDAVectorBackend>();
            if (!pImpl->cudaBackend->initialize()) {
                pImpl->cudaBackend.reset();
            }
        }
        
        if (pImpl->cudaBackend && pImpl->cudaBackend->isAvailable()) {
            pImpl->activeBackend = Backend::CUDA;
            pImpl->stats.activeBackend = Backend::CUDA;
            pImpl->stats.isGPUActive = true;
            return true;
        }
        
        // CUDA not available; use CPU as fallback
        pImpl->activeBackend = Backend::CPU;
        pImpl->stats.activeBackend = Backend::CPU;
        pImpl->stats.isGPUActive = false;
        return true;
    }
    
    if (backend == Backend::CUDA) {
        // Try to initialize CUDA backend if not already initialized
        if (!pImpl->cudaBackend) {
            pImpl->cudaBackend = std::make_unique<acceleration::CUDAVectorBackend>();
            if (!pImpl->cudaBackend->initialize()) {
                pImpl->cudaBackend.reset();
                return false;
            }
        }
        
        if (pImpl->cudaBackend && pImpl->cudaBackend->isAvailable()) {
            pImpl->activeBackend = Backend::CUDA;
            pImpl->stats.activeBackend = Backend::CUDA;
            pImpl->stats.isGPUActive = true;
            return true;
        }
        return false;
    }
    
    return false;
}

std::vector<GPUVectorIndex::Backend> GPUVectorIndex::getAvailableBackends() const {
    return pImpl->getAvailableBackends();
}

} // namespace index
} // namespace themis
