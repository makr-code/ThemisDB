#include "index/gpu_vector_index.h"
#include "acceleration/compute_backend.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <unordered_map>

namespace themis {
namespace index {

// Forward declarations for backend implementations
class VulkanVectorIndexBackend;
class CUDAVectorIndexBackend;
class HIPVectorIndexBackend;

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
    
    // Statistics
    Statistics stats;
    std::chrono::steady_clock::time_point lastQueryTime;
    size_t queryCount = 0;
    double totalQueryTimeMs = 0.0;
    
    // Backend implementations
    std::unique_ptr<VulkanVectorIndexBackend> vulkanBackend;
    std::unique_ptr<CUDAVectorIndexBackend> cudaBackend;
    std::unique_ptr<HIPVectorIndexBackend> hipBackend;
    
    Impl(const Config& cfg) : config(cfg) {
        // Validate configuration parameters
        if (config.M <= 0 || config.M > 256) {
            std::cerr << "Warning: M parameter " << config.M << " outside recommended range [1-256], using default 16" << std::endl;
            config.M = 16;
        }
        if (config.efConstruction <= 0 || config.efConstruction > 2000) {
            std::cerr << "Warning: efConstruction " << config.efConstruction << " outside recommended range [1-2000], using default 200" << std::endl;
            config.efConstruction = 200;
        }
        if (config.efSearch <= 0 || config.efSearch > 2000) {
            std::cerr << "Warning: efSearch " << config.efSearch << " outside recommended range [1-2000], using default 64" << std::endl;
            config.efSearch = 64;
        }
        if (config.batchSize <= 0 || config.batchSize > 10000) {
            std::cerr << "Warning: batchSize " << config.batchSize << " outside valid range [1-10000], clamping to valid range" << std::endl;
            config.batchSize = (config.batchSize <= 0) ? 512 : 10000;
        }
        if (config.maxVRAM_MB <= 0 || config.maxVRAM_MB > 1048576) { // Max 1TB
            std::cerr << "Warning: maxVRAM_MB " << config.maxVRAM_MB << " invalid, using default 8192 MB" << std::endl;
            config.maxVRAM_MB = 8192;
        }
        if (config.deviceId < 0 || config.deviceId > 16) {
            std::cerr << "Warning: deviceId " << config.deviceId << " outside valid range [0-16], using default 0" << std::endl;
            config.deviceId = 0;
        }
    }
    
    ~Impl() {
        shutdown();
    }
    
    bool initialize(int dim) {
        // Validate dimension
        if (dim <= 0) {
            std::cerr << "Invalid dimension: " << dim << " (must be > 0)" << std::endl;
            return false;
        }
        if (dim > 10000) {
            std::cerr << "Dimension " << dim << " exceeds maximum supported (10000)" << std::endl;
            return false;
        }
        
        dimension = dim;
        stats.dimension = dim;
        
        // Detect and initialize best backend
        if (config.backend == Backend::AUTO) {
            if (tryInitializeBackend(Backend::VULKAN)) {
                activeBackend = Backend::VULKAN;
            } else if (tryInitializeBackend(Backend::CUDA)) {
                activeBackend = Backend::CUDA;
            } else if (tryInitializeBackend(Backend::HIP)) {
                activeBackend = Backend::HIP;
            } else {
                activeBackend = Backend::CPU;
                std::cout << "GPU backends not available, falling back to CPU\n";
            }
        } else {
            if (!tryInitializeBackend(config.backend)) {
                if (config.allowCPUFallback) {
                    activeBackend = Backend::CPU;
                    std::cout << "Requested backend not available, falling back to CPU\n";
                } else {
                    return false;
                }
            } else {
                activeBackend = config.backend;
            }
        }
        
        stats.activeBackend = activeBackend;
        stats.isGPUActive = (activeBackend != Backend::CPU);
        initialized = true;
        return true;
    }
    
    void shutdown() {
        if (vulkanBackend) vulkanBackend->shutdown();
        if (cudaBackend) cudaBackend->shutdown();
        if (hipBackend) hipBackend->shutdown();
        initialized = false;
    }
    
