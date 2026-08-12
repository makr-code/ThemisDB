/**
 * @file test_lora_aql_functions.cpp
 * @brief Unit tests for LoRA AQL functions
 * 
 * Tests all 7 LoRA AQL functions:
 * - LORA_TRAIN: Train adapters from datasets
 * - LORA_QUERY: Execute inference with LoRA adapters
 * - LORA_SIMILAR: Find similar adapters
 * - LORA_PATH: Graph traversal for adaptation paths
 * - LORA_STATS: Get adapter statistics
 * - LORA_RECOMMEND: Recommend best adapter
 * - LORA_LINEAGE: Get adapter version history
 */

#include <gtest/gtest.h>
#include "query/functions/lora_functions.h"
#include "query/functions/function_registry.h"
#include <nlohmann/json.hpp>

using namespace themis::query::functions;
using json = nlohmann::json;

// ============================================================================
// Test Fixtures
// ============================================================================

class LoRAFunctionsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Get function registry and register LoRA functions
        registry_ = &FunctionRegistry::instance();
        registerLoRAFunctions(*registry_);
        
        // Create function context
        context_ = FunctionContext();
    }
    
    void TearDown() override {
        // Cleanup if needed
    }
    
    FunctionRegistry* registry_;
    FunctionContext context_;
};

// ============================================================================
// LORA_TRAIN Tests
// ============================================================================

TEST_F(LoRAFunctionsTest, LoraTrainBasic) {
    // Test basic training invocation
    std::vector<json> args;
    args.push_back("test_adapter");
    args.push_back("llama-2-7b");
    
    json dataset;
    dataset["task"] = "documentation_qa";
    dataset["samples"] = json::array();
    json sample;
    sample["input"] = "What is ThemisDB?";
    sample["output"] = "ThemisDB is a database system.";
    dataset["samples"].push_back(sample);
    args.push_back(dataset);
    
    json config;
    config["rank"] = 8;
    config["alpha"] = 16;
    config["learning_rate"] = 0.0003;
    config["epochs"] = 3;
    args.push_back(config);
    
    // Execute function
    ASSERT_TRUE(registry_->hasFunction("LORA_TRAIN"));
    auto result = registry_->call("LORA_TRAIN", args, context_);
    
    // Verify result structure
    ASSERT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("adapter_id"));
    EXPECT_TRUE(result.contains("status"));
    EXPECT_TRUE(result.contains("job_id"));
    
    // Verify values
    EXPECT_EQ(result["adapter_id"], "test_adapter");
    EXPECT_EQ(result["status"], "training");
}

TEST_F(LoRAFunctionsTest, LoraTrainMinimalArgs) {
    // Test with minimal arguments (no config)
    std::vector<json> args;
    args.push_back("minimal_adapter");
    args.push_back("llama-2-7b");
    
    json dataset;
    dataset["task"] = "qa";
    dataset["samples"] = json::array();
    args.push_back(dataset);
    
    auto result = registry_->call("LORA_TRAIN", args, context_);
    
    ASSERT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("adapter_id"));
}

TEST_F(LoRAFunctionsTest, LoraTrainInvalidArgs) {
    // Test with insufficient arguments
    std::vector<json> args;
    args.push_back("test_adapter");
    
    EXPECT_THROW(
        registry_->call("LORA_TRAIN", args, context_),
        std::runtime_error
    );
}

// ============================================================================
// LORA_QUERY Tests
// ============================================================================

TEST_F(LoRAFunctionsTest, LoraQueryBasic) {
    // Test basic query invocation
    std::vector<json> args;
    args.push_back("llama-2-7b");
    args.push_back("test_adapter");
    args.push_back("What is ThemisDB?");
    
    json options;
    options["max_tokens"] = 500;
    options["temperature"] = 0.7;
    args.push_back(options);
    
    ASSERT_TRUE(registry_->hasFunction("LORA_QUERY"));
    auto result = registry_->call("LORA_QUERY", args, context_);
    
    // Result should be a string (response text or error message)
    ASSERT_TRUE(result.is_string());
}

TEST_F(LoRAFunctionsTest, LoraQueryMinimalArgs) {
    // Test with minimal arguments (no options)
    std::vector<json> args;
    args.push_back("llama-2-7b");
    args.push_back("test_adapter");
    args.push_back("Tell me about databases");
    
    auto result = registry_->call("LORA_QUERY", args, context_);
    
    ASSERT_TRUE(result.is_string());
}

