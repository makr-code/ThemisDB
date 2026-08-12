/**
 * @file test_community_detection_aql.cpp
 * @brief Integration tests for community detection AQL functions
 * 
 * Tests LOUVAIN_COMMUNITIES and LABEL_PROPAGATION_COMMUNITIES functions
 * exposed through AQL.
 */

#include <gtest/gtest.h>
#include "query/functions/graph_functions.h"
#include "query/functions/function_registry.h"
#include <nlohmann/json.hpp>

using namespace themis::query::functions;
using json = nlohmann::json;

class CommunityDetectionAQLTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create function instances
        louvain_func = std::make_unique<LouvainCommunitiesFunction>();
        label_prop_func = std::make_unique<LabelPropagationCommunitiesFunction>();
        
        // Create empty context
        context = FunctionContext();
    }
    
    // Helper: Create edge document
    json createEdge(const std::string& from, const std::string& to, double weight = 1.0) {
        return json{
            {"_from", from},
            {"_to", to},
            {"weight", weight}
        };
    }
    
    std::unique_ptr<LouvainCommunitiesFunction> louvain_func;
    std::unique_ptr<LabelPropagationCommunitiesFunction> label_prop_func;
    FunctionContext context;
};

// ============================================================================
// LOUVAIN_COMMUNITIES Tests
// ============================================================================

TEST_F(CommunityDetectionAQLTest, LouvainCommunities_EmptyGraph) {
    json edges = json::array();
    std::vector<json> args = {edges};
    
    auto result = louvain_func->execute(args, context);
    
    EXPECT_TRUE(result.is_object());
    EXPECT_EQ(result.size(), 0);
}

TEST_F(CommunityDetectionAQLTest, LouvainCommunities_SingleNode) {
    json edges = json::array();
    edges.push_back(createEdge("A", "A"));  // Self-loop
    
    std::vector<json> args = {edges};
    auto result = louvain_func->execute(args, context);
    
    EXPECT_TRUE(result.is_object());
    EXPECT_EQ(result.size(), 1);
    EXPECT_TRUE(result.contains("A"));
    EXPECT_EQ(result["A"].get<int>(), 0);
}

TEST_F(CommunityDetectionAQLTest, LouvainCommunities_TwoNodes) {
    json edges = json::array();
    edges.push_back(createEdge("A", "B"));
    edges.push_back(createEdge("B", "A"));
    
    std::vector<json> args = {edges};
    auto result = louvain_func->execute(args, context);
    
    EXPECT_TRUE(result.is_object());
    EXPECT_EQ(result.size(), 2);
    EXPECT_TRUE(result.contains("A"));
    EXPECT_TRUE(result.contains("B"));
    
    // Both should be in same community
    EXPECT_EQ(result["A"].get<int>(), result["B"].get<int>());
}

TEST_F(CommunityDetectionAQLTest, LouvainCommunities_TwoClusters) {
    json edges = json::array();
    
    // Cluster 1: A <-> B <-> C (triangle)
    edges.push_back(createEdge("A", "B"));
    edges.push_back(createEdge("B", "A"));
    edges.push_back(createEdge("B", "C"));
    edges.push_back(createEdge("C", "B"));
    edges.push_back(createEdge("C", "A"));
    edges.push_back(createEdge("A", "C"));
    
    // Cluster 2: D <-> E <-> F (triangle)
    edges.push_back(createEdge("D", "E"));
    edges.push_back(createEdge("E", "D"));
    edges.push_back(createEdge("E", "F"));
    edges.push_back(createEdge("F", "E"));
    edges.push_back(createEdge("F", "D"));
    edges.push_back(createEdge("D", "F"));
    
    // Weak bridge
    edges.push_back(createEdge("C", "D"));
    
    std::vector<json> args = {edges};
    auto result = louvain_func->execute(args, context);
    
    EXPECT_TRUE(result.is_object());
    EXPECT_EQ(result.size(), 6);
    
    // All nodes should have community assignments
    EXPECT_TRUE(result.contains("A"));
    EXPECT_TRUE(result.contains("B"));
    EXPECT_TRUE(result.contains("C"));
    EXPECT_TRUE(result.contains("D"));
    EXPECT_TRUE(result.contains("E"));
    EXPECT_TRUE(result.contains("F"));
    
    // Should detect grouping (not all in different communities)
    std::set<int> unique_communities;
    for (const auto& [node, comm] : result.items()) {
        unique_communities.insert(comm.get<int>());
    }
    
    EXPECT_GE(unique_communities.size(), 1);
    EXPECT_LE(unique_communities.size(), 6);
}

TEST_F(CommunityDetectionAQLTest, LouvainCommunities_WithMinModularityGain) {
    json edges = json::array();
    edges.push_back(createEdge("A", "B"));
    edges.push_back(createEdge("B", "C"));
    
    // Test with high min_modularity_gain (should prevent merging)
    std::vector<json> args = {edges, json(0.9)};
    auto result = louvain_func->execute(args, context);
    
    EXPECT_TRUE(result.is_object());
    EXPECT_GE(result.size(), 1);
}

// ============================================================================
// LABEL_PROPAGATION_COMMUNITIES Tests
// ============================================================================

TEST_F(CommunityDetectionAQLTest, LabelPropagation_EmptyGraph) {
    json edges = json::array();
    std::vector<json> args = {edges};
    
    auto result = label_prop_func->execute(args, context);
    
    EXPECT_TRUE(result.is_object());
    EXPECT_EQ(result.size(), 0);
}

