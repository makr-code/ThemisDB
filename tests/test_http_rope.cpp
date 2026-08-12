#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <thread>
#include <chrono>
#include <filesystem>
#include <algorithm>
#include <string>
#include <cctype>

#include "server/http_server.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "index/rotary_embeddings.h"
#include "transaction/transaction_manager.h"
#include "storage/base_entity.h"

using json = nlohmann::json;
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class HttpRopeApiTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        std::string suffix = info ? std::string(info->test_suite_name()) + "_" + info->name() : "default";
        std::replace_if(suffix.begin(), suffix.end(), [](unsigned char c) { return !std::isalnum(c); }, '_');
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();

        db_path_ = std::filesystem::temp_directory_path() / ("themis_http_rope_test_" + suffix + "_" + std::to_string(now));
        std::filesystem::create_directories(db_path_);

        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_.string();
        cfg.memtable_size_mb = 64;
        cfg.block_cache_size_mb = 128;
        storage_ = std::make_shared<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());

        secondary_index_ = std::make_shared<themis::SecondaryIndexManager>(*storage_);
        graph_index_ = std::make_shared<themis::GraphIndexManager>(*storage_);
        vector_index_ = std::make_shared<themis::VectorIndexManager>(*storage_);
        tx_manager_ = std::make_shared<themis::TransactionManager>(*storage_, *secondary_index_, *graph_index_, *vector_index_);

        // Initialize vector index with appropriate dimension for RoPE
        auto st = vector_index_->init("test_rope", 768, themis::VectorIndexManager::Metric::COSINE, 16, 200, 64);
        ASSERT_TRUE(st.ok) << st.message;

        // Start HTTP server
        port_ = static_cast<uint16_t>(22000 + (now % 10000));
        themis::server::HttpServer::Config scfg;
        scfg.host = "127.0.0.1";
        scfg.port = port_;
        scfg.num_threads = 2;

        server_ = std::make_unique<themis::server::HttpServer>(scfg, storage_, secondary_index_, graph_index_, vector_index_, tx_manager_);
        server_->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override {
        if (server_) {
            server_->stop();
        }
        if (storage_) {
            storage_->close();
        }
        if (!db_path_.empty()) {
            std::filesystem::remove_all(db_path_);
        }
    }

    json httpPost(const std::string& target, const json& body) {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        auto const results = resolver.resolve("127.0.0.1", std::to_string(port_));
        stream.connect(results);

        http::request<http::string_body> req{http::verb::post, target, 11};
        req.set(http::field::host, "127.0.0.1");
        req.set(http::field::content_type, "application/json");
        req.body() = body.dump();
        req.prepare_payload();

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        return json::parse(res.body());
    }

    json httpGet(const std::string& target) {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        auto const results = resolver.resolve("127.0.0.1", std::to_string(port_));
        stream.connect(results);

        http::request<http::string_body> req{http::verb::get, target, 11};
        req.set(http::field::host, "127.0.0.1");

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        return json::parse(res.body());
    }

    json httpDelete(const std::string& target) {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        auto const results = resolver.resolve("127.0.0.1", std::to_string(port_));
        stream.connect(results);

        http::request<http::string_body> req{http::verb::delete_, target, 11};
        req.set(http::field::host, "127.0.0.1");

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        return json::parse(res.body());
    }

    std::filesystem::path db_path_;
    uint16_t port_;
    std::shared_ptr<themis::RocksDBWrapper> storage_;
    std::shared_ptr<themis::SecondaryIndexManager> secondary_index_;
    std::shared_ptr<themis::GraphIndexManager> graph_index_;
    std::shared_ptr<themis::VectorIndexManager> vector_index_;
    std::shared_ptr<themis::TransactionManager> tx_manager_;
    std::unique_ptr<themis::server::HttpServer> server_;
};

// Test 1: Configure RoPE
TEST_F(HttpRopeApiTest, ConfigureRoPE) {
    json config_request = {
        {"hidden_dim", 768},
        {"num_rotation_pairs", 384},
        {"base_theta", 10000.0},
        {"normalize_after", false}
    };

    auto response = httpPost("/api/v1/vector-index/test_rope/rope/config", config_request);
    
    ASSERT_TRUE(response.contains("status"));
    EXPECT_EQ(response["status"], "success");
    ASSERT_TRUE(response.contains("config"));
    EXPECT_EQ(response["config"]["hidden_dim"], 768);
    EXPECT_EQ(response["config"]["num_rotation_pairs"], 384);
}

