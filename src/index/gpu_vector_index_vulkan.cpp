/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gpu_vector_index_vulkan.cpp                        ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:48:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     936                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * Vulkan Backend Implementation for GPU Vector Index
 * 
 * Provides cross-platform GPU-accelerated vector similarity search using Vulkan compute shaders.
 * Supports NVIDIA, AMD, Intel, and Apple GPUs through native Vulkan or MoltenVK.
 * 
 * @file gpu_vector_index_vulkan.cpp
 * @brief Vulkan compute backend for vector indexing
 */

#include "index/gpu_vector_index.h"
#include "llm/lora_framework/vulkan_context.h"
#include "llm/lora_framework/vulkan_buffer.h"
#include "llm/lora_framework/vulkan_pipeline.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

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
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace index
} // namespace themis

#if THEMIS_HAS_VULKAN_IMPL

namespace themis {
namespace index {

/**
 * @brief Implementation class for Vulkan backend
 */
class VulkanVectorIndexBackend::Impl {
public:
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
    bool initialize(int dimension) {
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
                std::cerr << "VulkanVectorIndexBackend: Failed to initialize Vulkan context\n";
                return false;
            }
            
            // Query device properties
            const auto& props = context_->device_properties();
            std::cout << "VulkanVectorIndexBackend: Using GPU: " << props.deviceName << "\n";
            std::cout << "VulkanVectorIndexBackend: Vulkan API Version: " 
                      << VK_VERSION_MAJOR(props.apiVersion) << "."
                      << VK_VERSION_MINOR(props.apiVersion) << "."
                      << VK_VERSION_PATCH(props.apiVersion) << "\n";
            
            // Create compute pipelines for distance metrics
            if (!createPipelines()) {
                std::cerr << "VulkanVectorIndexBackend: Failed to create compute pipelines\n";
                shutdown();
                return false;
            }
            
            initialized_ = true;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "VulkanVectorIndexBackend: Initialization error: " << e.what() << "\n";
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
        
        // Clear buffers
        query_buffer_.reset();
        vector_buffer_.reset();
        distance_buffer_.reset();
        result_buffer_.reset();
        
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
            // Flatten vector data
            size_t totalSize = vectors.size() * dimension_ * sizeof(float);
            std::vector<float> flatData;
            flatData.reserve(vectors.size() * dimension_);
            
            for (const auto& vec : vectors) {
                if (vec.size() != static_cast<size_t>(dimension_)) {
                    std::cerr << "VulkanVectorIndexBackend: Vector dimension mismatch\n";
                    return false;
                }
                flatData.insert(flatData.end(), vec.begin(), vec.end());
            }
            
            // Create or recreate vector buffer
            vector_buffer_ = std::make_unique<lora::vulkan::VulkanBuffer>(
                context_.get(), totalSize, lora::vulkan::VulkanBuffer::Usage::DeviceLocal);
            
            // Upload data
            vector_buffer_->upload(flatData.data(), totalSize);
            
            num_vectors_ = vectors.size();
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "VulkanVectorIndexBackend: Upload error: " << e.what() << "\n";
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
            return {};
        }
        
        if (num_vectors_ == 0) {
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
            query_buffer_->upload(query.data(), querySize);
            
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
                    std::cerr << "VulkanVectorIndexBackend: Unknown distance metric\n";
                    return {};
            }
            
            if (!pipeline || !pipeline->is_ready()) {
                std::cerr << "VulkanVectorIndexBackend: Pipeline not ready\n";
                return {};
            }
            
            // Bind buffers to descriptor sets
            pipeline->bind_buffer(0, *query_buffer_);      // Query vectors
            pipeline->bind_buffer(1, *vector_buffer_);     // Database vectors
            pipeline->bind_buffer(2, *distance_buffer_);   // Output distances
            
            // Set push constants
            struct PushConstants {
                uint32_t numQueries;
                uint32_t numVectors;
                uint32_t dimension;
            } pushConstants = {
                1,                        // Single query
                static_cast<uint32_t>(num_vectors_),
                static_cast<uint32_t>(dimension_)
            };
            pipeline->set_push_constants(&pushConstants, sizeof(PushConstants));
            
