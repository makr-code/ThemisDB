/**
 * @file gpu_vector_index.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=6, H=16, M=15, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "index/gpu_vector_index.h"
#include "index/gpu_memory_oversubscription.h"
#include "acceleration/compute_backend.h"
#include "acceleration/cuda_backend.h"
#include "themis/gpu/memory_manager.h"
#include "utils/logger.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <unordered_map>
#include <memory>

#ifdef THEMIS_ENABLE_VULKAN
#include "llm/lora_framework/vulkan_context.h"
#include "llm/lora_framework/vulkan_buffer.h"
#include "llm/lora_framework/vulkan_pipeline.h"
#include "index/gpu_vector_index_vulkan.h"
#endif

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

/** @brief GPUVectorIndex::Impl. */
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
    bool gpuDataDirty = false;  // Track if GPU needs re-upload
    #endif

    #ifdef THEMIS_ENABLE_CUDA
    std::unique_ptr<themis::acceleration::CUDAVectorBackend> cudaBackend;
    std::vector<float> flatVectorCache;      // Flattened CPU-side copy for GPU transfer
    bool flatVectorCacheDirty = true;        // Rebuild before next GPU search
    #endif

    #ifdef THEMIS_ENABLE_HIP
    std::unique_ptr<themis::acceleration::HIPVectorBackend> hipBackend;
    std::vector<float> hipFlatVectorCache;   // Flattened CPU-side copy for HIP transfer
    bool hipFlatVectorCacheDirty = true;     // Rebuild before next HIP search
    #endif
    
    // Statistics
    Statistics stats;
    std::chrono::steady_clock::time_point lastQueryTime;
    size_t queryCount = 0;
    double totalQueryTimeMs = 0.0;

    // Per-index GPU memory budget (populated when config.maxVRAM_MB > 0)
    std::string vramBudgetTag;         // Unique tenant tag for GPUMemoryManager
    uint64_t vramAllocatedBytes = 0;   // Bytes currently tracked against the budget

    // GPU Memory Oversubscription (v1.7.0)
    // Active when config.enable_oversubscription == true.
    std::unique_ptr<GPUMemoryOversubscriptionManager> oversubManager;

    // When true, rebuildOversubPartitions() is a no-op.  Used by loadIndex()
    // to defer the (expensive) full partition rebuild until after all vectors
    // are loaded, avoiding O(n²) behaviour.
    std::atomic<bool> oversubBulkLoading_{false};

    // Rebuild the oversubscription manager partitions from the current vectorData.
    // Called after every vector mutation when oversubscription is enabled.
    void rebuildOversubPartitions() {
        if (!oversubManager || vectorData.empty() || oversubBulkLoading_) {
          return;
        }

        // Phase 5: Snapshot partition IDs to avoid iterator invalidation during removal
        const auto partIds = oversubManager->getAllPartitionIds();
        for (size_t pid : partIds) {
            oversubManager->removePartition(pid);
        }

        const size_t dim   = static_cast<size_t>(dimension);
        const size_t psize = (config.oversubscription_partition_vectors > 0)
                                 ? config.oversubscription_partition_vectors
                                 : static_cast<size_t>(65536);
        const size_t total = vectorData.size();

        for (size_t start = 0; start < total; start += psize) {
            const size_t end = std::min(start + psize, total);
            const size_t n   = end - start;

            std::vector<float> flat;
            flat.reserve(n * dim);
            for (size_t i = start; i < end; ++i) {
                flat.insert(flat.end(), vectorData[i].begin(), vectorData[i].end());
            }
            const std::string tag = "vecs[" + std::to_string(start) + "," +
                                    std::to_string(end) + ")";
            oversubManager->addPartition(flat, n, dim, tag);
        }
    }

    // Search all oversubscription partitions and return merged top-k results.
    std::vector<SearchResult> searchOversubscribed(const std::vector<float>& query, size_t k) {
        if (!oversubManager || vectorData.empty() ||
            query.size() != static_cast<size_t>(dimension)) {
            THEMIS_DEBUG("GPUVectorIndex::searchOversubscribed - no oversub manager or empty data or dim mismatch (oversubManager={} vector_count={} query_dim={} expected_dim={})",
                        static_cast<bool>(oversubManager), vectorData.size(), query.size(), dimension);
            return {};
        }

        auto startTime = std::chrono::steady_clock::now();

        const size_t dim   = static_cast<size_t>(dimension);
        const size_t psize = (config.oversubscription_partition_vectors > 0)
                                 ? config.oversubscription_partition_vectors
                                 : static_cast<size_t>(65536);

        // Accumulate candidates from each partition.
        std::vector<std::pair<float, size_t>> candidates;
        candidates.reserve(std::min(k * 4, vectorData.size()));

        {
            const auto partIds = oversubManager->getAllPartitionIds();
            size_t globalOffset = 0;

            for (size_t pid : partIds) {
            // Ensure this partition is VRAM-resident (triggers LRU eviction if needed).
            oversubManager->accessPartition(pid);

            const std::vector<float>* data = oversubManager->getPartitionData(pid);
            if (!data || data->empty()) {
                globalOffset += psize;
                continue;
            }

            const size_t numVecs = oversubManager->getPartitionVectorCount(pid);
            // Brute-force distance computation on the partition data.
            for (size_t vi = 0; vi < numVecs; ++vi) {
                const float* vecPtr = data->data() + vi * dim;
                float dist = computeDistance(query.data(), vecPtr, static_cast<int>(dim));
                candidates.emplace_back(dist, globalOffset + vi);
            }
            globalOffset += numVecs;
        }
        }

        // Select top-k from all candidates.
        const size_t topK = std::min(k, candidates.size());
        std::partial_sort(candidates.begin(), candidates.begin() + topK,
                          candidates.end(),
                          [](const auto& a, const auto& b) {
                              return a.first < b.first;
                          });

        std::vector<SearchResult> results;
        results.reserve(topK);
        for (size_t i = 0; i < topK; ++i) {
            const size_t idx = candidates[i].second;
            if (static_cast<int>(vectorIds.size()) > idx) {
                results.push_back({vectorIds[idx], candidates[i].first});
            }
        }

        auto endTime = std::chrono::steady_clock::now();
        updateQueryStats(startTime, endTime);

        return results;
    }

    // Returns the raw memory footprint of one vector in bytes
    uint64_t bytesPerVector() const {
        return static_cast<uint64_t>(dimension) * sizeof(float);
    }

    Impl(const Config& cfg) : config(cfg) {}

    static std::string prefetchStrategyToString(PrefetchStrategy s) {
        switch (s) {
        case PrefetchStrategy::LRU:        return "LRU";
        case PrefetchStrategy::MRU:        return "MRU";
        case PrefetchStrategy::SEQUENTIAL: return "SEQUENTIAL";
        default:                           return "NONE";
        }
    }
    
    ~Impl() {
        shutdown();
    }
    
    bool initialize([[maybe_unused]] int dim) {
        dimension = dim;
        stats.dimension = dim;
        
        // Determine which backend to use
        Backend requestedBackend = config.backend;
        bool backendInitialized = false;

        if (requestedBackend == Backend::AUTO) {
            // AUTO means: probe and initialize the best available GPU backend
            // in priority order, then fall back to CPU.
            for (Backend candidateBackend : getBackendPriorityOrder()) {
                if (tryInitializeBackend(candidateBackend, dim)) {
                    backendInitialized = true;
                    break;
                }
            }
        } else {
            backendInitialized = tryInitializeBackend(requestedBackend, dim);
        }
        
        // Fall back to CPU if requested backend failed or not available
        if (!backendInitialized) {
            if (requestedBackend != Backend::CPU && !config.allowCPUFallback) {
                THEMIS_WARN("GPUVectorIndex: Requested backend not available and CPU fallback disabled");
                return false;
            }
            
            activeBackend = Backend::CPU;
            stats.isGPUActive = false;
            if (requestedBackend != Backend::CPU) {
                THEMIS_WARN("GPUVectorIndex: Falling back to CPU backend");
            } else {
                THEMIS_INFO("GPUVectorIndex: Using CPU backend");
            }
        }
        
        stats.activeBackend = activeBackend;
        initialized = true;

        // Register per-index VRAM budget when a limit is configured and the
        // current edition supports GPU VRAM tracking (GetMaxGPUVRAMBytes() > 0).
        if (config.maxVRAM_MB > 0 &&
            themis::gpu::GPUMemoryManager::GetMaxGPUVRAMBytes() > 0) {
            static std::atomic<uint64_t> indexCounter{0};
            vramBudgetTag = "gpu_vector_index_" +
                            std::to_string(indexCounter.fetch_add(1, std::memory_order_relaxed));
            uint64_t budgetBytes =
                static_cast<uint64_t>(config.maxVRAM_MB) * 1024 * 1024;
            // Cap the per-index budget at the edition limit to avoid setting an
            // unreachable quota.
            uint64_t editionLimit = themis::gpu::GPUMemoryManager::GetMaxGPUVRAMBytes();
            if (budgetBytes > editionLimit) {
                budgetBytes = editionLimit;
            }
            themis::gpu::GPUMemoryManager::GetInstance().SetTenantQuota(
                vramBudgetTag, budgetBytes);
        }

        // Initialise the GPU memory oversubscription manager when requested.
        if (config.enable_oversubscription) {
            GPUMemoryOversubscriptionManager::Config osmCfg;
            osmCfg.enable_oversubscription = true;
            osmCfg.vram_budget_mb          = config.vram_budget_mb;
            osmCfg.prefetch_strategy       = config.prefetch_strategy;
            osmCfg.partition_vectors       = config.oversubscription_partition_vectors > 0
                                                 ? config.oversubscription_partition_vectors
                                                 : static_cast<size_t>(65536);
            osmCfg.use_unified_memory      = true;
            oversubManager = std::make_unique<GPUMemoryOversubscriptionManager>(osmCfg);
            THEMIS_INFO("GPUVectorIndex: GPU memory oversubscription enabled (VRAM budget: {}, prefetch: {})",
                        (config.vram_budget_mb > 0
                             ? std::to_string(config.vram_budget_mb) + " MB"
                             : std::string("unlimited")),
                        prefetchStrategyToString(config.prefetch_strategy));
        }

        return true;
    }
    
    void shutdown() {
        #ifdef THEMIS_ENABLE_VULKAN
        if (vulkanBackend) {
            vulkanBackend.reset();
        }
        #endif

        #ifdef THEMIS_ENABLE_CUDA
        if (cudaBackend) {
            cudaBackend->shutdown();
            cudaBackend.reset();
        }
        #endif

        #ifdef THEMIS_ENABLE_HIP
        if (hipBackend) {
            hipBackend->shutdown();
            hipBackend.reset();
        }
        #endif

        // Release any VRAM tracked against the per-index budget
        if (!vramBudgetTag.empty()) {
            auto& mgr = themis::gpu::GPUMemoryManager::GetInstance();
            if (vramAllocatedBytes > 0) {
                mgr.DeallocateGPU(vramAllocatedBytes, vramBudgetTag);
                vramAllocatedBytes = 0;
            }
            mgr.RemoveTenantQuota(vramBudgetTag);
            vramBudgetTag.clear();
        }

        // Destroy the oversubscription manager (evicts all hot partitions).
        oversubManager.reset();

        initialized = false;
    }
    
    std::vector<Backend> getBackendPriorityOrder() {
        std::vector<Backend> order;

        // Prefer vendor-native backends first, then cross-vendor Vulkan.
        #ifdef THEMIS_ENABLE_CUDA
        order.push_back(Backend::CUDA);
        #endif

        #ifdef THEMIS_ENABLE_HIP
        order.push_back(Backend::HIP);
        #endif

        #ifdef THEMIS_ENABLE_VULKAN
        order.push_back(Backend::VULKAN);
        #endif

        return order;
    }

    bool tryInitializeBackend(Backend backend, int dim) {
        #ifdef THEMIS_ENABLE_CUDA
        if (backend == Backend::CUDA) {
            auto candidate = std::make_unique<themis::acceleration::CUDAVectorBackend>();
            if (candidate->isAvailable() && candidate->initialize()) {
                cudaBackend = std::move(candidate);
                activeBackend = Backend::CUDA;
                stats.isGPUActive = true;
                flatVectorCacheDirty = true;
                THEMIS_INFO("GPUVectorIndex: Using CUDA backend");
                return true;
            }
            return false;
        }
        #endif

        #ifdef THEMIS_ENABLE_HIP
        if (backend == Backend::HIP) {
            auto candidate = std::make_unique<themis::acceleration::HIPVectorBackend>();
            if (candidate->isAvailable() && candidate->initialize()) {
                hipBackend = std::move(candidate);
                activeBackend = Backend::HIP;
                stats.isGPUActive = true;
                hipFlatVectorCacheDirty = true;
                THEMIS_INFO("GPUVectorIndex: Using HIP backend");
                return true;
            }
            return false;
        }
        #endif

        #ifdef THEMIS_ENABLE_VULKAN
        if (backend == Backend::VULKAN) {
            if (initializeVulkanBackend(dim)) {
                activeBackend = Backend::VULKAN;
                stats.isGPUActive = true;
                THEMIS_INFO("GPUVectorIndex: Using Vulkan backend");
                return true;
            }
            return false;
        }
        #endif

        return false;
    }

    Backend selectBestBackend() {
        const auto order = getBackendPriorityOrder();
        for (Backend backend : order) {
            switch (backend) {
            case Backend::CUDA:
                #ifdef THEMIS_ENABLE_CUDA
                {
                    themis::acceleration::CUDAVectorBackend checkBackend;
                    if (checkBackend.isAvailable()) {
                        return Backend::CUDA;
                    }
                }
                #endif
                break;
            case Backend::HIP:
                #ifdef THEMIS_ENABLE_HIP
                if (themis::acceleration::HIPVectorBackend().isAvailable()) {
                    return Backend::HIP;
                }
                #endif
                break;
            case Backend::VULKAN:
                #ifdef THEMIS_ENABLE_VULKAN
                if (isVulkanAvailable()) {
                    return Backend::VULKAN;
                }
                #endif
                break;
            default:
                break;
            }
        }

        // Fall back to CPU
        return Backend::CPU;
    }
    
    #ifdef THEMIS_ENABLE_VULKAN
    bool isVulkanAvailable() {
        // Check if Vulkan is available by trying to create a context
        try {
            lora::vulkan::VulkanContext testContext;
            return testContext.is_available();
        } catch (const std::exception& e) {
            THEMIS_DEBUG("GPUVectorIndex: Vulkan availability probe failed: {}", e.what());
            return false;
        }
    }
    
    bool initializeVulkanBackend([[maybe_unused]] int dim) {
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
            THEMIS_ERROR("GPUVectorIndex: Vulkan initialization failed: {}", e.what());
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
            // Update existing vector — no new VRAM needed
            vectorData[it->second] = vector;
        } else {
            // Enforce per-index VRAM budget for new vectors
            if (!vramBudgetTag.empty()) {
                uint64_t bytes = bytesPerVector();
                auto& mgr = themis::gpu::GPUMemoryManager::GetInstance();
                if (!mgr.TryAllocateGPU(bytes, "vector", vramBudgetTag)) {
                    THEMIS_WARN("GPUVectorIndex: VRAM budget exceeded (limit {} MB)", config.maxVRAM_MB);
                    return false;
                }
                vramAllocatedBytes += bytes;
            }

            // Add new vector
            size_t index = vectorData.size();
            vectorIds.push_back(id);
            vectorData.push_back(vector);
            idToIndex[id] = index;
        }
        
        // Invalidate flattened vector caches for GPU backends
        #ifdef THEMIS_ENABLE_CUDA
        if (activeBackend == Backend::CUDA) {
            flatVectorCacheDirty = true;
        }
        #endif
        #ifdef THEMIS_ENABLE_HIP
        if (activeBackend == Backend::HIP) {
            hipFlatVectorCacheDirty = true;
        }
        #endif
        
        stats.numVectors = vectorData.size();
        
        // Mark GPU data as dirty (will upload before next search)
        #ifdef THEMIS_ENABLE_VULKAN
        if (activeBackend == Backend::VULKAN && vulkanBackend) {
            gpuDataDirty = true;
        }
        #endif

        // Rebuild oversubscription partitions when the manager is active.
        if (oversubManager) {
            rebuildOversubPartitions();
        }
        
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

        // Release the per-vector VRAM budget allocation
        if (!vramBudgetTag.empty()) {
            uint64_t bytes = bytesPerVector();
            themis::gpu::GPUMemoryManager::GetInstance().DeallocateGPU(bytes, vramBudgetTag);
            if (vramAllocatedBytes >= bytes) {
                vramAllocatedBytes -= bytes;
            } else {
                vramAllocatedBytes = 0;
            }
        }
        
        // Invalidate flattened vector caches for GPU backends
        #ifdef THEMIS_ENABLE_CUDA
        if (activeBackend == Backend::CUDA) {
            flatVectorCacheDirty = true;
        }
        #endif
        #ifdef THEMIS_ENABLE_HIP
        if (activeBackend == Backend::HIP) {
            hipFlatVectorCacheDirty = true;
        }
        #endif
        
        stats.numVectors = vectorData.size();
        
        // Mark GPU data as dirty (will upload before next search)
        #ifdef THEMIS_ENABLE_VULKAN
        if (activeBackend == Backend::VULKAN && vulkanBackend) {
            gpuDataDirty = true;
        }
        #endif

        // Rebuild oversubscription partitions when the manager is active.
        if (oversubManager) {
            rebuildOversubPartitions();
        }
        
        return true;
    }
    
    std::vector<SearchResult> search(const std::vector<float>& query, size_t k) {
        if (!initialized) {
            THEMIS_WARN("GPUVectorIndex::search called on uninitialized index");
            return {};
        }

        // Use oversubscription-aware search path when the manager is active.
        // This handles LRU eviction, streaming, and prefetching automatically.
        if (oversubManager && !vectorData.empty()) {
            return searchOversubscribed(query, k);
        }
        
        // Upload GPU data if dirty
        #ifdef THEMIS_ENABLE_VULKAN
        if (activeBackend == Backend::VULKAN && vulkanBackend && gpuDataDirty) {
            vulkanBackend->uploadVectors(vectorData);
            gpuDataDirty = false;
        }
        #endif
        
        // Try GPU backend first if active
        #ifdef THEMIS_ENABLE_VULKAN
        if (activeBackend == Backend::VULKAN && vulkanBackend) {
            auto indices = vulkanBackend->searchIndices(query, k);
            if (!indices.empty()) {
                // Map indices to IDs
                std::vector<SearchResult> results = {};

                results.reserve(indices.size());
                for (const auto& [distance, index] : indices) {
                    if (static_cast<int>(vectorIds.size()) > index) {
                        results.push_back({vectorIds[index], distance});
                    }
                }
                return results;
            }
            // Fall through to CPU if GPU search fails
            THEMIS_WARN("GPUVectorIndex: Vulkan search failed, falling back to CPU");
        }
        #endif

        #ifdef THEMIS_ENABLE_CUDA
        if (activeBackend == Backend::CUDA && cudaBackend) {
            auto result = searchGPU(query, k);
            if (!result.empty()) {
                return result;
            }
            THEMIS_WARN("GPUVectorIndex: CUDA search failed, falling back to CPU");
        }
        #endif

        #ifdef THEMIS_ENABLE_HIP
        if (activeBackend == Backend::HIP && hipBackend) {
            auto result = searchHIP(query, k);
            if (!result.empty()) {
                return result;
            }
            THEMIS_WARN("GPUVectorIndex: HIP search failed, falling back to CPU");
        }
        #endif
        
        // Use CPU implementation
        return searchCPU(query, k);
    }
    
    std::vector<SearchResult> searchCPU(const std::vector<float>& query, size_t k) {
        if (vectorData.empty() || query.size() != static_cast<size_t>(dimension)) {
            THEMIS_DEBUG("GPUVectorIndex::searchCPU - empty data or dimension mismatch (vectors={} query_dim={} expected_dim={})",
                        vectorData.size(), query.size(), static_cast<size_t>(dimension));
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
    
#ifdef THEMIS_ENABLE_CUDA
    // CUDA backend search functions (currently not used, Vulkan is active)
    std::vector<SearchResult> searchGPU(const std::vector<float>& query, size_t k) {
        if (!cudaBackend || vectorData.empty() || query.size() != static_cast<size_t>(dimension)) {
            THEMIS_WARN("GPUVectorIndex::searchGPU - invalid state (cudaBackend={} vectors={} query_dim={} expected_dim={})",
                        static_cast<bool>(cudaBackend), vectorData.size(), query.size(), static_cast<size_t>(dimension));
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
        
        std::vector<SearchResult> results = {};

        if (!gpuResults.empty() && !gpuResults[0].empty()) {
            results.reserve(gpuResults[0].size());
            for (const auto& [idx, dist] : gpuResults[0]) {
                if (static_cast<int>(vectorIds.size()) > idx) {
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
            THEMIS_DEBUG("GPUVectorIndex::searchBatchGPU - invalid state (cudaBackend={} vectors={} queries={})",
                        static_cast<bool>(cudaBackend), vectorData.size(), queries.size());
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
        std::vector<float> flatQueries = {};

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
            std::vector<SearchResult> batch = {};

            batch.reserve(queryResults.size());
            for (const auto& [idx, dist] : queryResults) {
                if (static_cast<int>(vectorIds.size()) > idx) {
                    batch.push_back({vectorIds[idx], dist});
                }
            }
            results.push_back(std::move(batch));
        }
        
        auto endTime = std::chrono::steady_clock::now();
        updateQueryStats(startTime, endTime);
        
        return results;
    }
#endif // THEMIS_ENABLE_CUDA

#ifdef THEMIS_ENABLE_HIP
    // Helper: map GPUVectorIndex::DistanceMetric to HIPVectorBackend::DistanceMetric
    static themis::acceleration::HIPVectorBackend::DistanceMetric toHIPMetric(DistanceMetric m) {
        switch (m) {
            case DistanceMetric::COSINE:
                return themis::acceleration::HIPVectorBackend::DistanceMetric::COSINE;
            case DistanceMetric::INNER_PRODUCT:
                return themis::acceleration::HIPVectorBackend::DistanceMetric::INNER_PRODUCT;
            default:
                return themis::acceleration::HIPVectorBackend::DistanceMetric::L2;
        }
    }

    std::vector<SearchResult> searchHIP(const std::vector<float>& query, size_t k) {
        if (!hipBackend || vectorData.empty() ||
            query.size() != static_cast<size_t>(dimension)) {
            THEMIS_DEBUG("GPUVectorIndex::searchHIP - invalid state (hipBackend={} vectors={} query_dim={} expected_dim={})",
                        static_cast<bool>(hipBackend), vectorData.size(), query.size(), static_cast<size_t>(dimension));
            return {};
        }

        auto startTime = std::chrono::steady_clock::now();

        // Rebuild flattened vector cache when stale
        if (hipFlatVectorCacheDirty) {
            hipFlatVectorCache.clear();
            hipFlatVectorCache.reserve(vectorData.size() * dimension);
            for (const auto& vec : vectorData) {
                hipFlatVectorCache.insert(hipFlatVectorCache.end(), vec.begin(), vec.end());
            }
            hipFlatVectorCacheDirty = false;
        }

        const size_t effectiveK = std::min(k, vectorData.size());
        auto gpuResults = hipBackend->batchKnnSearchWithMetric(
            query.data(),
            1,  // Single query
            dimension,
            hipFlatVectorCache.data(),
            vectorData.size(),
            effectiveK,
            toHIPMetric(config.metric)
        );

        std::vector<SearchResult> results = {};

        if (!gpuResults.empty() && !gpuResults[0].empty()) {
            results.reserve(gpuResults[0].size());
            for (const auto& [idx, dist] : gpuResults[0]) {
                if (static_cast<int>(vectorIds.size()) > idx) {
                    results.push_back({vectorIds[idx], dist});
                }
            }
        }

        auto endTime = std::chrono::steady_clock::now();
        updateQueryStats(startTime, endTime);

        return results;
    }

    std::vector<std::vector<SearchResult>> searchBatchHIP(
        const std::vector<std::vector<float>>& queries, size_t k) {

        if (!hipBackend || vectorData.empty() || queries.empty()) {
            THEMIS_DEBUG("GPUVectorIndex::searchBatchHIP - invalid state (hipBackend={} vectors={} queries={})",
                        static_cast<bool>(hipBackend), vectorData.size(), queries.size());
            return {};
        }

        auto startTime = std::chrono::steady_clock::now();

        // Rebuild flattened vector cache when stale
        if (hipFlatVectorCacheDirty) {
            hipFlatVectorCache.clear();
            hipFlatVectorCache.reserve(vectorData.size() * dimension);
            for (const auto& vec : vectorData) {
                hipFlatVectorCache.insert(hipFlatVectorCache.end(), vec.begin(), vec.end());
            }
            hipFlatVectorCacheDirty = false;
        }

        // Flatten query vectors for GPU transfer; fall back to CPU on bad input
        std::vector<float> flatQueries = {};

        flatQueries.reserve(queries.size() * dimension);
        for (const auto& query : queries) {
            if (query.size() != static_cast<size_t>(dimension)) {
                std::vector<std::vector<SearchResult>> results;
                results.reserve(queries.size());
                for (const auto& q : queries) {
                    results.push_back(searchCPU(q, k));
                }
                return results;
            }
            flatQueries.insert(flatQueries.end(), query.begin(), query.end());
        }

        const size_t effectiveK = std::min(k, vectorData.size());
        auto gpuResults = hipBackend->batchKnnSearchWithMetric(
            flatQueries.data(),
            queries.size(),
            dimension,
            hipFlatVectorCache.data(),
            vectorData.size(),
            effectiveK,
            toHIPMetric(config.metric)
        );

        // Convert GPU results to SearchResult format
        std::vector<std::vector<SearchResult>> results;
        results.reserve(gpuResults.size());
        for (const auto& queryResults : gpuResults) {
            std::vector<SearchResult> batch = {};

            batch.reserve(queryResults.size());
            for (const auto& [idx, dist] : queryResults) {
                if (static_cast<int>(vectorIds.size()) > idx) {
                    batch.push_back({vectorIds[idx], dist});
                }
            }
            results.push_back(std::move(batch));
        }

        auto endTime = std::chrono::steady_clock::now();
        updateQueryStats(startTime, endTime);

        return results;
    }
#endif // THEMIS_ENABLE_HIP

    float computeDistance(const float* a, const float* b, int dim) {
        switch (config.metric) {
            case DistanceMetric::L2: {
                float sum = 0.0f;
                for (int i = 0; i < dim; ++i) {
                    float diff = a[i] - b[i];
                    sum += diff * diff;
                }
                return std::sqrt(sum);  // Return actual L2 distance (not squared)
            }
            case DistanceMetric::COSINE: {
                float dot = 0.0f, normA = 0.0f, normB = 0.0f;
                for (int i = 0; i < dim; ++i) {
                    dot += a[i] * b[i];
                    normA += a[i] * a[i];
                    normB += b[i] * b[i];
                }
                float denominator = std::sqrt(normA * normB);
                if (denominator < 1e-10f) {
                  return 1.0f;
                }
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

        #ifdef THEMIS_ENABLE_CUDA
        {
            themis::acceleration::CUDAVectorBackend checkBackend;
            if (checkBackend.isAvailable()) {
                backends.push_back(Backend::CUDA);
            }
        }
        #endif

        #ifdef THEMIS_ENABLE_HIP
        if (themis::acceleration::HIPVectorBackend().isAvailable()) {
            backends.push_back(Backend::HIP);
        }
        #endif
        
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

GPUVectorIndex::GPUVectorIndex()
    : GPUVectorIndex(Config{}) {
}

GPUVectorIndex::GPUVectorIndex(const Config& config)
    : pImpl(std::make_unique<Impl>(config)) {
}

// Phase 5: GPU destructor explicit cleanup with exception safety
GPUVectorIndex::~GPUVectorIndex() noexcept {
    try {
        shutdown();
    } catch (const std::exception& e) {
        THEMIS_WARN("GPU cleanup failed (ignored): {}", e.what());
    }
}

bool GPUVectorIndex::initialize([[maybe_unused]] int dimension) {
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
    if (!pImpl->initialized || ids.size() != vectors.size()) {
        return false;
    }

    if (ids.empty()) {
        return true;
    }

    // Snapshot dimension once to avoid a potential data race between a concurrent
    // initialize() call that writes pImpl->dimension and the reads below
    // (INDEX-GPU-DIM-RACE-01).
    const int dim = pImpl->dimension;

    // Fast-path for pure bulk inserts (all IDs are new, oversubscription disabled):
    // reserve once and append directly to avoid per-item upsert and backend checks.
    bool canUseFastPath = (pImpl->oversubManager == nullptr);
    if (canUseFastPath) {
        for (size_t i = 0; i < ids.size(); ++i) {
            if (vectors[i].size() != static_cast<size_t>(dim) ||
                pImpl->idToIndex.find(ids[i]) != pImpl->idToIndex.end()) {
                canUseFastPath = false;
                break;
            }
        }
    }

    if (canUseFastPath) {
        uint64_t allocatedBytes = 0;
        if (!pImpl->vramBudgetTag.empty()) {
            allocatedBytes = static_cast<uint64_t>(dim) * sizeof(float) * static_cast<uint64_t>(ids.size());
            auto& mgr = themis::gpu::GPUMemoryManager::GetInstance();
            if (!mgr.TryAllocateGPU(allocatedBytes, "vector_batch", pImpl->vramBudgetTag)) {
                THEMIS_WARN("GPUVectorIndex: VRAM budget exceeded (limit {} MB)", pImpl->config.maxVRAM_MB);
                return false;
            }
        }

        const size_t baseIndex = pImpl->vectorData.size();
        try {
            pImpl->vectorIds.reserve(baseIndex + ids.size());
            pImpl->vectorData.reserve(baseIndex + vectors.size());
            pImpl->idToIndex.reserve(baseIndex + ids.size());

            for (size_t i = 0; i < ids.size(); ++i) {
                pImpl->vectorIds.push_back(ids[i]);
                pImpl->vectorData.push_back(vectors[i]);
                pImpl->idToIndex.emplace(pImpl->vectorIds.back(), baseIndex + i);
            }
        } catch (...) {
            if (allocatedBytes > 0) {
                themis::gpu::GPUMemoryManager::GetInstance().DeallocateGPU(
                    allocatedBytes, pImpl->vramBudgetTag);
            }
            return false;
        }

        if (allocatedBytes > 0) {
            pImpl->vramAllocatedBytes += allocatedBytes;
        }

        #ifdef THEMIS_ENABLE_CUDA
        if (pImpl->activeBackend == Backend::CUDA) {
            pImpl->flatVectorCacheDirty = true;
        }
        #endif

        #ifdef THEMIS_ENABLE_HIP
        if (pImpl->activeBackend == Backend::HIP) {
            pImpl->hipFlatVectorCacheDirty = true;
        }
        #endif

        #ifdef THEMIS_ENABLE_VULKAN
        if (pImpl->activeBackend == Backend::VULKAN && pImpl->vulkanBackend) {
            pImpl->gpuDataDirty = true;
        }
        #endif

        pImpl->stats.numVectors = pImpl->vectorData.size();
        return true;
    }

    // Suppress per-vector partition rebuilds during bulk ingestion; a single
    // rebuild at the end is far cheaper than one rebuild per vector (O(n²)).
    pImpl->oversubBulkLoading_ = true;
    for (size_t i = 0; i < ids.size(); ++i) {
        if (!pImpl->addVector(ids[i], vectors[i])) {
            pImpl->oversubBulkLoading_ = false;
            return false;
        }
    }
    pImpl->oversubBulkLoading_ = false;
    if (pImpl->oversubManager && !pImpl->vectorData.empty()) {
        pImpl->rebuildOversubPartitions();
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
    
    if (!pImpl->initialized) {
        return {};
    }

    // Route through oversubscription manager when active: each query is served
    // through searchOversubscribed() which handles LRU eviction and prefetching.
    if (pImpl->oversubManager && !pImpl->vectorData.empty()) {
        std::vector<std::vector<SearchResult>> results;
        results.reserve(queries.size());
        for (const auto& query : queries) {
            results.push_back(pImpl->searchOversubscribed(query, k));
        }
        return results;
    }
    
    // Upload GPU data if dirty
    #ifdef THEMIS_ENABLE_VULKAN
    if (pImpl->activeBackend == Backend::VULKAN && pImpl->vulkanBackend && pImpl->gpuDataDirty) {
        pImpl->vulkanBackend->uploadVectors(pImpl->vectorData);
        pImpl->gpuDataDirty = false;
    }
    #endif
    
    // Try GPU backend batch search first if active
    #ifdef THEMIS_ENABLE_VULKAN
    if (pImpl->activeBackend == Backend::VULKAN && pImpl->vulkanBackend) {
        auto batchIndices = pImpl->vulkanBackend->searchBatchIndices(queries, k);
        if (!batchIndices.empty()) {
            // Map indices to IDs for all results
            std::vector<std::vector<SearchResult>> results;
            results.reserve(batchIndices.size());
            
            for (const auto& queryIndices : batchIndices) {
                std::vector<SearchResult> queryResults = {};

                queryResults.reserve(queryIndices.size());
                
                for (const auto& [distance, index] : queryIndices) {
                    if (index < pImpl->vectorIds.size()) {
                        queryResults.push_back({pImpl->vectorIds[index], distance});
                    }
                }
                results.push_back(std::move(queryResults));
            }
            
            return results;
        }
        THEMIS_WARN("GPUVectorIndex: Vulkan batch search failed, falling back to CPU");
    }
    #endif
    
    // Use appropriate backend or CPU fallback
    switch (pImpl->activeBackend) {
        case Backend::HIP: {
            #ifdef THEMIS_ENABLE_HIP
            if (pImpl->hipBackend) {
                auto results = pImpl->searchBatchHIP(queries, k);
                if (!results.empty()) {
                    return results;
                }
                THEMIS_WARN("GPUVectorIndex: HIP batch search failed, falling back to CPU");
            }
            #endif
            if (pImpl->config.allowCPUFallback) {
                std::vector<std::vector<SearchResult>> results;
                results.reserve(queries.size());
                for (const auto& query : queries) {
                    results.push_back(pImpl->searchCPU(query, k));
                }
                return results;
            }
            THEMIS_WARN("GPUVectorIndex::searchBatch: HIP backend failed and CPU fallback disabled");
            return {};
        }
        case Backend::CUDA: {
            #ifdef THEMIS_ENABLE_CUDA
            if (pImpl->cudaBackend) {
                auto results = pImpl->searchBatchGPU(queries, k);
                if (!results.empty()) {
                    return results;
                }
                THEMIS_WARN("GPUVectorIndex: CUDA batch search failed, falling back to CPU");
            }
            #endif
            if (pImpl->config.allowCPUFallback) {
                std::vector<std::vector<SearchResult>> results;
                results.reserve(queries.size());
                for (const auto& query : queries) {
                    results.push_back(pImpl->searchCPU(query, k));
                }
                return results;
            }
            THEMIS_WARN("GPUVectorIndex::searchBatch: CUDA backend failed and CPU fallback disabled");
            return {};
        }
        case Backend::CPU:
        [[fallthrough]];\n        case Backend::AUTO:
        [[fallthrough]];\n        default:
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
    if (!pImpl->initialized) {
        THEMIS_WARN("GPUVectorIndex: buildIndex() called on uninitialized index");
        return false;
    }

    // For GPU backends: batch-upload all vectors now so the first search does
    // not pay the upload cost.  This is the primary path for large-scale
    // dataset ingestion where callers add many vectors and then call
    // buildIndex() once before serving queries.
#ifdef THEMIS_ENABLE_VULKAN
    if (pImpl->activeBackend == Backend::VULKAN && pImpl->vulkanBackend) {
        if (!pImpl->vectorData.empty()) {
            bool ok = pImpl->vulkanBackend->uploadVectors(pImpl->vectorData);
            if (ok) {
                pImpl->gpuDataDirty = false;
            }
            return ok;
        }
        return true; // Nothing to upload yet; not an error
    }
#endif

    // CPU backend: brute-force search requires no pre-build step.
    return true;
}

bool GPUVectorIndex::saveIndex(const std::string& path) {
    if (!pImpl->initialized || path.empty()) {
        return false;
    }

    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs) {
        THEMIS_ERROR("GPUVectorIndex: Failed to open '{}' for writing", path);
        return false;
    }

    // File format:
    //   [uint32] magic   = 0x54484D53 ('THMS')
    //   [uint32] version = 1
    //   [int32]  dimension
    //   [size_t] numVectors
    //   [int32]  metric (DistanceMetric enum value)
    //   for each vector:
    //     [size_t] id length
    //     [char[]] id string (no null terminator)
    //     [float[dimension]] vector data
    const uint32_t magic   = 0x54484D53u;
    const uint32_t version = 1u;
    ofs.write(reinterpret_cast<const char*>(&magic),   sizeof(magic));
    ofs.write(reinterpret_cast<const char*>(&version), sizeof(version));

    int32_t dim = pImpl->dimension;
    ofs.write(reinterpret_cast<const char*>(&dim), sizeof(dim));

    size_t numVectors = pImpl->vectorIds.size();
    ofs.write(reinterpret_cast<const char*>(&numVectors), sizeof(numVectors));

    int32_t metric = static_cast<int32_t>(pImpl->config.metric);
    ofs.write(reinterpret_cast<const char*>(&metric), sizeof(metric));

    for (size_t i = 0; i < numVectors; ++i) {
        const std::string& id = pImpl->vectorIds[i];
        size_t idLen = id.size();
        ofs.write(reinterpret_cast<const char*>(&idLen), sizeof(idLen));
        ofs.write(id.data(), static_cast<std::streamsize>(idLen));

        ofs.write(reinterpret_cast<const char*>(pImpl->vectorData[i].data()),
                  static_cast<std::streamsize>(dim) * static_cast<std::streamsize>(sizeof(float)));
    }

    return ofs.good();
}

bool GPUVectorIndex::loadIndex(const std::string& path) {
    if (path.empty()) {
        return false;
    }

    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        THEMIS_ERROR("GPUVectorIndex: Failed to open '{}' for reading", path);
        return false;
    }

    // Read and validate header
    uint32_t magic   = 0;
    uint32_t version = 0;
    ifs.read(reinterpret_cast<char*>(&magic),   sizeof(magic));
    ifs.read(reinterpret_cast<char*>(&version), sizeof(version));

    if (!ifs || magic != 0x54484D53u || version != 1u) {
        THEMIS_ERROR("GPUVectorIndex: Invalid or unsupported index file format");
        return false;
    }

    int32_t dim = 0;
    ifs.read(reinterpret_cast<char*>(&dim), sizeof(dim));

    size_t numVectors = 0;
    ifs.read(reinterpret_cast<char*>(&numVectors), sizeof(numVectors));

    int32_t metric = 0;
    ifs.read(reinterpret_cast<char*>(&metric), sizeof(metric));
    // Stored for future compatibility; callers set the metric via Config

    if (!ifs || dim <= 0) {
        return false;
    }

    // Sanity cap: reject files claiming more vectors than could reasonably fit
    // in 64 GiB at the stored dimension (4 bytes/float).
    static constexpr uint64_t kMaxReasonableFileSizeBytes = 64 * 1024 * 1024 * 1024;
    const size_t maxReasonableVectors =
        static_cast<size_t>(kMaxReasonableFileSizeBytes /
        (static_cast<size_t>(dim) * sizeof(float) + 1));
    if (numVectors > maxReasonableVectors) {
        THEMIS_WARN("GPUVectorIndex: loadIndex rejected implausibly large vector count ({})", numVectors);
        return false;
    }

    // (Re-)initialize with dimension from file when not yet initialized or
    // dimension does not match.
    if (!pImpl->initialized || pImpl->dimension != dim) {
        if (!initialize(dim)) {
            return false;
        }
    }

    // Release existing VRAM budget allocations before clearing data so that
    // the budget counter stays accurate throughout the load.
    if (!pImpl->vramBudgetTag.empty() && pImpl->vramAllocatedBytes > 0) {
        themis::gpu::GPUMemoryManager::GetInstance().DeallocateGPU(
            pImpl->vramAllocatedBytes, pImpl->vramBudgetTag);
        pImpl->vramAllocatedBytes = 0;
    }

    // Clear existing data
    pImpl->vectorIds.clear();
    pImpl->vectorData.clear();
    pImpl->idToIndex.clear();
    pImpl->stats.numVectors = 0;
#ifdef THEMIS_ENABLE_VULKAN
    pImpl->gpuDataDirty = false;
#endif

    pImpl->vectorIds.reserve(numVectors);
    pImpl->vectorData.reserve(numVectors);

    // Suppress per-vector partition rebuilds; we do a single rebuild after
    // the entire set of vectors is loaded (avoids O(n²) partition churn).
    pImpl->oversubBulkLoading_ = true;

    for (size_t i = 0; i < numVectors; ++i) {
        size_t idLen = 0;
        ifs.read(reinterpret_cast<char*>(&idLen), sizeof(idLen));
        if (!ifs) {
            pImpl->oversubBulkLoading_ = false;
            THEMIS_ERROR("GPUVectorIndex: loadIndex read error at vector {} (ID length)", i);
            return false;
        }
        if (idLen > (1u << 20u)) { // Sanity cap at 1 MiB per ID
            pImpl->oversubBulkLoading_ = false;
            THEMIS_WARN("GPUVectorIndex: loadIndex rejected oversized ID ({} bytes) at vector {})", idLen, i);
            return false;
        }

        std::string id(idLen, '\0');
        ifs.read(id.data(), static_cast<std::streamsize>(idLen));

        std::vector<float> vec(dim);
        ifs.read(reinterpret_cast<char*>(vec.data()),
                 static_cast<std::streamsize>(dim) * static_cast<std::streamsize>(sizeof(float)));

        if (!ifs) {
            pImpl->oversubBulkLoading_ = false;
            return false;
        }

        // Use addVector() to respect VRAM budget tracking.
        // oversubBulkLoading_ is set to suppress the per-vector partition
        // rebuild; we do a single rebuild once all vectors are loaded.
        if (!pImpl->addVector(id, vec)) {
            pImpl->oversubBulkLoading_ = false;
            THEMIS_WARN("GPUVectorIndex: loadIndex aborted at vector {} (VRAM budget exceeded)", i);
            return false;
        }
    }

    // Single partition rebuild after bulk load.
    if (pImpl->oversubManager) {
        pImpl->oversubBulkLoading_ = false;
        pImpl->rebuildOversubPartitions();
    } else {
        pImpl->oversubBulkLoading_ = false;
    }

#ifdef THEMIS_ENABLE_VULKAN
    // Mark GPU data as dirty so the next search triggers an upload
    if (pImpl->activeBackend == Backend::VULKAN && pImpl->vulkanBackend) {
        pImpl->gpuDataDirty = true;
    }
#endif

    return true;
}

void GPUVectorIndex::setEfSearch([[maybe_unused]] int ef) {
    pImpl->config.efSearch = ef;
}

void GPUVectorIndex::setBatchSize([[maybe_unused]] int size) {
    pImpl->config.batchSize = size;
}

GPUVectorIndex::Backend GPUVectorIndex::getActiveBackend() const {
    return pImpl->activeBackend;
}

GPUVectorIndex::Statistics GPUVectorIndex::getStatistics() const {
    auto stats = pImpl->stats;
    
    // Merge Vulkan backend statistics if active
    #ifdef THEMIS_ENABLE_VULKAN
    if (pImpl->activeBackend == Backend::VULKAN && pImpl->vulkanBackend) {
        auto vulkanStats = pImpl->vulkanBackend->getStatistics();
        stats.vramUsageBytes = vulkanStats.vramUsageBytes;
        stats.avgQueryTimeMs = vulkanStats.avgQueryTimeMs;
        stats.throughputQPS = vulkanStats.throughputQPS;
    }
    #endif

    // Expose per-index budget usage when a VRAM limit is active
    if (!pImpl->vramBudgetTag.empty()) {
        stats.vramUsageBytes = pImpl->vramAllocatedBytes;
    }

    // Merge oversubscription statistics when the manager is active.
    if (pImpl->oversubManager) {
        const auto osmStats = pImpl->oversubManager->getStats();
        stats.oversubscriptionActive  = true;
        stats.oversubHotPartitions    = osmStats.hot_partitions;
        stats.oversubColdPartitions   = osmStats.cold_partitions;
        stats.oversubEvictions        = osmStats.evictions;
        stats.oversubLoads            = osmStats.loads;
        stats.oversubPrefetchHitRate  = osmStats.prefetch_hit_rate;
        // Reflect the VRAM used by the oversubscription manager.
        stats.vramUsageBytes          = osmStats.vram_used_bytes;
    }
    
    return stats;
}

GPUMemoryOversubscriptionManager::Stats
GPUVectorIndex::getOversubscriptionStats() const {
    if (pImpl->oversubManager) {
        return pImpl->oversubManager->getStats();
    }
    return GPUMemoryOversubscriptionManager::Stats{};
}

bool GPUVectorIndex::switchBackend(Backend backend) {
    if (!pImpl->initialized) {
        return false;
    }
    
    // Can't switch if backend is not available
    auto available = getAvailableBackends();
    if (std::find(available.begin(), available.end(), backend) == available.end()) {
        THEMIS_WARN("GPUVectorIndex: Requested backend not available");
        return false;
    }
    
    // Already using this backend
    if (pImpl->activeBackend == backend) {
        return true;
    }

    const Backend previousBackend = pImpl->config.backend;
    int dim = pImpl->dimension;

    auto restorePreviousBackend = [&]() -> bool {
        pImpl->shutdown();
        pImpl->config.backend = previousBackend;
        if (!pImpl->initialize(dim)) {
            THEMIS_ERROR("GPUVectorIndex: Failed to restore previous backend after switch failure");
            return false;
        }

        if (pImpl->oversubManager && !pImpl->vectorData.empty()) {
            pImpl->rebuildOversubPartitions();
        }

        pImpl->stats.numVectors = pImpl->vectorData.size();
        return true;
    };
    
    // Shutdown current backend
    pImpl->shutdown();
    
    // Switch to new backend
    pImpl->config.backend = backend;
    if (!pImpl->initialize(dim)) {
        THEMIS_WARN("GPUVectorIndex: Backend switch to requested backend failed; restoring previous backend");
        restorePreviousBackend();
        return false;
    }

    if (pImpl->oversubManager && !pImpl->vectorData.empty()) {
        pImpl->rebuildOversubPartitions();
    }

    pImpl->stats.numVectors = pImpl->vectorData.size();
    
    return true;
}

std::vector<GPUVectorIndex::Backend> GPUVectorIndex::getAvailableBackends() const {
    std::vector<Backend> backends;
    
    // CPU is always available
    backends.push_back(Backend::CPU);
    
    // Check for HIP availability (AMD GPUs)
#ifdef THEMIS_ENABLE_HIP
    if (themis::acceleration::HIPVectorBackend().isAvailable()) {
        backends.push_back(Backend::HIP);
    }
#endif

    // Check for CUDA availability (NVIDIA GPUs)
#ifdef THEMIS_ENABLE_CUDA
    {
        themis::acceleration::CUDAVectorBackend checkBackend;
        if (checkBackend.isAvailable()) {
            backends.push_back(Backend::CUDA);
        }
    }
#endif

#ifdef THEMIS_ENABLE_VULKAN
    if (pImpl->isVulkanAvailable()) {
        backends.push_back(Backend::VULKAN);
    }
#endif
    
    return backends;
}

// =============================================================================
// Vulkan Backend Implementation
// =============================================================================

} // namespace index
} // namespace themis

