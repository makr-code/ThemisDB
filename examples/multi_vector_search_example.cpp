/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            multi_vector_search_example.cpp                    ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     364                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file multi_vector_search_example.cpp
 * @brief Example usage of MultiVectorSearch for complex similarity queries
 * 
 * This example demonstrates various use cases of the MultiVectorSearch algorithm:
 * - Query expansion with multiple reformulations
 * - Multi-field search across different vector fields
 * - Hybrid search combining vector and keyword scores
 * - Different fusion strategies (RRF, Linear Combination, etc.)
 */

#include "index/multi_vector_search.h"
#include "index/vector_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <iostream>
#include <vector>

using namespace themis;
using namespace themis::vector;

/**
 * Example 1: Query Expansion
 * Use case: Search with multiple query reformulations to improve recall
 */
void example_query_expansion(MultiVectorSearch& multi_search) {
    std::cout << "\n=== Example 1: Query Expansion ===" << std::endl;
    
    // Multiple reformulations of the same query
    // e.g., "machine learning" -> ["machine learning", "ML", "artificial intelligence"]
    std::vector<std::vector<float>> query_variants = {
        {0.8f, 0.2f, 0.1f},  // Original query
        {0.7f, 0.3f, 0.15f}, // Reformulation 1
        {0.75f, 0.25f, 0.12f} // Reformulation 2
    };
    
    MultiVectorSearch::SearchConfig config;
    config.fusion = MultiVectorSearch::FusionStrategy::RECIPROCAL_RANK;
    config.top_k = 10;
    config.rrf_k = 60.0f; // Standard RRF constant
    
    auto result = multi_search.searchWithExpansion(query_variants, config);
    if (result) {
        std::cout << "Found " << result.value().results.size() << " results" << std::endl;
        std::cout << "Computation time: " << result.value().computation_time_ms << " ms" << std::endl;
        
        // Display top results
        for (size_t i = 0; i < std::min(size_t(5), result.value().results.size()); ++i) {
            const auto& res = result.value().results[i];
            std::cout << "  " << (i+1) << ". ID: " << res.id 
                      << ", Score: " << res.fused_score << std::endl;
        }
    } else {
        std::cerr << "Error: " << result.error().message() << std::endl;
    }
}

/**
 * Example 2: Multi-Field Search
 * Use case: Search across multiple vector fields (title, content, metadata)
 */
void example_multi_field_search(MultiVectorSearch& multi_search) {
    std::cout << "\n=== Example 2: Multi-Field Search ===" << std::endl;
    
    std::vector<float> query_vector = {0.8f, 0.2f, 0.1f};
    std::vector<std::string> field_names = {"title_embedding", "content_embedding"};
    
    MultiVectorSearch::SearchConfig config;
    config.fusion = MultiVectorSearch::FusionStrategy::LINEAR_COMBINATION;
    config.weights = {0.6f, 0.4f}; // 60% weight on title, 40% on content
    config.top_k = 10;
    
    auto result = multi_search.searchMultiField(query_vector, field_names, config);
    if (result) {
        std::cout << "Multi-field search completed" << std::endl;
        std::cout << "Total candidates: " << result.value().total_candidates << std::endl;
        std::cout << "Weights used: [";
        for (size_t i = 0; i < result.value().weights_used.size(); ++i) {
            std::cout << result.value().weights_used[i];
            if (i < result.value().weights_used.size() - 1) {
              std::cout << ", ";
            }
        }
        std::cout << "]" << std::endl;
    }
}

/**
 * Example 3: Hybrid Search
 * Use case: Combine semantic (vector) search with lexical (keyword) search
 */