            // Dispatch compute shader
            // Local size is 16x16, so we need to dispatch enough workgroups
            uint32_t workgroupsX = (num_vectors_ + 15) / 16;
            uint32_t workgroupsY = 1; // Single query
            pipeline->dispatch(workgroupsX, workgroupsY, 1);
            
            // Wait for completion
            pipeline->wait();
            
            // Download results
            std::vector<float> distances(num_vectors_);
            distance_buffer_->download(distances.data(), distanceSize);
            
            // Find top-k
            std::vector<std::pair<float, size_t>> distanceIndices;
            distanceIndices.reserve(num_vectors_);
            for (size_t i = 0; i < num_vectors_; ++i) {
                distanceIndices.emplace_back(distances[i], i);
            }
            
            // Partial sort to get top-k
            size_t topK = std::min(k, num_vectors_);
            std::partial_sort(distanceIndices.begin(), 
                            distanceIndices.begin() + topK, 
                            distanceIndices.end(),
                            [](const auto& a, const auto& b) { return a.first < b.first; });
            
            // Keep only top-k
            distanceIndices.resize(topK);
            
            auto endTime = std::chrono::steady_clock::now();
            updateQueryStats(startTime, endTime);
            
            return distanceIndices;
            
        } catch (const std::exception& e) {
            std::cerr << "VulkanVectorIndexBackend: Search error: " << e.what() << "\n";
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
        std::vector<GPUVectorIndex::SearchResult> results;
        results.reserve(indices.size());
        for (const auto& [distance, index] : indices) {
            results.push_back({"", distance});
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
            return {};
        }
        
        // For small batches, use single-query path to avoid overhead
        if (queries.size() < 4) {
            std::vector<std::vector<std::pair<float, size_t>>> results;
            results.reserve(queries.size());
            for (const auto& query : queries) {
                results.push_back(searchIndices(query, k));
            }
            return results;
        }
        
        try {
            auto startTime = std::chrono::steady_clock::now();
            
            size_t numQueries = queries.size();
            
            // Flatten all query vectors
            std::vector<float> flatQueries;
            flatQueries.reserve(numQueries * dimension_);
            for (const auto& query : queries) {
                if (query.size() != static_cast<size_t>(dimension_)) {
                    std::cerr << "VulkanVectorIndexBackend: Query dimension mismatch in batch\n";
                    return {};
                }
                flatQueries.insert(flatQueries.end(), query.begin(), query.end());
            }
            
            // Upload all queries
            size_t queryBufferSize = numQueries * dimension_ * sizeof(float);
            if (!query_buffer_ || query_buffer_->size() < queryBufferSize) {
                query_buffer_ = std::make_unique<lora::vulkan::VulkanBuffer>(
                    context_.get(), queryBufferSize, lora::vulkan::VulkanBuffer::Usage::DeviceLocal);
            }
            query_buffer_->upload(flatQueries.data(), queryBufferSize);
            
            // Allocate distance buffer for all query-vector pairs
            size_t totalDistances = numQueries * num_vectors_;
            size_t distanceBufferSize = totalDistances * sizeof(float);
            if (!distance_buffer_ || distance_buffer_->size() < distanceBufferSize) {
                distance_buffer_ = std::make_unique<lora::vulkan::VulkanBuffer>(
                    context_.get(), distanceBufferSize, lora::vulkan::VulkanBuffer::Usage::DeviceLocal);
            }
            
            // Select pipeline
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
                    std::cerr << "VulkanVectorIndexBackend: Unknown distance metric\n";
                    return {};
            }
            
            if (!pipeline || !pipeline->is_ready()) {
                std::cerr << "VulkanVectorIndexBackend: Pipeline not ready\n";
                return {};
            }
            
            // Bind buffers
            pipeline->bind_buffer(0, *query_buffer_);
            pipeline->bind_buffer(1, *vector_buffer_);
            pipeline->bind_buffer(2, *distance_buffer_);
            
            // Set push constants
            struct PushConstants {
                uint32_t numQueries;
                uint32_t numVectors;
                uint32_t dimension;
            } pushConstants = {
                static_cast<uint32_t>(numQueries),
                static_cast<uint32_t>(num_vectors_),
                static_cast<uint32_t>(dimension_)
            };
            pipeline->set_push_constants(&pushConstants, sizeof(PushConstants));
            
            // Dispatch compute shader (16x16 local size)
            uint32_t workgroupsX = (num_vectors_ + 15) / 16;
            uint32_t workgroupsY = (numQueries + 15) / 16;
            pipeline->dispatch(workgroupsX, workgroupsY, 1);
            
            // Wait for completion
            pipeline->wait();
            
            // Download all distances
            std::vector<float> allDistances(totalDistances);
            distance_buffer_->download(allDistances.data(), distanceBufferSize);
            
            // Process each query's results
            std::vector<std::vector<std::pair<float, size_t>>> results;
            results.reserve(numQueries);
            
            for (size_t q = 0; q < numQueries; ++q) {
                // Extract distances for this query
                std::vector<std::pair<float, size_t>> queryDistances;
                queryDistances.reserve(num_vectors_);
                
                size_t offset = q * num_vectors_;
                for (size_t i = 0; i < num_vectors_; ++i) {
                    queryDistances.emplace_back(allDistances[offset + i], i);
                }
                
                // Find top-k for this query
                size_t topK = std::min(k, num_vectors_);
                std::partial_sort(queryDistances.begin(),
                                queryDistances.begin() + topK,
                                queryDistances.end(),
                                [](const auto& a, const auto& b) { return a.first < b.first; });
                
                // Keep only top-k
                queryDistances.resize(topK);
                results.push_back(std::move(queryDistances));
            }
            
            auto endTime = std::chrono::steady_clock::now();
            updateQueryStats(startTime, endTime);
            
            return results;
            
        } catch (const std::exception& e) {
            std::cerr << "VulkanVectorIndexBackend: Batch search error: " << e.what() << "\n";
            // Fall back to single-query path
            std::vector<std::vector<std::pair<float, size_t>>> results;
            results.reserve(queries.size());
            for (const auto& query : queries) {
                results.push_back(searchIndices(query, k));
            }
            return results;
        }
    }
    
    /**
     * @brief Batch search for nearest neighbors (true batch GPU implementation)
     * @param queries Query vectors
     * @param k Number of nearest neighbors
     * @return Batch search results
     */
    std::vector<std::vector<GPUVectorIndex::SearchResult>> searchBatch(
        const std::vector<std::vector<float>>& queries, size_t k) {
        
        if (queries.empty() || !initialized_) {
            return {};
        }
        
        // For small batches, use single-query path to avoid overhead
        if (queries.size() < 4) {
            std::vector<std::vector<GPUVectorIndex::SearchResult>> results;
            results.reserve(queries.size());
            for (const auto& query : queries) {
                results.push_back(search(query, k));
            }
            return results;
        }
        
        try {
            auto startTime = std::chrono::steady_clock::now();
            
            size_t numQueries = queries.size();
            
            // Flatten all query vectors
            std::vector<float> flatQueries;
            flatQueries.reserve(numQueries * dimension_);
            for (const auto& query : queries) {
                if (query.size() != static_cast<size_t>(dimension_)) {
                    std::cerr << "VulkanVectorIndexBackend: Query dimension mismatch in batch\n";
                    return {};
                }
                flatQueries.insert(flatQueries.end(), query.begin(), query.end());
            }
            
            // Upload all queries
            size_t queryBufferSize = numQueries * dimension_ * sizeof(float);
            if (!query_buffer_ || query_buffer_->size() < queryBufferSize) {
                query_buffer_ = std::make_unique<lora::vulkan::VulkanBuffer>(
                    context_.get(), queryBufferSize, lora::vulkan::VulkanBuffer::Usage::DeviceLocal);
            }
            query_buffer_->upload(flatQueries.data(), queryBufferSize);
            
            // Allocate distance buffer for all query-vector pairs
            size_t totalDistances = numQueries * num_vectors_;
            size_t distanceBufferSize = totalDistances * sizeof(float);
            if (!distance_buffer_ || distance_buffer_->size() < distanceBufferSize) {
                distance_buffer_ = std::make_unique<lora::vulkan::VulkanBuffer>(
                    context_.get(), distanceBufferSize, lora::vulkan::VulkanBuffer::Usage::DeviceLocal);
            }
            
            // Select pipeline
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
                    std::cerr << "VulkanVectorIndexBackend: Unknown distance metric\n";
                    return {};
            }
            
            if (!pipeline || !pipeline->is_ready()) {
                std::cerr << "VulkanVectorIndexBackend: Pipeline not ready\n";
                return {};
            }
            
            // Bind buffers
            pipeline->bind_buffer(0, *query_buffer_);
            pipeline->bind_buffer(1, *vector_buffer_);
            pipeline->bind_buffer(2, *distance_buffer_);
            
            // Set push constants
            struct PushConstants {
                uint32_t numQueries;
                uint32_t numVectors;
                uint32_t dimension;
            } pushConstants = {
                static_cast<uint32_t>(numQueries),
                static_cast<uint32_t>(num_vectors_),
                static_cast<uint32_t>(dimension_)
            };
            pipeline->set_push_constants(&pushConstants, sizeof(PushConstants));
            
            // Dispatch compute shader (16x16 local size)
            uint32_t workgroupsX = (num_vectors_ + 15) / 16;
            uint32_t workgroupsY = (numQueries + 15) / 16;
            pipeline->dispatch(workgroupsX, workgroupsY, 1);
            
            // Wait for completion
            pipeline->wait();
            
            // Download all distances
            std::vector<float> allDistances(totalDistances);
            distance_buffer_->download(allDistances.data(), distanceBufferSize);
            
            // Process each query's results
            std::vector<std::vector<GPUVectorIndex::SearchResult>> results;
            results.reserve(numQueries);
            
            for (size_t q = 0; q < numQueries; ++q) {
                // Extract distances for this query
                std::vector<std::pair<float, size_t>> queryDistances;
                queryDistances.reserve(num_vectors_);
                
                size_t offset = q * num_vectors_;
                for (size_t i = 0; i < num_vectors_; ++i) {
                    queryDistances.emplace_back(allDistances[offset + i], i);
                }
                
                // Find top-k for this query
                size_t topK = std::min(k, num_vectors_);
                std::partial_sort(queryDistances.begin(),
                                queryDistances.begin() + topK,
                                queryDistances.end(),
                                [](const auto& a, const auto& b) { return a.first < b.first; });
                
                // Convert to SearchResult (IDs filled by main implementation)
                std::vector<GPUVectorIndex::SearchResult> queryResults;
                queryResults.reserve(topK);
                for (size_t i = 0; i < topK; ++i) {
                    queryResults.push_back({"", queryDistances[i].first});
                }
                results.push_back(std::move(queryResults));
            }
            
            auto endTime = std::chrono::steady_clock::now();
            updateQueryStats(startTime, endTime);
            
            return results;
            
        } catch (const std::exception& e) {
            std::cerr << "VulkanVectorIndexBackend: Batch search error: " << e.what() << "\n";
            // Fall back to single-query path
            std::vector<std::vector<GPUVectorIndex::SearchResult>> results;
            results.reserve(queries.size());
            for (const auto& query : queries) {
                results.push_back(search(query, k));
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
                "share/themis/shaders/vector_index/",       // Install directory
                "/usr/share/themis/shaders/vector_index/",  // System install
                "/usr/local/share/themis/shaders/vector_index/"  // Local install
            };
            
            std::string shaderDir;
            for (const auto& path : searchPaths) {
                std::string testPath = path + "l2_distance.comp.spv";
                std::ifstream testFile(testPath);
                if (testFile.good()) {
                    shaderDir = path;
                    break;
                }
            }
            
            if (shaderDir.empty()) {
                std::cerr << "VulkanVectorIndexBackend: Failed to locate shader directory\n";
                std::cerr << "Searched paths:\n";
                for (const auto& path : searchPaths) {
                    std::cerr << "  - " << path << "\n";
                }
                return false;
            }
            
            std::cout << "VulkanVectorIndexBackend: Using shader directory: " << shaderDir << "\n";
            
            // Create L2 distance pipeline
            std::string l2ShaderPath = shaderDir + "l2_distance.comp.spv";
            l2_pipeline_ = std::make_unique<lora::vulkan::VulkanComputePipeline>(
                context_.get(), l2ShaderPath);
            
            // Push constants: numQueries, numVectors, dimension
            size_t pushConstantSize = sizeof(uint32_t) * 3;
            if (!l2_pipeline_->create(pushConstantSize)) {
                std::cerr << "VulkanVectorIndexBackend: Failed to create L2 distance pipeline\n";
                return false;
            }
            
            // Create Cosine distance pipeline
            std::string cosineShaderPath = shaderDir + "cosine_distance.comp.spv";
            cosine_pipeline_ = std::make_unique<lora::vulkan::VulkanComputePipeline>(
                context_.get(), cosineShaderPath);
            if (!cosine_pipeline_->create(pushConstantSize)) {
                std::cerr << "VulkanVectorIndexBackend: Failed to create Cosine distance pipeline\n";
                return false;
            }
            
            // Create Inner Product distance pipeline
            std::string innerProductShaderPath = shaderDir + "inner_product_distance.comp.spv";
            inner_product_pipeline_ = std::make_unique<lora::vulkan::VulkanComputePipeline>(
                context_.get(), innerProductShaderPath);
            if (!inner_product_pipeline_->create(pushConstantSize)) {
                std::cerr << "VulkanVectorIndexBackend: Failed to create Inner Product pipeline\n";
                return false;
            }
            
            std::cout << "VulkanVectorIndexBackend: All compute pipelines created successfully\n";
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "VulkanVectorIndexBackend: Pipeline creation error: " << e.what() << "\n";
            return false;
        }
    }
    
    /**
     * @brief Calculate VRAM usage
     */
    size_t calculateVRAMUsage() const {
        size_t usage = 0;
        if (vector_buffer_) usage += vector_buffer_->size();
        if (query_buffer_) usage += query_buffer_->size();
        if (distance_buffer_) usage += distance_buffer_->size();
        if (result_buffer_) usage += result_buffer_->size();
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

bool VulkanVectorIndexBackend::initialize(int dimension) {
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

// Stub implementations when Vulkan is not available
namespace themis {
namespace index {

class VulkanVectorIndexBackend::Impl {
public:
    explicit Impl(const GPUVectorIndex::Config&) {}
    ~Impl() = default;
    bool initialize(int) { return false; }
    void shutdown() {}
    bool uploadVectors(const std::vector<std::vector<float>>&) { return false; }
    std::vector<std::pair<float, size_t>> searchIndices(const std::vector<float>&, size_t) { return {}; }
    std::vector<std::vector<std::pair<float, size_t>>> searchBatchIndices(
        const std::vector<std::vector<float>>&, size_t) { return {}; }
    std::vector<GPUVectorIndex::SearchResult> search(const std::vector<float>&, size_t) { return {}; }
    std::vector<std::vector<GPUVectorIndex::SearchResult>> searchBatch(
        const std::vector<std::vector<float>>&, size_t) { return {}; }
    GPUVectorIndex::Statistics getStatistics() const { return {}; }
    bool isInitialized() const { return false; }
};

VulkanVectorIndexBackend::VulkanVectorIndexBackend(const GPUVectorIndex::Config& config)
    : pImpl(std::make_unique<Impl>(config)) {}

VulkanVectorIndexBackend::~VulkanVectorIndexBackend() = default;

bool VulkanVectorIndexBackend::initialize(int) { return false; }
void VulkanVectorIndexBackend::shutdown() {}
bool VulkanVectorIndexBackend::uploadVectors(const std::vector<std::vector<float>>&) { return false; }

std::vector<std::pair<float, size_t>> VulkanVectorIndexBackend::searchIndices(
    const std::vector<float>&, size_t) { return {}; }

std::vector<std::vector<std::pair<float, size_t>>> VulkanVectorIndexBackend::searchBatchIndices(
    const std::vector<std::vector<float>>&, size_t) { return {}; }

std::vector<GPUVectorIndex::SearchResult> VulkanVectorIndexBackend::search(
    const std::vector<float>&, size_t) { return {}; }

std::vector<std::vector<GPUVectorIndex::SearchResult>> VulkanVectorIndexBackend::searchBatch(
    const std::vector<std::vector<float>>&, size_t) { return {}; }

GPUVectorIndex::Statistics VulkanVectorIndexBackend::getStatistics() const { return {}; }
bool VulkanVectorIndexBackend::isInitialized() const { return false; }

} // namespace index
} // namespace themis

#endif // THEMIS_HAS_VULKAN_IMPL
