/**
 * @file test_mcp_kg_tools.cpp
 * @brief Unit tests for MCP Group 1 Knowledge Graph tools:
 *        kg_neighbours, kg_shortest_path, kg_node_properties
 *
 * Labels: wave_b release_critical
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#ifdef THEMIS_ENABLE_MCP
#include "server/mcp_server.h"
#include <boost/asio/io_context.hpp>
#include <string>
#endif

using json = nlohmann::json;

#ifndef THEMIS_ENABLE_MCP

TEST(McpKgTools, RequiresMcpBuildFlag) {
    GTEST_SKIP() << "THEMIS_ENABLE_MCP is not enabled in this build.";
}

#else

namespace {

json callTool(themis::server::McpServer& server,
              const std::string& tool_name,
              const json& args = json::object()) {
    const json request = {
        {"jsonrpc", "2.0"},
        {"method", "tools/call"},
        {"params", {{"name", tool_name}, {"arguments", args}}}
    };

    const json response = server.handleRequest(request);
    EXPECT_TRUE(response.contains("result"));
    EXPECT_TRUE(response["result"].contains("content"));
    EXPECT_FALSE(response["result"]["content"].empty());

    const std::string payload = response["result"]["content"][0]["text"].get<std::string>();
    return json::parse(payload);
}

} // namespace

// ============================================================================
// kg_neighbours tests
// ============================================================================

TEST(KgNeighboursTest, MissingNodeIdReturnsError) {
    boost::asio::io_context io;
    themis::server::McpServer server(io);

    const json result = callTool(server, "kg_neighbours", json::object());
    ASSERT_TRUE(result.contains("error"));
    EXPECT_EQ(result["error"], "missing parameter: node_id");
}

TEST(KgNeighboursTest, DefaultDepthAndMaxNodesAreApplied) {
    boost::asio::io_context io;
    themis::server::McpServer server(io);

    const json result = callTool(server, "kg_neighbours", {{"node_id", "persons/1"}});
    EXPECT_EQ(result["depth_reached"], 1);
    EXPECT_FALSE(result["truncated"].get<bool>());
}

TEST(KgNeighboursTest, DepthClampedToMaximum) {
    boost::asio::io_context io;
    themis::server::McpServer server(io);

    const json result = callTool(server, "kg_neighbours", {{"node_id", "persons/1"}, {"depth", 99}});
    EXPECT_EQ(result["depth_reached"], 5);
}

TEST(KgNeighboursTest, OutputShapeContainsRequiredFields) {
    boost::asio::io_context io;
    themis::server::McpServer server(io);

    const json result = callTool(server, "kg_neighbours", {{"node_id", "persons/1"}, {"max_nodes", 1}});
    EXPECT_TRUE(result.contains("node_id"));
    EXPECT_TRUE(result.contains("depth_reached"));
    EXPECT_TRUE(result.contains("nodes"));
    EXPECT_TRUE(result.contains("edges"));
    EXPECT_TRUE(result.contains("truncated"));
}

// ============================================================================
// kg_shortest_path tests
// ============================================================================

TEST(KgShortestPathTest, MissingFromNodeReturnsError) {
    boost::asio::io_context io;
    themis::server::McpServer server(io);

    const json result = callTool(server, "kg_shortest_path", {{"to_node", "n/2"}});
    ASSERT_TRUE(result.contains("error"));
    EXPECT_EQ(result["error"], "missing parameter: from_node");
}

TEST(KgShortestPathTest, MissingToNodeReturnsError) {
    boost::asio::io_context io;
    themis::server::McpServer server(io);

    const json result = callTool(server, "kg_shortest_path", {{"from_node", "n/1"}});
    ASSERT_TRUE(result.contains("error"));
    EXPECT_EQ(result["error"], "missing parameter: to_node");
}

TEST(KgShortestPathTest, SameNodeReturnsHopCountZero) {
    boost::asio::io_context io;
    themis::server::McpServer server(io);

    const json result = callTool(server, "kg_shortest_path", {{"from_node", "n/1"}, {"to_node", "n/1"}});
    EXPECT_EQ(result["hop_count"], 0);
    EXPECT_TRUE(result["found"].get<bool>());
}

TEST(KgShortestPathTest, OutputShapeContainsRequiredFields) {
    boost::asio::io_context io;
    themis::server::McpServer server(io);

    const json result = callTool(server, "kg_shortest_path", {{"from_node", "n/1"}, {"to_node", "n/2"}});
    EXPECT_TRUE(result.contains("path"));
    EXPECT_TRUE(result.contains("edges"));
    EXPECT_TRUE(result.contains("hop_count"));
    EXPECT_TRUE(result.contains("found"));
}

// ============================================================================
// kg_node_properties tests
// ============================================================================

TEST(KgNodePropertiesTest, MissingNodeIdReturnsError) {
    boost::asio::io_context io;
    themis::server::McpServer server(io);

    const json result = callTool(server, "kg_node_properties", json::object());
    ASSERT_TRUE(result.contains("error"));
    EXPECT_EQ(result["error"], "missing parameter: node_id");
}

TEST(KgNodePropertiesTest, MissingNodeReturnsFoundFalse) {
    boost::asio::io_context io;
    themis::server::McpServer server(io);

    const json result = callTool(server, "kg_node_properties", {{"node_id", "persons/999"}});
    EXPECT_EQ(result["id"], "persons/999");
    EXPECT_TRUE(result.contains("properties"));
    EXPECT_TRUE(result.contains("collection"));
    EXPECT_TRUE(result.contains("found"));
    EXPECT_FALSE(result["found"].get<bool>());
}

#endif