// ============================================================================
// LORA_SIMILAR Tests
// ============================================================================

TEST_F(LoRAFunctionsTest, LoraSimilarBasic) {
    // Test finding similar adapters
    std::vector<json> args;
    args.push_back("test_adapter");
    args.push_back(5);  // k
    args.push_back(0.85);  // threshold
    
    ASSERT_TRUE(registry_->hasFunction("LORA_SIMILAR"));
    auto result = registry_->call("LORA_SIMILAR", args, context_);
    
    // Result should be an array
    ASSERT_TRUE(result.is_array());
}

TEST_F(LoRAFunctionsTest, LoraSimilarNoThreshold) {
    // Test without threshold (optional parameter)
    std::vector<json> args;
    args.push_back("test_adapter");
    args.push_back(3);
    
    auto result = registry_->call("LORA_SIMILAR", args, context_);
    
    ASSERT_TRUE(result.is_array());
}

TEST_F(LoRAFunctionsTest, LoraSimilarResultFormat) {
    // Verify result format
    std::vector<json> args;
    args.push_back("test_adapter");
    args.push_back(5);
    args.push_back(0.8);
    
    auto result = registry_->call("LORA_SIMILAR", args, context_);
    
    ASSERT_TRUE(result.is_array());
    // Each result should have required fields
    for (const auto& item : result) {
        if (item.is_object()) {
            EXPECT_TRUE(item.contains("adapter_id"));
            EXPECT_TRUE(item.contains("score"));
        }
    }
}

// ============================================================================
// LORA_PATH Tests
// ============================================================================

TEST_F(LoRAFunctionsTest, LoraPathBasic) {
    // Test path finding
    std::vector<json> args;
    args.push_back("llama-2-7b");
    args.push_back("llama-2-13b");
    args.push_back(3);  // max_depth
    
    ASSERT_TRUE(registry_->hasFunction("LORA_PATH"));
    auto result = registry_->call("LORA_PATH", args, context_);
    
    // Result should be an array representing the path
    ASSERT_TRUE(result.is_array());
}

TEST_F(LoRAFunctionsTest, LoraPathDefaultDepth) {
    // Test with default max_depth
    std::vector<json> args;
    args.push_back("llama-2-7b");
    args.push_back("llama-2-13b");
    
    auto result = registry_->call("LORA_PATH", args, context_);
    
    ASSERT_TRUE(result.is_array());
}

TEST_F(LoRAFunctionsTest, LoraPathResultFormat) {
    // Verify path result format
    std::vector<json> args;
    args.push_back("llama-2-7b");
    args.push_back("llama-2-13b");
    args.push_back(3);
    
    auto result = registry_->call("LORA_PATH", args, context_);
    
    ASSERT_TRUE(result.is_array());
    // Each step should have required fields
    for (const auto& step : result) {
        if (step.is_object()) {
            EXPECT_TRUE(step.contains("node"));
            EXPECT_TRUE(step.contains("type"));
        }
    }
}

// ============================================================================
// LORA_STATS Tests
// ============================================================================

TEST_F(LoRAFunctionsTest, LoraStatsBasic) {
    // Test getting stats with specific metrics
    std::vector<json> args;
    args.push_back("test_adapter");
    
    json metrics = json::array();
    metrics.push_back("validation_accuracy");
    metrics.push_back("inference_count");
    metrics.push_back("avg_latency");
    args.push_back(metrics);
    
    ASSERT_TRUE(registry_->hasFunction("LORA_STATS"));
    auto result = registry_->call("LORA_STATS", args, context_);
    
    // Result should be an object with stats
    ASSERT_TRUE(result.is_object());
}

TEST_F(LoRAFunctionsTest, LoraStatsAllMetrics) {
    // Test getting all metrics (no specific metrics requested)
    std::vector<json> args;
    args.push_back("test_adapter");
    
    auto result = registry_->call("LORA_STATS", args, context_);
    
    ASSERT_TRUE(result.is_object());
}