    bool tryInitializeBackend(Backend backend) {
        try {
            switch (backend) {
#ifdef THEMIS_ENABLE_VULKAN
                case Backend::VULKAN:
                    if (!vulkanBackend) {
                        vulkanBackend = std::make_unique<VulkanVectorIndexBackend>();
                    }
                    return vulkanBackend->initialize(dimension, config);
#endif
#ifdef THEMIS_ENABLE_CUDA
                case Backend::CUDA:
                    if (!cudaBackend) {
                        cudaBackend = std::make_unique<CUDAVectorIndexBackend>();
                    }
                    return cudaBackend->initialize(dimension, config);
#endif
#ifdef THEMIS_ENABLE_HIP
                case Backend::HIP:
                    if (!hipBackend) {
                        hipBackend = std::make_unique<HIPVectorIndexBackend>();
                    }
                    return hipBackend->initialize(dimension, config);
#endif
                default:
                    return false;
            }
        } catch (const std::exception& e) {
            std::cerr << "Backend initialization failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool addVector(const std::string& id, const std::vector<float>& vector) {
        if (!initialized) {
            std::cerr << "Index not initialized" << std::endl;
            return false;
        }
        if (id.empty()) {
            std::cerr << "Vector ID cannot be empty" << std::endl;
            return false;
        }
        if (vector.size() != static_cast<size_t>(dimension)) {
            std::cerr << "Vector dimension mismatch: expected " << dimension 
                     << ", got " << vector.size() << std::endl;
            return false;
        }
        // Check for NaN or infinity values
        for (size_t i = 0; i < vector.size(); ++i) {
            if (!std::isfinite(vector[i])) {
                std::cerr << "Vector contains non-finite values (NaN or Inf)" << std::endl;
                return false;
            }
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
        if (vectorData.empty()) {
            return {};
        }
        if (query.size() != static_cast<size_t>(dimension)) {
            std::cerr << "Query dimension mismatch: expected " << dimension 
                     << ", got " << query.size() << std::endl;
            return {};
        }
        if (k == 0) {
            std::cerr << "k must be greater than 0" << std::endl;
            return {};
        }
        // Check for NaN or infinity in query
        for (size_t i = 0; i < query.size(); ++i) {
            if (!std::isfinite(query[i])) {
                std::cerr << "Query contains non-finite values (NaN or Inf)" << std::endl;
                return {};
            }
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
        backends.push_back(Backend::VULKAN);
#endif
#ifdef THEMIS_ENABLE_CUDA
        backends.push_back(Backend::CUDA);
#endif
#ifdef THEMIS_ENABLE_HIP
        backends.push_back(Backend::HIP);
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
    
    // For now, use CPU implementation
    // GPU implementations will be added in specialized backend files
    return pImpl->searchCPU(query, k);
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
    if (ef <= 0) {
        std::cerr << "efSearch must be greater than 0, ignoring invalid value: " << ef << std::endl;
        return;
    }
    if (ef > 2000) {
        std::cerr << "Warning: efSearch " << ef << " exceeds recommended range (1-2000), using default 64" << std::endl;
        pImpl->config.efSearch = 64;
        return;
    }
    pImpl->config.efSearch = ef;
}

void GPUVectorIndex::setBatchSize(int size) {
    if (size <= 0) {
        std::cerr << "Batch size must be greater than 0, ignoring invalid value: " << size << std::endl;
        return;
    }
    if (size > 10000) {
        std::cerr << "Batch size " << size << " exceeds maximum (10000), clamping to 10000" << std::endl;
        pImpl->config.batchSize = 10000;
        return;
    }
    pImpl->config.batchSize = size;
}

GPUVectorIndex::Backend GPUVectorIndex::getActiveBackend() const {
    return pImpl->activeBackend;
}

GPUVectorIndex::Statistics GPUVectorIndex::getStatistics() const {
    return pImpl->stats;
}

bool GPUVectorIndex::switchBackend(Backend backend) {
    if (pImpl->tryInitializeBackend(backend)) {
        pImpl->activeBackend = backend;
        pImpl->stats.activeBackend = backend;
        pImpl->stats.isGPUActive = (backend != Backend::CPU);
        return true;
    }
    return false;
}

std::vector<GPUVectorIndex::Backend> GPUVectorIndex::getAvailableBackends() const {
    return pImpl->getAvailableBackends();
}

} // namespace index
} // namespace themis