// Test 2: Get RoPE Configuration
TEST_F(HttpRopeApiTest, GetRoPEConfig) {
    // First configure RoPE
    json config_request = {
        {"hidden_dim", 768},
        {"num_rotation_pairs", 384},
        {"base_theta", 10000.0}
    };
    httpPost("/api/v1/vector-index/test_rope/rope/config", config_request);

    // Now get the config
    auto response = httpGet("/api/v1/vector-index/test_rope/rope/config");
    
    ASSERT_TRUE(response.contains("enabled"));
    EXPECT_TRUE(response["enabled"]);
    ASSERT_TRUE(response.contains("config"));
    EXPECT_EQ(response["config"]["hidden_dim"], 768);
}

// Test 3: Add Entity with Rotation
TEST_F(HttpRopeApiTest, AddEntityWithRotation) {
    // First configure RoPE
    json config_request = {
        {"hidden_dim", 768},
        {"num_rotation_pairs", 384}
    };
    httpPost("/api/v1/vector-index/test_rope/rope/config", config_request);

    // Create a sample 768-dimensional embedding
    std::vector<float> embedding(768, 0.1f);
    json embedding_json(embedding);

    json add_request = {
        {"entity", {
            {"id", "doc1"},
            {"embedding", embedding_json},
            {"content", "Test document"}
        }},
        {"vector_field", "embedding"},
        {"position", 42}
    };

    auto response = httpPost("/api/v1/vector-index/test_rope/rope/add", add_request);
    
    ASSERT_TRUE(response.contains("status"));
    EXPECT_EQ(response["status"], "success");
    ASSERT_TRUE(response.contains("entity_id"));
    EXPECT_EQ(response["entity_id"], "doc1");
}

// Test 4: Add Entity with Relational Rotation
TEST_F(HttpRopeApiTest, AddEntityWithRelationalRotation) {
    // First configure RoPE
    json config_request = {
        {"hidden_dim", 768},
        {"num_rotation_pairs", 384}
    };
    httpPost("/api/v1/vector-index/test_rope/rope/config", config_request);

    std::vector<float> embedding(768, 0.2f);
    json embedding_json(embedding);

    json add_request = {
        {"entity", {
            {"id", "entity_a"},
            {"embedding", embedding_json},
            {"name", "Entity A"}
        }},
        {"vector_field", "embedding"},
        {"relation_type", "parent_of"}
    };

    auto response = httpPost("/api/v1/vector-index/test_rope/rope/add-relational", add_request);
    
    ASSERT_TRUE(response.contains("status"));
    EXPECT_EQ(response["status"], "success");
    ASSERT_TRUE(response.contains("relation_type"));
    EXPECT_EQ(response["relation_type"], "parent_of");
}

// Test 5: Search with Rotation
TEST_F(HttpRopeApiTest, SearchWithRotation) {
    // First configure RoPE
    json config_request = {
        {"hidden_dim", 768},
        {"num_rotation_pairs", 384}
    };
    httpPost("/api/v1/vector-index/test_rope/rope/config", config_request);

    // Add a few entities with rotation
    for (int i = 0; i < 3; ++i) {
        std::vector<float> embedding(768, 0.1f * (i + 1));
        json embedding_json(embedding);
        
        json add_request = {
            {"entity", {
                {"id", "doc" + std::to_string(i)},
                {"embedding", embedding_json}
            }},
            {"vector_field", "embedding"},
            {"position", i * 10}
        };
        httpPost("/api/v1/vector-index/test_rope/rope/add", add_request);
    }

    // Search with rotation
    std::vector<float> query_embedding(768, 0.15f);
    json query_json(query_embedding);

    json search_request = {
        {"query", query_json},
        {"k", 2},
        {"position", 5}
    };

    auto response = httpPost("/api/v1/vector-index/test_rope/rope/search", search_request);
    
    ASSERT_TRUE(response.contains("status"));
    EXPECT_EQ(response["status"], "success");
    ASSERT_TRUE(response.contains("results"));
    ASSERT_TRUE(response.contains("rotation_enabled"));
    EXPECT_TRUE(response["rotation_enabled"]);
}

