/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gpu_vector_index_example.cpp                       ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     519                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "index/gpu_vector_index.h"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>

using namespace themis::index;

// NOTE: This example demonstrates GPU-accelerated vector indexing in v2.3+
// HIP backend (AMD GPUs) is available when compiled with THEMIS_ENABLE_HIP=ON
// CUDA backend (NVIDIA GPUs) is planned for v2.4
// See docs/GPU_SUPPORT_ROADMAP.md for GPU capabilities and performance

// Generate random vectors for demonstration
std::vector<std::vector<float>> generateVectors(size_t count, int dimension) {
    std::mt19937 gen(42);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    
    std::vector<std::vector<float>> vectors;
    for (size_t i = 0; i < count; ++i) {
        std::vector<float> vec(dimension);
        for (int j = 0; j < dimension; ++j) {
            vec[j] = dist(gen);
        }
        vectors.push_back(vec);
    }
    return vectors;
}

void printBackendInfo(const GPUVectorIndex& index) {
    auto backend = index.getActiveBackend();
    std::cout << "\n=== Active Backend ===\n";
    
    switch (backend) {
        case GPUVectorIndex::Backend::CPU:
            std::cout << "Backend: CPU (SIMD-optimized)\n";
            break;
        case GPUVectorIndex::Backend::HIP:
            std::cout << "Backend: HIP (AMD GPU)\n";
            break;
        case GPUVectorIndex::Backend::CUDA:
            std::cout << "Backend: CUDA (NVIDIA GPU)\n";
            break;
        case GPUVectorIndex::Backend::VULKAN:
            std::cout << "Backend: Vulkan (Cross-platform GPU)\n";
            break;
        case GPUVectorIndex::Backend::AUTO:
            std::cout << "Backend: AUTO (auto-selected)\n";
            break;
        default:
            std::cout << "Backend: Unknown\n";
    }
    
    // Print available backends
    auto available = index.getAvailableBackends();
    std::cout << "Available backends: ";
    for (auto b : available) {
        switch (b) {
            case GPUVectorIndex::Backend::CPU: std::cout << "CPU "; break;
            case GPUVectorIndex::Backend::HIP: std::cout << "HIP "; break;
            case GPUVectorIndex::Backend::CUDA: std::cout << "CUDA "; break;
            case GPUVectorIndex::Backend::VULKAN: std::cout << "Vulkan "; break;
            default: break;
        }
    }
    std::cout << "\n";
}

void printStatistics(const GPUVectorIndex::Statistics& stats) {
    std::cout << "\n=== Index Statistics ===\n";
    std::cout << "Vectors: " << stats.numVectors << "\n";
    std::cout << "Dimension: " << stats.dimension << "\n";
    std::cout << "VRAM Usage: " << (stats.vramUsageBytes / (1024.0 * 1024.0)) << " MB\n";
    std::cout << "Avg Query Time: " << std::fixed << std::setprecision(3) 
              << stats.avgQueryTimeMs << " ms\n";
    std::cout << "Throughput: " << std::fixed << std::setprecision(0) 
              << stats.throughputQPS << " queries/sec\n";
    std::cout << "GPU Active: " << (stats.isGPUActive ? "Yes" : "No") << "\n";
}

void demonstrateBasicUsage() {
    std::cout << "\n======================================\n";
    std::cout << "Example 1: Basic Vector Search\n";
    std::cout << "======================================\n";
    
    // Create index with automatic backend selection
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::AUTO;
    config.metric = GPUVectorIndex::DistanceMetric::COSINE;
    
    GPUVectorIndex index(config);
    
    // Initialize with 128-dimensional vectors
    int dimension = 128;
    if (!index.initialize(dimension)) {
        std::cerr << "Failed to initialize index\n";
        return;
    }
    
    printBackendInfo(index);
    
    // Add some vectors
    std::cout << "\nAdding vectors...\n";
    auto vectors = generateVectors(1000, dimension);
    
    for (size_t i = 0; i < vectors.size(); ++i) {
        std::string id = "doc_" + std::to_string(i);
        index.addVector(id, vectors[i]);
    }
    
    std::cout << "Added " << vectors.size() << " vectors\n";
    
    // Search for similar vectors
    std::cout << "\nSearching for top-5 similar vectors...\n";
    auto query = vectors[0]; // Use first vector as query
    auto results = index.search(query, 5);
    
    std::cout << "\nSearch Results:\n";
    for (size_t i = 0; i < results.size(); ++i) {
        std::cout << "  " << (i+1) << ". " << results[i].id 
                  << " (distance: " << std::fixed << std::setprecision(4) 
                  << results[i].distance << ")\n";
    }
    
    printStatistics(index.getStatistics());
    
    index.shutdown();
}

