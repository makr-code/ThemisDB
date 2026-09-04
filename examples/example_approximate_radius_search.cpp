/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            example_approximate_radius_search.cpp              ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     225                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file example_approximate_radius_search.cpp
 * @brief Example usage of ApproximateRadiusSearch for GAP-006
 * 
 * This example demonstrates how to use the ApproximateRadiusSearch API
 * to find all vectors within a distance threshold.
 */

#include "index/approximate_radius_search.h"
#include "index/vector_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <iostream>
#include <vector>
#include <random>

void printResults(const themis::vector::ApproximateRadiusSearch::SearchResult& result) {
    std::cout << "Found " << result.results.size() << " results:" << std::endl;
    std::cout << "  Total candidates evaluated: " << result.total_candidates << std::endl;
    std::cout << "  Max distance: " << result.actual_max_distance << std::endl;
    std::cout << "  Computation time: " << result.computation_time_ms << " ms" << std::endl;
    std::cout << "  Truncated: " << (result.truncated ? "yes" : "no") << std::endl;
    
    for (size_t i = 0; i < std::min(size_t(5), result.results.size()); ++i) {
        std::cout << "    [" << i << "] ID: " << result.results[i].id 
                  << ", Distance: " << result.results[i].distance << std::endl;
    }
    if (result.results.size() > 5) {
        std::cout << "    ... and " << (result.results.size() - 5) << " more" << std::endl;
    }
}

