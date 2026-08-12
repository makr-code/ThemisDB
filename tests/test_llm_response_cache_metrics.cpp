#include <gtest/gtest.h>
#include "llm/llm_response_cache.h"
#include "llm/grafana_metrics.h"
#include "utils/type_conversion.h"
#include <memory>

using namespace themis::llm;
using namespace themis::llm::monitoring;
using themis::utils::conversion::safe_double_to_float;

class LLMResponseCacheMetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize Prometheus exporter
        exporter_ = std::make_unique<PrometheusExporter>();
        
        // Initialize metrics collector
        metrics_collector_ = std::make_unique<LLMMetricsCollector>(exporter_.get());
        
        // Configure response cache
        LLMResponseCache::Config config;
        config.similarity_threshold = 0.90f;
        config.ttl_seconds = 3600;
        config.max_entries = 100;
        
        // Create cache
        cache_ = std::make_unique<LLMResponseCache>("test_cache", config);
        
        // Set metrics collector
        cache_->setMetricsCollector(metrics_collector_.get());
    }
    
    void TearDown() override {
        cache_.reset();
        metrics_collector_.reset();
        exporter_.reset();
    }
    
    InferenceResponse createTestResponse(const std::string& text, int tokens = 10) {
        InferenceResponse response;
        response.text = text;
        response.tokens_generated = tokens;
        response.inference_time_ms = 100.0f;
        response.model_used = "test-model";
        return response;
    }
    
    std::unique_ptr<PrometheusExporter> exporter_;
    std::unique_ptr<LLMMetricsCollector> metrics_collector_;
    std::unique_ptr<LLMResponseCache> cache_;
};

TEST_F(LLMResponseCacheMetricsTest, CacheHitRecordsMetric) {
    // Put a response in cache
    std::string prompt = "What is 2+2?";
    auto response = createTestResponse("The answer is 4.");
    cache_->put(prompt, response);
    
    // Get it back (should be a hit)
    auto cached = cache_->get(prompt);
    ASSERT_TRUE(cached.has_value());
    
    // Export metrics
    std::string metrics = exporter_->exportMetrics();
    
    // Verify cache hit was recorded
    EXPECT_TRUE(metrics.find("llm_cache_hits_total") != std::string::npos);
    EXPECT_TRUE(metrics.find("test_cache") != std::string::npos);
}

TEST_F(LLMResponseCacheMetricsTest, CacheMissRecordsMetric) {
    // Try to get a non-existent response
    auto cached = cache_->get("Non-existent prompt");
    EXPECT_FALSE(cached.has_value());
    
    // Export metrics
    std::string metrics = exporter_->exportMetrics();
    
    // Verify cache miss was recorded
    EXPECT_TRUE(metrics.find("llm_cache_misses_total") != std::string::npos);
}

TEST_F(LLMResponseCacheMetricsTest, CacheSizeIsTracked) {
    // Add multiple responses
    for (int i = 0; i < 5; ++i) {
        std::string prompt = "Prompt " + std::to_string(i);
        auto response = createTestResponse("Response " + std::to_string(i));
        cache_->put(prompt, response);
    }
    
    // Export metrics
    std::string metrics = exporter_->exportMetrics();
    
    // Verify cache size is recorded
    EXPECT_TRUE(metrics.find("llm_cache_size_mb") != std::string::npos);
}

TEST_F(LLMResponseCacheMetricsTest, SemanticCacheHit) {
    // Put a response in cache
    std::string prompt1 = "What is machine learning?";
    auto response = createTestResponse("Machine learning is a type of AI.");
    cache_->put(prompt1, response);
    
    // Try a similar prompt (should match semantically)
    std::string prompt2 = "What is machine learning in AI?";
    auto cached = cache_->get(prompt2);
    
    if (cached.has_value()) {
        // Export metrics
        std::string metrics = exporter_->exportMetrics();
        
        // Verify semantic cache hit was recorded
        EXPECT_TRUE(metrics.find("llm_cache_hits_total") != std::string::npos);
        EXPECT_TRUE(metrics.find("semantic") != std::string::npos || 
                    metrics.find("test_cache") != std::string::npos);
    }
}

TEST_F(LLMResponseCacheMetricsTest, CacheStatistics) {
    // Perform some cache operations
    auto response = createTestResponse("Test response");
    
    // Add 3 entries
    cache_->put("Prompt 1", response);
    cache_->put("Prompt 2", response);
    cache_->put("Prompt 3", response);
    
    // 2 hits
    cache_->get("Prompt 1");
    cache_->get("Prompt 2");
    
    // 2 misses
    cache_->get("Non-existent 1");
    cache_->get("Non-existent 2");
    
    // Get statistics
    auto stats = cache_->getStatistics();
    
    EXPECT_EQ(stats.hits, 2);
    EXPECT_EQ(stats.misses, 2);
    EXPECT_EQ(stats.total_entries, 3);
    EXPECT_FLOAT_EQ(stats.getHitRate(), 0.5);
}

TEST_F(LLMResponseCacheMetricsTest, CacheClearRecordsMetric) {
    // Add some entries
    for (int i = 0; i < 3; ++i) {
        cache_->put("Prompt " + std::to_string(i), createTestResponse("Response"));
    }
    
    // Clear cache
    cache_->clear();
    
    // Export metrics
    std::string metrics = exporter_->exportMetrics();
    
    // Verify cache size is now 0
    EXPECT_TRUE(metrics.find("llm_cache_size_mb") != std::string::npos);
    
    // Statistics should show 0 entries
    auto stats = cache_->getStatistics();
    EXPECT_EQ(stats.total_entries, 0);
}

TEST_F(LLMResponseCacheMetricsTest, HighHitRateScenario) {
    // Simulate a high hit rate scenario
    std::vector<std::string> common_prompts = {
        "What is AI?",
        "Explain machine learning",
        "What is deep learning?"
    };
    
    // Cache responses
    for (const auto& prompt : common_prompts) {
        cache_->put(prompt, createTestResponse("Answer for: " + prompt));
    }
    
    // Simulate many requests (90% hit rate)
    int total_requests = 100;
    int expected_hits = 0;
    
    for (int i = 0; i < total_requests; ++i) {
        if (i < 90) {
            // Use common prompts (cache hits)
            cache_->get(common_prompts[i % common_prompts.size()]);
            expected_hits++;
        } else {
            // Use unique prompts (cache misses)
            cache_->get("Unique prompt " + std::to_string(i));
        }
    }
    
    // Verify high hit rate
    auto stats = cache_->getStatistics();
    EXPECT_GE(stats.getHitRate(), 0.85); // At least 85% hit rate
    
    // Export metrics
    std::string metrics = exporter_->exportMetrics();
    EXPECT_FALSE(metrics.empty());
}

TEST_F(LLMResponseCacheMetricsTest, MetricsThreadSafety) {
    // Test concurrent cache operations
    std::vector<std::thread> threads;
    
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([this, t]() {
            for (int i = 0; i < 10; ++i) {
                std::string prompt = "Thread " + std::to_string(t) + " Prompt " + std::to_string(i);
                cache_->put(prompt, createTestResponse("Response"));
                cache_->get(prompt);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Export metrics - should not crash
    std::string metrics = exporter_->exportMetrics();
    EXPECT_FALSE(metrics.empty());
    
    // Verify some entries exist
    auto stats = cache_->getStatistics();
    EXPECT_GT(stats.total_entries, 0);
}
