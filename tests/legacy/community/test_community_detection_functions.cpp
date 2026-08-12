/**
 * @file test_community_detection_functions.cpp
 * @brief Tests for Community Detection AQL Functions
 * 
 * Tests the LOUVAIN_COMMUNITIES and LABEL_PROPAGATION_COMMUNITIES functions.
 */

#include <gtest/gtest.h>
#include "query/functions/function_registry.h"
#include "query/functions/graph_extensions.h"

using namespace themis::query::functions;
using json = nlohmann::json;

class CommunityDetectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto& registry = FunctionRegistry::instance();
        registerGraphExtensions(registry);
    }
    
    FunctionContext ctx;
    
    // Helper: Create edge document
    json makeEdge(const std::string& from, const std::string& to) {
        return json{
            {"_from", from},
            {"_to", to}
        };
    }
};

// ============================================================================
// Louvain Communities Tests
// ============================================================================

TEST_F(CommunityDetectionTest, LouvainCommunities_EmptyGraph) {
    auto& reg = FunctionRegistry::instance();
    
    json edges = json::array();
    auto result = reg.call("LOUVAIN_COMMUNITIES", {edges}, ctx);
    
    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("communities"));
    EXPECT_TRUE(result["communities"].is_array());
    EXPECT_EQ(result["communities"].size(), 0);
}

TEST_F(CommunityDetectionTest, LouvainCommunities_SingleNode) {
    auto& reg = FunctionRegistry::instance();
    
    json edges = json::array();
    edges.push_back(makeEdge("A", "A"));  // Self-loop
    
    auto result = reg.call("LOUVAIN_COMMUNITIES", {edges}, ctx);
    
    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("communities"));
    EXPECT_GE(result["communities"].size(), 1);
}

TEST_F(CommunityDetectionTest, LouvainCommunities_TwoNodes) {
    auto& reg = FunctionRegistry::instance();
    
    json edges = json::array();
    edges.push_back(makeEdge("A", "B"));
    
    auto result = reg.call("LOUVAIN_COMMUNITIES", {edges}, ctx);
    
    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("communities"));
    EXPECT_TRUE(result.contains("num_communities"));
    EXPECT_TRUE(result.contains("overall_modularity"));
    
    // Should have at least 1 community
    EXPECT_GE(result["num_communities"].get<int>(), 1);
}

TEST_F(CommunityDetectionTest, LouvainCommunities_Triangle) {
    auto& reg = FunctionRegistry::instance();
    
    // Create a triangle: A <-> B <-> C <-> A
    json edges = json::array();
    edges.push_back(makeEdge("A", "B"));
    edges.push_back(makeEdge("B", "A"));
    edges.push_back(makeEdge("B", "C"));
    edges.push_back(makeEdge("C", "B"));
    edges.push_back(makeEdge("C", "A"));
    edges.push_back(makeEdge("A", "C"));
    
    auto result = reg.call("LOUVAIN_COMMUNITIES", {edges}, ctx);
    
    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("communities"));
    
    // All nodes should be in the same community or close communities
    auto communities = result["communities"];
    EXPECT_GE(communities.size(), 1);
    
    // Check structure of community objects
    for (const auto& comm : communities) {
        EXPECT_TRUE(comm.contains("id"));
        EXPECT_TRUE(comm.contains("members"));
        EXPECT_TRUE(comm.contains("size"));
        EXPECT_TRUE(comm.contains("modularity"));
        EXPECT_TRUE(comm.contains("density"));
        
        EXPECT_TRUE(comm["members"].is_array());
        EXPECT_EQ(comm["size"].get<int>(), comm["members"].size());
    }
}