int main() {
    std::cout << "=== ApproximateRadiusSearch Example ===" << std::endl << std::endl;
    
    // 1. Setup database and vector index
    std::cout << "1. Setting up database and vector index..." << std::endl;
    themis::RocksDBWrapper db;
    auto db_init = db.init("/tmp/example_radius_search_db");
    if (!db_init.ok) {
        std::cerr << "Failed to initialize database: " << db_init.message << std::endl;
        return 1;
    }
    
    themis::VectorIndexManager vector_manager(db);
    constexpr int DIMENSION = 128;
    auto vec_init = vector_manager.init(
        "products", 
        DIMENSION, 
        themis::VectorIndexManager::Metric::COSINE,
        16,   // M
        200,  // efConstruction
        64    // efSearch
    );
    if (!vec_init.ok) {
        std::cerr << "Failed to initialize vector index: " << vec_init.message << std::endl;
        return 1;
    }
    std::cout << "  ✓ Database and index initialized" << std::endl << std::endl;
    
    // 2. Add some sample vectors
    std::cout << "2. Adding sample product vectors..." << std::endl;
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::normal_distribution<float> dist(0.0f, 1.0f);
    
    const int NUM_PRODUCTS = 1000;
    for (int i = 0; i < NUM_PRODUCTS; ++i) {
        // Generate random embedding
        std::vector<float> embedding(DIMENSION);
        for (int j = 0; j < DIMENSION; ++j) {
            embedding[j] = dist(gen);
        }
        
        // Create entity
        std::string product_id = "product_" + std::to_string(i);
        themis::BaseEntity entity(product_id);
        entity.setField("embedding", embedding);
        entity.setField("name", "Product " + std::to_string(i));
        entity.setField("category", i % 5 == 0 ? "electronics" : "clothing");
        
        // Add to index
        auto add_result = vector_manager.addEntity(entity, "embedding");
        if (!add_result.ok) {
            std::cerr << "Failed to add entity: " << add_result.message << std::endl;
            return 1;
        }
    }
    std::cout << "  ✓ Added " << NUM_PRODUCTS << " product vectors" << std::endl << std::endl;
    
    // 3. Create ApproximateRadiusSearch instance
    std::cout << "3. Creating ApproximateRadiusSearch..." << std::endl;
    themis::vector::ApproximateRadiusSearch radius_search(vector_manager);
    std::cout << "  ✓ ApproximateRadiusSearch created" << std::endl << std::endl;
    
    // 4. Example 1: Basic radius search
    std::cout << "4. Example 1: Basic radius search" << std::endl;
    std::vector<float> query_vector(DIMENSION);
    for (int i = 0; i < DIMENSION; ++i) {
        query_vector[i] = dist(gen);
    }
    
    themis::vector::ApproximateRadiusSearch::SearchConfig config;
    config.radius = 0.5f;
    config.metric = themis::vector::ApproximateRadiusSearch::Metric::COSINE;
    config.max_results = 100;
    config.sort_results = true;
    
    auto search_result = radius_search.search(query_vector, config);
    if (!search_result.has_value()) {
        std::cerr << "Search failed: " << search_result.error().message << std::endl;
        return 1;
    }
    printResults(search_result.value());
    std::cout << std::endl;
    
    // 5. Example 2: Batch search
    std::cout << "5. Example 2: Batch search (3 queries)" << std::endl;
    std::vector<std::vector<float>> batch_queries;
    for (int i = 0; i < 3; ++i) {
        std::vector<float> query(DIMENSION);
        for (int j = 0; j < DIMENSION; ++j) {
            query[j] = dist(gen);
        }
        batch_queries.push_back(query);
    }
    
    config.radius = 0.3f;
    auto batch_result = radius_search.batchSearch(batch_queries, config);
    if (!batch_result.has_value()) {
        std::cerr << "Batch search failed: " << batch_result.error().message << std::endl;
        return 1;
    }
    
    for (size_t i = 0; i < batch_result.value().size(); ++i) {
        std::cout << "  Query " << i << ":" << std::endl;
        std::cout << "    Found " << batch_result.value()[i].results.size() << " results" << std::endl;
    }
    std::cout << std::endl;
    
    // 6. Example 3: Search with target count
    std::cout << "6. Example 3: Search with target count (want 20 results)" << std::endl;
    config.radius = 1.0f;  // Starting radius
    auto target_result = radius_search.searchWithTargetCount(query_vector, 20, config);
    if (!target_result.has_value()) {
        std::cerr << "Target count search failed: " << target_result.error().message << std::endl;
        return 1;
    }
    std::cout << "  Requested: 20 results" << std::endl;
    std::cout << "  Got: " << target_result.value().results.size() << " results" << std::endl;
    std::cout << "  Max distance: " << target_result.value().actual_max_distance << std::endl;
    std::cout << std::endl;
    
    // 7. Example 4: Estimate result count
    std::cout << "7. Example 4: Estimate result count" << std::endl;
    auto estimate_result = radius_search.estimateResultCount(
        query_vector,
        0.5f,
        themis::vector::ApproximateRadiusSearch::Metric::COSINE
    );
    if (!estimate_result.has_value()) {
        std::cerr << "Estimation failed: " << estimate_result.error().message << std::endl;
        return 1;
    }
    std::cout << "  Estimated " << estimate_result.value() << " results within radius 0.5" << std::endl;
    
    // Compare with actual
    config.radius = 0.5f;
    config.max_results = 10000;
    auto actual_search = radius_search.search(query_vector, config);
    if (actual_search.has_value()) {
        std::cout << "  Actual count: " << actual_search.value().results.size() << std::endl;
        float ratio = static_cast<float>(estimate_result.value()) / 
                     static_cast<float>(actual_search.value().results.size());
        std::cout << "  Estimation accuracy: " << (ratio * 100.0f) << "%" << std::endl;
    }
    std::cout << std::endl;
    
    // 8. Show statistics
    std::cout << "8. Search statistics:" << std::endl;
    auto stats = radius_search.getStatistics();
    std::cout << "  Total searches: " << stats.total_searches << std::endl;
    std::cout << "  Avg results per search: " << stats.avg_results_per_search << std::endl;
    std::cout << "  Avg time per search: " << stats.avg_time_ms << " ms" << std::endl;
    std::cout << std::endl;
    
    // 9. Different radius values
    std::cout << "9. Comparing different radius values:" << std::endl;
    std::vector<float> radii = {0.1f, 0.3f, 0.5f, 0.7f, 1.0f};
    for (float r : radii) {
        config.radius = r;
        auto result = radius_search.search(query_vector, config);
        if (result.has_value()) {
            std::cout << "  Radius " << r << ": " 
                      << result.value().results.size() << " results, "
                      << result.value().computation_time_ms << " ms" << std::endl;
        }
    }
    std::cout << std::endl;
    
    std::cout << "=== Example completed successfully ===" << std::endl;
    return 0;
}
