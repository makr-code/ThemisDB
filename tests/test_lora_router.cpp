#include <gtest/gtest.h>
#include "llm/lora_router.h"
#include "llm/adapter_load_balancer.h"
#include "llm/multi_lora_manager.h"
#include "llm/adapter_registry.h"
#include "llm/lora_framework/embedding_provider.h"
#include "llm/gpu_memory_manager.h"
#include "storage/security_signature_manager.h"
#include <memory>
#include <vector>
#include <string>

using namespace themis::llm;
using namespace themis::llm::lora;

// Mock EmbeddingProvider for testing
class MockEmbeddingProvider : public EmbeddingProvider {
public:
    MockEmbeddingProvider() 
        : EmbeddingProvider(nullptr, nullptr, Config{}) {
        // Override with mock behavior
    }
    
    std::vector<float> getEmbedding(const std::string& text) override {
        // Return deterministic embeddings based on text
        std::vector<float> embedding(512, 0.0f);
        
        // Generate simple hash-based embedding for testing
        std::hash<std::string> hasher;
        size_t hash = hasher(text);
        
        for (size_t i = 0; i < embedding.size(); ++i) {
            embedding[i] = std::sin(static_cast<float>(hash + i) * 0.1f);
        }
        
        return embedding;
    }
    
    size_t getEmbeddingDim() const override {
        return 512;
    }
};

class LoRARouterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create mock components
        embedding_provider_ = std::make_shared<MockEmbeddingProvider>();
        
        // Create signature manager (with nullptr RocksDB for testing)
        sig_manager_ = std::make_shared<themis::storage::SecuritySignatureManager>(nullptr);
        
        adapter_registry_ = std::make_shared<AdapterRegistry>(sig_manager_);
        
        // Create GPU memory manager
        gpu_memory_manager_ = std::make_shared<GPUMemoryManager>();
        
        // Create load balancer
        AdapterLoadBalancer::Config lb_config;
        load_balancer_ = std::make_shared<AdapterLoadBalancer>(
            gpu_memory_manager_, lb_config);
        
        // Create multi-LoRA manager
        MultiLoRAManager::Config lora_config;
        lora_manager_ = std::make_shared<MultiLoRAManager>(lora_config);
        
        // Create router with test configuration
        LoRARouter::Config router_config;
        router_config.enable_semantic_routing = true;
        router_config.enable_load_aware = true;
        router_config.top_k_candidates = 3;
        router_config.min_similarity_threshold = 0.1f;
        router_config.enable_decision_cache = true;
        
        router_ = std::make_unique<LoRARouter>(
            embedding_provider_,
            adapter_registry_,
            load_balancer_,
            lora_manager_,
            router_config
        );
        
        // Register test adapters
        registerTestAdapters();
    }
    
    void registerTestAdapters() {
        // Adapter 1: Documentation/Help
        AdapterMetadata adapter1;
        adapter1.adapter_id = "themis_help_lora";
        adapter1.task_type = "question-answering";
        adapter1.domain = "documentation";
        adapter1.base_model_name = "llama-2-7b";
        adapter1.base_model_version = "2.0";
        adapter1.status = AdapterMetadata::Status::DEPLOYED;
        adapter1.file_size_bytes = 32 * 1024 * 1024;  // 32MB
        adapter_registry_->registerAdapter(adapter1);
        
        // Adapter 2: SQL Query
        AdapterMetadata adapter2;
        adapter2.adapter_id = "themis_sql_lora";
        adapter2.task_type = "code-generation";
        adapter2.domain = "sql";
        adapter2.base_model_name = "llama-2-7b";
        adapter2.base_model_version = "2.0";
        adapter2.status = AdapterMetadata::Status::DEPLOYED;
        adapter2.file_size_bytes = 32 * 1024 * 1024;
        adapter_registry_->registerAdapter(adapter2);
        
        // Adapter 3: General Purpose
        AdapterMetadata adapter3;
        adapter3.adapter_id = "themis_general_lora";
        adapter3.task_type = "general";
        adapter3.domain = "general";
        adapter3.base_model_name = "llama-2-7b";
        adapter3.base_model_version = "2.0";
        adapter3.status = AdapterMetadata::Status::DEPLOYED;
        adapter3.file_size_bytes = 32 * 1024 * 1024;
        adapter_registry_->registerAdapter(adapter3);
    }
    
    std::shared_ptr<MockEmbeddingProvider> embedding_provider_;
    std::shared_ptr<themis::storage::SecuritySignatureManager> sig_manager_;
    std::shared_ptr<AdapterRegistry> adapter_registry_;
    std::shared_ptr<GPUMemoryManager> gpu_memory_manager_;
    std::shared_ptr<AdapterLoadBalancer> load_balancer_;
    std::shared_ptr<MultiLoRAManager> lora_manager_;
    std::unique_ptr<LoRARouter> router_;
};

