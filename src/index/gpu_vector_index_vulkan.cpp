/**
 * @file gpu_vector_index_vulkan.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 87/100
 * @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=2, H=5, M=10, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "index/gpu_vector_index.h"
#include "llm/lora_framework/vulkan_context.h"
#include "llm/lora_framework/vulkan_buffer.h"
#include "llm/lora_framework/vulkan_pipeline.h"
#include "utils/logger.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

// Check if Vulkan headers are available
#if defined(__has_include)
#  if __has_include(<vulkan/vulkan.h>)
#    define THEMIS_HAS_VULKAN_IMPL 1
#  else
#    define THEMIS_HAS_VULKAN_IMPL 0
#  endif
#else
#  if defined(THEMIS_ENABLE_VULKAN)
#    define THEMIS_HAS_VULKAN_IMPL 1
#  else
#    define THEMIS_HAS_VULKAN_IMPL 0
#  endif
#endif

// Forward declaration (visible in both #if and #else branches)
namespace themis {
namespace index {

#ifndef THEMIS_VULKAN_VECTOR_INDEX_BACKEND_DECLARED
/**
 * @brief Vulkan backend implementation for GPU vector indexing (forward declaration)
 */
class VulkanVectorIndexBackend {
public:
    explicit VulkanVectorIndexBackend(const GPUVectorIndex::Config& config);
    ~VulkanVectorIndexBackend();
    
    bool initialize(int dimension);
    void shutdown();
    bool uploadVectors(const std::vector<std::vector<float>>& vectors);
    std::vector<std::pair<float, size_t>> searchIndices(const std::vector<float>& query, size_t k);
    std::vector<std::vector<std::pair<float, size_t>>> searchBatchIndices(
        const std::vector<std::vector<float>>& queries, size_t k);
    std::vector<GPUVectorIndex::SearchResult> search(const std::vector<float>& query, size_t k);
    std::vector<std::vector<GPUVectorIndex::SearchResult>> searchBatch(
        const std::vector<std::vector<float>>& queries, size_t k);
    GPUVectorIndex::Statistics getStatistics() const;
    bool isInitialized() const;

    // Callback bridge types for non-Vulkan builds (VVI-BRIDGE)
    using InitializeFn   = std::function<bool(int /*dimension*/)>;
    using UploadFn       = std::function<bool(const std::vector<std::vector<float>>&)>;
    using SearchFn       = std::function<std::vector<GPUVectorIndex::SearchResult>(
                               const std::vector<float>&, size_t /*k*/)>;
    using SearchBatchFn  = std::function<std::vector<std::vector<GPUVectorIndex::SearchResult>>(
                               const std::vector<std::vector<float>>&, size_t /*k*/)>;

    static void setInitializeFn(InitializeFn fn) {
        std::lock_guard<std::mutex> lk(initializeFnMutex());
        initializeFnStorage() = std::move(fn);
    }
    static void setUploadFn(UploadFn fn) {
        std::lock_guard<std::mutex> lk(uploadFnMutex());
        uploadFnStorage() = std::move(fn);
    }
    static void setSearchFn(SearchFn fn) {
        std::lock_guard<std::mutex> lk(searchFnMutex());
        searchFnStorage() = std::move(fn);
    }
    static void setSearchBatchFn(SearchBatchFn fn) {
        std::lock_guard<std::mutex> lk(searchBatchFnMutex());
        searchBatchFnStorage() = std::move(fn);
    }

    static std::mutex& initializeFnMutex()  { static std::mutex m; return m; }
    static InitializeFn&  initializeFnStorage()  { static InitializeFn f;  return f; }
    static std::mutex& uploadFnMutex()      { static std::mutex m; return m; }
    static UploadFn&      uploadFnStorage()      { static UploadFn f;      return f; }
    static std::mutex& searchFnMutex()      { static std::mutex m; return m; }
    static SearchFn&      searchFnStorage()      { static SearchFn f;      return f; }
    static std::mutex& searchBatchFnMutex() { static std::mutex m; return m; }
    static SearchBatchFn& searchBatchFnStorage() { static SearchBatchFn f; return f; }

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};
#endif

} // namespace index
} // namespace themis

#if THEMIS_HAS_VULKAN_IMPL

namespace themis {
namespace index {

/**
 * @brief Move-only RAII scope guard for a raw VkBuffer + VkDeviceMemory pair.
 *
 * Use this for ad-hoc allocations (e.g., staging buffers in Wave-B compute
 * shader kernels) that do not go through lora::vulkan::VulkanBuffer.  For
 * buffers that need re-use, upload helpers, or copy_from, prefer
 * lora::vulkan::VulkanBuffer instead.
 *
 * Removal plan: Once Wave-B GPU ANN kernels (L2/cosine/inner-product) are
 * integrated, graduate any remaining uses to lora::vulkan::VulkanBuffer and
 * remove this guard.  Roadmap ref: src/index/FUTURE_ENHANCEMENTS.md
 * §"GPU Vector Index (Vulkan)" — Wave-B Q4 2026.
 */
struct VkBufferRaii {
    VkDevice       device = VK_NULL_HANDLE;
    VkBuffer       buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;

    VkBufferRaii() = default;
    VkBufferRaii(VkDevice dev, VkBuffer buf, VkDeviceMemory mem) noexcept
        : device(dev), buffer(buf), memory(mem) {}

    ~VkBufferRaii() noexcept {
        if (buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, buffer, nullptr);
            buffer = VK_NULL_HANDLE;
        }
        if (memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, memory, nullptr);
            memory = VK_NULL_HANDLE;
        }
    }

