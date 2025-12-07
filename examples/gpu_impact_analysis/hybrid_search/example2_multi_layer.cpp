/**
 * Example 2: Multi-Layer Analysis with Hybrid Search
 * 
 * This example demonstrates cross-layer impact analysis using
 * hybrid search with layer-specific configurations.
 */

#include "enterprise/gpu_impact_analysis_plugin.h"
#include <iostream>

int main() {
    using namespace themis::enterprise;
    
    auto plugin = createGPUImpactAnalysisPlugin();
    plugin->initialize({{"gpu_backend", "cuda"}});
    
    // Database schema change
    IGPUImpactAnalysisPlugin::DocumentChange db_change;
    db_change.document_id = "schema/customers/email_column";
    db_change.change_type = "column_removed";
    db_change.source_layer = "database";
    db_change.magnitude = 0.85;
    
    // Layer metadata
    IGPUImpactAnalysisPlugin::LayerMetadata layer_meta;
    layer_meta.layer_type = IGPUImpactAnalysisPlugin::LayerType::DATABASE;
    layer_meta.layer_name = "customers_schema";
    layer_meta.criticality = 0.90;
    db_change.layer_metadata = layer_meta;
    
    // Multi-Layer Hybrid Search Configuration
    nlohmann::json config = {
        {"use_hybrid_search", true},
        {"hybrid_search_layers", {"api", "process", "ui"}},
        
        // Layer-specific hybrid weights
        {"layer_hybrid_weights", {
            {"database->api", {
                {"alpha", 0.8},   // High semantic similarity for DB->API
                {"beta", 0.1},
                {"gamma", 0.1}
            }},
            {"api->ui", {
                {"alpha", 0.6},   // Moderate for API->UI
                {"beta", 0.3},
                {"gamma", 0.1}
            }},
            {"api->process", {
                {"alpha", 0.7},
                {"beta", 0.2},
                {"gamma", 0.1}
            }}
        }},
        
        // Cross-layer semantic thresholds
        {"cross_layer_semantic_thresholds", {
            {"database->api", 0.5},    // High threshold: DB→API must be very similar
            {"api->ui", 0.3},          // Moderate
            {"api->process", 0.4}      // Moderate-High
        }},
        
        // FEM config
        {"fem", {
            {"enable_cross_layer_propagation", true},
            {"layer_damping_factors", {
                {"database", 0.80},
                {"api", 0.95},
                {"ui", 0.75},
                {"process", 0.85}
            }}
        }}
    };
    
    std::cout << "Analyzing database schema change across multiple layers...\n\n";
    
    // Perform multi-layer analysis
    auto result = plugin->analyzeMultiLayerImpact(
        db_change,
        {"api", "ui", "process"},  // Target layers
        config
    );
    
    // Display layer-specific results
    std::cout << "=== Multi-Layer Impact Results ===\n";
    std::cout << "Total affected nodes: " << result.total_affected_count << "\n";
    std::cout << "Cross-layer transitions: " << result.cross_layer_transitions << "\n\n";
    
    std::cout << "=== Impact per Layer ===\n";
    for (const auto& [layer, count] : result.affected_nodes_per_layer) {
        std::cout << "Layer '" << layer << "':\n";
        std::cout << "  - Nodes affected: " << count << "\n";
        std::cout << "  - Max impact: " << result.max_impact_per_layer.at(layer) << "\n\n";
    }
    
    // Show cross-layer transition paths
    std::cout << "=== Cross-Layer Transition Paths ===\n";
    std::map<std::pair<std::string, std::string>, int> transition_counts;
    for (const auto& [from, to] : result.layer_transition_paths) {
        transition_counts[{from, to}]++;
    }
    
    for (const auto& [transition, count] : transition_counts) {
        std::cout << transition.first << " → " << transition.second 
                  << ": " << count << " transitions\n";
    }
    std::cout << "\n";
    
    // Show nodes with cross-layer impact
    std::cout << "=== Cross-Layer Impact Nodes ===\n";
    for (const auto& node : result.affected_nodes) {
        if (node.is_cross_layer_impact) {
            std::cout << "Node: " << node.node_id << "\n";
            std::cout << "  Current Layer: " << node.node_layer << "\n";
            std::cout << "  Crossed Layers: ";
            for (const auto& layer : node.crossed_layers) {
                std::cout << layer << " ";
            }
            std::cout << "\n";
            std::cout << "  Impact Score: " << node.impact_score << "\n";
            
            // Show hybrid scores
            if (node.impact_details.contains("hybrid_scores")) {
                auto hs = node.impact_details["hybrid_scores"];
                std::cout << "  Cross-Layer Similarity: " << hs["cross_layer_similarity"] << "\n";
            }
            std::cout << "\n";
        }
    }
    
    // Recommendations based on analysis
    std::cout << "=== Recommendations ===\n";
    
    if (result.affected_nodes_per_layer["api"] > 10) {
        std::cout << "- Many API endpoints affected. Consider backward compatibility.\n";
    }
    
    if (result.affected_nodes_per_layer["ui"] > 15) {
        std::cout << "- Significant UI impact. Update user-facing documentation.\n";
    }
    
    if (result.cross_layer_transitions > 20) {
        std::cout << "- High cross-layer coupling detected. Consider refactoring.\n";
    }
    
    plugin->shutdown();
    return 0;
}
