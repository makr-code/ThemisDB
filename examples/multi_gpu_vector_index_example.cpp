/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            multi_gpu_vector_index_example.cpp                 ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     259                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * Multi-GPU Vector Index Example
 * 
 * Demonstrates how to use the MultiGPUVectorIndex for distributed
 * vector search across multiple GPUs.
 */

#include "index/multi_gpu_vector_index.h"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>

using namespace themis::index;

// Generate random vector
std::vector<float> generateRandomVector(int dimension, std::mt19937& rng) {
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::vector<float> vec(dimension);
    for (int i = 0; i < dimension; ++i) {
        vec[i] = dist(rng);
    }
    return vec;
}

// Print statistics in a formatted way
void printStatistics(const MultiGPUVectorIndex::Statistics& stats) {
    std::cout << "\n=== Multi-GPU Vector Index Statistics ===\n";
    std::cout << "Total Vectors:      " << stats.totalVectors << "\n";
    std::cout << "Dimension:          " << stats.totalDimension << "\n";
    std::cout << "Active GPUs:        " << stats.numActiveGPUs << "\n";
    std::cout << "Failed GPUs:        " << stats.numFailedGPUs << "\n";
    std::cout << "Avg Query Time:     " << std::fixed << std::setprecision(2) 
              << stats.avgQueryTimeMs << " ms\n";
    std::cout << "Throughput:         " << std::fixed << std::setprecision(0) 
              << stats.throughputQPS << " QPS\n";
    std::cout << "Scaling Efficiency: " << std::fixed << std::setprecision(1) 
              << (stats.scalingEfficiency * 100.0) << "%\n";
    std::cout << "Load Imbalance:     " << std::fixed << std::setprecision(1) 
              << (stats.loadImbalance * 100.0) << "%\n";
    
    std::cout << "\nPer-GPU Statistics:\n";
    for (const auto& gpuStat : stats.perGPUStats) {
        std::cout << "  GPU " << gpuStat.deviceId << ":\n";
        std::cout << "    Vectors:    " << gpuStat.numVectors << "\n";
        std::cout << "    VRAM:       " << (gpuStat.vramUsageBytes / 1024 / 1024) << " MB\n";
        std::cout << "    Avg Query:  " << std::fixed << std::setprecision(2) 
                  << gpuStat.avgQueryTimeMs << " ms\n";
        std::cout << "    Active:     " << (gpuStat.isActive ? "Yes" : "No") << "\n";
    }
    std::cout << "========================================\n\n";
}