    // Move-only
    VkBufferRaii(const VkBufferRaii&) = delete;
    VkBufferRaii& operator=(const VkBufferRaii&) = delete;

    VkBufferRaii(VkBufferRaii&& o) noexcept
        : device(o.device), buffer(o.buffer), memory(o.memory) {
        o.device = VK_NULL_HANDLE;
        o.buffer = VK_NULL_HANDLE;
        o.memory = VK_NULL_HANDLE;
    }
    VkBufferRaii& operator=(VkBufferRaii&& o) noexcept {
        if (this != &o) {
            if (buffer != VK_NULL_HANDLE) {
              vkDestroyBuffer(device, buffer, nullptr);
            }
            if (memory != VK_NULL_HANDLE) {
              vkFreeMemory(device, memory, nullptr);
            }
            device = o.device;  buffer = o.buffer;  memory = o.memory;
            o.device = VK_NULL_HANDLE;
            o.buffer = VK_NULL_HANDLE;
            o.memory = VK_NULL_HANDLE;
        }
        return *this;
    }

    /// Release ownership without destroying (e.g. after successful vkBindBufferMemory).
    void release() noexcept {
        device = VK_NULL_HANDLE;
        buffer = VK_NULL_HANDLE;
        memory = VK_NULL_HANDLE;
    }
};

/**
 * @brief Implementation class for Vulkan backend
 */
class VulkanVectorIndexBackend::Impl {
public:
    struct QueryFingerprint {
        size_t size = 0;
        float first = 0.0f;
        float middle = 0.0f;
        float last = 0.0f;
    };

    static QueryFingerprint makeQueryFingerprint(const std::vector<float>& query) {
        QueryFingerprint fp;
        fp.size = query.size();
        if (!query.empty()) {
            fp.first = query.front();
            fp.middle = query[query.size() / 2];
            fp.last = query.back();
        }
        return fp;
    }

    static bool fingerprintEqual(const QueryFingerprint& a, const QueryFingerprint& b) {
        return a.size == b.size &&
               a.first == b.first &&
               a.middle == b.middle &&
               a.last == b.last;
    }

        static void bindSearchBuffersIfNeeded(
            lora::vulkan::VulkanComputePipeline* pipeline,
            lora::vulkan::VulkanBuffer* queryBuffer,
            lora::vulkan::VulkanBuffer* vectorBuffer,
            lora::vulkan::VulkanBuffer* distanceBuffer,
            lora::vulkan::VulkanComputePipeline*& lastPipeline,
            lora::vulkan::VulkanBuffer*& lastQueryBuffer,
            lora::vulkan::VulkanBuffer*& lastVectorBuffer,
            lora::vulkan::VulkanBuffer*& lastDistanceBuffer) {

            if (pipeline == nullptr || queryBuffer == nullptr ||
                vectorBuffer == nullptr || distanceBuffer == nullptr) {
                return;
            }

            if (pipeline != lastPipeline || queryBuffer != lastQueryBuffer) {
                pipeline->bind_buffer(0, *queryBuffer);
                lastQueryBuffer = queryBuffer;
            }
            if (pipeline != lastPipeline || vectorBuffer != lastVectorBuffer) {
                pipeline->bind_buffer(1, *vectorBuffer);
                lastVectorBuffer = vectorBuffer;
            }
            if (pipeline != lastPipeline || distanceBuffer != lastDistanceBuffer) {
                pipeline->bind_buffer(2, *distanceBuffer);
                lastDistanceBuffer = distanceBuffer;
            }
            lastPipeline = pipeline;
        }

    static std::vector<std::pair<float, size_t>> selectTopK(
        const float* distances, size_t count, size_t k) {
        const size_t topK = std::min(k, count);
        std::vector<std::pair<float, size_t>> top;
        if (topK == 0 || distances == nullptr) {
            return top;
        }

        top.reserve(topK);
        for (size_t i = 0; i < topK; ++i) {
            top.emplace_back(distances[i], i);
        }

        const auto compareDistance = [](const std::pair<float, size_t>& a,
                                        const std::pair<float, size_t>& b) {
            return a.first < b.first;
        };

        std::make_heap(top.begin(), top.end(), compareDistance);

        for (size_t i = topK; i < count; ++i) {
            const float distance = distances[i];
            if (distance < top.front().first) {
                std::pop_heap(top.begin(), top.end(), compareDistance);
                top.back() = {distance, i};
                std::push_heap(top.begin(), top.end(), compareDistance);
            }
        }

        std::sort_heap(top.begin(), top.end(), compareDistance);
        return top;
    }

    /**
     * @brief Constructor
     * @param config Configuration for the backend
     */
    explicit Impl(const GPUVectorIndex::Config& config)
        : config_(config), context_(nullptr), initialized_(false), dimension_(0) {}
    
    /**
     * @brief Destructor - cleanup Vulkan resources
     */
    ~Impl() {
        shutdown();
    }
    