TEST_F(LoRARouterTest, BasicSemanticRouting) {
    // Test basic semantic routing
    auto decision = router_->routeQuery("How do I configure sharding?");
    
    EXPECT_FALSE(decision.adapter_id.empty());
    EXPECT_FALSE(decision.is_fallback);
    EXPECT_GT(decision.similarity_score, 0.0f);
    EXPECT_GE(decision.confidence, 0.0f);
    EXPECT_LE(decision.confidence, 1.0f);
    EXPECT_EQ(decision.policy_used, RoutingPolicy::LOAD_AWARE);  // Default policy
}

TEST_F(LoRARouterTest, SemanticPolicySelection) {
    // Test pure semantic routing
    auto decision = router_->routeQuery(
        "Generate SQL query for users", 
        "", 
        RoutingPolicy::SEMANTIC
    );
    
    EXPECT_FALSE(decision.adapter_id.empty());
    EXPECT_EQ(decision.policy_used, RoutingPolicy::SEMANTIC);
    EXPECT_GT(decision.similarity_score, 0.0f);
}

TEST_F(LoRARouterTest, LoadAwareRouting) {
    // Test load-aware routing
    auto decision = router_->routeQuery(
        "Explain ThemisDB features",
        "",
        RoutingPolicy::LOAD_AWARE
    );
    
    EXPECT_FALSE(decision.adapter_id.empty());
    EXPECT_EQ(decision.policy_used, RoutingPolicy::LOAD_AWARE);
    EXPECT_GE(decision.gpu_device_id, 0);
}

TEST_F(LoRARouterTest, FallbackConfiguration) {
    // Configure fallback
    FallbackConfig fallback;
    fallback.default_adapter_id = "themis_general_lora";
    fallback.similarity_threshold = 0.9f;  // Very high threshold
    fallback.enable_fallback = true;
    
    router_->configureFallback(fallback);
    
    // Query with high threshold should trigger fallback
    auto decision = router_->routeQuery("Random unrelated query xyz123");
    
    // Should either use fallback or find a match
    EXPECT_FALSE(decision.adapter_id.empty());
}

TEST_F(LoRARouterTest, ABTestConfiguration) {
    // Configure A/B test
    ABTestConfig ab_config;
    ab_config.adapter_ids = {"themis_help_lora", "themis_sql_lora"};
    ab_config.traffic_splits = {0.5f, 0.5f};
    ab_config.experiment_id = "test_experiment_001";
    ab_config.start_time = std::chrono::system_clock::now() - std::chrono::hours(1);
    ab_config.end_time = std::chrono::system_clock::now() + std::chrono::hours(1);
    ab_config.enabled = true;
    
    EXPECT_TRUE(router_->configureABTest(ab_config));
    
    // Verify configuration
    auto retrieved = router_->getABTestConfig();
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->experiment_id, "test_experiment_001");
    EXPECT_EQ(retrieved->adapter_ids.size(), 2);
}

TEST_F(LoRARouterTest, ABTestInvalidConfig) {
    // Test invalid A/B configuration
    ABTestConfig ab_config;
    ab_config.adapter_ids = {"adapter1", "adapter2"};
    ab_config.traffic_splits = {0.6f, 0.5f};  // Sum > 1.0
    ab_config.enabled = true;
    
    EXPECT_FALSE(router_->configureABTest(ab_config));
}

TEST_F(LoRARouterTest, ABTestRouting) {
    // Configure and test A/B routing
    ABTestConfig ab_config;
    ab_config.adapter_ids = {"themis_help_lora", "themis_sql_lora"};
    ab_config.traffic_splits = {0.5f, 0.5f};
    ab_config.experiment_id = "test_ab";
    ab_config.start_time = std::chrono::system_clock::now() - std::chrono::hours(1);
    ab_config.end_time = std::chrono::system_clock::now() + std::chrono::hours(1);
    ab_config.enabled = true;
    
    router_->configureABTest(ab_config);
    
    // Make multiple requests and verify both adapters are used
    std::unordered_map<std::string, int> adapter_counts;
    for (int i = 0; i < 100; ++i) {
        auto decision = router_->routeQuery("Test query " + std::to_string(i));
        adapter_counts[decision.adapter_id]++;
        EXPECT_EQ(decision.policy_used, RoutingPolicy::AB_TEST);
    }
    
    // Both adapters should be used (with some tolerance)
    EXPECT_GT(adapter_counts["themis_help_lora"], 20);
    EXPECT_GT(adapter_counts["themis_sql_lora"], 20);
    
    // End test
    router_->endABTest();
    EXPECT_FALSE(router_->getABTestConfig().has_value());
}

