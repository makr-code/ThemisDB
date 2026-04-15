/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_lora_router.cpp                               ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:19:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     184                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "llm/lora_router.h"
#include <nlohmann/json.hpp>

// Use specific types instead of namespace-wide using
using themis::llm::ABTestConfig;
using themis::llm::RolloutConfig;
using themis::llm::FallbackConfig;
using themis::llm::RoutingDecision;
using themis::llm::RoutingMetrics;
using themis::llm::RoutingPolicy;
using themis::llm::LoRARouter;

/**
 * LoRA Router Unit Tests
 * 
 * These tests validate the data structures, configurations, and policy types.
 * Full end-to-end routing tests require real EmbeddingProvider with llama.cpp model,
 * which should be performed in integration testing environment.
 */

TEST(LoRARouterStructures, ABTestConfigValidation) {
    ABTestConfig config;
    config.adapter_ids = {"adapter1", "adapter2"};
    config.traffic_splits = {0.5f, 0.5f};
    config.experiment_id = "test_001";
    config.enabled = true;
    
    // Verify structure
    EXPECT_EQ(config.adapter_ids.size(), 2);
    EXPECT_EQ(config.traffic_splits.size(), 2);
    EXPECT_FLOAT_EQ(config.traffic_splits[0] + config.traffic_splits[1], 1.0f);
}

TEST(LoRARouterStructures, RolloutConfigValidation) {
    RolloutConfig config;
    config.new_adapter_id = "adapter_v2";
    config.baseline_adapter_id = "adapter_v1";
    config.rollout_percentage = 0.2f;
    config.increment_step = 0.1f;
    config.enabled = true;
    
    // Verify structure
    EXPECT_EQ(config.new_adapter_id, "adapter_v2");
    EXPECT_EQ(config.baseline_adapter_id, "adapter_v1");
    EXPECT_FLOAT_EQ(config.rollout_percentage, 0.2f);
    EXPECT_FLOAT_EQ(config.increment_step, 0.1f);
    EXPECT_TRUE(config.enabled);
}

TEST(LoRARouterStructures, FallbackConfigValidation) {
    FallbackConfig config;
    config.default_adapter_id = "general_adapter";
    config.similarity_threshold = 0.5f;
    config.enable_fallback = true;
    
    // Verify structure
    EXPECT_EQ(config.default_adapter_id, "general_adapter");
    EXPECT_FLOAT_EQ(config.similarity_threshold, 0.5f);
    EXPECT_TRUE(config.enable_fallback);
}

TEST(LoRARouterStructures, RoutingDecisionStructure) {
    RoutingDecision decision;
    decision.adapter_id = "test_adapter";
    decision.base_model_id = "llama-2-7b";
    decision.gpu_device_id = 0;
    decision.similarity_score = 0.85f;
    decision.confidence = 0.9f;
    decision.policy_used = RoutingPolicy::SEMANTIC;
    decision.is_fallback = false;
    decision.reason = "Test decision";
    decision.routing_latency_ms = std::chrono::milliseconds(5);
    
    // Verify structure
    EXPECT_EQ(decision.adapter_id, "test_adapter");
    EXPECT_EQ(decision.base_model_id, "llama-2-7b");
    EXPECT_EQ(decision.gpu_device_id, 0);
    EXPECT_FLOAT_EQ(decision.similarity_score, 0.85f);
    EXPECT_FLOAT_EQ(decision.confidence, 0.9f);
    EXPECT_EQ(decision.policy_used, RoutingPolicy::SEMANTIC);
    EXPECT_FALSE(decision.is_fallback);
    EXPECT_EQ(decision.routing_latency_ms.count(), 5);
}

TEST(LoRARouterStructures, RoutingMetricsStructure) {
    RoutingMetrics metrics;
    metrics.total_requests = 100;
    metrics.successful_routes = 90;
    metrics.fallback_routes = 10;
    metrics.adapter_usage_count["adapter1"] = 50;
    metrics.adapter_usage_count["adapter2"] = 40;
    metrics.adapter_avg_similarity["adapter1"] = 0.8;
    metrics.adapter_avg_similarity["adapter2"] = 0.75;
    metrics.avg_routing_latency_ms = 3.5;
    metrics.avg_similarity_score = 0.78;
    
    // Verify structure
    EXPECT_EQ(metrics.total_requests, 100);
    EXPECT_EQ(metrics.successful_routes, 90);
    EXPECT_EQ(metrics.fallback_routes, 10);
    EXPECT_EQ(metrics.adapter_usage_count.size(), 2);
    EXPECT_EQ(metrics.adapter_usage_count["adapter1"], 50);
    
    // Test JSON export
    auto json = metrics.toJson();
    EXPECT_TRUE(json.contains("total_requests"));
    EXPECT_TRUE(json.contains("successful_routes"));
    EXPECT_TRUE(json.contains("fallback_routes"));
    EXPECT_TRUE(json.contains("fallback_rate"));
    EXPECT_TRUE(json.contains("adapter_usage_count"));
    EXPECT_TRUE(json.contains("adapter_avg_similarity"));
    
    EXPECT_EQ(json["total_requests"].get<size_t>(), 100);
    EXPECT_EQ(json["successful_routes"].get<size_t>(), 90);
    EXPECT_EQ(json["fallback_routes"].get<size_t>(), 10);
    EXPECT_NEAR(json["fallback_rate"].get<double>(), 0.1, 0.01);
}

TEST(LoRARouterStructures, RouterConfigValidation) {
    LoRARouter::Config config;
    config.enable_semantic_routing = true;
    config.enable_load_aware = true;
    config.top_k_candidates = 5;
    config.min_similarity_threshold = 0.3f;
    config.load_weight = 0.3f;
    config.default_policy = RoutingPolicy::LOAD_AWARE;
    config.enable_metrics = true;
    config.metrics_window_size = 1000;
    config.enable_decision_cache = true;
    config.decision_cache_size = 500;
    config.decision_cache_ttl = std::chrono::seconds(300);
    
    // Verify all fields
    EXPECT_TRUE(config.enable_semantic_routing);
    EXPECT_TRUE(config.enable_load_aware);
    EXPECT_EQ(config.top_k_candidates, 5);
    EXPECT_FLOAT_EQ(config.min_similarity_threshold, 0.3f);
    EXPECT_FLOAT_EQ(config.load_weight, 0.3f);
    EXPECT_EQ(config.default_policy, RoutingPolicy::LOAD_AWARE);
    EXPECT_TRUE(config.enable_metrics);
    EXPECT_EQ(config.metrics_window_size, 1000);
    EXPECT_TRUE(config.enable_decision_cache);
    EXPECT_EQ(config.decision_cache_size, 500);
    EXPECT_EQ(config.decision_cache_ttl.count(), 300);
}

TEST(LoRARouterStructures, RoutingPolicyEnum) {
    // Test routing policy enum values
    EXPECT_NE(RoutingPolicy::SEMANTIC, RoutingPolicy::LOAD_AWARE);
    EXPECT_NE(RoutingPolicy::AB_TEST, RoutingPolicy::ROLLOUT);
    EXPECT_NE(RoutingPolicy::FALLBACK, RoutingPolicy::SEMANTIC);
    
    // Can be assigned
    RoutingPolicy policy = RoutingPolicy::SEMANTIC;
    EXPECT_EQ(policy, RoutingPolicy::SEMANTIC);
    
    policy = RoutingPolicy::LOAD_AWARE;
    EXPECT_EQ(policy, RoutingPolicy::LOAD_AWARE);
}