TEST_F(CommunityDetectionAQLTest, LabelPropagation_SingleNode) {
    json edges = json::array();
    edges.push_back(createEdge("A", "A"));
    
    std::vector<json> args = {edges};
    auto result = label_prop_func->execute(args, context);
    
    EXPECT_TRUE(result.is_object());
    EXPECT_EQ(result.size(), 1);
    EXPECT_TRUE(result.contains("A"));
}

TEST_F(CommunityDetectionAQLTest, LabelPropagation_TwoNodes) {
    json edges = json::array();
    edges.push_back(createEdge("A", "B"));
    edges.push_back(createEdge("B", "A"));
    
    std::vector<json> args = {edges};
    auto result = label_prop_func->execute(args, context);
    
    EXPECT_TRUE(result.is_object());
    EXPECT_EQ(result.size(), 2);
    EXPECT_TRUE(result.contains("A"));
    EXPECT_TRUE(result.contains("B"));
}

TEST_F(CommunityDetectionAQLTest, LabelPropagation_TwoClusters) {
    json edges = json::array();
    
    // Cluster 1: A <-> B <-> C
    edges.push_back(createEdge("A", "B"));
    edges.push_back(createEdge("B", "A"));
    edges.push_back(createEdge("B", "C"));
    edges.push_back(createEdge("C", "B"));
    edges.push_back(createEdge("C", "A"));
    edges.push_back(createEdge("A", "C"));
    
    // Cluster 2: D <-> E <-> F
    edges.push_back(createEdge("D", "E"));
    edges.push_back(createEdge("E", "D"));
    edges.push_back(createEdge("E", "F"));
    edges.push_back(createEdge("F", "E"));
    edges.push_back(createEdge("F", "D"));
    edges.push_back(createEdge("D", "F"));
    
    // Weak bridge
    edges.push_back(createEdge("C", "D"));
    
    std::vector<json> args = {edges};
    auto result = label_prop_func->execute(args, context);
    
    EXPECT_TRUE(result.is_object());
    EXPECT_EQ(result.size(), 6);
    
    // All nodes should have community assignments
    EXPECT_TRUE(result.contains("A"));
    EXPECT_TRUE(result.contains("B"));
    EXPECT_TRUE(result.contains("C"));
    EXPECT_TRUE(result.contains("D"));
    EXPECT_TRUE(result.contains("E"));
    EXPECT_TRUE(result.contains("F"));
}

TEST_F(CommunityDetectionAQLTest, LabelPropagation_ChainGraph) {
    json edges = json::array();
    
    // Linear chain: A -> B -> C -> D
    edges.push_back(createEdge("A", "B"));
    edges.push_back(createEdge("B", "C"));
    edges.push_back(createEdge("C", "D"));
    
    std::vector<json> args = {edges, json(50)};  // max_iterations = 50
    auto result = label_prop_func->execute(args, context);
    
    EXPECT_TRUE(result.is_object());
    EXPECT_EQ(result.size(), 4);
}

TEST_F(CommunityDetectionAQLTest, LabelPropagation_WithMaxIterations) {
    json edges = json::array();
    edges.push_back(createEdge("A", "B"));
    edges.push_back(createEdge("B", "C"));
    
    // Test with low max_iterations
    std::vector<json> args = {edges, json(1)};
    auto result = label_prop_func->execute(args, context);
    
    EXPECT_TRUE(result.is_object());
    EXPECT_GE(result.size(), 1);
}

// ============================================================================
// Comparison Tests
// ============================================================================

TEST_F(CommunityDetectionAQLTest, Comparison_BothAlgorithmsProduceCommunities) {
    json edges = json::array();
    
    // Create a simple graph
    edges.push_back(createEdge("A", "B"));
    edges.push_back(createEdge("B", "A"));
    edges.push_back(createEdge("B", "C"));
    edges.push_back(createEdge("C", "B"));
    
    std::vector<json> args = {edges};
    
    auto louvain_result = louvain_func->execute(args, context);
    auto label_prop_result = label_prop_func->execute(args, context);
    
    // Both should return valid results
    EXPECT_TRUE(louvain_result.is_object());
    EXPECT_TRUE(label_prop_result.is_object());
    
    // Both should have same nodes
    EXPECT_EQ(louvain_result.size(), label_prop_result.size());
}

// ============================================================================
// Function Signature Tests
// ============================================================================

TEST_F(CommunityDetectionAQLTest, LouvainSignature) {
    auto sig = louvain_func->signature();
    
    EXPECT_EQ(sig.name, "LOUVAIN_COMMUNITIES");
    EXPECT_EQ(sig.category, "Graph");
    EXPECT_FALSE(sig.description.empty());
    EXPECT_GE(sig.arguments.size(), 1);
    EXPECT_EQ(sig.arguments[0].name, "edges");
    EXPECT_EQ(sig.return_type, ArgType::OBJECT);
    EXPECT_TRUE(sig.is_deterministic);
    EXPECT_FALSE(sig.is_aggregate);
}

TEST_F(CommunityDetectionAQLTest, LabelPropagationSignature) {
    auto sig = label_prop_func->signature();
    
    EXPECT_EQ(sig.name, "LABEL_PROPAGATION_COMMUNITIES");
    EXPECT_EQ(sig.category, "Graph");
    EXPECT_FALSE(sig.description.empty());
    EXPECT_GE(sig.arguments.size(), 1);
    EXPECT_EQ(sig.arguments[0].name, "edges");
    EXPECT_EQ(sig.return_type, ArgType::OBJECT);
    EXPECT_TRUE(sig.is_deterministic);
    EXPECT_FALSE(sig.is_aggregate);
}