void example_hybrid_search(MultiVectorSearch& multi_search) {
    std::cout << "\n=== Example 3: Hybrid Search ===" << std::endl;
    
    std::vector<float> query_vector = {0.8f, 0.2f, 0.1f};
    
    // Simulate BM25/keyword scores for documents
    std::unordered_map<std::string, float> keyword_scores = {
        {"doc1", 0.85f},
        {"doc2", 0.72f},
        {"doc3", 0.91f},
        {"doc5", 0.68f}
    };
    
    MultiVectorSearch::SearchConfig config;
    config.fusion = MultiVectorSearch::FusionStrategy::LINEAR_COMBINATION;
    config.weights = {0.7f, 0.3f}; // 70% vector, 30% keyword
    config.top_k = 10;
    
    auto result = multi_search.hybridSearch(query_vector, keyword_scores, config);
    if (result) {
        std::cout << "Hybrid search combining vector + keyword scores" << std::endl;
        std::cout << "Found " << result.value().results.size() << " results" << std::endl;
        
        for (size_t i = 0; i < std::min(size_t(5), result.value().results.size()); ++i) {
            const auto& res = result.value().results[i];
            std::cout << "  " << (i+1) << ". ID: " << res.id 
                      << ", Fused Score: " << res.fused_score;
            if (res.individual_scores.size() >= 2) {
                std::cout << " (Vector: " << res.individual_scores[0] 
                          << ", Keyword: " << res.individual_scores[1] << ")";
            }
            std::cout << std::endl;
        }
    }
}

/**
 * Example 4: Multiple Query Vectors with Different Fusion Strategies
 * Use case: Compare different fusion strategies
 */
void example_fusion_strategies(MultiVectorSearch& multi_search) {
    std::cout << "\n=== Example 4: Fusion Strategy Comparison ===" << std::endl;
    
    MultiVectorSearch::MultiQuery query;
    query.vectors = {
        {0.8f, 0.2f, 0.1f},
        {0.7f, 0.3f, 0.15f}
    };
    
    // Try different fusion strategies
    std::vector<MultiVectorSearch::FusionStrategy> strategies = {
        MultiVectorSearch::FusionStrategy::LINEAR_COMBINATION,
        MultiVectorSearch::FusionStrategy::RECIPROCAL_RANK,
        MultiVectorSearch::FusionStrategy::MAX_SCORE,
        MultiVectorSearch::FusionStrategy::AVG_SCORE
    };
    
    std::vector<std::string> strategy_names = {
        "Linear Combination",
        "Reciprocal Rank Fusion (RRF)",
        "Max Score",
        "Average Score"
    };
    
    for (size_t i = 0; i < strategies.size(); ++i) {
        MultiVectorSearch::SearchConfig config;
        config.fusion = strategies[i];
        config.top_k = 5;
        
        if (strategies[i] == MultiVectorSearch::FusionStrategy::LINEAR_COMBINATION) {
            config.weights = {0.5f, 0.5f};
        }
        
        auto result = multi_search.search(query, config);
        if (result) {
            std::cout << "\n" << strategy_names[i] << ":" << std::endl;
            std::cout << "  Top result: " << result.value().results[0].id 
                      << " (score: " << result.value().results[0].fused_score << ")" << std::endl;
            std::cout << "  Time: " << result.value().computation_time_ms << " ms" << std::endl;
        }
    }
}

/**
 * Example 5: Batch Search
 * Use case: Process multiple queries efficiently
 */
void example_batch_search(MultiVectorSearch& multi_search) {
    std::cout << "\n=== Example 5: Batch Search ===" << std::endl;
    
    std::vector<MultiVectorSearch::MultiQuery> queries;
    
    // Query 1
    MultiVectorSearch::MultiQuery q1;
    q1.vectors = {{0.8f, 0.2f, 0.1f}};
    queries.push_back(q1);
    
    // Query 2
    MultiVectorSearch::MultiQuery q2;
    q2.vectors = {{0.3f, 0.7f, 0.2f}};
    queries.push_back(q2);
    
    // Query 3
    MultiVectorSearch::MultiQuery q3;
    q3.vectors = {{0.1f, 0.1f, 0.8f}};
    queries.push_back(q3);
    
    MultiVectorSearch::SearchConfig config;
    config.fusion = MultiVectorSearch::FusionStrategy::LINEAR_COMBINATION;
    config.top_k = 5;
    
    auto result = multi_search.batchSearch(queries, config);
    if (result) {
        std::cout << "Processed " << result.value().size() << " queries" << std::endl;
        for (size_t i = 0; i < result.value().size(); ++i) {
            std::cout << "  Query " << (i+1) << ": " 
                      << result.value()[i].results.size() << " results, "
                      << result.value()[i].computation_time_ms << " ms" << std::endl;
        }
    }
}

/**
 * Example 6: Weight Optimization
 * Use case: Learn optimal fusion weights from training data
 */
