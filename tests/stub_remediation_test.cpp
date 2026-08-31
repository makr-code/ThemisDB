/**
 * @file stub_remediation_test.cpp
 * @brief Tests for Stub #301 and #307 remediation:
 *        - Timeseries real aggregates and retention policies
 *        - RoPE real runtime metrics
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <thread>
#include <chrono>
#include <filesystem>

#include "server/timeseries_api_handler.h"
#include "server/rope_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "timeseries/tsstore.h"
#include "timeseries/continuous_agg.h"
#include "index/vector_index.h"
#include "index/rotary_embeddings.h"
#include "storage/base_entity.h"

using json = nlohmann::json;
namespace beast = boost::beast;
namespace http = beast::http;

class StubRemediationTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = std::filesystem::temp_directory_path() / "themis_stub_remediation_test";
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }
        std::filesystem::create_directories(db_path_);

        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_.string();
        storage_ = std::make_shared<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());
    }

    void TearDown() override {
        storage_->close();
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }
    }

    std::filesystem::path db_path_;
    std::shared_ptr<themis::RocksDBWrapper> storage_;
};

// ============================================================================
// Stub #301: Timeseries Metadata Endpoints
// ============================================================================

TEST_F(StubRemediationTest, TimeSeriesAggregatesWithProvider) {
    auto ts_store = std::make_shared<themis::TSStore>(*storage_);
    auto agg_manager = std::make_shared<themis::ContinuousAggregateManager>(*storage_);
    auto handler = std::make_shared<themis::server::TimeSeriesApiHandler>(
        storage_, ts_store, agg_manager, nullptr
    );

    // Set a real aggregates provider that returns actual aggregate names
    handler->setAggregatesProvider([]() {
        return std::vector<std::string>{"min", "max", "avg", "sum", "count", "stddev", "p95", "p99"};
    });

    // Create a mock request
    http::request<http::string_body> req;
    req.method(http::verb::get);
    req.target("/ts/aggregates");
    req.version(11);

    auto response = handler->handleAggregatesGet(req);
    ASSERT_EQ(response.result_int(), 200);

    json response_body = json::parse(response.body());
    ASSERT_TRUE(response_body.contains("aggregates"));
    
    auto& aggregates = response_body["aggregates"];
    ASSERT_TRUE(aggregates.is_array());
    ASSERT_GE(aggregates.size(), 8);  // At least the provided aggregates
    EXPECT_EQ(response_body["source"].get<std::string>(), "provider");
    EXPECT_FALSE(response_body["degraded_mode"].get<bool>());
    
    // Verify all provided aggregates are present
    std::set<std::string> agg_set;
    for (const auto& agg : aggregates) {
        agg_set.insert(agg.get<std::string>());
    }
    
    EXPECT_TRUE(agg_set.count("min") > 0);
    EXPECT_TRUE(agg_set.count("max") > 0);
    EXPECT_TRUE(agg_set.count("stddev") > 0);
    EXPECT_TRUE(agg_set.count("p95") > 0);
    EXPECT_TRUE(agg_set.count("p99") > 0);
}

TEST_F(StubRemediationTest, TimeSeriesRetentionWithProvider) {
    auto ts_store = std::make_shared<themis::TSStore>(*storage_);
    auto agg_manager = std::make_shared<themis::ContinuousAggregateManager>(*storage_);
    auto handler = std::make_shared<themis::server::TimeSeriesApiHandler>(
        storage_, ts_store, agg_manager, nullptr
    );

    // Set a real retention policies provider
    handler->setRetentionPoliciesProvider([]() {
        std::map<std::string, int64_t> policies;
        policies["cpu_usage"] = 86400;      // 1 day
        policies["memory_usage"] = 604800;  // 7 days
        policies[""] = 2592000;             // 30 days (global)
        return policies;
    });

    // Create a mock request
    http::request<http::string_body> req;
    req.method(http::verb::get);
    req.target("/ts/retention");
    req.version(11);

    auto response = handler->handleRetentionGet(req);
    ASSERT_EQ(response.result_int(), 200);

    json response_body = json::parse(response.body());
    ASSERT_TRUE(response_body.contains("policies"));
    
    auto& policies = response_body["policies"];
    ASSERT_TRUE(policies.is_array());
    ASSERT_GE(policies.size(), 3);  // At least the provided policies
    EXPECT_EQ(response_body["source"].get<std::string>(), "provider");
    EXPECT_FALSE(response_body["degraded_mode"].get<bool>());

    // Verify policies contain expected metrics
    std::set<std::string> metric_set;
    for (const auto& policy : policies) {
        ASSERT_TRUE(policy.contains("metric"));
        ASSERT_TRUE(policy.contains("retain_seconds"));
        metric_set.insert(policy["metric"].get<std::string>());
    }
    
    EXPECT_TRUE(metric_set.count("cpu_usage") > 0);
    EXPECT_TRUE(metric_set.count("memory_usage") > 0);
}

TEST_F(StubRemediationTest, TimeSeriesAggregatesDefaultFallback) {
    auto ts_store = std::make_shared<themis::TSStore>(*storage_);
    auto agg_manager = std::make_shared<themis::ContinuousAggregateManager>(*storage_);
    auto handler = std::make_shared<themis::server::TimeSeriesApiHandler>(
        storage_, ts_store, agg_manager, nullptr
    );

    // Do NOT set a provider - should fall back to defaults
    http::request<http::string_body> req;
    req.method(http::verb::get);
    req.target("/ts/aggregates");
    req.version(11);

    auto response = handler->handleAggregatesGet(req);
    ASSERT_EQ(response.result_int(), 200);

    json response_body = json::parse(response.body());
    auto& aggregates = response_body["aggregates"];
    EXPECT_EQ(response_body["source"].get<std::string>(), "builtin");
    EXPECT_TRUE(response_body["degraded_mode"].get<bool>());
    EXPECT_TRUE(response_body.contains("degraded_reason"));
    
    // Should still have the built-in defaults
    ASSERT_GE(aggregates.size(), 5);
    std::set<std::string> agg_set;
    for (const auto& agg : aggregates) {
        agg_set.insert(agg.get<std::string>());
    }
    
    EXPECT_TRUE(agg_set.count("min") > 0);
    EXPECT_TRUE(agg_set.count("max") > 0);
    EXPECT_TRUE(agg_set.count("avg") > 0);
}

// ============================================================================
// Stub #307: RoPE Statistics
// ============================================================================

TEST_F(StubRemediationTest, RopeStatsWithRealMetrics) {
    auto vector_index = std::make_shared<themis::VectorIndexManager>(*storage_);
    auto handler = std::make_shared<themis::server::RopeApiHandler>(
        storage_, vector_index, nullptr
    );

    // Enable RoPE and configure it
    themis::RotationConfig rope_config;
    rope_config.hidden_dim = 768;
    rope_config.num_rotation_pairs = 384;
    rope_config.base_theta = 10000.0;
    rope_config.normalize_after = false;
    rope_config.computeThetaCache();

    ASSERT_TRUE(vector_index->enableRotaryEmbedding(rope_config).ok);

    // Simulate some rotations by adding entities with positions
    for (int i = 0; i < 5; ++i) {
        themis::BaseEntity entity;
        entity.pk = "entity_" + std::to_string(i);
        entity.entity_name = "test_entity";
        
        std::vector<float> embedding(768);
        for (int j = 0; j < 768; ++j) {
            embedding[j] = static_cast<float>(j) / 768.0f;
        }
        entity.embedding_data = embedding;
        
        auto status = vector_index->addEntityWithRotation(entity, "embedding_data", i);
        ASSERT_TRUE(status.ok) << status.message;
    }

    // Create a mock request for RoPE stats
    http::request<http::string_body> req;
    req.method(http::verb::get);
    req.target("/api/v1/vector-index/test_index/rope/stats");
    req.version(11);

    auto response = handler->handleStatsGet(req);
    ASSERT_EQ(response.result_int(), 200);

    json response_body = json::parse(response.body());
    ASSERT_TRUE(response_body.contains("enabled"));
    ASSERT_TRUE(response_body["enabled"].get<bool>());

    // STUB #307 REMEDIATION: Verify real metrics are returned
    ASSERT_TRUE(response_body.contains("rope_metrics"));
    auto& rope_metrics = response_body["rope_metrics"];
    
    ASSERT_TRUE(rope_metrics.contains("rotation_count"));
    ASSERT_TRUE(rope_metrics.contains("relational_rotation_count"));
    ASSERT_TRUE(rope_metrics.contains("avg_rotation_latency_ms"));
    ASSERT_TRUE(rope_metrics.contains("status"));
    
    EXPECT_EQ(rope_metrics["status"].get<std::string>(), "available");
    EXPECT_GE(rope_metrics["rotation_count"].get<uint64_t>(), 5);  // At least 5 rotations
    EXPECT_GE(rope_metrics["avg_rotation_latency_ms"].get<double>(), 0.0);
}

TEST_F(StubRemediationTest, RopeStatsDisabledReturnsNoMetrics) {
    auto vector_index = std::make_shared<themis::VectorIndexManager>(*storage_);
    auto handler = std::make_shared<themis::server::RopeApiHandler>(
        storage_, vector_index, nullptr
    );

    // Do NOT enable RoPE
    
    http::request<http::string_body> req;
    req.method(http::verb::get);
    req.target("/api/v1/vector-index/test_index/rope/stats");
    req.version(11);

    auto response = handler->handleStatsGet(req);
    ASSERT_EQ(response.result_int(), 200);

    json response_body = json::parse(response.body());
    ASSERT_TRUE(response_body.contains("enabled"));
    ASSERT_FALSE(response_body["enabled"].get<bool>());
    
    // When disabled, rope_metrics should not be present or should indicate unavailable
    if (response_body.contains("rope_metrics")) {
        EXPECT_EQ(response_body["rope_metrics"]["status"].get<std::string>(), "unavailable");
    }
}

TEST_F(StubRemediationTest, RopeStatsIncludesConfiguration) {
    auto vector_index = std::make_shared<themis::VectorIndexManager>(*storage_);
    auto handler = std::make_shared<themis::server::RopeApiHandler>(
        storage_, vector_index, nullptr
    );

    themis::RotationConfig rope_config;
    rope_config.hidden_dim = 512;
    rope_config.num_rotation_pairs = 256;
    rope_config.base_theta = 10000.0;
    rope_config.normalize_after = true;
    rope_config.computeThetaCache();

    ASSERT_TRUE(vector_index->enableRotaryEmbedding(rope_config).ok);

    http::request<http::string_body> req;
    req.method(http::verb::get);
    req.target("/api/v1/vector-index/test_index/rope/stats");
    req.version(11);

    auto response = handler->handleStatsGet(req);
    ASSERT_EQ(response.result_int(), 200);

    json response_body = json::parse(response.body());
    ASSERT_TRUE(response_body.contains("config"));
    
    auto& config = response_body["config"];
    EXPECT_EQ(config["hidden_dim"].get<size_t>(), 512);
    EXPECT_EQ(config["num_rotation_pairs"].get<size_t>(), 256);
    EXPECT_EQ(config["base_theta"].get<double>(), 10000.0);
    EXPECT_TRUE(config["normalize_after"].get<bool>());
}

} // namespace