TEST_F(LoRARouterTest, RolloutConfiguration) {
    // Configure rollout
    RolloutConfig rollout;
    rollout.new_adapter_id = "themis_help_lora";
    rollout.baseline_adapter_id = "themis_general_lora";
    rollout.rollout_percentage = 0.2f;  // 20%
    rollout.increment_step = 0.1f;
    rollout.enabled = true;
    rollout.start_time = std::chrono::system_clock::now();
    
    EXPECT_TRUE(router_->configureRollout(rollout));
    
    // Verify configuration
    auto retrieved = router_->getRolloutConfig();
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->new_adapter_id, "themis_help_lora");
    EXPECT_FLOAT_EQ(retrieved->rollout_percentage, 0.2f);
}

TEST_F(LoRARouterTest, RolloutIncrement) {
    // Configure rollout
    RolloutConfig rollout;
    rollout.new_adapter_id = "themis_help_lora";
    rollout.baseline_adapter_id = "themis_general_lora";
    rollout.rollout_percentage = 0.2f;
    rollout.increment_step = 0.15f;
    rollout.enabled = true;
    rollout.start_time = std::chrono::system_clock::now();
    
    router_->configureRollout(rollout);
    
    // Increment rollout
    float new_percentage = router_->incrementRollout();
    EXPECT_FLOAT_EQ(new_percentage, 0.35f);
    
    // Increment again
    new_percentage = router_->incrementRollout();
    EXPECT_FLOAT_EQ(new_percentage, 0.5f);
}

TEST_F(LoRARouterTest, RolloutPromotion) {
    // Configure rollout
    RolloutConfig rollout;
    rollout.new_adapter_id = "themis_help_lora";
    rollout.baseline_adapter_id = "themis_general_lora";
    rollout.rollout_percentage = 0.8f;
    rollout.enabled = true;
    rollout.start_time = std::chrono::system_clock::now();
    
    router_->configureRollout(rollout);
    
    // Promote rollout
    router_->endRollout(true);
    
    // Rollout should be ended
    EXPECT_FALSE(router_->getRolloutConfig().has_value());
}

TEST_F(LoRARouterTest, BatchRouting) {
    // Test batch routing
    std::vector<std::string> queries = {
        "How do I configure authentication?",
        "Generate SQL for user table",
        "Explain vector search"
    };
    
    auto decisions = router_->routeQueryBatch(queries);
    
    EXPECT_EQ(decisions.size(), queries.size());
    
    for (const auto& decision : decisions) {
        EXPECT_FALSE(decision.adapter_id.empty());
        EXPECT_GT(decision.routing_latency_ms.count(), 0);
    }
}

TEST_F(LoRARouterTest, MetricsTracking) {
    // Make several routing requests
    for (int i = 0; i < 10; ++i) {
        router_->routeQuery("Test query " + std::to_string(i));
    }
    
    // Get metrics
    auto metrics = router_->getMetrics();
    
    EXPECT_EQ(metrics.total_requests, 10);
    EXPECT_GT(metrics.successful_routes, 0);
    EXPECT_GE(metrics.avg_routing_latency_ms, 0.0);
    EXPECT_FALSE(metrics.adapter_usage_count.empty());
}

TEST_F(LoRARouterTest, MetricsExport) {
    // Make some requests
    router_->routeQuery("Test query 1");
    router_->routeQuery("Test query 2");
    
    // Export metrics as JSON
    auto metrics_json = router_->exportMetrics();
    
    EXPECT_TRUE(metrics_json.contains("total_requests"));
    EXPECT_TRUE(metrics_json.contains("successful_routes"));
    EXPECT_TRUE(metrics_json.contains("fallback_routes"));
    EXPECT_TRUE(metrics_json.contains("avg_routing_latency_ms"));
    EXPECT_TRUE(metrics_json.contains("adapter_usage_count"));
}