    /**
     * @brief Initialize Vulkan backend
     * @param dimension Vector dimension
     * @return true if initialization successful
     */
    bool initialize([[maybe_unused]] int dimension) {
        if (initialized_) {
            return true;
        }
        
        dimension_ = dimension;
        
        try {
            // Create Vulkan context
            context_ = std::make_unique<lora::vulkan::VulkanContext>();
            
            // Initialize with device ID and validation (debug mode only)
            bool enableValidation = config_.enableValidation;
            #ifdef _DEBUG
            if (!enableValidation) {
                enableValidation = true; // Force validation in debug
            }
            #endif
            
            if (!context_->initialize(config_.deviceId, enableValidation)) {
                THEMIS_ERROR("VulkanVectorIndexBackend: Failed to initialize Vulkan context");
                return false;
            }
            
            // Query device properties
            const auto& props = context_->device_properties();
            THEMIS_INFO("VulkanVectorIndexBackend: Using GPU: {}", props.deviceName);
            THEMIS_INFO("VulkanVectorIndexBackend: Vulkan API Version: {}.{}.{}",
                        VK_VERSION_MAJOR(props.apiVersion),
                        VK_VERSION_MINOR(props.apiVersion),
                        VK_VERSION_PATCH(props.apiVersion));
            
            // Create compute pipelines for distance metrics
            if (!createPipelines()) {
                THEMIS_ERROR("VulkanVectorIndexBackend: Failed to create compute pipelines");
                shutdown();
                return false;
            }
            
            initialized_ = true;
            return true;
            
        } catch (const std::exception& e) {
            THEMIS_ERROR("VulkanVectorIndexBackend: Initialization error: {}", e.what());
            shutdown();
            return false;
        }
    }
    
    /**
     * @brief Shutdown and cleanup Vulkan resources
     */
    void shutdown() {
        // Always clean up resources, even if initialization failed
        
        // Clear pipelines
        l2_pipeline_.reset();
        cosine_pipeline_.reset();
        inner_product_pipeline_.reset();
        topk_pipeline_.reset();
        lastBoundPipeline_ = nullptr;
        lastBoundQueryBuffer_ = nullptr;
        lastBoundVectorBuffer_ = nullptr;
        lastBoundDistanceBuffer_ = nullptr;
        
        // Clear buffers
        query_buffer_.reset();
        vector_buffer_.reset();
        distance_buffer_.reset();
        result_buffer_.reset();
        cachedSingleQuery_.clear();
        cachedSingleQueryValid_ = false;
        cachedSearchResult_.clear();
        cachedSearchResultValid_ = false;
        
        // Clear context
        if (context_) {
            context_->cleanup();
            context_.reset();
        }
        
        initialized_ = false;
    }
    
    /**
     * @brief Add vectors to GPU memory
     * @param vectors Vector data to upload
     * @return true if successful
     */
    bool uploadVectors(const std::vector<std::vector<float>>& vectors) {
        if (!initialized_ || vectors.empty()) {
            return false;
        }
        
        try {
            // Flatten vector data with a single allocation to avoid repeated
            // growth/copy overhead for large batches.
            const size_t vectorsCount = vectors.size();
            const size_t dim = static_cast<size_t>(dimension_);
            const size_t totalFloatCount = vectorsCount * dim;
            const size_t totalSize = totalFloatCount * sizeof(float);
            std::vector<float> flatData(totalFloatCount);

            {
                size_t offset = 0;
                for (const auto& vec : vectors) {
                    if (static_cast<int>(vec.size()) != static_cast<size_t>(dimension_)) {
                        THEMIS_ERROR("VulkanVectorIndexBackend: Vector dimension mismatch");
                        return false;
                    }
                    std::copy(vec.begin(), vec.end(), flatData.begin() + static_cast<std::ptrdiff_t>(offset));
                    offset += dim;
                }
            }
            
            // Create or recreate vector buffer
            vector_buffer_ = std::make_unique<lora::vulkan::VulkanBuffer>(
                context_.get(), totalSize, lora::vulkan::VulkanBuffer::Usage::DeviceLocal);
            
            // Upload data
            vector_buffer_->upload(flatData.data(), totalSize);
            
            num_vectors_ = vectors.size();
            cachedSingleQueryValid_ = false;
            cachedSearchResultValid_ = false;
            return true;
            
        } catch (const std::exception& e) {
            THEMIS_ERROR("VulkanVectorIndexBackend: Upload error: {}", e.what());
            return false;
        }
    }
    
