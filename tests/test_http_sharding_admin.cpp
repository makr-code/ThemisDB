/*
 * Integration tests for HttpServer /v1/admin/shards endpoints.
 *
 * Verifies that:
 *  1. POST /v1/admin/shards creates a shard node and returns 201 Created.
 *  2. GET  /v1/admin/shards lists all created shard nodes.
 *  3. Three consecutive POSTs result in all three nodes visible via GET.
 *  4. POST with missing node_address returns 400 Bad Request.
 */

#include <gtest/gtest.h>
#include "server/http_server.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "transaction/transaction_manager.h"
#include "sharding/sharding_manager.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <thread>
#include <chrono>
#include <algorithm>
#include <string>
#include <cctype>

using namespace themis;
using namespace themis::server;
using json = nlohmann::json;

namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;
using tcp = net::ip::tcp;

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class HttpShardingAdminTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        std::string suffix = info
            ? std::string(info->test_suite_name()) + "_" + info->name()
            : "default";
        std::replace_if(suffix.begin(), suffix.end(),
                        [](unsigned char c) { return !std::isalnum(c); }, '_');
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();

        test_db_path_ = std::filesystem::temp_directory_path() /
                        ("themis_http_sharding_test_" + suffix + "_" + std::to_string(now));
        std::filesystem::create_directories(test_db_path_);

        RocksDBWrapper::Config db_config;
        db_config.db_path = test_db_path_.string();

        storage_ = std::make_shared<RocksDBWrapper>(db_config);
        ASSERT_TRUE(storage_->open()) << "Failed to open RocksDB";

        secondary_index_ = std::make_shared<SecondaryIndexManager>(*storage_);
        graph_index_     = std::make_shared<GraphIndexManager>(*storage_);
        vector_index_    = std::make_shared<VectorIndexManager>(*storage_);
        tx_manager_      = std::make_shared<TransactionManager>(
            *storage_, *secondary_index_, *graph_index_, *vector_index_);

        HttpServer::Config server_config;
        server_config.host        = "127.0.0.1";
        // Use a port in the ephemeral range derived from timestamp to avoid conflicts
        port_ = static_cast<uint16_t>(24000 + (now % 10000));
        server_config.port        = port_;
        server_config.num_threads = 2;

        server_ = std::make_unique<HttpServer>(
            server_config, storage_, secondary_index_,
            graph_index_, vector_index_, tx_manager_);

        // Inject the live ShardingManager
        server_->setShardingManager(&sharding::ShardingManager::GetInstance());

        server_->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }

    void TearDown() override {
        if (server_) server_->stop();
        server_.reset();
        tx_manager_.reset();
        vector_index_.reset();
        graph_index_.reset();
        secondary_index_.reset();
        storage_.reset();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::filesystem::remove_all(test_db_path_);
    }

    // ---- HTTP helpers -------------------------------------------------------

    struct Response {
        http::status status;
        json         body;
    };

    Response httpGet(const std::string& path) {
        net::io_context ioc;
        tcp::resolver   resolver(ioc);
        beast::tcp_stream stream(ioc);

        auto results = resolver.resolve("127.0.0.1", std::to_string(port_));
        stream.connect(results);

        http::request<http::string_body> req{http::verb::get, path, 11};
        req.set(http::field::host, "127.0.0.1");
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        http::write(stream, req);

        beast::flat_buffer               buf;
        http::response<http::string_body> res;
        http::read(stream, buf, res);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        return {res.result(), json::parse(res.body())};
    }

    Response httpPost(const std::string& path, const json& body) {
        net::io_context   ioc;
        tcp::resolver     resolver(ioc);
        beast::tcp_stream stream(ioc);

        auto results = resolver.resolve("127.0.0.1", std::to_string(port_));
        stream.connect(results);

        http::request<http::string_body> req{http::verb::post, path, 11};
        req.set(http::field::host, "127.0.0.1");
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/json");
        req.body() = body.dump();
        req.prepare_payload();

        http::write(stream, req);

        beast::flat_buffer               buf;
        http::response<http::string_body> res;
        http::read(stream, buf, res);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        return {res.result(), json::parse(res.body())};
    }

    // ---- Members ------------------------------------------------------------

    std::filesystem::path               test_db_path_;
    std::shared_ptr<RocksDBWrapper>     storage_;
    std::shared_ptr<SecondaryIndexManager> secondary_index_;
    std::shared_ptr<GraphIndexManager>  graph_index_;
    std::shared_ptr<VectorIndexManager> vector_index_;
    std::shared_ptr<TransactionManager> tx_manager_;
    std::unique_ptr<HttpServer>         server_;
    uint16_t                            port_{0};
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