void demonstrateBatchSearch() {
    std::cout << "\n======================================\n";
    std::cout << "Example 2: Batch Search\n";
    std::cout << "======================================\n";
    
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::AUTO;
    config.metric = GPUVectorIndex::DistanceMetric::L2;
    
    GPUVectorIndex index(config);
    
    int dimension = 128;
    index.initialize(dimension);
    
    printBackendInfo(index);
    
    // Add vectors
    std::cout << "\nBuilding index with 10,000 vectors...\n";
    auto vectors = generateVectors(10000, dimension);
    std::vector<std::string> ids = {};

    for (size_t i = 0; i < vectors.size(); ++i) {
        ids.push_back("vec_" + std::to_string(i));
    }
    
    auto start = std::chrono::steady_clock::now();
    index.addVectorBatch(ids, vectors);
    auto end = std::chrono::steady_clock::now();
    
    auto buildTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Index built in " << buildTime.count() << " ms\n";
    
    // Batch search
    std::cout << "\nPerforming batch search with 100 queries...\n";
    auto queries = generateVectors(100, dimension);
    
    start = std::chrono::steady_clock::now();
    auto batchResults = index.searchBatch(queries, 10);
    end = std::chrono::steady_clock::now();
    
    auto searchTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Batch search completed in " << searchTime.count() << " ms\n";
    std::cout << "Average time per query: " 
              << (searchTime.count() / 100.0) << " ms\n";
    std::cout << "Throughput: " 
              << (100000.0 / searchTime.count()) << " queries/sec\n";
    
    // Show first query results
    std::cout << "\nFirst query results (top-5):\n";
    for (size_t i = 0; i < 5 && i < batchResults[0].size(); ++i) {
        std::cout << "  " << (i+1) << ". " << batchResults[0][i].id 
                  << " (distance: " << std::fixed << std::setprecision(4) 
                  << batchResults[0][i].distance << ")\n";
    }
    
    printStatistics(index.getStatistics());
    
    index.shutdown();
}

void demonstrateBackendComparison() {
    std::cout << "\n======================================\n";
    std::cout << "Example 3: Backend Comparison\n";
    std::cout << "======================================\n";
    
    int dimension = 128;
    size_t numVectors = 5000;
    
    // Generate test data
    auto vectors = generateVectors(numVectors, dimension);
    std::vector<std::string> ids = {};

    for (size_t i = 0; i < vectors.size(); ++i) {
        ids.push_back("vec_" + std::to_string(i));
    }
    auto query = vectors[0];
    
    // List available backends
    GPUVectorIndex tempIndex(GPUVectorIndex::Config{});
    tempIndex.initialize(dimension);
    auto availableBackends = tempIndex.getAvailableBackends();
    tempIndex.shutdown();
    
    std::cout << "\nAvailable backends:\n";
    for (auto backend : availableBackends) {
        switch (backend) {
            case GPUVectorIndex::Backend::CPU:
                std::cout << "  - CPU\n";
                break;
            case GPUVectorIndex::Backend::VULKAN:
                std::cout << "  - Vulkan\n";
                break;
            case GPUVectorIndex::Backend::CUDA:
                std::cout << "  - CUDA\n";
                break;
            case GPUVectorIndex::Backend::HIP:
                std::cout << "  - HIP\n";
                break;
            default:
                break;
        }
    }
    
    // Test each backend
    std::cout << "\nBenchmarking each backend:\n";
    std::cout << std::string(60, '-') << "\n";
    
    for (auto backend : availableBackends) {
        GPUVectorIndex::Config config;
        config.backend = backend;
        config.metric = GPUVectorIndex::DistanceMetric::COSINE;
        
        GPUVectorIndex index(config);
        if (!index.initialize(dimension)) {
            continue;
        }
        
        // Build index
        auto buildStart = std::chrono::steady_clock::now();
        index.addVectorBatch(ids, vectors);
        auto buildEnd = std::chrono::steady_clock::now();
        
        // Perform searches
        int numSearches = 100;
        auto searchStart = std::chrono::steady_clock::now();
        for (int i = 0; i < numSearches; ++i) {
            index.search(query, 10);
        }
        auto searchEnd = std::chrono::steady_clock::now();
        
        auto buildTime = std::chrono::duration_cast<std::chrono::milliseconds>(buildEnd - buildStart);
        auto searchTime = std::chrono::duration_cast<std::chrono::microseconds>(searchEnd - searchStart);
        
        std::string backendName = {};
        switch (backend) {
            case GPUVectorIndex::Backend::CPU: backendName = "CPU"; break;
            case GPUVectorIndex::Backend::VULKAN: backendName = "Vulkan"; break;
            case GPUVectorIndex::Backend::CUDA: backendName = "CUDA"; break;
            case GPUVectorIndex::Backend::HIP: backendName = "HIP"; break;
            default: backendName = "Unknown"; break;
        }
        
        std::cout << "\n" << backendName << ":\n";
        std::cout << "  Build time: " << buildTime.count() << " ms\n";
        std::cout << "  Avg search time: " << (searchTime.count() / numSearches) << " µs\n";
        std::cout << "  Throughput: " << std::fixed << std::setprecision(0)
                  << (numSearches * 1000000.0 / searchTime.count()) << " QPS\n";
        
        index.shutdown();
    }
}

