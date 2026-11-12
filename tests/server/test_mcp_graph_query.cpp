#include <gtest/gtest.h>

#include "index/graph_index.h"
#include "server/mcp_server.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"

#include <boost/asio.hpp>
#include <chrono>
#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <system_error>

namespace {

using json = nlohmann::json;

struct TestResources {
    std::filesystem::path database_path;
    std::shared_ptr<themis::RocksDBWrapper> database;
    std::shared_ptr<themis::server::McpServer> server;

    ~TestResources() {
        if (server) {
            server->stop();
        }
        if (database) {
            database->close();
        }
        std::error_code error;
        std::filesystem::remove_all(database_path, error);
    }
};

TEST(McpGraphQueryTest, AttachedQueryEngineRetainsGraphBackend) {
    TestResources resources;
    resources.database_path = std::filesystem::temp_directory_path() /
        ("themis_mcp_graph_query_test_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));

    resources.database = std::make_shared<themis::RocksDBWrapper>(
        resources.database_path.string());
    ASSERT_TRUE(resources.database->open());

    boost::asio::io_context io_context;
    themis::server::McpServer::Config config;
    config.enable_stdio = false;
    config.enable_sse = false;
    config.enable_websocket = false;
    resources.server = std::make_shared<themis::server::McpServer>(io_context, config);
    resources.server->attachDatabase(resources.database);
    resources.server->start();

    {
        themis::GraphIndexManager graph_writer(*resources.database);
        themis::BaseEntity edge("edge1");
        edge.setField("id", std::string{"edge1"});
        edge.setField("_from", std::string{"user1"});
        edge.setField("_to", std::string{"user2"});
        ASSERT_TRUE(graph_writer.addEdge(edge).ok);
    }

    resources.server->attachDatabase(resources.database);

    const auto response = resources.server->handleRequest({
        {"jsonrpc", "2.0"},
        {"method", "tools/call"},
        {"params", {
            {"name", "query"},
            {"arguments", {
                {"query", "FOR v IN 1..1 OUTBOUND 'user1' GRAPH 'social' RETURN v"},
                {"language", "aql"}
            }}
        }},
        {"id", 1}
    });

    ASSERT_TRUE(response.contains("result"));
    ASSERT_TRUE(response["result"].contains("content"));
    const auto result_text = response["result"]["content"][0]["text"].get<std::string>();
    const auto parsed = json::parse(result_text);
    ASSERT_EQ(parsed["status"], "success") << parsed.dump();
    ASSERT_EQ(parsed["results"].size(), 1);
}

} // namespace