TEST_F(CommunityDetectionTest, LouvainCommunities_TwoClusters) {
    auto& reg = FunctionRegistry::instance();
    
    // Create two distinct triangles with weak connection
    // Cluster 1: A <-> B <-> C
    // Cluster 2: D <-> E <-> F
    // Bridge: C -> D
    json edges = json::array();
    
    // Cluster 1
    edges.push_back(makeEdge("A", "B"));
    edges.push_back(makeEdge("B", "A"));
    edges.push_back(makeEdge("B", "C"));
    edges.push_back(makeEdge("C", "B"));
    edges.push_back(makeEdge("C", "A"));
    edges.push_back(makeEdge("A", "C"));
    
    // Cluster 2
    edges.push_back(makeEdge("D", "E"));
    edges.push_back(makeEdge("E", "D"));
    edges.push_back(makeEdge("E", "F"));
    edges.push_back(makeEdge("F", "E"));
    edges.push_back(makeEdge("F", "D"));
    edges.push_back(makeEdge("D", "F"));
    
    // Bridge
    edges.push_back(makeEdge("C", "D"));
    
    auto result = reg.call("LOUVAIN_COMMUNITIES", {edges}, ctx);
    
    EXPECT_TRUE(result.is_object());
    auto num_communities = result["num_communities"].get<int>();
    
    // Should detect multiple communities (typically 1-6)
    EXPECT_GE(num_communities, 1);
    EXPECT_LE(num_communities, 6);
}

TEST_F(CommunityDetectionTest, LouvainCommunities_WithOptions) {
    auto& reg = FunctionRegistry::instance();
    
    json edges = json::array();
    edges.push_back(makeEdge("A", "B"));
    edges.push_back(makeEdge("B", "C"));
    
    json options = json{
        {"min_modularity_gain", 0.001}
    };
    
    auto result = reg.call("LOUVAIN_COMMUNITIES", {edges, options}, ctx);
    
    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("communities"));
}

// ============================================================================
// Label Propagation Communities Tests
// ============================================================================

TEST_F(CommunityDetectionTest, LabelPropagation_EmptyGraph) {
    auto& reg = FunctionRegistry::instance();
    
    json edges = json::array();
    auto result = reg.call("LABEL_PROPAGATION_COMMUNITIES", {edges}, ctx);
    
    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("communities"));
    EXPECT_TRUE(result["communities"].is_array());
    EXPECT_EQ(result["communities"].size(), 0);
}

TEST_F(CommunityDetectionTest, LabelPropagation_TwoNodes) {
    auto& reg = FunctionRegistry::instance();
    
    json edges = json::array();
    edges.push_back(makeEdge("A", "B"));
    
    auto result = reg.call("LABEL_PROPAGATION_COMMUNITIES", {edges}, ctx);
    
    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("communities"));
    EXPECT_TRUE(result.contains("num_communities"));
    
    // Should have at least 1 community
    EXPECT_GE(result["num_communities"].get<int>(), 1);
}

TEST_F(CommunityDetectionTest, LabelPropagation_Triangle) {
    auto& reg = FunctionRegistry::instance();
    
    // Create a triangle
    json edges = json::array();
    edges.push_back(makeEdge("A", "B"));
    edges.push_back(makeEdge("B", "A"));
    edges.push_back(makeEdge("B", "C"));
    edges.push_back(makeEdge("C", "B"));
    edges.push_back(makeEdge("C", "A"));
    edges.push_back(makeEdge("A", "C"));
    
    auto result = reg.call("LABEL_PROPAGATION_COMMUNITIES", {edges}, ctx);
    
    EXPECT_TRUE(result.is_object());
    auto communities = result["communities"];
    
    EXPECT_GE(communities.size(), 1);
    
    // Check structure
    for (const auto& comm : communities) {
        EXPECT_TRUE(comm.contains("id"));
        EXPECT_TRUE(comm.contains("members"));
        EXPECT_TRUE(comm.contains("size"));
        
        EXPECT_TRUE(comm["members"].is_array());
        EXPECT_EQ(comm["size"].get<int>(), comm["members"].size());
    }
}