    /**
     * @brief Search for nearest neighbors using Vulkan compute (returns indices)
     * @param query Query vector
     * @param k Number of nearest neighbors
     * @return Pairs of (distance, index) sorted by distance
     */
    std::vector<std::pair<float, size_t>> searchIndices(
        const std::vector<float>& query, size_t k) {
        
        if (!initialized_ || query.size() != static_cast<size_t>(dimension_)) {
            THEMIS_WARN("VulkanVectorIndexBackend::searchIndices: uninitialized or query dimension mismatch (initialized={} dim={} expected={})",
                        initialized_, query.size(), static_cast<size_t>(dimension_));
            return {};
        }

        if (num_vectors_ == 0) {
            THEMIS_DEBUG("VulkanVectorIndexBackend::searchIndices: no vectors loaded (num_vectors_=0)");
            return {};
        }
        
        try {
            auto startTime = std::chrono::steady_clock::now();
            
            // Create or update query buffer
            size_t querySize = dimension_ * sizeof(float);
            if (!query_buffer_ || query_buffer_->size() < querySize) {
                query_buffer_ = std::make_unique<lora::vulkan::VulkanBuffer>(
                    context_.get(), querySize, lora::vulkan::VulkanBuffer::Usage::DeviceLocal);
            }
            const QueryFingerprint queryFingerprint = makeQueryFingerprint(query);
            const bool hasPotentialMatch = cachedSingleQueryValid_ &&
                                           fingerprintEqual(queryFingerprint, cachedSingleQueryFingerprint_);
            bool queryChanged = true;
            if (hasPotentialMatch) {
                queryChanged = !std::equal(query.begin(), query.end(), cachedSingleQuery_.begin());
            }
            if (!queryChanged && cachedSearchResultValid_ &&
                cachedSearchResultMetric_ == config_.metric &&
                cachedSearchResultNumVectors_ == num_vectors_ &&
                cachedSearchResultK_ >= k) {
                std::vector<std::pair<float, size_t>> cached;
                cached.reserve(k);
                cached.insert(
                    cached.end(),
                    cachedSearchResult_.begin(),
                    cachedSearchResult_.begin() + static_cast<std::ptrdiff_t>(k));
                return cached;
            }

            if (queryChanged) {
                query_buffer_->upload(query.data(), querySize);
                cachedSingleQuery_ = query;
                cachedSingleQueryFingerprint_ = queryFingerprint;
                cachedSingleQueryValid_ = true;
                cachedSearchResultValid_ = false;
            }
            
            // Create or update distance buffer
            size_t distanceSize = num_vectors_ * sizeof(float);
            if (!distance_buffer_ || distance_buffer_->size() < distanceSize) {
                distance_buffer_ = std::make_unique<lora::vulkan::VulkanBuffer>(
                    context_.get(), distanceSize, lora::vulkan::VulkanBuffer::Usage::DeviceLocal);
            }
            
            // Select pipeline based on distance metric
            lora::vulkan::VulkanComputePipeline* pipeline = nullptr;
            switch (config_.metric) {
                case GPUVectorIndex::DistanceMetric::L2:
                    pipeline = l2_pipeline_.get();
                    break;
                case GPUVectorIndex::DistanceMetric::COSINE:
                    pipeline = cosine_pipeline_.get();
                    break;
                case GPUVectorIndex::DistanceMetric::INNER_PRODUCT:
                    pipeline = inner_product_pipeline_.get();
                    break;
                default:
                    THEMIS_ERROR("VulkanVectorIndexBackend: Unknown distance metric");
                    return {};
            }
            
            if (!pipeline || !pipeline->is_ready()) {
                THEMIS_ERROR("VulkanVectorIndexBackend: Pipeline not ready");
                return {};
            }
            
            // Bind buffers only when descriptor bindings changed.
            bindSearchBuffersIfNeeded(
                pipeline,
                query_buffer_.get(),
                vector_buffer_.get(),
                distance_buffer_.get(),
                lastBoundPipeline_,
                lastBoundQueryBuffer_,
                lastBoundVectorBuffer_,
                lastBoundDistanceBuffer_);
            
            // Set push constants
            struct PushConstants {
                uint32_t numQueries = 0;
                uint32_t numVectors;
                uint32_t dimension = {};
            } pushConstants = {
                1,                        // Single query
                static_cast<uint32_t>(num_vectors_),
                static_cast<uint32_t>(dimension_)
            };
            pipeline->set_push_constants(&pushConstants, sizeof(PushConstants));
            
            // Dispatch compute shader
            // Local size is 16x16, so we need to dispatch enough workgroups
            uint32_t workgroupsX = (static_cast<uint32_t>(num_vectors_) + 15u) / 16u;
            uint32_t workgroupsY = 1; // Single query
            pipeline->dispatch(workgroupsX, workgroupsY, 1);
            
            // Wait for completion — detect GPU hang via 30-second timeout
            if (!pipeline->wait()) {
                THEMIS_ERROR("VulkanVectorIndexBackend: pipeline->wait() timed out (GPU hang?)");
                return {};
            }
            
            // Download results into reusable scratch space.
            distanceScratch_.resize(num_vectors_);
            distance_buffer_->download(distanceScratch_.data(), distanceSize);

            const auto distanceIndices = selectTopK(distanceScratch_.data(), num_vectors_, k);
            cachedSearchResult_ = distanceIndices;
            cachedSearchResultK_ = distanceIndices.size();
            cachedSearchResultNumVectors_ = num_vectors_;
            cachedSearchResultMetric_ = config_.metric;
            cachedSearchResultValid_ = true;
            
            auto endTime = std::chrono::steady_clock::now();
            updateQueryStats(startTime, endTime);
            
            return distanceIndices;
            
        } catch (const std::exception& e) {
            THEMIS_ERROR("VulkanVectorIndexBackend: Search error: {}", e.what());
            return {};
        }
    }
    
    /**
     * @brief Search for nearest neighbors using Vulkan compute
     * @param query Query vector
     * @param k Number of nearest neighbors
     * @return Search results (without IDs, to be filled by main implementation)
     */
    std::vector<GPUVectorIndex::SearchResult> search(
        const std::vector<float>& query, size_t k) {
        
        auto indices = searchIndices(query, k);
        
        // Convert to SearchResult format (IDs will be filled by main implementation)
        std::vector<GPUVectorIndex::SearchResult> results = {};

        results.reserve(indices.size());
        for (const auto& [distance, index] : indices) {
            results.push_back({"", distance});
        }
        
        return results;
    }

    std::vector<std::vector<GPUVectorIndex::SearchResult>> searchBatch(
        const std::vector<std::vector<float>>& queries, size_t k) {
        const auto indexedResults = searchBatchIndices(queries, k);

        std::vector<std::vector<GPUVectorIndex::SearchResult>> results;
        results.reserve(indexedResults.size());

        for (const auto& queryResults : indexedResults) {
            std::vector<GPUVectorIndex::SearchResult> converted = {};

            converted.reserve(queryResults.size());
            for (const auto& [distance, index] : queryResults) {
                (void)index;
                converted.push_back({"", distance});
            }
            results.push_back(std::move(converted));
        }

        return results;
    }
    