void example_weight_optimization(MultiVectorSearch& multi_search) {
    std::cout << "\n=== Example 6: Weight Optimization ===" << std::endl;
    
    // Training queries
    std::vector<MultiVectorSearch::MultiQuery> training_queries;
    MultiVectorSearch::MultiQuery q1;
    q1.vectors = {
        {0.8f, 0.2f, 0.1f},
        {0.7f, 0.3f, 0.15f}
    };
    training_queries.push_back(q1);
    
    // Relevance judgments (which documents are relevant for each query)
    std::vector<std::vector<std::string>> relevance_judgments = {
        {"doc1", "doc3", "doc5"}  // Relevant docs for q1
    };
    
    std::cout << "Optimizing weights using NDCG metric..." << std::endl;
    auto result = multi_search.optimizeWeights(training_queries, relevance_judgments);
    if (result) {
        std::cout << "Optimal weights found: [";
        for (size_t i = 0; i < result.value().size(); ++i) {
            std::cout << result.value()[i];
            if (i < result.value().size() - 1) {
              std::cout << ", ";
            }
        }
        std::cout << "]" << std::endl;
    }
}

/**
 * Example 7: Statistics and Monitoring
 * Use case: Track performance metrics
 */
void example_statistics(MultiVectorSearch& multi_search) {
    std::cout << "\n=== Example 7: Statistics ===" << std::endl;
    
    // Reset statistics
    multi_search.resetStatistics();
    
    // Perform some searches
    MultiVectorSearch::MultiQuery query;
    query.vectors = {{0.8f, 0.2f, 0.1f}};
    
    MultiVectorSearch::SearchConfig config;
    config.top_k = 10;
    
    // Try different strategies
    config.fusion = MultiVectorSearch::FusionStrategy::LINEAR_COMBINATION;
    multi_search.search(query, config);
    
    config.fusion = MultiVectorSearch::FusionStrategy::RECIPROCAL_RANK;
    multi_search.search(query, config);
    
    config.fusion = MultiVectorSearch::FusionStrategy::MAX_SCORE;
    multi_search.search(query, config);
    
    // Get statistics
    const auto& stats = multi_search.getStatistics();
    std::cout << "Total searches: " << stats.total_searches << std::endl;
    std::cout << "Average time: " << stats.avg_time_ms << " ms" << std::endl;
    std::cout << "Average results per search: " << stats.avg_results_per_search << std::endl;
    std::cout << "\nStrategy usage:" << std::endl;
    for (const auto& [strategy, count] : stats.strategy_usage) {
        std::cout << "  Strategy " << static_cast<int>(strategy) << ": " << count << " times" << std::endl;
    }
}

int main() {
    std::cout << "MultiVectorSearch Examples" << std::endl;
    std::cout << "==========================" << std::endl;
    
    try {
        // Setup database and vector index
        RocksDBWrapper::Config db_config;
        db_config.db_path = "/tmp/themis_multi_vector_example";
        
        RocksDBWrapper db(db_config);
        if (!db.open()) {
            std::cerr << "Failed to open database" << std::endl;
            return 1;
        }
        
        VectorIndexManager vector_mgr(db);
        auto status = vector_mgr.init("documents", 3, VectorIndexManager::Metric::COSINE);
        if (!status.ok) {
            std::cerr << "Failed to initialize vector index: " << status.message << std::endl;
            return 1;
        }
        
        // Add some sample documents
        for (int i = 1; i <= 10; ++i) {
            BaseEntity entity("doc" + std::to_string(i));
            entity.setField("id", "doc" + std::to_string(i));
            
            // Random vectors for demonstration
            std::vector<float> vec = {
                static_cast<float>(i) / 10.0f,
                static_cast<float>(10 - i) / 10.0f,
                0.5f
            };
            entity.setField("embedding", vec);
            
            vector_mgr.addEntity(entity, "embedding");
        }
        
        // Create MultiVectorSearch instance
        MultiVectorSearch multi_search(vector_mgr);
        
        // Run examples
        example_query_expansion(multi_search);
        example_multi_field_search(multi_search);
        example_hybrid_search(multi_search);
        example_fusion_strategies(multi_search);
        example_batch_search(multi_search);
        example_weight_optimization(multi_search);
        example_statistics(multi_search);
        
        std::cout << "\n==========================" << std::endl;
        std::cout << "All examples completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