TEST_F(CommunityDetectionTest, LabelPropagation_Chain) {
    auto& reg = FunctionRegistry::instance();
    
    // Linear chain: A -> B -> C -> D
    json edges = json::array();
    edges.push_back(makeEdge("A", "B"));
    edges.push_back(makeEdge("B", "C"));
    edges.push_back(makeEdge("C", "D"));
    
    auto result = reg.call("LABEL_PROPAGATION_COMMUNITIES", {edges}, ctx);
    
    EXPECT_TRUE(result.is_object());
    auto num_communities = result["num_communities"].get<int>();
    
    // Chain should converge to some community structure
    EXPECT_GE(num_communities, 1);
    EXPECT_LE(num_communities, 4);
}

TEST_F(CommunityDetectionTest, LabelPropagation_WithOptions) {
    auto& reg = FunctionRegistry::instance();
    
    json edges = json::array();
    edges.push_back(makeEdge("A", "B"));
    edges.push_back(makeEdge("B", "C"));
    
    json options = json{
        {"max_iterations", 10}
    };
    
    auto result = reg.call("LABEL_PROPAGATION_COMMUNITIES", {edges, options}, ctx);
    
    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("communities"));
}

TEST_F(CommunityDetectionTest, LabelPropagation_HugeMaxIterationsOptionDoesNotOverflow) {
    auto& reg = FunctionRegistry::instance();

    json edges = json::array();
    edges.push_back(makeEdge("A", "B"));
    edges.push_back(makeEdge("B", "C"));

    json options = json{
        {"max_iterations", 1e300}
    };

    auto result = reg.call("LABEL_PROPAGATION_COMMUNITIES", {edges, options}, ctx);

    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("communities"));
}

TEST_F(CommunityDetectionTest, LabelPropagation_HugeScalarMaxIterationsDoesNotOverflow) {
    auto& reg = FunctionRegistry::instance();

    json edges = json::array();
    edges.push_back(makeEdge("A", "B"));
    edges.push_back(makeEdge("B", "C"));

    try {
        (void)reg.call("LABEL_PROPAGATION_COMMUNITIES", {edges, json(1e300)}, ctx);
        FAIL() << "Expected invalid scalar options to be rejected";
    } catch (const std::exception& e) {
        EXPECT_NE(std::string(e.what()).find("options"), std::string::npos);
    } catch (...) {
        FAIL() << "Expected std::exception for invalid scalar options";
    }
}

// ============================================================================
// Comparison Tests
// ============================================================================

TEST_F(CommunityDetectionTest, CompareAlgorithms_SameGraph) {
    auto& reg = FunctionRegistry::instance();
    
    // Create a simple graph
    json edges = json::array();
    edges.push_back(makeEdge("A", "B"));
    edges.push_back(makeEdge("B", "C"));
    edges.push_back(makeEdge("C", "A"));
    
    auto louvain_result = reg.call("LOUVAIN_COMMUNITIES", {edges}, ctx);
    auto label_prop_result = reg.call("LABEL_PROPAGATION_COMMUNITIES", {edges}, ctx);
    
    // Both should return valid results
    EXPECT_TRUE(louvain_result.is_object());
    EXPECT_TRUE(label_prop_result.is_object());
    
    // Both should have communities
    EXPECT_TRUE(louvain_result.contains("communities"));
    EXPECT_TRUE(label_prop_result.contains("communities"));
    
    // Both should detect at least one community
    EXPECT_GE(louvain_result["num_communities"].get<int>(), 1);
    EXPECT_GE(label_prop_result["num_communities"].get<int>(), 1);
}

// ============================================================================
// Betweenness Centrality Tests (via function registry)
// ============================================================================

TEST_F(CommunityDetectionTest, BetweennessCentrality_EmptyGraph) {
    auto& reg = FunctionRegistry::instance();

    json edges = json::array();
    auto result = reg.call("BETWEENNESS_CENTRALITY", {edges}, ctx);

    EXPECT_TRUE(result.is_object());
    EXPECT_EQ(result.size(), 0);
}