    /**
     * @brief Batch search for nearest neighbors returning indices
     * @param queries Query vectors
     * @param k Number of nearest neighbors
     * @return Batch search results as (distance, index) pairs
     */
    std::vector<std::vector<std::pair<float, size_t>>> searchBatchIndices(
        const std::vector<std::vector<float>>& queries, size_t k) {
        if (queries.empty() || !initialized_) {
            THEMIS_DEBUG("VulkanVectorIndexBackend::searchBatchIndices: empty queries or not initialized (queries={} initialized={})",
                        queries.size(), initialized_);
            return {};
        }

        // For small batches, use the single-query path to avoid dispatch overhead.
        if (static_cast<int>(queries.size()) < 4) {
            std::vector<std::vector<std::pair<float, size_t>>> results;
            results.reserve(queries.size());
            for (const auto& query : queries) {
                results.push_back(searchIndices(query, k));
            }
            return results;
        }

        try {
            auto startTime = std::chrono::steady_clock::now();
            const size_t numQueries = queries.size();

            const size_t dim = static_cast<size_t>(dimension_);
            std::vector<float> flatQueries(numQueries * dim);
            {
                size_t queryOffset = 0;
                for (const auto& query : queries) {
                    if (static_cast<int>(query.size()) != static_cast<size_t>(dimension_)) {
                        THEMIS_ERROR("VulkanVectorIndexBackend: Query dimension mismatch in batch");
                        return {};
                    }
                    std::copy(query.begin(), query.end(),
                              flatQueries.begin() + static_cast<std::ptrdiff_t>(queryOffset));
                    queryOffset += dim;
                }
            }

            const size_t queryBufferSize = numQueries * dim * sizeof(float);
            if (!query_buffer_ || query_buffer_->size() < queryBufferSize) {
                query_buffer_ = std::make_unique<lora::vulkan::VulkanBuffer>(
                    context_.get(), queryBufferSize, lora::vulkan::VulkanBuffer::Usage::DeviceLocal);
            }
            query_buffer_->upload(flatQueries.data(), queryBufferSize);
            cachedSingleQueryValid_ = false;
            cachedSearchResultValid_ = false;

            const size_t totalDistances = numQueries * num_vectors_;
            const size_t distanceBufferSize = totalDistances * sizeof(float);
            if (!distance_buffer_ || distance_buffer_->size() < distanceBufferSize) {
                distance_buffer_ = std::make_unique<lora::vulkan::VulkanBuffer>(
                    context_.get(), distanceBufferSize, lora::vulkan::VulkanBuffer::Usage::DeviceLocal);
            }

            lora::vulkan::VulkanComputePipeline* pipeline = nullptr;
            switch (config_.metric) {
                case GPUVectorIndex::DistanceMetric::L2:
                    pipeline = l2_pipeline_.get();
                    break;
                case GPUVectorIndex::DistanceMetric::COSINE:
                    pipeline = cosine_pipeline_.get();
                    break;
                case GPUVectorIndex::DistanceMetric::INNER_PRODUCT:
                    pipeline = inner_product_pipeline_.get();
                    break;
                default:
                    THEMIS_ERROR("VulkanVectorIndexBackend: Unknown distance metric");
                    return {};
            }

            if (!pipeline || !pipeline->is_ready()) {
                THEMIS_ERROR("VulkanVectorIndexBackend: Pipeline not ready");
                return {};
            }

            bindSearchBuffersIfNeeded(
                pipeline,
                query_buffer_.get(),
                vector_buffer_.get(),
                distance_buffer_.get(),
                lastBoundPipeline_,
                lastBoundQueryBuffer_,
                lastBoundVectorBuffer_,
                lastBoundDistanceBuffer_);

            struct PushConstants {
                uint32_t numQueries = 0;
                uint32_t numVectors;
                uint32_t dimension = {};
            } pushConstants = {
                static_cast<uint32_t>(numQueries),
                static_cast<uint32_t>(num_vectors_),
                static_cast<uint32_t>(dimension_)
            };

            pipeline->set_push_constants(&pushConstants, sizeof(PushConstants));
            const uint32_t workgroupsX = (static_cast<uint32_t>(num_vectors_) + 15u) / 16u;
            const uint32_t workgroupsY = (static_cast<uint32_t>(numQueries) + 15u) / 16u;
            pipeline->dispatch(workgroupsX, workgroupsY, 1);
            if (!pipeline->wait()) {
                THEMIS_ERROR("VulkanVectorIndexBackend: pipeline->wait() timed out in batchSearch (GPU hang?)");
                return {};
            }

            allDistancesScratch_.resize(totalDistances);
            distance_buffer_->download(allDistancesScratch_.data(), distanceBufferSize);

            std::vector<std::vector<std::pair<float, size_t>>> results;
            results.reserve(numQueries);

            for (size_t q = 0; q < numQueries; ++q) {
                const size_t offset = q * num_vectors_;
                results.push_back(
                    selectTopK(allDistancesScratch_.data() + static_cast<std::ptrdiff_t>(offset),
                               num_vectors_,
                               k));
            }

            auto endTime = std::chrono::steady_clock::now();
            updateQueryStats(startTime, endTime);
            return results;

        } catch (const std::exception& e) {
            THEMIS_ERROR("VulkanVectorIndexBackend: Batch search error: {}", e.what());
            std::vector<std::vector<std::pair<float, size_t>>> results;
            results.reserve(queries.size());
            for (const auto& query : queries) {
                results.push_back(searchIndices(query, k));
            }
            return results;
        }
    }
    