TEST_F(LoRAFunctionsTest, LoraStatsResultFormat) {
    // Verify stats result format
    std::vector<json> args;
    args.push_back("test_adapter");
    
    json metrics = json::array();
    metrics.push_back("validation_accuracy");
    args.push_back(metrics);
    
    auto result = registry_->call("LORA_STATS", args, context_);
    
    ASSERT_TRUE(result.is_object());
    // Should contain requested metric
    if (!result.contains("error")) {
        EXPECT_TRUE(result.contains("validation_accuracy"));
    }
}

// ============================================================================
// LORA_RECOMMEND Tests
// ============================================================================

TEST_F(LoRAFunctionsTest, LoraRecommendBasic) {
    // Test recommendation
    std::vector<json> args;
    args.push_back("How do I configure replication?");
    args.push_back("llama-2-7b");
    args.push_back("documentation_qa");
    
    json options;
    options["min_accuracy"] = 0.85;
    options["max_latency_ms"] = 100;
    args.push_back(options);
    
    ASSERT_TRUE(registry_->hasFunction("LORA_RECOMMEND"));
    auto result = registry_->call("LORA_RECOMMEND", args, context_);
    
    // Result should be an object with recommendation
    ASSERT_TRUE(result.is_object());
}

TEST_F(LoRAFunctionsTest, LoraRecommendNoOptions) {
    // Test without options
    std::vector<json> args;
    args.push_back("Database query");
    args.push_back("llama-2-7b");
    args.push_back("general");
    
    auto result = registry_->call("LORA_RECOMMEND", args, context_);
    
    ASSERT_TRUE(result.is_object());
}

TEST_F(LoRAFunctionsTest, LoraRecommendResultFormat) {
    // Verify recommendation result format
    std::vector<json> args;
    args.push_back("Test query");
    args.push_back("llama-2-7b");
    args.push_back("qa");
    args.push_back(json::object());
    
    auto result = registry_->call("LORA_RECOMMEND", args, context_);
    
    ASSERT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("adapter_id"));
    EXPECT_TRUE(result.contains("confidence"));
    EXPECT_TRUE(result.contains("reason"));
}

// ============================================================================
// LORA_LINEAGE Tests
// ============================================================================

TEST_F(LoRAFunctionsTest, LoraLineageBasic) {
    // Test getting lineage
    std::vector<json> args;
    args.push_back("test_adapter");
    args.push_back(10);  // depth
    
    ASSERT_TRUE(registry_->hasFunction("LORA_LINEAGE"));
    auto result = registry_->call("LORA_LINEAGE", args, context_);
    
    // Result should be an array of versions
    ASSERT_TRUE(result.is_array());
}

TEST_F(LoRAFunctionsTest, LoraLineageDefaultDepth) {
    // Test with default depth
    std::vector<json> args;
    args.push_back("test_adapter");
    
    auto result = registry_->call("LORA_LINEAGE", args, context_);
    
    ASSERT_TRUE(result.is_array());
}

TEST_F(LoRAFunctionsTest, LoraLineageResultFormat) {
    // Verify lineage result format
    std::vector<json> args;
    args.push_back("test_adapter");
    args.push_back(5);
    
    auto result = registry_->call("LORA_LINEAGE", args, context_);
    
    ASSERT_TRUE(result.is_array());
    // Each version should have required fields
    for (const auto& version : result) {
        if (version.is_object()) {
            EXPECT_TRUE(version.contains("version"));
            EXPECT_TRUE(version.contains("created"));
        }
    }
}

// ============================================================================
// Function Signature Tests
// ============================================================================

TEST_F(LoRAFunctionsTest, AllFunctionsRegistered) {
    // Verify all 7 LoRA functions are registered
    EXPECT_TRUE(registry_->hasFunction("LORA_TRAIN"));
    EXPECT_TRUE(registry_->hasFunction("LORA_QUERY"));
    EXPECT_TRUE(registry_->hasFunction("LORA_SIMILAR"));
    EXPECT_TRUE(registry_->hasFunction("LORA_PATH"));
    EXPECT_TRUE(registry_->hasFunction("LORA_STATS"));
    EXPECT_TRUE(registry_->hasFunction("LORA_RECOMMEND"));
    EXPECT_TRUE(registry_->hasFunction("LORA_LINEAGE"));
}