int main() {
    std::cout << "Multi-GPU Vector Index Example\n";
    std::cout << "==============================\n\n";
    
    // Configuration
    const int dimension = 128;
    const size_t numVectors = 10000;
    const size_t k = 10;
    
    std::cout << "Configuration:\n";
    std::cout << "  Dimension:     " << dimension << "\n";
    std::cout << "  Num Vectors:   " << numVectors << "\n";
    std::cout << "  Top-K:         " << k << "\n\n";
    
    // Setup multi-GPU configuration
    MultiGPUVectorIndex::Config config;
    config.enableMultiGPU = true;
    config.deviceIds = {0, 1};  // Use GPUs 0 and 1
    config.partitionStrategy = MultiGPUVectorIndex::PartitionStrategy::ROUND_ROBIN;
    config.loadBalancing = MultiGPUVectorIndex::LoadBalancingMode::STATIC;
    config.backend = GPUVectorIndex::Backend::AUTO;
    config.metric = GPUVectorIndex::DistanceMetric::COSINE;
    config.allowCPUFallback = true;
    config.enableFaultTolerance = true;
    
    std::cout << "Multi-GPU Config:\n";
    std::cout << "  Devices:       [";
    for (size_t i = 0; i < config.deviceIds.size(); ++i) {
        std::cout << config.deviceIds[i];
        if (i < config.deviceIds.size() - 1) {
          std::cout << ", ";
        }
    }
    std::cout << "]\n";
    std::cout << "  Strategy:      Round-Robin\n";
    std::cout << "  Load Balance:  Static\n";
    std::cout << "  P2P:           " << (config.enableP2P ? "Enabled" : "Disabled") << "\n";
    std::cout << "  Fault Tolerance: " << (config.enableFaultTolerance ? "Enabled" : "Disabled") << "\n\n";
    
    // Create and initialize index
    std::cout << "Initializing multi-GPU vector index...\n";
    MultiGPUVectorIndex index(config);
    
    if (!index.initialize(dimension)) {
        std::cerr << "Failed to initialize multi-GPU index\n";
        std::cerr << "Note: This example requires GPU hardware.\n";
        std::cerr << "In environments without GPUs, the index will use CPU fallback.\n";
        return 1;
    }
    
    std::cout << "Successfully initialized!\n\n";
    
    // Generate and add random vectors
    std::cout << "Generating " << numVectors << " random vectors...\n";
    std::mt19937 rng(42);
    
    std::vector<std::string> ids;
    std::vector<std::vector<float>> vectors;
    
    for (size_t i = 0; i < numVectors; ++i) {
        ids.push_back("vec_" + std::to_string(i));
        vectors.push_back(generateRandomVector(dimension, rng));
    }
    
    std::cout << "Adding vectors to index...\n";
    auto startAdd = std::chrono::steady_clock::now();
    
    if (!index.addVectorBatch(ids, vectors)) {
        std::cerr << "Failed to add vectors\n";
        return 1;
    }
    
    auto endAdd = std::chrono::steady_clock::now();
    auto addDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endAdd - startAdd);
    
    std::cout << "Added " << numVectors << " vectors in " << addDuration.count() << " ms\n";
    std::cout << "Throughput: " << (numVectors * 1000.0 / addDuration.count()) << " vectors/sec\n\n";
    
    // Perform searches
    std::cout << "Performing sample searches...\n";
    const size_t numQueries = 100;
    
    std::vector<std::vector<float>> queries;
    for (size_t i = 0; i < numQueries; ++i) {
        queries.push_back(generateRandomVector(dimension, rng));
    }
    
    auto startSearch = std::chrono::steady_clock::now();
    
    for (const auto& query : queries) {
        auto results = index.search(query, k);
        
        // Verify results
        if (results.empty()) {
            std::cerr << "Warning: Empty search results\n";
        }
    }
    
    auto endSearch = std::chrono::steady_clock::now();
    auto searchDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endSearch - startSearch);
    
    std::cout << "Performed " << numQueries << " queries in " << searchDuration.count() << " ms\n";
    std::cout << "Average latency: " << (searchDuration.count() / static_cast<double>(numQueries)) 
              << " ms per query\n";
    std::cout << "Throughput: " << (numQueries * 1000.0 / searchDuration.count()) << " QPS\n\n";
    
    // Display a sample search result
    auto sampleQuery = generateRandomVector(dimension, rng);
    auto sampleResults = index.search(sampleQuery, 5);
    
    std::cout << "Sample search results (top-5):\n";
    for (size_t i = 0; i < sampleResults.size(); ++i) {
        std::cout << "  " << (i+1) << ". " << sampleResults[i].id 
                  << " (distance: " << std::fixed << std::setprecision(4) 
                  << sampleResults[i].distance 
                  << ", GPU: " << sampleResults[i].sourceGPU << ")\n";
    }
    std::cout << "\n";
    
    // Display statistics
    auto stats = index.getStatistics();
    printStatistics(stats);
    
    // Demonstrate partition strategy switching
    std::cout << "Switching to HASH_BASED partitioning...\n";
    index.setPartitionStrategy(MultiGPUVectorIndex::PartitionStrategy::HASH_BASED);
    std::cout << "Partition strategy updated\n\n";
    
    // Demonstrate load balancing parameter adjustment
    std::cout << "Adjusting search parameters...\n";
    index.setEfSearch(128);
    std::cout << "efSearch set to 128\n\n";
    
    // Get active GPUs
    auto activeGPUs = index.getActiveGPUs();
    std::cout << "Active GPUs: [";
    for (size_t i = 0; i < activeGPUs.size(); ++i) {
        std::cout << activeGPUs[i];
        if (i < activeGPUs.size() - 1) {
          std::cout << ", ";
        }
    }
    std::cout << "]\n\n";
    
    // Demonstrate batch search
    std::cout << "Testing batch search with " << numQueries << " queries...\n";
    auto batchStartTime = std::chrono::steady_clock::now();
    
    auto batchResults = index.searchBatch(queries, k);
    
    auto batchEndTime = std::chrono::steady_clock::now();
    auto batchDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        batchEndTime - batchStartTime);
    
    std::cout << "Batch search completed in " << batchDuration.count() << " ms\n";
    std::cout << "Average latency: " << (batchDuration.count() / static_cast<double>(numQueries)) 
              << " ms per query\n";
    std::cout << "Throughput: " << (numQueries * 1000.0 / batchDuration.count()) << " QPS\n\n";
    
    // Final statistics
    auto finalStats = index.getStatistics();
    printStatistics(finalStats);
    
    // Demonstrate vector removal
    std::cout << "Demonstrating vector removal...\n";
    if (index.removeVector("vec_0")) {
        std::cout << "Successfully removed vector 'vec_0'\n";
    } else {
        std::cout << "Failed to remove vector\n";
    }
    
    // Demonstrate vector update
    std::cout << "Demonstrating vector update...\n";
    auto newVector = generateRandomVector(dimension, rng);
    if (index.updateVector("vec_1", newVector)) {
        std::cout << "Successfully updated vector 'vec_1'\n";
    } else {
        std::cout << "Failed to update vector\n";
    }
    
    std::cout << "\nShutting down...\n";
    index.shutdown();
    
    std::cout << "Multi-GPU Vector Index Example Complete!\n";
    
    return 0;
}