    /**
     * @brief Get statistics
     */
    GPUVectorIndex::Statistics getStatistics() const {
        GPUVectorIndex::Statistics stats;
        stats.numVectors = num_vectors_;
        stats.dimension = dimension_;
        stats.activeBackend = GPUVectorIndex::Backend::VULKAN;  // Vulkan backend
        stats.isGPUActive = initialized_;
        stats.vramUsageBytes = calculateVRAMUsage();
        stats.avgQueryTimeMs = avg_query_time_ms_;
        stats.throughputQPS = throughput_qps_;
        return stats;
    }
    
    /**
     * @brief Check if backend is initialized
     */
    bool isInitialized() const {
        return initialized_;
    }

    /**
     * @brief Update query statistics
     * 
     * Note: This method is const because it only updates cached statistics
     * (marked mutable) and doesn't modify the logical state of the backend.
     */
    void updateQueryStats(const std::chrono::steady_clock::time_point& start,
                         const std::chrono::steady_clock::time_point& end) const {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double queryTimeMs = duration.count() / 1000.0;
        
        // Simple exponential moving average
        if (avg_query_time_ms_ == 0.0) {
            avg_query_time_ms_ = queryTimeMs;
        } else {
            avg_query_time_ms_ = 0.9 * avg_query_time_ms_ + 0.1 * queryTimeMs;
        }
        
        // Estimate throughput based on query time
        if (avg_query_time_ms_ > 0.0) {
            throughput_qps_ = 1000.0 / avg_query_time_ms_;
        }
    }
    
    /**
     * @brief Create compute pipelines for distance metrics
     */
    bool createPipelines() {
        try {
            // Try multiple shader search paths
            std::vector<std::string> searchPaths = {
                "shaders/vector_index/",                    // Build directory
                "../shaders/vector_index/",                 // One level up
                "build/shaders/vector_index/",              // Workspace-local build folder
                "build/windows-release/shaders/vector_index/",       // Common Windows preset
                "build/windows-bench-release/shaders/vector_index/", // Benchmark preset
                "share/themis/shaders/vector_index/",       // Install directory
                "/usr/share/themis/shaders/vector_index/",  // System install
                "/usr/local/share/themis/shaders/vector_index/"  // Local install
            };
            
            std::string shaderDir = {};
            for (const auto& path : searchPaths) {
                std::string testPath = path + "l2_distance.comp.spv";
                std::ifstream testFile(testPath);
                if (testFile.good()) {
                    shaderDir = path;
                    break;
                }
            }
            
            if (shaderDir.empty()) {
                THEMIS_ERROR("VulkanVectorIndexBackend: Failed to locate shader directory");
                THEMIS_ERROR("VulkanVectorIndexBackend: Searched paths:");
                for (const auto& path : searchPaths) {
                    THEMIS_ERROR("  - {}", path);
                }
                return false;
            }
            
            THEMIS_INFO("VulkanVectorIndexBackend: Using shader directory: {}", shaderDir);
            
            // Create L2 distance pipeline
            std::string l2ShaderPath = shaderDir + "l2_distance.comp.spv";
            l2_pipeline_ = std::make_unique<lora::vulkan::VulkanComputePipeline>(
                context_.get(), l2ShaderPath);
            
            // Push constants: numQueries, numVectors, dimension
            size_t pushConstantSize = sizeof([[maybe_unused]] uint32_t) * 3;
            if (!l2_pipeline_->create(pushConstantSize)) {
                THEMIS_ERROR("VulkanVectorIndexBackend: Failed to create L2 distance pipeline");
                return false;
            }
            
            // Create Cosine distance pipeline
            std::string cosineShaderPath = shaderDir + "cosine_distance.comp.spv";
            cosine_pipeline_ = std::make_unique<lora::vulkan::VulkanComputePipeline>(
                context_.get(), cosineShaderPath);
            if (!cosine_pipeline_->create(pushConstantSize)) {
                THEMIS_ERROR("VulkanVectorIndexBackend: Failed to create Cosine distance pipeline");
                return false;
            }
            
            // Create Inner Product distance pipeline
            std::string innerProductShaderPath = shaderDir + "inner_product_distance.comp.spv";
            inner_product_pipeline_ = std::make_unique<lora::vulkan::VulkanComputePipeline>(
                context_.get(), innerProductShaderPath);
            if (!inner_product_pipeline_->create(pushConstantSize)) {
                THEMIS_ERROR("VulkanVectorIndexBackend: Failed to create Inner Product pipeline");
                return false;
            }
            
            THEMIS_INFO("VulkanVectorIndexBackend: All compute pipelines created successfully");
            return true;
            
        } catch (const std::exception& e) {
            THEMIS_ERROR("VulkanVectorIndexBackend: Pipeline creation error: {}", e.what());
            return false;
        }
    }
    
    /**
     * @brief Calculate VRAM usage
     */
    size_t calculateVRAMUsage() const {
        size_t usage = 0;
        if (vector_buffer_) {
          usage += vector_buffer_->size();
        }
        if (query_buffer_) {
          usage += query_buffer_->size();
        }
        if (distance_buffer_) {
          usage += distance_buffer_->size();
        }
        if (result_buffer_) {
          usage += result_buffer_->size();
        }
        return usage;
    }
    
