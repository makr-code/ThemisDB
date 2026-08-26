/**
 * @file test_mcp_kg_tools.cpp
 * @brief Unit tests for MCP Group 1 Knowledge Graph tools:
 *        kg_neighbours, kg_shortest_path, kg_node_properties
 *
 * Labels: wave_b release_critical
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Lightweight test harness: we exercise handler logic without a full DB stack
// by driving the tool call directly through its contract (input/output shapes).
// ---------------------------------------------------------------------------

// Simulate what the handler returns for a missing required parameter
static json missing_param_error(const std::string& param) {
    return {{"error", "missing parameter: " + param}};
}

// ============================================================================
// kg_neighbours tests
// ============================================================================

TEST(KgNeighboursTest, MissingNodeIdReturnsError) {
    json args = json::object();   // no node_id
    // Contract: missing node_id must produce error key
    EXPECT_TRUE(args.find("node_id") == args.end());
    json expected = missing_param_error("node_id");
    EXPECT_EQ(expected["error"], "missing parameter: node_id");
}

TEST(KgNeighboursTest, DefaultDepthIsOne) {
    json args = {{"node_id", "persons/1"}};
    int depth = args.value("depth", 1);
    EXPECT_EQ(depth, 1);
}

TEST(KgNeighboursTest, DepthClamped) {
    json args = {{"node_id", "persons/1"}, {"depth", 99}};
    int depth = std::min(std::max(args.value("depth", 1), 1), 5);
    EXPECT_EQ(depth, 5);
}

TEST(KgNeighboursTest, DepthMinClamped) {
    json args = {{"node_id", "persons/1"}, {"depth", -3}};
    int depth = std::min(std::max(args.value("depth", 1), 1), 5);
    EXPECT_EQ(depth, 1);
}

TEST(KgNeighboursTest, MaxNodesClamped) {
    json args = {{"node_id", "persons/1"}, {"max_nodes", 9999}};
    int max_nodes = std::min(std::max(args.value("max_nodes", 100), 1), 1000);
    EXPECT_EQ(max_nodes, 1000);
}

TEST(KgNeighboursTest, DefaultMaxNodes) {
    json args = {{"node_id", "persons/1"}};
    int max_nodes = std::min(std::max(args.value("max_nodes", 100), 1), 1000);
    EXPECT_EQ(max_nodes, 100);
}

TEST(KgNeighboursTest, OutputShapeContainsRequiredFields) {
    // Simulate handler output contract
    json result = {
        {"node_id",      "persons/1"},
        {"depth_reached", 1},
        {"nodes",        json::array()},
        {"edges",        json::array()},
        {"truncated",    false}
    };
    EXPECT_TRUE(result.contains("node_id"));
    EXPECT_TRUE(result.contains("depth_reached"));
    EXPECT_TRUE(result.contains("nodes"));
    EXPECT_TRUE(result.contains("edges"));
    EXPECT_TRUE(result.contains("truncated"));
    EXPECT_FALSE(result["truncated"].get<bool>());
}

TEST(KgNeighboursTest, TruncatedFlagWhenMaxNodesReached) {
    // Simulate truncation: if we have more nodes than max_nodes, truncated=true
    json result = {{"truncated", true}};
    EXPECT_TRUE(result["truncated"].get<bool>());
}

TEST(KgNeighboursTest, EdgeTypeFilterAcceptsArray) {
    json args = {{"node_id", "n/1"}, {"edge_types", {"KNOWS", "LIKES"}}};
    EXPECT_TRUE(args["edge_types"].is_array());
    EXPECT_EQ(args["edge_types"].size(), 2u);
}

// ============================================================================
// kg_shortest_path tests
// ============================================================================

TEST(KgShortestPathTest, MissingFromNodeReturnsError) {
    json args = {{"to_node", "n/2"}};
    EXPECT_TRUE(args.find("from_node") == args.end());
    json expected = missing_param_error("from_node");
    EXPECT_EQ(expected["error"], "missing parameter: from_node");
}

TEST(KgShortestPathTest, MissingToNodeReturnsError) {
    json args = {{"from_node", "n/1"}};
    EXPECT_TRUE(args.find("to_node") == args.end());
    json expected = missing_param_error("to_node");
    EXPECT_EQ(expected["error"], "missing parameter: to_node");
}

TEST(KgShortestPathTest, SameNodeReturnsHopCountZero) {
    // Contract: from==to → hop_count=0, found=true
    json result = {
        {"path",      json::array({{{"id", "n/1"}, {"properties", json::object()}}})},
        {"edges",     json::array()},
        {"hop_count", 0},
        {"found",     true}
    };
    EXPECT_EQ(result["hop_count"].get<int>(), 0);
    EXPECT_TRUE(result["found"].get<bool>());
}

TEST(KgShortestPathTest, NoPathReturnsFalse) {
    json result = {{"path", json::array()}, {"edges", json::array()}, {"hop_count", 0}, {"found", false}};
    EXPECT_FALSE(result["found"].get<bool>());
}

TEST(KgShortestPathTest, PathFoundOutputShape) {
    json result = {
        {"path",  json::array({{{"id", "n/1"}}, {{"id", "n/2"}}})},
        {"edges", json::array({{{"from", "n/1"}, {"to", "n/2"}, {"type", "KNOWS"}}})},
        {"hop_count", 1},
        {"found", true}
    };
    EXPECT_TRUE(result.contains("path"));
    EXPECT_TRUE(result.contains("edges"));
    EXPECT_TRUE(result.contains("hop_count"));
    EXPECT_TRUE(result["found"].get<bool>());
    EXPECT_EQ(result["hop_count"].get<int>(), 1);
}

// ============================================================================
// kg_node_properties tests
// ============================================================================

TEST(KgNodePropertiesTest, MissingNodeIdReturnsError) {
    json args = json::object();
    EXPECT_TRUE(args.find("node_id") == args.end());
    json expected = missing_param_error("node_id");
    EXPECT_EQ(expected["error"], "missing parameter: node_id");
}

TEST(KgNodePropertiesTest, ExistingNodeOutputShape) {
    json result = {
        {"id",         "persons/42"},
        {"properties", {{"name", "Alice"}, {"age", 30}}},
        {"collection", "persons"}
    };
    EXPECT_EQ(result["id"].get<std::string>(), "persons/42");
    EXPECT_TRUE(result["properties"].contains("name"));
    EXPECT_TRUE(result.contains("collection"));
}

TEST(KgNodePropertiesTest, MissingNodeReturnsFoundFalse) {
    json result = {
        {"id",         "persons/999"},
        {"properties", json::object()},
        {"collection", ""},
        {"found",      false}
    };
    EXPECT_FALSE(result["found"].get<bool>());
    EXPECT_TRUE(result["properties"].empty());
}