// POST a single shard and verify it is returned by GET
TEST_F(HttpShardingAdminTest, CreateShard_Returns201) {
    json body = {
        {"node_id",      900001},
        {"node_address", "10.0.0.1:7001"},
        {"node_role",    "PRIMARY"},
        {"is_healthy",   true}
    };

    auto [status, resp_body] = httpPost("/v1/admin/shards", body);

    EXPECT_EQ(status, http::status::created);
    EXPECT_EQ(resp_body["node_id"],      900001);
    EXPECT_EQ(resp_body["node_address"], "10.0.0.1:7001");
    EXPECT_EQ(resp_body["node_role"],    "PRIMARY");
    EXPECT_EQ(resp_body["is_healthy"],   true);
}

// POST with missing node_address must return 400
TEST_F(HttpShardingAdminTest, CreateShard_MissingAddress_Returns400) {
    json body = {{"node_id", 900099}};

    auto [status, resp_body] = httpPost("/v1/admin/shards", body);

    EXPECT_EQ(status, http::status::bad_request);
    EXPECT_TRUE(resp_body.contains("error"));
}

// GET /v1/admin/shards returns the shards array and metadata
TEST_F(HttpShardingAdminTest, ListShards_ReturnsExpectedFields) {
    auto [status, resp_body] = httpGet("/v1/admin/shards");

    EXPECT_EQ(status, http::status::ok);
    EXPECT_TRUE(resp_body.contains("shards"));
    EXPECT_TRUE(resp_body.contains("total"));
    EXPECT_TRUE(resp_body.contains("max_nodes"));
    EXPECT_TRUE(resp_body.contains("remaining"));
    EXPECT_TRUE(resp_body.contains("healthy_count"));
    EXPECT_TRUE(resp_body["shards"].is_array());
}

// Create 3 shards via POST, then verify all 3 appear in GET
TEST_F(HttpShardingAdminTest, Create3Shards_AllAppearInList) {
    // Use high node_ids unlikely to collide with other tests
    const std::vector<uint32_t> node_ids = {980001, 980002, 980003};
    const std::vector<std::string> addresses = {
        "10.1.0.1:7000", "10.1.0.2:7000", "10.1.0.3:7000"
    };

    for (size_t i = 0; i < node_ids.size(); ++i) {
        json body = {
            {"node_id",      node_ids[i]},
            {"node_address", addresses[i]},
            {"node_role",    "PRIMARY"},
            {"is_healthy",   true}
        };
        auto [post_status, _] = httpPost("/v1/admin/shards", body);
        ASSERT_EQ(post_status, http::status::created)
            << "POST shard " << i << " failed";
    }

    auto [get_status, list] = httpGet("/v1/admin/shards");
    ASSERT_EQ(get_status, http::status::ok);

    const auto& shards = list["shards"];
    ASSERT_TRUE(shards.is_array());

    // Verify each created node appears in the list
    for (size_t i = 0; i < node_ids.size(); ++i) {
        bool found = false;
        for (const auto& shard : shards) {
            if (shard["node_id"].get<uint32_t>() == node_ids[i] &&
                shard["node_address"].get<std::string>() == addresses[i]) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found)
            << "Node " << node_ids[i] << " (" << addresses[i] << ") not in list";
    }

    // total must reflect the node count
    EXPECT_GE(list["total"].get<size_t>(), node_ids.size());
}