TEST_F(LoRAFunctionsTest, FunctionSignatures) {
    // Verify function signatures are correct
    auto& train_func = registry_->getFunction("LORA_TRAIN");
    auto train_sig = train_func.signature();
    EXPECT_EQ(train_sig.name, "LORA_TRAIN");
    EXPECT_EQ(train_sig.category, "LoRA");
    EXPECT_GE(train_sig.arguments.size(), 3);
    
    auto& query_func = registry_->getFunction("LORA_QUERY");
    auto query_sig = query_func.signature();
    EXPECT_EQ(query_sig.name, "LORA_QUERY");
    EXPECT_EQ(query_sig.category, "LoRA");
    EXPECT_GE(query_sig.arguments.size(), 3);
}

TEST_F(LoRAFunctionsTest, FunctionCategories) {
    // Verify LoRA category exists
    auto categories = registry_->getCategories();
    EXPECT_TRUE(
        std::find(categories.begin(), categories.end(), "LoRA") != categories.end()
    );
    
    // Get all LoRA functions
    auto lora_funcs = registry_->getByCategory("LoRA");
    EXPECT_EQ(lora_funcs.size(), 7);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(LoRAFunctionsTest, TrainAndQuery) {
    // Test training followed by query
    // 1. Train adapter
    std::vector<json> train_args;
    train_args.push_back("integration_adapter");
    train_args.push_back("llama-2-7b");
    
    json dataset;
    dataset["task"] = "qa";
    dataset["samples"] = json::array();
    train_args.push_back(dataset);
    train_args.push_back(json::object());
    
    auto train_result = registry_->call("LORA_TRAIN", train_args, context_);
    ASSERT_TRUE(train_result.is_object());
    
    // 2. Query with adapter (might fail if not actually trained, but should not crash)
    std::vector<json> query_args;
    query_args.push_back("llama-2-7b");
    query_args.push_back("integration_adapter");
    query_args.push_back("Test query");
    
    auto query_result = registry_->call("LORA_QUERY", query_args, context_);
    ASSERT_TRUE(query_result.is_string());
}

TEST_F(LoRAFunctionsTest, RecommendAndQuery) {
    // Test recommendation followed by query
    // 1. Get recommendation
    std::vector<json> recommend_args;
    recommend_args.push_back("Database operations");
    recommend_args.push_back("llama-2-7b");
    recommend_args.push_back("general");
    recommend_args.push_back(json::object());
    
    auto recommend_result = registry_->call("LORA_RECOMMEND", recommend_args, context_);
    ASSERT_TRUE(recommend_result.is_object());
    
    // 2. Use recommended adapter (if available)
    auto hasValidAdapter = [](const json& result) {
        return result.contains("adapter_id") && !result["adapter_id"].is_null();
    };
    
    if (hasValidAdapter(recommend_result)) {
        std::vector<json> query_args;
        query_args.push_back("llama-2-7b");
        query_args.push_back(recommend_result["adapter_id"]);
        query_args.push_back("Follow-up query");
        
        auto query_result = registry_->call("LORA_QUERY", query_args, context_);
        ASSERT_TRUE(query_result.is_string());
    }
}

// ============================================================================
// Main
// ============================================================================

// ============================================================================
// Provenance AQL Function Tests
// ============================================================================

TEST_F(LoRAFunctionsTest, LoraProvenance_SignatureIsCorrect) {
    LoraProvenanceFunction fn;
    auto sig = fn.signature();
    EXPECT_EQ(sig.name,        "LORA_PROVENANCE");
    EXPECT_EQ(sig.category,    "LoRA");
    EXPECT_EQ(sig.return_type, ArgType::OBJECT);
    ASSERT_EQ(sig.arguments.size(), 1u);
    EXPECT_EQ(sig.arguments[0].name, "adapter_id");
    EXPECT_TRUE(sig.arguments[0].required);
}

TEST_F(LoRAFunctionsTest, LoraProvenance_UnknownAdapterReturnsNull) {
    std::vector<json> args;
    args.push_back("no-such-adapter");

    auto result = registry_->call("LORA_PROVENANCE", args, context_);
    EXPECT_TRUE(result.is_null());
}

TEST_F(LoRAFunctionsTest, LoraProvenance_IsRegistered) {
    EXPECT_TRUE(registry_->hasFunction("LORA_PROVENANCE"));
}

// ============================================================================

TEST_F(LoRAFunctionsTest, LoraAuditLog_SignatureIsCorrect) {
    LoraAuditLogFunction fn;
    auto sig = fn.signature();
    EXPECT_EQ(sig.name,        "LORA_AUDIT_LOG");
    EXPECT_EQ(sig.category,    "LoRA");
    EXPECT_EQ(sig.return_type, ArgType::ARRAY);
    ASSERT_EQ(sig.arguments.size(), 2u);
    EXPECT_EQ(sig.arguments[0].name, "adapter_id");
    EXPECT_TRUE(sig.arguments[0].required);
    EXPECT_EQ(sig.arguments[1].name, "limit");
    EXPECT_FALSE(sig.arguments[1].required);
}

TEST_F(LoRAFunctionsTest, LoraAuditLog_EmptyLogReturnsArray) {
    std::vector<json> args;
    args.push_back("empty-adapter");
    args.push_back(50);

    auto result = registry_->call("LORA_AUDIT_LOG", args, context_);
    ASSERT_TRUE(result.is_array());
}

TEST_F(LoRAFunctionsTest, LoraAuditLog_IsRegistered) {
    EXPECT_TRUE(registry_->hasFunction("LORA_AUDIT_LOG"));
}

// ============================================================================

TEST_F(LoRAFunctionsTest, LoraSnapshots_SignatureIsCorrect) {
    LoraSnapshotsFunction fn;
    auto sig = fn.signature();
    EXPECT_EQ(sig.name,        "LORA_SNAPSHOTS");
    EXPECT_EQ(sig.category,    "LoRA");
    EXPECT_EQ(sig.return_type, ArgType::ARRAY);
    ASSERT_EQ(sig.arguments.size(), 1u);
    EXPECT_EQ(sig.arguments[0].name, "adapter_id");
}

TEST_F(LoRAFunctionsTest, LoraSnapshots_NoSnapshotsReturnsEmptyArray) {
    std::vector<json> args;
    args.push_back("no-snap-adapter");

    auto result = registry_->call("LORA_SNAPSHOTS", args, context_);
    ASSERT_TRUE(result.is_array());
    EXPECT_TRUE(result.empty());
}

TEST_F(LoRAFunctionsTest, LoraSnapshots_IsRegistered) {
    EXPECT_TRUE(registry_->hasFunction("LORA_SNAPSHOTS"));
}

// ============================================================================

TEST_F(LoRAFunctionsTest, LoraVerifyChain_SignatureIsCorrect) {
    LoraVerifyChainFunction fn;
    auto sig = fn.signature();
    EXPECT_EQ(sig.name,        "LORA_VERIFY_CHAIN");
    EXPECT_EQ(sig.category,    "LoRA");
    EXPECT_EQ(sig.return_type, ArgType::OBJECT);
    ASSERT_EQ(sig.arguments.size(), 1u);
    EXPECT_EQ(sig.arguments[0].name, "adapter_id");
    EXPECT_FALSE(sig.is_deterministic);  // depends on live state
}

TEST_F(LoRAFunctionsTest, LoraVerifyChain_EmptyChainIsValid) {
    std::vector<json> args;
    args.push_back("empty-audit-adapter");

    auto result = registry_->call("LORA_VERIFY_CHAIN", args, context_);
    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("chain_valid"));
    EXPECT_TRUE(result["chain_valid"].get<bool>());
    ASSERT_TRUE(result.contains("entry_count"));
    EXPECT_EQ(result["entry_count"].get<int>(), 0);
    ASSERT_TRUE(result.contains("message"));
}

TEST_F(LoRAFunctionsTest, LoraVerifyChain_IsRegistered) {
    EXPECT_TRUE(registry_->hasFunction("LORA_VERIFY_CHAIN"));
}

TEST_F(LoRAFunctionsTest, AllProvenanceFunctionsRegistered) {
    EXPECT_TRUE(registry_->hasFunction("LORA_PROVENANCE"));
    EXPECT_TRUE(registry_->hasFunction("LORA_AUDIT_LOG"));
    EXPECT_TRUE(registry_->hasFunction("LORA_SNAPSHOTS"));
    EXPECT_TRUE(registry_->hasFunction("LORA_VERIFY_CHAIN"));
}

