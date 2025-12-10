// Test for invalid UTF-8 handling (Security Issue: Server Crash on Invalid UTF-8 Input)
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <thread>
#include <chrono>
#include <filesystem>

#include "server/http_server.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "transaction/transaction_manager.h"

using json = nlohmann::json;
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class HttpInvalidUtf8Test : public ::testing::Test {
protected:
    void SetUp() override {
        // Create isolated test database
        const std::string db_path = "data/themis_http_invalid_utf8_test";
        
        // Clean up old test data
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
        tx_manager_ = std::make_shared<themis::TransactionManager>(*storage_, *secondary_index_, *graph_index_, *vector_index_);

        // Start HTTP server
        themis::server::HttpServer::Config scfg;
        scfg.host = "127.0.0.1";
        scfg.port = 18099; // unique port to avoid clashes
        scfg.num_threads = 2;

        server_ = std::make_unique<themis::server::HttpServer>(scfg, storage_, secondary_index_, graph_index_, vector_index_, tx_manager_);
        server_->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override {
        if (server_) server_->stop();
        storage_->close();
    }

    // Send raw bytes (potentially invalid UTF-8) using PUT method
    http::response<http::string_body> putRaw(const std::string& target, const std::string& body) {
        return sendRaw(target, body, http::verb::put);
    }

    // Send raw bytes (potentially invalid UTF-8) using POST method
    http::response<http::string_body> postRaw(const std::string& target, const std::string& body) {
        return sendRaw(target, body, http::verb::post);
    }

    // Generic method to send raw bytes with specified HTTP verb
    http::response<http::string_body> sendRaw(const std::string& target, const std::string& body, http::verb method) {
        try {
            net::io_context ioc;
            tcp::resolver resolver(ioc);
            beast::tcp_stream stream(ioc);
            
            auto const results = resolver.resolve("127.0.0.1", "18099");
            stream.connect(results);
            
            http::request<http::string_body> req{method, target, 11};
            req.set(http::field::host, "127.0.0.1");
            req.set(http::field::content_type, "application/json");
            req.body() = body;
            req.prepare_payload();
            
            http::write(stream, req);
            
            beast::flat_buffer buffer;
            http::response<http::string_body> res;
            http::read(stream, buffer, res);
            
            beast::error_code ec;
            stream.socket().shutdown(tcp::socket::shutdown_both, ec);
            
            return res;
        } catch (std::exception const& e) {
            ADD_FAILURE() << "HTTP request failed: " << e.what();
            http::response<http::string_body> error_res{http::status::internal_server_error, 11};
            error_res.body() = std::string("Exception: ") + e.what();
            return error_res;
        }
    }

    std::shared_ptr<themis::RocksDBWrapper> storage_;
    std::shared_ptr<themis::SecondaryIndexManager> secondary_index_;
    std::shared_ptr<themis::GraphIndexManager> graph_index_;
    std::shared_ptr<themis::VectorIndexManager> vector_index_;
    std::shared_ptr<themis::TransactionManager> tx_manager_;
    std::unique_ptr<themis::server::HttpServer> server_;
};

TEST_F(HttpInvalidUtf8Test, PutEntityWithInvalidUtf8ShouldReturn400) {
    // Test case 1: Invalid UTF-8 in the middle of a string (0xFC which is invalid in UTF-8)
    // This simulates the Windows-1252 encoded umlaut that causes the crash
    std::string invalid_json = R"({"blob":"{\"name\":\"Test-Uml)" "\xFC" R"(ut\"}"})";
    
    auto response = putRaw("/entities/test:1", invalid_json);
    
    // Server should NOT crash and should return 400 Bad Request
    EXPECT_EQ(response.result(), http::status::bad_request) 
        << "Expected 400 Bad Request for invalid UTF-8, got: " 
        << static_cast<int>(response.result());
    
    // Response should be JSON with error message
    EXPECT_EQ(response[http::field::content_type], "application/json");
    
    // Parse response body
    try {
        auto body = json::parse(response.body());
        EXPECT_TRUE(body.contains("error"));
        EXPECT_TRUE(body.contains("message"));
        // Message should mention JSON parse error or invalid UTF-8
        std::string msg = body["message"].get<std::string>();
        EXPECT_TRUE(msg.find("JSON") != std::string::npos || 
                    msg.find("UTF-8") != std::string::npos ||
                    msg.find("parse") != std::string::npos)
            << "Error message should mention JSON/UTF-8/parse error: " << msg;
    } catch (const json::exception& e) {
        ADD_FAILURE() << "Response body is not valid JSON: " << response.body();
    }
}

TEST_F(HttpInvalidUtf8Test, QueryWithInvalidUtf8ShouldReturn400) {
    // Test case 2: Invalid UTF-8 in query endpoint
    std::string invalid_json = R"({"table":"users","filter":{"name":")" "\xC3\x28" R"("}})";
    
    auto response = postRaw("/query", invalid_json);
    
    // Server should NOT crash and should return 400 Bad Request
    EXPECT_EQ(response.result(), http::status::bad_request)
        << "Expected 400 Bad Request for invalid UTF-8 in query";
    
    EXPECT_EQ(response[http::field::content_type], "application/json");
}

TEST_F(HttpInvalidUtf8Test, AqlQueryWithInvalidUtf8ShouldReturn400) {
    // Test case 3: Invalid UTF-8 in AQL endpoint
    std::string invalid_json = R"({"query":"FOR u IN users FILTER u.name == ')" "\xFC" R"(' RETURN u"})";
    
    auto response = postRaw("/query/aql", invalid_json);
    
    // Server should NOT crash and should return 400 Bad Request
    EXPECT_EQ(response.result(), http::status::bad_request)
        << "Expected 400 Bad Request for invalid UTF-8 in AQL";
    
    EXPECT_EQ(response[http::field::content_type], "application/json");
}

TEST_F(HttpInvalidUtf8Test, ValidUtf8ShouldStillWork) {
    // Test case 4: Make sure valid UTF-8 with special characters still works
    std::string valid_json = R"({"blob":"{\"name\":\"Test-Umlaut-ü\"}"})";
    
    auto response = putRaw("/entities/test:valid", valid_json);
    
    // This should succeed (200 or 201)
    EXPECT_TRUE(response.result() == http::status::ok || 
                response.result() == http::status::created)
        << "Valid UTF-8 should be accepted, got: " 
        << static_cast<int>(response.result());
}
