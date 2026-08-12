/**
 * @file test_http_changefeed_governance.cpp
 * @brief HTTP tests for Changefeed Governance Headers
 * 
 * This test suite validates that governance headers are properly applied
 * to all changefeed endpoints, ensuring compliance and audit requirements.
 */

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

class HttpChangefeedGovernanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        const std::string db_path = "data/themis_http_cdc_gov_test";
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
        
        themis::server::HttpServer::Config scfg;
        scfg.host = "127.0.0.1";
        scfg.port = 18095;  // Different port to avoid conflicts
        scfg.num_threads = 2;
        scfg.feature_cdc = true; // Enable CDC
        
        server_ = std::make_unique<themis::server::HttpServer>(
            scfg, storage_, secondary_index_, graph_index_, vector_index_, tx_manager_
        );
        server_->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Insert some test data to generate changefeed events
        insertTestData();
    }
    
    void TearDown() override {
        if (server_) {
            server_->stop();
        }
        storage_->close();
        
        const std::string db_path = "data/themis_http_cdc_gov_test";
        if (std::filesystem::exists(db_path)) {
            std::filesystem::remove_all(db_path);
        }
    }
    
    void insertTestData() {
        // Insert some test entities to generate changefeed events
        for (int i = 0; i < 3; i++) {
            std::string key = "test:entity:" + std::to_string(i);
            json entity = {
                {"id", key},
                {"type", "test"},
                {"value", i * 100}
            };
            storage_->put(key, entity.dump());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    http::response<http::string_body> get_with_headers(
        const std::string& target,
        const std::map<std::string, std::string>& headers = {}
    ) {
        try {
            net::io_context ioc;
            tcp::resolver resolver(ioc);
            beast::tcp_stream stream(ioc);
            
            auto const results = resolver.resolve("127.0.0.1", "18095");
            stream.connect(results);
            
            http::request<http::string_body> req{http::verb::get, target, 11};
            req.set(http::field::host, "127.0.0.1");
            
            // Add custom headers
            for (const auto& [key, value] : headers) {
                req.set(key, value);
            }
            
            http::write(stream, req);
            
            beast::flat_buffer buf;
            http::response<http::string_body> res;
            http::read(stream, buf, res);
            
            beast::error_code ec;
            stream.socket().shutdown(tcp::socket::shutdown_both, ec);
            
            return res;
        } catch (const std::exception& e) {
            ADD_FAILURE() << "GET failed: " << e.what();
            return http::response<http::string_body>{http::status::internal_server_error, 11};
        }
    }
    
    http::response<http::string_body> post_with_headers(
        const std::string& target,
        const json& body,
        const std::map<std::string, std::string>& headers = {}
    ) {
        try {
            net::io_context ioc;
            tcp::resolver resolver(ioc);
            beast::tcp_stream stream(ioc);
            
            auto const results = resolver.resolve("127.0.0.1", "18095");
            stream.connect(results);
            
            http::request<http::string_body> req{http::verb::post, target, 11};
            req.set(http::field::host, "127.0.0.1");
            req.set(http::field::content_type, "application/json");
            
            // Add custom headers
            for (const auto& [key, value] : headers) {
                req.set(key, value);
            }
            
            req.body() = body.dump();
            req.prepare_payload();
            
            http::write(stream, req);
            
            beast::flat_buffer buf;
            http::response<http::string_body> res;
            http::read(stream, buf, res);
            
            beast::error_code ec;
            stream.socket().shutdown(tcp::socket::shutdown_both, ec);
            
            return res;
        } catch (const std::exception& e) {
            ADD_FAILURE() << "POST failed: " << e.what();
            return http::response<http::string_body>{http::status::internal_server_error, 11};
        }
    }
    
    bool has_header(const http::response<http::string_body>& res, const std::string& header_name) {
        return res.find(header_name) != res.end();
    }
    
    std::string get_header(const http::response<http::string_body>& res, const std::string& header_name) {
        auto it = res.find(header_name);
        if (it != res.end()) {
            return std::string(it->value());
        }
        return "";
    }

    std::unique_ptr<themis::server::HttpServer> server_;
    std::shared_ptr<themis::RocksDBWrapper> storage_;
    std::shared_ptr<themis::SecondaryIndexManager> secondary_index_;
    std::shared_ptr<themis::GraphIndexManager> graph_index_;
    std::shared_ptr<themis::VectorIndexManager> vector_index_;
    std::shared_ptr<themis::TransactionManager> tx_manager_;
};

// Test: Default governance headers on changefeed GET endpoint
TEST_F(HttpChangefeedGovernanceTest, ChangefeedGet_HasDefaultGovernanceHeaders) {
    auto res = get_with_headers("/changefeed?from_seq=0&limit=10");
    
    ASSERT_EQ(res.result(), http::status::ok);
    
    // Check for required governance headers
    EXPECT_TRUE(has_header(res, "X-Themis-Policy"));
    EXPECT_TRUE(has_header(res, "X-Themis-Content-Enc"));
    EXPECT_TRUE(has_header(res, "X-Themis-Export"));
    EXPECT_TRUE(has_header(res, "X-Themis-Cache"));
    EXPECT_TRUE(has_header(res, "X-Themis-Retention-Days"));
    
    // Check for CDC-specific governance headers
    EXPECT_TRUE(has_header(res, "X-Themis-CDC-Encryption"));
    EXPECT_TRUE(has_header(res, "X-Themis-CDC-Audit"));
    EXPECT_TRUE(has_header(res, "X-Themis-CDC-Classification"));
    EXPECT_TRUE(has_header(res, "X-Themis-CDC-Source"));
    EXPECT_TRUE(has_header(res, "X-Themis-CDC-Timestamp"));
    
    // Verify default classification for changefeed is vs-nfd (sensitive)
    std::string policy = get_header(res, "X-Themis-Policy");
    EXPECT_TRUE(policy.find("classification=vs-nfd") != std::string::npos);
    
    // CDC should always have audit enabled
    EXPECT_EQ(get_header(res, "X-Themis-CDC-Audit"), "enabled");
}

// Test: Changefeed stats endpoint has governance headers
TEST_F(HttpChangefeedGovernanceTest, ChangefeedStats_HasGovernanceHeaders) {
    auto res = get_with_headers("/changefeed/stats");
    
    ASSERT_EQ(res.result(), http::status::ok);
    
    // Check for governance headers
    EXPECT_TRUE(has_header(res, "X-Themis-Policy"));
    EXPECT_TRUE(has_header(res, "X-Themis-CDC-Audit"));
    EXPECT_TRUE(has_header(res, "X-Themis-CDC-Classification"));
}

// Test: Changefeed retention endpoint has governance headers
TEST_F(HttpChangefeedGovernanceTest, ChangefeedRetention_HasGovernanceHeaders) {
    json body = {
        {"before_sequence", 1}
    };
    
    auto res = post_with_headers("/changefeed/retention", body);
    
    ASSERT_EQ(res.result(), http::status::ok);
    
    // Check for governance headers
    EXPECT_TRUE(has_header(res, "X-Themis-Policy"));
    EXPECT_TRUE(has_header(res, "X-Themis-CDC-Audit"));
}

// Test: Classification "offen" allows less restrictive policies
TEST_F(HttpChangefeedGovernanceTest, Classification_Offen_LessRestrictive) {
    std::map<std::string, std::string> headers = {
        {"X-Classification", "offen"}
    };
    
    auto res = get_with_headers("/changefeed?from_seq=0&limit=10", headers);
    
    ASSERT_EQ(res.result(), http::status::ok);
    
    std::string policy = get_header(res, "X-Themis-Policy");
    EXPECT_TRUE(policy.find("classification=offen") != std::string::npos);
    
    // Offen should have less restrictive settings
    EXPECT_EQ(get_header(res, "X-Themis-Retention-Days"), "365");
    EXPECT_EQ(get_header(res, "X-Themis-CDC-Encryption"), "optional");
}

// Test: Classification "geheim" enforces stricter policies
TEST_F(HttpChangefeedGovernanceTest, Classification_Geheim_StricterPolicies) {
    std::map<std::string, std::string> headers = {
        {"X-Classification", "geheim"}
    };
    
    auto res = get_with_headers("/changefeed?from_seq=0&limit=10", headers);
    
    ASSERT_EQ(res.result(), http::status::ok);
    
    std::string policy = get_header(res, "X-Themis-Policy");
    EXPECT_TRUE(policy.find("classification=geheim") != std::string::npos);
    
    // Geheim should have stricter settings
    EXPECT_EQ(get_header(res, "X-Themis-Cache"), "disabled");
    EXPECT_EQ(get_header(res, "X-Themis-Retention-Days"), "730");
    EXPECT_EQ(get_header(res, "X-Themis-CDC-Encryption"), "recommended");
}

// Test: Classification "streng-geheim" enforces most restrictive policies
TEST_F(HttpChangefeedGovernanceTest, Classification_StrengGeheim_MostRestrictive) {
    std::map<std::string, std::string> headers = {
        {"X-Classification", "streng-geheim"}
    };
    
    auto res = get_with_headers("/changefeed?from_seq=0&limit=10", headers);
    
    ASSERT_EQ(res.result(), http::status::ok);
    
    std::string policy = get_header(res, "X-Themis-Policy");
    EXPECT_TRUE(policy.find("classification=streng-geheim") != std::string::npos);
    
    // Streng-geheim should have most restrictive settings
    EXPECT_EQ(get_header(res, "X-Themis-Content-Enc"), "required");
    EXPECT_EQ(get_header(res, "X-Themis-Export"), "forbidden");
    EXPECT_EQ(get_header(res, "X-Themis-Cache"), "disabled");
    EXPECT_EQ(get_header(res, "X-Themis-Retention-Days"), "1095"); // 3 years
    EXPECT_EQ(get_header(res, "X-Themis-CDC-Encryption"), "required");
    
    // Verify redaction is strict
    EXPECT_TRUE(policy.find("redaction=strict") != std::string::npos);
}

// Test: VS-NFD classification for classified data
TEST_F(HttpChangefeedGovernanceTest, Classification_VsNfd_RequiresEncryption) {
    std::map<std::string, std::string> headers = {
        {"X-Classification", "vs-nfd"}
    };
    
    auto res = get_with_headers("/changefeed?from_seq=0&limit=10", headers);
    
    ASSERT_EQ(res.result(), http::status::ok);
    
    std::string policy = get_header(res, "X-Themis-Policy");
    EXPECT_TRUE(policy.find("classification=vs-nfd") != std::string::npos);
    
    // VS-NFD should require encryption
    EXPECT_EQ(get_header(res, "X-Themis-Content-Enc"), "required");
    EXPECT_EQ(get_header(res, "X-Themis-Retention-Days"), "730");
    EXPECT_EQ(get_header(res, "X-Themis-CDC-Encryption"), "recommended");
}

// Test: Security headers are present
TEST_F(HttpChangefeedGovernanceTest, SecurityHeaders_Present) {
    auto res = get_with_headers("/changefeed?from_seq=0&limit=10");
    
    ASSERT_EQ(res.result(), http::status::ok);
    
    // Check for security headers
    EXPECT_TRUE(has_header(res, "X-Frame-Options"));
    EXPECT_TRUE(has_header(res, "X-Content-Type-Options"));
    EXPECT_TRUE(has_header(res, "Referrer-Policy"));
    EXPECT_TRUE(has_header(res, "Content-Security-Policy"));
    EXPECT_TRUE(has_header(res, "X-XSS-Protection"));
    
    // Verify values
    EXPECT_EQ(get_header(res, "X-Frame-Options"), "DENY");
    EXPECT_EQ(get_header(res, "X-Content-Type-Options"), "nosniff");
    EXPECT_EQ(get_header(res, "Referrer-Policy"), "no-referrer");
}

// Test: CDC cache should be disabled by default
TEST_F(HttpChangefeedGovernanceTest, CDC_CacheDisabledByDefault) {
    auto res = get_with_headers("/changefeed?from_seq=0&limit=10");
    
    ASSERT_EQ(res.result(), http::status::ok);
    
    // CDC streams should never be cached
    EXPECT_EQ(get_header(res, "X-Themis-Cache"), "disabled");
}

// Test: CDC timestamp header is present and valid
TEST_F(HttpChangefeedGovernanceTest, CDC_TimestampPresent) {
    auto res = get_with_headers("/changefeed?from_seq=0&limit=10");
    
    ASSERT_EQ(res.result(), http::status::ok);
    
    std::string timestamp_str = get_header(res, "X-Themis-CDC-Timestamp");
    EXPECT_FALSE(timestamp_str.empty());
    
    // Verify it's a valid number
    try {
        int64_t timestamp = std::stoll(timestamp_str);
        EXPECT_GT(timestamp, 0);
    } catch (const std::exception& e) {
        static_cast<void>(e);
        FAIL() << "Invalid timestamp format: " << timestamp_str;
    }
}

// Test: CDC source header indicates ThemisDB
TEST_F(HttpChangefeedGovernanceTest, CDC_SourceHeader) {
    auto res = get_with_headers("/changefeed?from_seq=0&limit=10");
    
    ASSERT_EQ(res.result(), http::status::ok);
    
    EXPECT_EQ(get_header(res, "X-Themis-CDC-Source"), "ThemisDB");
}

// Test: Governance mode "enforce" is reflected in policy header
TEST_F(HttpChangefeedGovernanceTest, GovernanceMode_Enforce) {
    std::map<std::string, std::string> headers = {
        {"X-Classification", "geheim"},
        {"X-Governance-Mode", "enforce"}
    };
    
    auto res = get_with_headers("/changefeed?from_seq=0&limit=10", headers);
    
    ASSERT_EQ(res.result(), http::status::ok);
    
    std::string policy = get_header(res, "X-Themis-Policy");
    EXPECT_TRUE(policy.find("mode=enforce") != std::string::npos);
}

// Test: Encrypt logs flag is reflected in policy header
TEST_F(HttpChangefeedGovernanceTest, EncryptLogs_Flag) {
    std::map<std::string, std::string> headers = {
        {"X-Encrypt-Logs", "true"}
    };
    
    auto res = get_with_headers("/changefeed?from_seq=0&limit=10", headers);
    
    ASSERT_EQ(res.result(), http::status::ok);
    
    std::string policy = get_header(res, "X-Themis-Policy");
    EXPECT_TRUE(policy.find("encrypt_logs=true") != std::string::npos);
}

// Test: Multiple governance headers work together
TEST_F(HttpChangefeedGovernanceTest, MultipleGovernanceHeaders_Combined) {
    std::map<std::string, std::string> headers = {
        {"X-Classification", "streng-geheim"},
        {"X-Governance-Mode", "enforce"},
        {"X-Encrypt-Logs", "true"}
    };
    
    auto res = get_with_headers("/changefeed?from_seq=0&limit=10", headers);
    
    ASSERT_EQ(res.result(), http::status::ok);
    
    std::string policy = get_header(res, "X-Themis-Policy");
    EXPECT_TRUE(policy.find("classification=streng-geheim") != std::string::npos);
    EXPECT_TRUE(policy.find("mode=enforce") != std::string::npos);
    EXPECT_TRUE(policy.find("encrypt_logs=true") != std::string::npos);
    EXPECT_TRUE(policy.find("redaction=strict") != std::string::npos);
}
