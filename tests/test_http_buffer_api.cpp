/**
 * @file test_http_buffer_api.cpp
 * @brief Integration tests for Buffer API HTTP endpoints
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <thread>
#include <chrono>
#include <filesystem>

#include "server/http_server.h"
#include "server/buffer_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "transaction/transaction_manager.h"
#include "timeseries/tsstore.h"

using json = nlohmann::json;
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class HttpBufferAPITest : public ::testing::Test {
protected:
    void SetUp() override {
        const std::string db_path = "data/themis_http_buffer_test";
        if (std::filesystem::exists(db_path)) {
            std::filesystem::remove_all(db_path);
        }
        
        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = db_path;
        cfg.memtable_size_mb = 64;
        cfg.block_cache_size_mb = 128;
        
        storage_ = std::make_shared<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());
        
        secondary_index_ = std::make_shared<themis::SecondaryIndexManager>(*storage_);
        graph_index_ = std::make_shared<themis::GraphIndexManager>(*storage_);
        vector_index_ = std::make_shared<themis::VectorIndexManager>(*storage_);
        tx_manager_ = std::make_shared<themis::TransactionManager>(
            *storage_, *secondary_index_, *graph_index_, *vector_index_
        );
        
        // Initialize TSStore
        tsstore_ = std::make_shared<themis::TSStore>(*storage_);
        
        // Initialize Buffer API Handler
        buffer_handler_ = std::make_unique<themis::server::BufferAPIHandler>(
            tsstore_, vector_index_, nullptr  // No graph manager for this test
        );
        buffer_handler_->start();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    void TearDown() override {
        if (buffer_handler_) {
            buffer_handler_->stop();
        }
        storage_->close();
        
        const std::string db_path = "data/themis_http_buffer_test";
        if (std::filesystem::exists(db_path)) {
            std::filesystem::remove_all(db_path);
        }
    }
    
    std::shared_ptr<themis::RocksDBWrapper> storage_;
    std::shared_ptr<themis::SecondaryIndexManager> secondary_index_;
    std::shared_ptr<themis::GraphIndexManager> graph_index_;
    std::shared_ptr<themis::VectorIndexManager> vector_index_;
    std::shared_ptr<themis::TransactionManager> tx_manager_;
    std::shared_ptr<themis::TSStore> tsstore_;
    std::unique_ptr<themis::server::BufferAPIHandler> buffer_handler_;
};

TEST_F(HttpBufferAPITest, TSPutBuffered_SinglePoint) {
    // Create HTTP request
    http::request<http::string_body> req{http::verb::post, "/ts/put/buffered", 11};
    req.set(http::field::host, "localhost");
    req.set(http::field::content_type, "application/json");
    
    json body = {
        {"metric", "cpu.usage"},
        {"entity", "server01"},
        {"timestamp", 1700000000},
        {"value", 75.5}
    };
    req.body() = body.dump();
    req.prepare_payload();
    
    // Handle request
    auto res = buffer_handler_->handleTSPutBuffered(req);
    
    // Verify response
    EXPECT_EQ(res.result(), http::status::ok);
    
    auto response_body = json::parse(res.body());
    EXPECT_EQ(response_body["status"], "buffered");
    EXPECT_EQ(response_body["metric"], "cpu.usage");
    EXPECT_EQ(response_body["entity"], "server01");
    EXPECT_TRUE(response_body.contains("buffer_stats"));
    EXPECT_GT(response_body["buffer_stats"]["points_buffered"], 0);
}

TEST_F(HttpBufferAPITest, TSPutBuffered_MissingFields) {
    http::request<http::string_body> req{http::verb::post, "/ts/put/buffered", 11};
    req.set(http::field::host, "localhost");
    req.set(http::field::content_type, "application/json");
    
    // Missing 'value' field
    json body = {
        {"metric", "cpu.usage"},
        {"entity", "server01"},
        {"timestamp", 1700000000}
    };
    req.body() = body.dump();
    req.prepare_payload();
    
    auto res = buffer_handler_->handleTSPutBuffered(req);
    
    EXPECT_EQ(res.result(), http::status::bad_request);
    
    auto response_body = json::parse(res.body());
    EXPECT_TRUE(response_body.contains("error"));
}

TEST_F(HttpBufferAPITest, TSPutBuffered_BatchInsert) {
    http::request<http::string_body> req{http::verb::post, "/ts/put/buffered", 11};
    req.set(http::field::host, "localhost");
    req.set(http::field::content_type, "application/json");
    
    // Insert multiple points
    for (int i = 0; i < 10; ++i) {
        json body = {
            {"metric", "cpu.usage"},
            {"entity", "server01"},
            {"timestamp", 1700000000 + i},
            {"value", 70.0 + i}
        };
        req.body() = body.dump();
        req.prepare_payload();
        
        auto res = buffer_handler_->handleTSPutBuffered(req);
        EXPECT_EQ(res.result(), http::status::ok);
    }
    
    // Check stats
    http::request<http::string_body> stats_req{http::verb::get, "/buffer/stats", 11};
    auto stats_res = buffer_handler_->handleBufferStats(stats_req);
    
    EXPECT_EQ(stats_res.result(), http::status::ok);
    
    auto stats_body = json::parse(stats_res.body());
    EXPECT_TRUE(stats_body["buffers"]["ts_buffer"]["enabled"]);
    EXPECT_GE(stats_body["buffers"]["ts_buffer"]["points_buffered"], 10);
}

TEST_F(HttpBufferAPITest, VectorAddBuffered_SingleVector) {
    http::request<http::string_body> req{http::verb::post, "/vectors/add/buffered", 11};
    req.set(http::field::host, "localhost");
    req.set(http::field::content_type, "application/json");
    
    json body = {
        {"pk", "doc123"},
        {"embedding", {0.1, 0.2, 0.3, 0.4}},
        {"metadata", {
            {"title", "Test Document"},
            {"content", "Sample content"}
        }}
    };
    req.body() = body.dump();
    req.prepare_payload();
    
    auto res = buffer_handler_->handleVectorAddBuffered(req);
    
    EXPECT_EQ(res.result(), http::status::ok);
    
    auto response_body = json::parse(res.body());
    EXPECT_EQ(response_body["status"], "buffered");
    EXPECT_EQ(response_body["pk"], "doc123");
    EXPECT_TRUE(response_body.contains("buffer_stats"));
}

TEST_F(HttpBufferAPITest, BufferStats_AllBuffers) {
    http::request<http::string_body> req{http::verb::get, "/buffer/stats", 11};
    req.set(http::field::host, "localhost");
    
    auto res = buffer_handler_->handleBufferStats(req);
    
    EXPECT_EQ(res.result(), http::status::ok);
    
    auto body = json::parse(res.body());
    EXPECT_TRUE(body.contains("buffers"));
    EXPECT_TRUE(body["buffers"].contains("ts_buffer"));
    EXPECT_TRUE(body["buffers"].contains("vector_buffer"));
    EXPECT_TRUE(body["buffers"].contains("graph_buffer"));
    
    // TS buffer should be enabled
    EXPECT_TRUE(body["buffers"]["ts_buffer"]["enabled"]);
    
    // Vector buffer should be enabled
    EXPECT_TRUE(body["buffers"]["vector_buffer"]["enabled"]);
    
    // Graph buffer should be disabled (not initialized in SetUp)
    EXPECT_FALSE(body["buffers"]["graph_buffer"]["enabled"]);
}

TEST_F(HttpBufferAPITest, BufferFlush_TSBuffer) {
    // Insert some data first
    http::request<http::string_body> put_req{http::verb::post, "/ts/put/buffered", 11};
    put_req.set(http::field::host, "localhost");
    put_req.set(http::field::content_type, "application/json");
    
    json put_body = {
        {"metric", "mem.usage"},
        {"entity", "server02"},
        {"timestamp", 1700000100},
        {"value", 80.0}
    };
    put_req.body() = put_body.dump();
    put_req.prepare_payload();
    
    buffer_handler_->handleTSPutBuffered(put_req);
    
    // Flush TS buffer
    http::request<http::string_body> flush_req{http::verb::post, "/buffer/flush", 11};
    flush_req.set(http::field::host, "localhost");
    flush_req.set(http::field::content_type, "application/json");
    
    json flush_body = {{"buffer", "ts"}};
    flush_req.body() = flush_body.dump();
    flush_req.prepare_payload();
    
    auto res = buffer_handler_->handleBufferFlush(flush_req);
    
    EXPECT_EQ(res.result(), http::status::ok);
    
    auto response_body = json::parse(res.body());
    EXPECT_TRUE(response_body.contains("flushed"));
    EXPECT_TRUE(response_body["flushed"].contains("ts_buffer"));
}

TEST_F(HttpBufferAPITest, BufferFlush_AllBuffers) {
    // Flush all buffers
    http::request<http::string_body> req{http::verb::post, "/buffer/flush", 11};
    req.set(http::field::host, "localhost");
    req.set(http::field::content_type, "application/json");
    
    json body = {{"buffer", "all"}};
    req.body() = body.dump();
    req.prepare_payload();
    
    auto res = buffer_handler_->handleBufferFlush(req);
    
    EXPECT_EQ(res.result(), http::status::ok);
    
    auto response_body = json::parse(res.body());
    EXPECT_TRUE(response_body.contains("flushed"));
}

TEST_F(HttpBufferAPITest, ConcurrentInserts) {
    const int num_threads = 4;
    const int inserts_per_thread = 25;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, inserts_per_thread]() {
            for (int i = 0; i < inserts_per_thread; ++i) {
                http::request<http::string_body> req{http::verb::post, "/ts/put/buffered", 11};
                req.set(http::field::host, "localhost");
                req.set(http::field::content_type, "application/json");
                
                json body = {
                    {"metric", "test.metric"},
                    {"entity", "server" + std::to_string(t)},
                    {"timestamp", 1700000000 + (t * inserts_per_thread) + i},
                    {"value", static_cast<double>(i)}
                };
                req.body() = body.dump();
                req.prepare_payload();
                
                auto res = buffer_handler_->handleTSPutBuffered(req);
                EXPECT_EQ(res.result(), http::status::ok);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all inserts were buffered
    http::request<http::string_body> stats_req{http::verb::get, "/buffer/stats", 11};
    auto stats_res = buffer_handler_->handleBufferStats(stats_req);
    
    auto stats_body = json::parse(stats_res.body());
    EXPECT_GE(stats_body["buffers"]["ts_buffer"]["points_buffered"], num_threads * inserts_per_thread);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
