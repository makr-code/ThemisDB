/**
 * Example 1: Basic Hybrid Search Usage
 * 
 * This example shows how to enable and use GPU Hybrid Search
 * for a simple API breaking change analysis.
 */

#include "enterprise/gpu_impact_analysis_plugin.h"
#include <iostream>

int main() {
    using namespace themis::enterprise;
    
    // Create plugin instance
    auto plugin = createGPUImpactAnalysisPlugin();
    
    // Initialize with Hybrid Search enabled
    nlohmann::json init_config = {
        {"gpu_backend", "cuda"},  // Use CUDA for GPU acceleration
        {"use_hybrid_search", true}
    };
    
    if (!plugin->initialize(init_config)) {
        std::cerr << "Failed to initialize plugin\n";
        return 1;
    }
    
    // Define an API breaking change
    IGPUImpactAnalysisPlugin::DocumentChange api_change;
    api_change.document_id = "api/v2/payment/process";
    api_change.change_type = "breaking_change";
    api_change.magnitude = 0.95;
    api_change.timestamp = std::time(nullptr) * 1000;
    api_change.user_id = "admin";
    
    // Configure Hybrid Search parameters
    nlohmann::json analysis_config = {
        // Hybrid Search Settings
        {"use_hybrid_search", true},
        {"hybrid_k_neighbors", 50},        // Top-50 most relevant neighbors
        {"semantic_threshold", 0.3},       // Minimum similarity score
        
        // Hybrid Scoring Weights
        {"alpha", 0.6},   // Vector similarity weight (60%)
        {"beta", 0.3},    // Text match weight (30%)
        {"gamma", 0.1},   // Graph structure weight (10%)
        
        // Analysis Options
        {"max_depth", 5},
        {"impact_threshold", 0.01},
        
        // Output Options
        {"return_hybrid_scores", true},
        {"explain_scoring", true}
    };
    
    std::cout << "Analyzing API breaking change with Hybrid Search...\n\n";
    
    // Perform analysis
    auto result = plugin->analyzeDocumentChangeImpact(api_change, analysis_config);
    
    // Display results
    std::cout << "=== Analysis Results ===\n";
    std::cout << "Total affected nodes: " << result.total_affected_count << "\n";
    std::cout << "Max impact score: " << result.max_impact_score << "\n";
    std::cout << "Avg impact score: " << result.avg_impact_score << "\n";
    std::cout << "Computation time: " << result.computation_time.count() << "ms\n\n";
    
    // Show top 10 affected nodes with hybrid scores
    std::cout << "=== Top 10 Affected Nodes ===\n";
    
    // Sort by impact score
    auto sorted_nodes = result.affected_nodes;
    std::sort(sorted_nodes.begin(), sorted_nodes.end(),
        [](const auto& a, const auto& b) {
            return a.impact_score > b.impact_score;
        });
    
    int count = 0;
    for (const auto& node : sorted_nodes) {
        if (count++ >= 10) break;
        
        std::cout << count << ". " << node.node_id << "\n";
        std::cout << "   Impact Score: " << node.impact_score << "\n";
        std::cout << "   Distance: " << node.distance_from_source << " hops\n";
        
        // Show hybrid scores if available
        if (node.impact_details.contains("hybrid_scores")) {
            auto hs = node.impact_details["hybrid_scores"];
            std::cout << "   Hybrid Scores:\n";
            std::cout << "     - Vector Similarity: " << hs["vector_similarity"] << "\n";
            std::cout << "     - Text Match: " << hs["text_score"] << "\n";
            std::cout << "     - Graph Weight: " << hs["graph_weight"] << "\n";
            std::cout << "     - Combined: " << hs["hybrid_score"] << "\n";
        }
        std::cout << "\n";
    }
    
    // Performance comparison (if metadata available)
    if (result.metadata.contains("performance")) {
        auto perf = result.metadata["performance"];
        std::cout << "=== Performance Metrics ===\n";
        std::cout << "Nodes visited: " << perf["nodes_visited"] << "\n";
        std::cout << "Nodes skipped (Hybrid): " << perf["nodes_skipped_hybrid"] << "\n";
        std::cout << "GPU time: " << perf["gpu_time_ms"] << "ms\n";
        std::cout << "Speedup vs CPU: " << perf["speedup"] << "x\n";
    }
    
    plugin->shutdown();
    return 0;
}