void demonstrateDistanceMetrics() {
    std::cout << "\n======================================\n";
    std::cout << "Example 4: Distance Metrics\n";
    std::cout << "======================================\n";
    
    int dimension = 128;
    
    // Generate test vectors
    auto vectors = generateVectors(1000, dimension);
    std::vector<std::string> ids = {};

    for (size_t i = 0; i < vectors.size(); ++i) {
        ids.push_back("vec_" + std::to_string(i));
    }
    
    std::vector<GPUVectorIndex::DistanceMetric> metrics = {
        GPUVectorIndex::DistanceMetric::L2,
        GPUVectorIndex::DistanceMetric::COSINE,
        GPUVectorIndex::DistanceMetric::INNER_PRODUCT
    };
    
    std::vector<std::string> metricNames = {"L2", "Cosine", "Inner Product"};
    
    for (size_t i = 0; i < metrics.size(); ++i) {
        std::cout << "\n--- " << metricNames[i] << " Distance ---\n";
        
        GPUVectorIndex::Config config;
        config.backend = GPUVectorIndex::Backend::AUTO;
        config.metric = metrics[i];
        
        GPUVectorIndex index(config);
        index.initialize(dimension);
        index.addVectorBatch(ids, vectors);
        
        auto query = vectors[0];
        auto results = index.search(query, 5);
        
        std::cout << "Top-5 results:\n";
        for (size_t j = 0; j < results.size(); ++j) {
            std::cout << "  " << (j+1) << ". " << results[j].id 
                      << " (distance: " << std::fixed << std::setprecision(4) 
                      << results[j].distance << ")\n";
        }
        
        index.shutdown();
    }
}

