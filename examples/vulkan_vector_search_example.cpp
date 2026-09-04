/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            vulkan_vector_search_example.cpp                   ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     212                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file vulkan_vector_search_example.cpp
 * @brief Example demonstrating GPU-accelerated vector search using Vulkan backend
 * 
 * This example shows how to use the VulkanVectorIndexBackend for cross-platform
 * GPU-accelerated vector similarity search.
 * 
 * Requirements:
 * - Vulkan SDK 1.2+
 * - Vulkan-capable GPU (NVIDIA, AMD, Intel, or Apple via MoltenVK)
 * - CMake with THEMIS_ENABLE_VULKAN=ON
 */

#include "index/gpu_vector_index.h"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>

using namespace themis::index;

// Generate random vectors for testing
std::vector<std::vector<float>> generateRandomVectors(size_t count, size_t dimension) {
    std::vector<std::vector<float>> vectors;
    vectors.reserve(count);
    
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
    
    for (size_t i = 0; i < count; ++i) {
        std::vector<float> vec(dimension);
        for (size_t j = 0; j < dimension; ++j) {
            vec[j] = dis(gen);
        }
        vectors.push_back(vec);
    }
    
    return vectors;
}

int main() {
    std::cout << "=== Vulkan Vector Search Example ===\n\n";
    
    // Configuration
    constexpr size_t DIMENSION = 128;
    constexpr size_t NUM_VECTORS = 10000;
    constexpr size_t K = 10; // Top-10 nearest neighbors
    
    try {
        // 1. Create GPUVectorIndex with Vulkan backend
        std::cout << "1. Initializing GPU Vector Index...\n";
        
        GPUVectorIndex::Config config;
        config.backend = GPUVectorIndex::Backend::VULKAN;  // Use Vulkan
        config.metric = GPUVectorIndex::DistanceMetric::COSINE;
        config.deviceId = 0;  // Use first GPU
        config.enableValidation = false;  // Disable validation layers for production
        config.allowCPUFallback = true;  // Fall back to CPU if Vulkan unavailable
        
        GPUVectorIndex index(config);
        
        if (!index.initialize(DIMENSION)) {
            std::cerr << "Failed to initialize GPU Vector Index\n";
            return 1;
        }
        
        // Check which backend is active
        auto activeBackend = index.getActiveBackend();
        std::cout << "Active backend: ";
        switch (activeBackend) {
            case GPUVectorIndex::Backend::VULKAN:
                std::cout << "Vulkan GPU\n";
                break;
            case GPUVectorIndex::Backend::CPU:
                std::cout << "CPU (fallback)\n";
                break;
            default:
                std::cout << "Unknown\n";
        }
        std::cout << "\n";
        
        // 2. Generate and add vectors
        std::cout << "2. Generating " << NUM_VECTORS << " random vectors...\n";
        auto vectors = generateRandomVectors(NUM_VECTORS, DIMENSION);
        
        std::cout << "3. Adding vectors to index...\n";
        auto startAdd = std::chrono::high_resolution_clock::now();
        
        std::vector<std::string> ids = {};

        for (size_t i = 0; i < NUM_VECTORS; ++i) {
            ids.push_back("vec_" + std::to_string(i));
        }
        
        if (!index.addVectorBatch(ids, vectors)) {
            std::cerr << "Failed to add vectors\n";
            return 1;
        }
        
        auto endAdd = std::chrono::high_resolution_clock::now();
        auto addDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endAdd - startAdd);
        std::cout << "Added " << NUM_VECTORS << " vectors in " << addDuration.count() << " ms\n\n";
        
        // 3. Perform searches
        std::cout << "4. Performing similarity searches...\n";
        
        // Generate a query vector
        auto query = generateRandomVectors(1, DIMENSION)[0];
        
        // Single query benchmark
        auto startSearch = std::chrono::high_resolution_clock::now();
        auto results = index.search(query, K);
        auto endSearch = std::chrono::high_resolution_clock::now();
        auto searchDuration = std::chrono::duration_cast<std::chrono::microseconds>(endSearch - startSearch);
        
        std::cout << "Single query latency: " << searchDuration.count() / 1000.0 << " ms\n";
        std::cout << "Top-" << K << " results:\n";
        for (size_t i = 0; i < std::min(size_t(5), results.size()); ++i) {
            std::cout << "  " << (i+1) << ". " << results[i].id 
                      << " (distance: " << results[i].distance << ")\n";
        }
        std::cout << "\n";
        
        // Batch search benchmark
        std::cout << "5. Batch search performance test...\n";
        constexpr size_t BATCH_SIZE = 100;
        auto queries = generateRandomVectors(BATCH_SIZE, DIMENSION);
        
        auto startBatch = std::chrono::high_resolution_clock::now();
        auto batchResults = index.searchBatch(queries, K);
        auto endBatch = std::chrono::high_resolution_clock::now();
        auto batchDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endBatch - startBatch);
        
        std::cout << "Batch search (" << BATCH_SIZE << " queries): " 
                  << batchDuration.count() << " ms\n";
        std::cout << "Average latency per query: " 
                  << batchDuration.count() / double(BATCH_SIZE) << " ms\n";
        std::cout << "Throughput: " 
                  << (BATCH_SIZE * 1000.0) / batchDuration.count() << " QPS\n\n";
        
        // 4. Display statistics
        std::cout << "6. Index Statistics:\n";
        auto stats = index.getStatistics();
        std::cout << "  Vectors: " << stats.numVectors << "\n";
        std::cout << "  Dimension: " << stats.dimension << "\n";
        std::cout << "  VRAM usage: " << (stats.vramUsageBytes / 1024.0 / 1024.0) << " MB\n";
        std::cout << "  GPU active: " << (stats.isGPUActive ? "Yes" : "No") << "\n";
        std::cout << "  Avg query time: " << stats.avgQueryTimeMs << " ms\n";
        std::cout << "  Throughput: " << stats.throughputQPS << " QPS\n\n";
        
        // 5. Test different distance metrics
        std::cout << "7. Testing different distance metrics...\n";
        
        GPUVectorIndex::DistanceMetric metrics[] = {
            GPUVectorIndex::DistanceMetric::L2,
            GPUVectorIndex::DistanceMetric::COSINE,
            GPUVectorIndex::DistanceMetric::INNER_PRODUCT
        };
        
        const char* metricNames[] = {"L2", "Cosine", "Inner Product"};
        
        for (size_t i = 0; i < 3; ++i) {
            GPUVectorIndex::Config testConfig = config;
            testConfig.metric = metrics[i];
            
            GPUVectorIndex testIndex(testConfig);
            testIndex.initialize(DIMENSION);
            testIndex.addVectorBatch(ids, vectors);
            
            auto start = std::chrono::high_resolution_clock::now();
            auto testResults = testIndex.search(query, K);
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            std::cout << "  " << metricNames[i] << ": " 
                      << duration.count() / 1000.0 << " ms\n";
        }
        
        std::cout << "\n=== Example completed successfully ===\n";
        
        // Cleanup
        index.shutdown();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