    // Configuration
    GPUVectorIndex::Config config_;
    
    // Vulkan resources
    std::unique_ptr<lora::vulkan::VulkanContext> context_;
    std::unique_ptr<lora::vulkan::VulkanComputePipeline> l2_pipeline_;
    std::unique_ptr<lora::vulkan::VulkanComputePipeline> cosine_pipeline_;
    std::unique_ptr<lora::vulkan::VulkanComputePipeline> inner_product_pipeline_;
    std::unique_ptr<lora::vulkan::VulkanComputePipeline> topk_pipeline_;
    
    // Buffers
    std::unique_ptr<lora::vulkan::VulkanBuffer> query_buffer_;
    std::unique_ptr<lora::vulkan::VulkanBuffer> vector_buffer_;
    std::unique_ptr<lora::vulkan::VulkanBuffer> distance_buffer_;
    std::unique_ptr<lora::vulkan::VulkanBuffer> result_buffer_;
    
    // State
    bool initialized_;
    int dimension_;
    size_t num_vectors_ = 0;

    // Reused CPU scratch buffers to reduce per-query allocation overhead.
    std::vector<float> distanceScratch_;
    std::vector<float> allDistancesScratch_;
    std::vector<float> cachedSingleQuery_;
    QueryFingerprint cachedSingleQueryFingerprint_;
    bool cachedSingleQueryValid_ = false;
    std::vector<std::pair<float, size_t>> cachedSearchResult_;
    size_t cachedSearchResultK_ = 0;
    size_t cachedSearchResultNumVectors_ = 0;
    GPUVectorIndex::DistanceMetric cachedSearchResultMetric_ =
        GPUVectorIndex::DistanceMetric::L2;
    bool cachedSearchResultValid_ = false;

    // Descriptor binding cache for repeated search dispatches.
    lora::vulkan::VulkanComputePipeline* lastBoundPipeline_ = nullptr;
    lora::vulkan::VulkanBuffer* lastBoundQueryBuffer_ = nullptr;
    lora::vulkan::VulkanBuffer* lastBoundVectorBuffer_ = nullptr;
    lora::vulkan::VulkanBuffer* lastBoundDistanceBuffer_ = nullptr;
    
    // Statistics
    mutable double avg_query_time_ms_ = 0.0;
    mutable double throughput_qps_ = 0.0;
};

// =============================================================================
// VulkanVectorIndexBackend public interface (PIMPL)
// =============================================================================

VulkanVectorIndexBackend::VulkanVectorIndexBackend(const GPUVectorIndex::Config& config)
    : pImpl(std::make_unique<Impl>(config)) {
}

VulkanVectorIndexBackend::~VulkanVectorIndexBackend() = default;

bool VulkanVectorIndexBackend::initialize([[maybe_unused]] int dimension) {
    return pImpl->initialize(dimension);
}

void VulkanVectorIndexBackend::shutdown() {
    pImpl->shutdown();
}

bool VulkanVectorIndexBackend::uploadVectors(const std::vector<std::vector<float>>& vectors) {
    return pImpl->uploadVectors(vectors);
}

std::vector<std::pair<float, size_t>> VulkanVectorIndexBackend::searchIndices(
    const std::vector<float>& query, size_t k) {
    return pImpl->searchIndices(query, k);
}

std::vector<std::vector<std::pair<float, size_t>>> VulkanVectorIndexBackend::searchBatchIndices(
    const std::vector<std::vector<float>>& queries, size_t k) {
    return pImpl->searchBatchIndices(queries, k);
}

std::vector<GPUVectorIndex::SearchResult> VulkanVectorIndexBackend::search(
    const std::vector<float>& query, size_t k) {
    return pImpl->search(query, k);
}

std::vector<std::vector<GPUVectorIndex::SearchResult>> VulkanVectorIndexBackend::searchBatch(
    const std::vector<std::vector<float>>& queries, size_t k) {
    return pImpl->searchBatch(queries, k);
}

GPUVectorIndex::Statistics VulkanVectorIndexBackend::getStatistics() const {
    return pImpl->getStatistics();
}

bool VulkanVectorIndexBackend::isInitialized() const {
    return pImpl->isInitialized();
}

} // namespace index
} // namespace themis

#else // !THEMIS_HAS_VULKAN_IMPL

// STUB/SIMULATION NOTE:
// Purpose: Provide link-compatible no-op implementations of
//   VulkanVectorIndexBackend when the Vulkan SDK is not present, so that the
//   vector search subsystem can be compiled and run without GPU drivers.
//   All methods check for an injected bridge callback first; if none is set
//   they return false or empty containers.
// Activation: THEMIS_HAS_VULKAN_IMPL is 0 — set when the Vulkan headers
//   and loader library (libvulkan.so) are not found by CMake.
// Production Delta: GPU-accelerated vector similarity search (HNSW on Vulkan
//   compute shaders) is silently disabled when no callback is injected.
//   All vector queries fall back to the CPU-based HNSW/FAISS index
//   (AdvancedVectorIndex).  Throughput is reduced by 5–20× for ANN search on
//   large corpora (≥ 1 M vectors).
// Removal Plan: Install the Vulkan SDK and a compatible GPU driver; rebuild
//   with -DTHEMIS_HAS_VULKAN_IMPL=1.  The Pimpl implementation block above
//   (inside #if THEMIS_HAS_VULKAN_IMPL) is then compiled instead.
// Roadmap ref: src/index/FUTURE_ENHANCEMENTS.md §"GPU Vector Index (Vulkan)"
namespace themis {
namespace index {

/** @brief Implementation detail. */
class VulkanVectorIndexBackend::Impl {
public:
    explicit Impl(const GPUVectorIndex::Config&) {}
    ~Impl() = default;
    bool initialized_ = false;