void demonstrateHIPBackend() {
    std::cout << "\n======================================\n";
    std::cout << "Example 5: HIP/AMD GPU Backend\n";
    std::cout << "======================================\n";
    
#ifdef THEMIS_ENABLE_HIP
    std::cout << "HIP backend is enabled in this build\n";
    
    // Check if HIP backend is available
    GPUVectorIndex testIndex;
    auto available = testIndex.getAvailableBackends();
    bool hipAvailable = false;
    for (auto b : available) {
        if (b == GPUVectorIndex::Backend::HIP) {
            hipAvailable = true;
            break;
        }
    }
    
    if (!hipAvailable) {
        std::cout << "HIP backend not available (no AMD GPU detected)\n";
        std::cout << "Skipping HIP-specific examples\n";
        return;
    }
    
    std::cout << "AMD GPU detected! Running HIP examples...\n";
    
    int dimension = 128;
    int numVectors = 10000;
    
    // Generate test data
    auto vectors = generateVectors(numVectors, dimension);
    std::vector<std::string> ids = {};

    for (int i = 0; i < numVectors; ++i) {
        ids.push_back("vec_" + std::to_string(i));
    }
    
    // Example 5.1: Explicit HIP backend configuration
    std::cout << "\n--- Example 5.1: Explicit HIP Configuration ---\n";
    {
        GPUVectorIndex::Config config;
        config.backend = GPUVectorIndex::Backend::HIP;
        config.deviceId = 0;              // Select first AMD GPU
        config.waveSize = 0;              // Auto-detect (32 for RDNA, 64 for CDNA)
        config.enableRocBLAS = false;     // Use custom kernels
        config.allowCPUFallback = true;   // Enable CPU fallback
        config.metric = GPUVectorIndex::DistanceMetric::COSINE;
        
        GPUVectorIndex index(config);
        index.initialize(dimension);
        
        // Add vectors
        index.addVectorBatch(ids, vectors);
        
        printBackendInfo(index);
        
        // Single query (GPU slower due to PCIe overhead)
        auto query = generateVectors(1, dimension)[0];
        auto start = std::chrono::steady_clock::now();
        auto results = index.search(query, 10);
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "\nSingle query time: " << (duration.count() / 1000.0) << " ms\n";
        std::cout << "Top 3 results:\n";
        size_t num_results = std::min(size_t(3), results.size());
        for (size_t i = 0; i < num_results; ++i) {
            std::cout << "  " << (i+1) << ". " << results[i].id 
                      << " (distance: " << results[i].distance << ")\n";
        }
        
        index.shutdown();
    }
    
    // Example 5.2: Batch search performance (GPU advantage)
    std::cout << "\n--- Example 5.2: Batch Search Performance ---\n";
    {
        GPUVectorIndex::Config config;
        config.backend = GPUVectorIndex::Backend::HIP;
        config.metric = GPUVectorIndex::DistanceMetric::L2;
        
        GPUVectorIndex index(config);
        index.initialize(dimension);
        index.addVectorBatch(ids, vectors);
        
        // Generate batch of queries
        std::vector<int> batchSizes = {1, 16, 64, 256, 512};
        
        std::cout << "Batch size performance (10000 vectors, dim=128):\n";
        for (int batchSize : batchSizes) {
            auto queries = generateVectors(batchSize, dimension);
            
            auto start = std::chrono::steady_clock::now();
            auto results = index.searchBatch(queries, 10);
            auto end = std::chrono::steady_clock::now();
            auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            double duration_ms = duration_us.count() / 1000.0;
            double qps = 0.0;
            if (duration_us.count() > 0) {
                qps = (batchSize * 1000000.0) / duration_us.count();
            }
            
            std::cout << "  Batch " << std::setw(3) << batchSize << ": " 
                      << std::setw(7) << std::fixed << std::setprecision(2) << duration_ms << " ms"
                      << " (" << std::fixed << std::setprecision(0) << qps << " QPS)\n";
        }
        
        index.shutdown();
    }
    
    // Example 5.3: Backend switching
    std::cout << "\n--- Example 5.3: Dynamic Backend Switching ---\n";
    {
        GPUVectorIndex::Config config;
        config.backend = GPUVectorIndex::Backend::AUTO;  // Start with auto-detection
        
        GPUVectorIndex index(config);
        index.initialize(dimension);
        index.addVectorBatch(ids, vectors);
        
        std::cout << "Initial backend:\n";
        printBackendInfo(index);
        
        // Switch to CPU
        std::cout << "\nSwitching to CPU backend...\n";
        if (index.switchBackend(GPUVectorIndex::Backend::CPU)) {
            printBackendInfo(index);
        }
        
        // Switch back to HIP
        std::cout << "\nSwitching back to HIP backend...\n";
        if (index.switchBackend(GPUVectorIndex::Backend::HIP)) {
            printBackendInfo(index);
        }
        
        index.shutdown();
    }
    
#else
    std::cout << "HIP backend not compiled in this build\n";
    std::cout << "To enable HIP support, rebuild with: -DTHEMIS_ENABLE_HIP=ON\n";
    std::cout << "See docs/GPU_SUPPORT_ROADMAP.md for setup instructions\n";
#endif
}

int main() {
    std::cout << "========================================\n";
    std::cout << "GPU Vector Index Examples\n";
    std::cout << "========================================\n";
    
    try {
        // Run all examples
        demonstrateBasicUsage();
        demonstrateBatchSearch();
        demonstrateBackendComparison();
        demonstrateDistanceMetrics();
        demonstrateHIPBackend();  // New HIP-specific examples
        
        std::cout << "\n========================================\n";
        std::cout << "All examples completed successfully!\n";
        std::cout << "========================================\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