TEST_F(CommunityDetectionTest, BetweennessCentrality_TwoNodes) {
    auto& reg = FunctionRegistry::instance();

    json edges = json::array();
    edges.push_back(makeEdge("A", "B"));

    auto result = reg.call("BETWEENNESS_CENTRALITY", {edges}, ctx);

    EXPECT_TRUE(result.is_object());
    EXPECT_EQ(result.size(), 2);
    EXPECT_TRUE(result.contains("A"));
    EXPECT_TRUE(result.contains("B"));

    // A is a source with no incoming shortest-path dependencies; B is a sink
    EXPECT_GE(result["A"].get<double>(), 0.0);
    EXPECT_GE(result["B"].get<double>(), 0.0);
}

TEST_F(CommunityDetectionTest, BetweennessCentrality_HubNode) {
    auto& reg = FunctionRegistry::instance();

    // Graph: A->Hub->B, C->Hub->D — Hub lies on all inter-cluster shortest paths
    json edges = json::array();
    edges.push_back(makeEdge("A",   "Hub"));
    edges.push_back(makeEdge("C",   "Hub"));
    edges.push_back(makeEdge("Hub", "B"));
    edges.push_back(makeEdge("Hub", "D"));

    auto result = reg.call("BETWEENNESS_CENTRALITY", {edges}, ctx);

    EXPECT_TRUE(result.is_object());
    EXPECT_EQ(result.size(), 5);  // A, C, Hub, B, D

    // Hub must have the highest betweenness score
    double hub_bc = result["Hub"].get<double>();
    EXPECT_GT(hub_bc, 0.0);

    for (const auto& [node, score] : result.items()) {
        if (node != "Hub") {
            EXPECT_LE(score.get<double>(), hub_bc);
        }
    }
}

TEST_F(CommunityDetectionTest, BetweennessCentrality_NormalizeOption) {
    auto& reg = FunctionRegistry::instance();

    json edges = json::array();
    edges.push_back(makeEdge("A", "B"));
    edges.push_back(makeEdge("B", "C"));
    edges.push_back(makeEdge("A", "C"));

    // Without normalization
    auto raw = reg.call("BETWEENNESS_CENTRALITY", {edges}, ctx);
    // With normalization
    json opts = json{{"normalize", true}};
    auto normalized = reg.call("BETWEENNESS_CENTRALITY", {edges, opts}, ctx);

    EXPECT_TRUE(raw.is_object());
    EXPECT_TRUE(normalized.is_object());

    // Normalized scores must be <= raw scores for n>=3
    for (const auto& [node, raw_score] : raw.items()) {
        EXPECT_GE(raw_score.get<double>(), normalized[node].get<double>());
    }
}

TEST_F(CommunityDetectionTest, BetweennessCentrality_InvalidInput) {
    auto& reg = FunctionRegistry::instance();

    // Non-array input should return empty object
    json invalid = json::object();
    auto result = reg.call("BETWEENNESS_CENTRALITY", {invalid}, ctx);

    EXPECT_TRUE(result.is_object());
    EXPECT_EQ(result.size(), 0);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(CommunityDetectionTest, InvalidInput_NotArray) {
    auto& reg = FunctionRegistry::instance();
    
    // Test with non-array input
    json invalid = json::object();
    
    // The function should handle gracefully and return empty result
    auto result = reg.call("LOUVAIN_COMMUNITIES", {invalid}, ctx);
    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("communities"));
    EXPECT_EQ(result["communities"].size(), 0);
}

TEST_F(CommunityDetectionTest, InvalidEdge_MissingFields) {
    auto& reg = FunctionRegistry::instance();
    
    // Edge missing _to field
    json edges = json::array();
    edges.push_back(json{{"_from", "A"}});
    
    auto result = reg.call("LOUVAIN_COMMUNITIES", {edges}, ctx);
    
    // Should handle gracefully and return empty communities
    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result["communities"].is_array());
}