    bool initialize([[maybe_unused]] int dimension) {
        InitializeFn fn;
        {
            std::lock_guard<std::mutex> lk(VulkanVectorIndexBackend::initializeFnMutex());
            fn = VulkanVectorIndexBackend::initializeFnStorage();
        }
        if (fn) {
            try {
                initialized_ = fn(dimension);
            } catch (...) {
                THEMIS_WARN("VulkanVectorIndexBackend::initialize: exception during initialization");
                initialized_ = false;
            }
            return initialized_;
        }
        THEMIS_DEBUG("VulkanVectorIndexBackend::initialize: no initialize function registered");
        return false;
    }

    void shutdown() { initialized_ = false; }

    bool uploadVectors(const std::vector<std::vector<float>>& vectors) {
        UploadFn fn;
        {
            std::lock_guard<std::mutex> lk(VulkanVectorIndexBackend::uploadFnMutex());
            fn = VulkanVectorIndexBackend::uploadFnStorage();
        }
        if (fn) {
            try { return fn(vectors); } catch (...) { THEMIS_ERROR("VulkanVectorIndexBackend::uploadVectors: exception in upload function"); return false; }
        }
        THEMIS_DEBUG("VulkanVectorIndexBackend::uploadVectors: no upload function registered");
        return false;
    }

    std::vector<std::pair<float, size_t>> searchIndices(
        const std::vector<float>& query, size_t k) {
        THEMIS_DEBUG("VulkanVectorIndexBackend::searchIndices: stub backend - returning empty result (dim={}, k={})", query.size(), k);
        return {};
    }
    std::vector<std::vector<std::pair<float, size_t>>> searchBatchIndices(
        const std::vector<std::vector<float>>& queries, size_t k) {
        THEMIS_DEBUG("VulkanVectorIndexBackend::searchBatchIndices: stub backend - returning empty batch result (queries={} k={})", queries.size(), k);
        return {};
    }

    std::vector<GPUVectorIndex::SearchResult> search(
        const std::vector<float>& query, size_t k) {
        SearchFn fn;
        {
            std::lock_guard<std::mutex> lk(VulkanVectorIndexBackend::searchFnMutex());
            fn = VulkanVectorIndexBackend::searchFnStorage();
        }
        if (fn) {
            try { return fn(query, k); } catch (...) { THEMIS_ERROR("VulkanVectorIndexBackend::search: exception in search function"); return {}; }
        }
        THEMIS_DEBUG("VulkanVectorIndexBackend::search: no search function registered");
        return {};
    }

    std::vector<std::vector<GPUVectorIndex::SearchResult>> searchBatch(
        const std::vector<std::vector<float>>& queries, size_t k) {
        SearchBatchFn fn;
        {
            std::lock_guard<std::mutex> lk(VulkanVectorIndexBackend::searchBatchFnMutex());
            fn = VulkanVectorIndexBackend::searchBatchFnStorage();
        }
        if (fn) {
            try { return fn(queries, k); } catch (...) { THEMIS_ERROR("VulkanVectorIndexBackend::searchBatch: exception in searchBatch function"); return {}; }
        }
        THEMIS_DEBUG("VulkanVectorIndexBackend::searchBatch: no searchBatch function registered");
        return {};
    }

    GPUVectorIndex::Statistics getStatistics() const { return {}; }
    bool isInitialized() const { return initialized_; }
};

VulkanVectorIndexBackend::VulkanVectorIndexBackend(const GPUVectorIndex::Config& config)
    : pImpl(std::make_unique<Impl>(config)) {}

VulkanVectorIndexBackend::~VulkanVectorIndexBackend() = default;

bool VulkanVectorIndexBackend::initialize([[maybe_unused]] int dimension) {
    return pImpl->initialize(dimension);
}
void VulkanVectorIndexBackend::shutdown() { pImpl->shutdown(); }
bool VulkanVectorIndexBackend::uploadVectors(const std::vector<std::vector<float>>& v) {
    return pImpl->uploadVectors(v);
}

std::vector<std::pair<float, size_t>> VulkanVectorIndexBackend::searchIndices(
    const std::vector<float>& q, size_t k) { return pImpl->searchIndices(q, k); }

std::vector<std::vector<std::pair<float, size_t>>> VulkanVectorIndexBackend::searchBatchIndices(
    const std::vector<std::vector<float>>& qs, size_t k) {
    return pImpl->searchBatchIndices(qs, k);
}

std::vector<GPUVectorIndex::SearchResult> VulkanVectorIndexBackend::search(
    const std::vector<float>& q, size_t k) { return pImpl->search(q, k); }

std::vector<std::vector<GPUVectorIndex::SearchResult>> VulkanVectorIndexBackend::searchBatch(
    const std::vector<std::vector<float>>& qs, size_t k) {
    return pImpl->searchBatch(qs, k);
}

GPUVectorIndex::Statistics VulkanVectorIndexBackend::getStatistics() const {
    return pImpl->getStatistics();
}
bool VulkanVectorIndexBackend::isInitialized() const { return pImpl->isInitialized(); }

} // namespace index
} // namespace themis

#endif // THEMIS_HAS_VULKAN_IMPL

