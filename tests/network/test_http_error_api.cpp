#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <thread>
#include <chrono>

#include "server/http_server.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "transaction/transaction_manager.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class ErrorApiHttpTest : public ::testing::Test {
protected:
    void SetUp() override {
        const std::string db_path = "data/themis_http_error_api_test";
        if (std::filesystem::exists(db_path)) std::filesystem::remove_all(db_path);
        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = db_path;
        cfg.memtable_size_mb = 32;
        cfg.block_cache_size_mb = 64;
        storage_ = std::make_shared<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());
        secondary_index_ = std::make_shared<themis::SecondaryIndexManager>(*storage_);
        graph_index_ = std::make_shared<themis::GraphIndexManager>(*storage_);
        vector_index_ = std::make_shared<themis::VectorIndexManager>(*storage_);
        tx_manager_ = std::make_shared<themis::TransactionManager>(*storage_, *secondary_index_, *graph_index_, *vector_index_);

        themis::server::HttpServer::Config scfg;
        scfg.host = "127.0.0.1";
        scfg.port = 18112; // dedicated port
        scfg.num_threads = 2;
        server_ = std::make_unique<themis::server::HttpServer>(scfg, storage_, secondary_index_, graph_index_, vector_index_, tx_manager_);
        server_->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }

    void TearDown() override {
        if (server_) server_->stop();
        if (storage_) storage_->close();
        const std::string db_path = "data/themis_http_error_api_test";
        if (std::filesystem::exists(db_path)) std::filesystem::remove_all(db_path);
    }

    static http::response<http::string_body> http_get(const std::string& host, const std::string& port, const std::string& target) {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);
        auto const results = resolver.resolve(host, port);
        stream.connect(results);
        http::request<http::string_body> req{http::verb::get, target, 11};
        req.set(http::field::host, host);
        http::write(stream, req);
        beast::flat_buffer buf;
        http::response<http::string_body> res;
        http::read(stream, buf, res);
        beast::error_code ec; stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        return res;
    }

    std::unique_ptr<themis::server::HttpServer> server_;
    std::shared_ptr<themis::RocksDBWrapper> storage_;
    std::shared_ptr<themis::SecondaryIndexManager> secondary_index_;
    std::shared_ptr<themis::GraphIndexManager> graph_index_;
    std::shared_ptr<themis::VectorIndexManager> vector_index_;
    std::shared_ptr<themis::TransactionManager> tx_manager_;
};

TEST_F(ErrorApiHttpTest, ListAllErrorsReturnsSuccess) {
    auto res = http_get("127.0.0.1", "18112", "/api/v1/errors");
    ASSERT_EQ(res.result(), http::status::ok) << res.body();
    auto body = nlohmann::json::parse(res.body());
    ASSERT_TRUE(body.contains("status"));
    EXPECT_EQ(body["status"], "success");
    ASSERT_TRUE(body.contains("errors"));
    EXPECT_TRUE(body["errors"].is_array());
}

TEST_F(ErrorApiHttpTest, GetErrorByCodeReturnsErrorDetails) {
    // Test with a known error code (e.g., 2000 - LLM model not found)
    auto res = http_get("127.0.0.1", "18112", "/api/v1/errors/2000");
    ASSERT_EQ(res.result(), http::status::ok) << res.body();
    auto body = nlohmann::json::parse(res.body());
    ASSERT_TRUE(body.contains("status"));
    EXPECT_EQ(body["status"], "success");
    ASSERT_TRUE(body.contains("error"));
    auto error = body["error"];
    EXPECT_EQ(error["code"], 2000);
    EXPECT_TRUE(error.contains("message_template"));
    EXPECT_TRUE(error.contains("category"));
}

TEST_F(ErrorApiHttpTest, GetErrorByCodeReturns404ForUnknownCode) {
    // Test with a non-existent error code
    auto res = http_get("127.0.0.1", "18112", "/api/v1/errors/9999");
    ASSERT_EQ(res.result(), http::status::not_found) << res.body();
    auto body = nlohmann::json::parse(res.body());
    ASSERT_TRUE(body.contains("status"));
    EXPECT_EQ(body["status"], "not_found");
}

TEST_F(ErrorApiHttpTest, GetCategoriesReturnsList) {
    auto res = http_get("127.0.0.1", "18112", "/api/v1/errors/categories");
    ASSERT_EQ(res.result(), http::status::ok) << res.body();
    auto body = nlohmann::json::parse(res.body());
    ASSERT_TRUE(body.contains("status"));
    EXPECT_EQ(body["status"], "success");
    ASSERT_TRUE(body.contains("categories"));
    EXPECT_TRUE(body["categories"].is_array());
    ASSERT_TRUE(body.contains("count"));
}

TEST_F(ErrorApiHttpTest, SearchErrorsByKeyword) {
    // Search for "model" keyword
    auto res = http_get("127.0.0.1", "18112", "/api/v1/errors/search?q=model");
    ASSERT_EQ(res.result(), http::status::ok) << res.body();
    auto body = nlohmann::json::parse(res.body());
    ASSERT_TRUE(body.contains("status"));
    EXPECT_EQ(body["status"], "success");
    ASSERT_TRUE(body.contains("query"));
    EXPECT_EQ(body["query"], "model");
    ASSERT_TRUE(body.contains("errors"));
    EXPECT_TRUE(body["errors"].is_array());
    ASSERT_TRUE(body.contains("count"));
}

TEST_F(ErrorApiHttpTest, SearchWithoutQueryReturns400) {
    // Search without query parameter should return 400
    auto res = http_get("127.0.0.1", "18112", "/api/v1/errors/search");
    ASSERT_EQ(res.result(), http::status::bad_request) << res.body();
    auto body = nlohmann::json::parse(res.body());
    ASSERT_TRUE(body.contains("status"));
    EXPECT_EQ(body["status"], "error");
    ASSERT_TRUE(body.contains("message"));
}

TEST_F(ErrorApiHttpTest, FilterErrorsByCategory) {
    // Filter by LLM category
    auto res = http_get("127.0.0.1", "18112", "/api/v1/errors?category=LLM");
    ASSERT_EQ(res.result(), http::status::ok) << res.body();
    auto body = nlohmann::json::parse(res.body());
    ASSERT_TRUE(body.contains("status"));
    EXPECT_EQ(body["status"], "success");
    ASSERT_TRUE(body.contains("category"));
    EXPECT_EQ(body["category"], "LLM");
    ASSERT_TRUE(body.contains("errors"));
    EXPECT_TRUE(body["errors"].is_array());
}