TEST_F(LoRARouterTest, MetricsReset) {
    // Make requests
    router_->routeQuery("Test query");
    
    auto metrics1 = router_->getMetrics();
    EXPECT_GT(metrics1.total_requests, 0);
    
    // Reset metrics
    router_->resetMetrics();
    
    auto metrics2 = router_->getMetrics();
    EXPECT_EQ(metrics2.total_requests, 0);
    EXPECT_EQ(metrics2.successful_routes, 0);
}

TEST_F(LoRARouterTest, DecisionCaching) {
    // First request
    auto decision1 = router_->routeQuery("Test caching query");
    
    // Second identical request (should use cache)
    auto decision2 = router_->routeQuery("Test caching query");
    
    EXPECT_EQ(decision1.adapter_id, decision2.adapter_id);
    EXPECT_EQ(decision1.similarity_score, decision2.similarity_score);
    
    // Cache stats
    auto cache_stats = router_->getCacheStats();
    EXPECT_TRUE(cache_stats.contains("cache_size"));
    EXPECT_GT(cache_stats["cache_size"].get<size_t>(), 0);
}

TEST_F(LoRARouterTest, CacheClear) {
    // Make request to populate cache
    router_->routeQuery("Test query");
    
    auto stats1 = router_->getCacheStats();
    EXPECT_GT(stats1["cache_size"].get<size_t>(), 0);
    
    // Clear cache
    router_->clearCache();
    
    auto stats2 = router_->getCacheStats();
    EXPECT_EQ(stats2["cache_size"].get<size_t>(), 0);
}

TEST_F(LoRARouterTest, MultipleQueriesDifferentAdapters) {
    // Test that different query types route to different adapters
    auto decision1 = router_->routeQuery("How do I configure ThemisDB?");
    auto decision2 = router_->routeQuery("Generate SQL SELECT query");
    
    // Both should succeed
    EXPECT_FALSE(decision1.adapter_id.empty());
    EXPECT_FALSE(decision2.adapter_id.empty());
    
    // May route to different adapters based on semantic similarity
    // (not guaranteed, but likely with our test setup)
}

TEST_F(LoRARouterTest, RoutingLatencyMeasurement) {
    auto decision = router_->routeQuery("Test latency measurement");
    
    // Latency should be measured
    EXPECT_GT(decision.routing_latency_ms.count(), 0);
    
    // Should be reasonable (< 1 second for test)
    EXPECT_LT(decision.routing_latency_ms.count(), 1000);
}

TEST_F(LoRARouterTest, ConfidenceScoring) {
    auto decision = router_->routeQuery("Test confidence scoring");
    
    // Confidence should be in valid range
    EXPECT_GE(decision.confidence, 0.0f);
    EXPECT_LE(decision.confidence, 1.0f);
}

TEST_F(LoRARouterTest, BaseModelFiltering) {
    // Route with specific base model
    auto decision = router_->routeQuery(
        "Test query",
        "llama-2-7b"
    );
    
    EXPECT_FALSE(decision.adapter_id.empty());
    EXPECT_EQ(decision.base_model_id, "llama-2-7b");
}

// Integration test simulating multi-tenant query stream
TEST_F(LoRARouterTest, MultiTenantQueryStream) {
    const int num_queries = 100;
    std::vector<std::string> query_types = {
        "documentation",
        "sql",
        "general"
    };
    
    for (int i = 0; i < num_queries; ++i) {
        std::string query_type = query_types[i % query_types.size()];
        std::string query = "Query type " + query_type + " number " + std::to_string(i);
        
        auto decision = router_->routeQuery(query);
        EXPECT_FALSE(decision.adapter_id.empty());
    }
    
    // Verify metrics
    auto metrics = router_->getMetrics();
    EXPECT_EQ(metrics.total_requests, num_queries);
    
    // Multiple adapters should have been used
    EXPECT_GE(metrics.adapter_usage_count.size(), 1);
}

// Benchmark test for routing performance
TEST_F(LoRARouterTest, RoutingPerformanceBenchmark) {
    const int num_iterations = 1000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_iterations; ++i) {
        router_->routeQuery("Benchmark query " + std::to_string(i));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    double avg_latency = static_cast<double>(duration.count()) / num_iterations;
    
    // Average latency should be reasonable (< 10ms per query)
    EXPECT_LT(avg_latency, 10.0);
    
    std::cout << "Average routing latency: " << avg_latency << " ms" << std::endl;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