// Test 6: Batch Add with Rotation
TEST_F(HttpRopeApiTest, BatchAddWithRotation) {
    // First configure RoPE
    json config_request = {
        {"hidden_dim", 768},
        {"num_rotation_pairs", 384}
    };
    httpPost("/api/v1/vector-index/test_rope/rope/config", config_request);

    // Create batch request
    json batch_request = {
        {"vector_field", "embedding"},
        {"entities", json::array()}
    };

    for (int i = 0; i < 5; ++i) {
        std::vector<float> embedding(768, 0.1f * (i + 1));
        json embedding_json(embedding);
        
        batch_request["entities"].push_back({
            {"entity", {
                {"id", "batch_doc" + std::to_string(i)},
                {"embedding", embedding_json}
            }},
            {"position", i}
        });
    }

    auto response = httpPost("/api/v1/vector-index/test_rope/rope/batch-add", batch_request);
    
    ASSERT_TRUE(response.contains("status"));
    EXPECT_EQ(response["status"], "success");
    ASSERT_TRUE(response.contains("inserted"));
    EXPECT_EQ(response["inserted"], 5);
    ASSERT_TRUE(response.contains("errors"));
    EXPECT_EQ(response["errors"], 0);
}

// Test 7: Get RoPE Statistics
TEST_F(HttpRopeApiTest, GetRoPEStats) {
    // First configure RoPE
    json config_request = {
        {"hidden_dim", 768},
        {"num_rotation_pairs", 384}
    };
    httpPost("/api/v1/vector-index/test_rope/rope/config", config_request);

    // Add one rotated entity to produce runtime stats.
    std::vector<float> embedding(768, 0.1f);
    json add_request = {
        {"entity", {
            {"id", "stats_doc"},
            {"embedding", embedding}
        }},
        {"vector_field", "embedding"},
        {"position", 7}
    };
    auto add_response = httpPost("/api/v1/vector-index/test_rope/rope/add", add_request);
    ASSERT_TRUE(add_response.contains("status"));
    EXPECT_EQ(add_response["status"], "success");

    auto response = httpGet("/api/v1/vector-index/test_rope/rope/stats");
    
    ASSERT_TRUE(response.contains("enabled"));
    EXPECT_TRUE(response["enabled"]);
    ASSERT_TRUE(response.contains("config"));
    ASSERT_TRUE(response.contains("statistics"));
    ASSERT_TRUE(response["statistics"].contains("status"));
    EXPECT_EQ(response["statistics"]["status"], "ok");
    ASSERT_TRUE(response["statistics"].contains("vector_count"));
    ASSERT_TRUE(response["statistics"].contains("distance_metric"));
}

// Test 8: Disable RoPE
TEST_F(HttpRopeApiTest, DisableRoPE) {
    // First configure RoPE
    json config_request = {
        {"hidden_dim", 768},
        {"num_rotation_pairs", 384}
    };
    httpPost("/api/v1/vector-index/test_rope/rope/config", config_request);

    // Now disable it
    auto response = httpDelete("/api/v1/vector-index/test_rope/rope/config");
    
    ASSERT_TRUE(response.contains("status"));
    EXPECT_EQ(response["status"], "success");
}

// Test 9: Invalid Configuration (hidden_dim not even)
TEST_F(HttpRopeApiTest, InvalidConfigOddDimension) {
    json config_request = {
        {"hidden_dim", 127},  // Invalid: not even
        {"num_rotation_pairs", 63}
    };

    auto response = httpPost("/api/v1/vector-index/test_rope/rope/config", config_request);
    
    ASSERT_TRUE(response.contains("error"));
    EXPECT_TRUE(response["error"]);
}

// Test 10: Try to use RoPE before configuration
TEST_F(HttpRopeApiTest, UseRoPEBeforeConfiguration) {
    std::vector<float> embedding(768, 0.1f);
    json embedding_json(embedding);

    json add_request = {
        {"entity", {
            {"id", "doc1"},
            {"embedding", embedding_json}
        }},
        {"vector_field", "embedding"},
        {"position", 0}
    };

    auto response = httpPost("/api/v1/vector-index/test_rope/rope/add", add_request);
    
    ASSERT_TRUE(response.contains("error"));
    EXPECT_TRUE(response["error"]);
    ASSERT_TRUE(response.contains("message"));
}
