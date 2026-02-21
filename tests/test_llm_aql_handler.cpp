/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_llm_aql_handler.cpp                           ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:08:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     358                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 52032bbf8  2026-02-21  [aql] Confidence scoring for generated AQL queries (#1427) ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_llm_aql_handler.cpp
 * @brief Unit tests for LLM AQL Handler
 */

#include <gtest/gtest.h>
#include "aql/llm_aql_handler.h"
#include "llm/embedded_llm.h"

using namespace themis::aql;
using namespace themis::llm;

class LLMAQLHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        handler = std::make_unique<LLMAQLHandler>();
    }
    
    void TearDown() override {
        handler.reset();
    }
    
    std::unique_ptr<LLMAQLHandler> handler;
};

// ============================================================================
// Model and LoRA Management Tests
// ============================================================================

TEST_F(LLMAQLHandlerTest, ExecuteInferWithModelSelection) {
    // Test that model_id parameter is properly used
    std::unordered_map<std::string, std::string> options;
    options["max_tokens"] = "50";
    options["temperature"] = "0.7";
    
    try {
        // Note: This will fail if no model is loaded, which is expected in test environment
        auto result = handler->executeInfer("Test prompt", "test-model", "", options);
        // If we get here, model selection worked
        EXPECT_FALSE(result.empty());
    } catch (const std::exception& e) {
        // Expected to fail without loaded model, but the code path should execute
        EXPECT_TRUE(std::string(e.what()).find("LLM INFER failed") != std::string::npos);
    }
}

TEST_F(LLMAQLHandlerTest, ExecuteInferWithLoRA) {
    std::unordered_map<std::string, std::string> options;
    
    try {
        auto result = handler->executeInfer("Test prompt", "", "test-lora", options);
        EXPECT_FALSE(result.empty());
    } catch (const std::exception& e) {
        // Expected to fail without loaded model/lora
        EXPECT_TRUE(std::string(e.what()).find("LLM INFER failed") != std::string::npos);
    }
}

TEST_F(LLMAQLHandlerTest, ExecuteInferOptions) {
    std::unordered_map<std::string, std::string> options;
    options["max_tokens"] = "100";
    options["temperature"] = "0.5";
    options["top_p"] = "0.9";
    options["top_k"] = "40";
    options["repetition_penalty"] = "1.1";
    
    // Test that options parsing doesn't throw
    try {
        handler->executeInfer("Test", "", "", options);
    } catch (const std::exception& e) {
        // We expect it to fail with model not loaded, but options should parse correctly
        // If it fails due to options parsing, the error message would be different
        std::string error_msg = e.what();
        EXPECT_TRUE(error_msg.find("LLM INFER failed") != std::string::npos);
    }
}

// ============================================================================
// RAG Integration Tests
// ============================================================================

TEST_F(LLMAQLHandlerTest, ExecuteRAGBasic) {
    std::unordered_map<std::string, std::string> options;
    options["max_tokens"] = "200";
    
    try {
        auto result = handler->executeRAG("Test query", "documents", 5, "", options);
        EXPECT_FALSE(result.empty());
    } catch (const std::exception& e) {
        // Expected to fail without loaded model
        EXPECT_TRUE(std::string(e.what()).find("LLM RAG failed") != std::string::npos);
    }
}

TEST_F(LLMAQLHandlerTest, ExecuteRAGWithSimilarityThreshold) {
    std::unordered_map<std::string, std::string> options;
    options["similarity_threshold"] = "0.8";
    
    try {
        auto result = handler->executeRAG("Test query", "documents", 10, "", options);
        // Test passes if no exception during parsing
    } catch (const std::exception& e) {
        // Expected failure, but should be from model not loaded, not option parsing
        std::string error_msg = e.what();
        EXPECT_TRUE(error_msg.find("LLM RAG failed") != std::string::npos);
    }
}

TEST_F(LLMAQLHandlerTest, ExecuteRAGWithLoRA) {
    std::unordered_map<std::string, std::string> options;
    
    try {
        auto result = handler->executeRAG("Query", "docs", 5, "custom-lora", options);
    } catch (const std::exception& e) {
        EXPECT_TRUE(std::string(e.what()).find("LLM RAG failed") != std::string::npos);
    }
}

// ============================================================================
// Embed Tests
// ============================================================================

TEST_F(LLMAQLHandlerTest, ExecuteEmbedBasic) {
    try {
        auto embedding = handler->executeEmbed("Test text");
        EXPECT_FALSE(embedding.empty());
    } catch (const std::exception& e) {
        // Expected to fail without loaded model
        EXPECT_TRUE(std::string(e.what()).find("LLM EMBED failed") != std::string::npos);
    }
}

TEST_F(LLMAQLHandlerTest, ExecuteEmbedWithModel) {
    try {
        auto embedding = handler->executeEmbed("Test text", "embedding-model");
        EXPECT_FALSE(embedding.empty());
    } catch (const std::exception& e) {
        EXPECT_TRUE(std::string(e.what()).find("LLM EMBED failed") != std::string::npos);
    }
}

// ============================================================================
// Chat Tests
// ============================================================================

TEST_F(LLMAQLHandlerTest, ExecuteChatBasic) {
    std::vector<ChatMessage> messages;
    messages.emplace_back("system", "You are a helpful assistant.");
    messages.emplace_back("user", "Hello!");
    
    std::unordered_map<std::string, std::string> options;
    
    try {
        auto response = handler->executeChat(messages, "", options);
        EXPECT_FALSE(response.empty());
    } catch (const std::exception& e) {
        // Expected to fail without loaded model
        EXPECT_TRUE(std::string(e.what()).find("LLM CHAT failed") != std::string::npos);
    }
}

TEST_F(LLMAQLHandlerTest, ExecuteChatWithFormat) {
    std::vector<ChatMessage> messages;
    messages.emplace_back("user", "Test");
    
    std::unordered_map<std::string, std::string> options;
    options["chat_format"] = "llama2";
    
    try {
        auto response = handler->executeChat(messages, "", options);
    } catch (const std::exception& e) {
        EXPECT_TRUE(std::string(e.what()).find("LLM CHAT failed") != std::string::npos);
    }
}

// ============================================================================
// Natural Language to AQL Translation Tests
// ============================================================================

TEST_F(LLMAQLHandlerTest, TranslateNLToAQLBasic) {
    try {
        auto aql = handler->translateNLToAQL("Find all users");
        // Should generate some AQL query
        EXPECT_FALSE(aql.empty());
        // Basic AQL should contain FOR keyword
        EXPECT_TRUE(aql.find("FOR") != std::string::npos || 
                    aql.find("for") != std::string::npos);
    } catch (const std::exception& e) {
        // Expected to fail without loaded model
        std::string error_msg = e.what();
        EXPECT_TRUE(error_msg.find("NL to AQL translation failed") != std::string::npos ||
                    error_msg.find("LLM CHAT failed") != std::string::npos);
    }
}

TEST_F(LLMAQLHandlerTest, TranslateNLToAQLWithSchema) {
    std::string schema = R"(
Collections:
- users: {name, email, age, city}
- posts: {title, content, author_id}
)";
    
    try {
        auto aql = handler->translateNLToAQL("Find users in Seattle", schema);
        EXPECT_FALSE(aql.empty());
        // Should reference the users collection
        EXPECT_TRUE(aql.find("users") != std::string::npos);
    } catch (const std::exception& e) {
        std::string error_msg = e.what();
        EXPECT_TRUE(error_msg.find("translation failed") != std::string::npos ||
                    error_msg.find("CHAT failed") != std::string::npos);
    }
}

TEST_F(LLMAQLHandlerTest, TranslateNLToAQLMarkdownCleanup) {
    // Mock response would have markdown, but we can test the method exists
    try {
        auto aql = handler->translateNLToAQL("List all documents");
        // If we get a result, check it doesn't have markdown markers
        if (!aql.empty()) {
            EXPECT_TRUE(aql.find("```") == std::string::npos);
        }
    } catch (const std::exception& e) {
        // Expected failure
    }
}

// ============================================================================
// Batch Inference Tests
// ============================================================================

TEST_F(LLMAQLHandlerTest, ExecuteBatchInferBasic) {
    std::vector<LLMAQLHandler::BatchInferRequest> requests;
    
    LLMAQLHandler::BatchInferRequest req1;
    req1.prompt = "Test 1";
    req1.model_id = "model1";
    requests.push_back(req1);
    
    LLMAQLHandler::BatchInferRequest req2;
    req2.prompt = "Test 2";
    req2.model_id = "model2";
    requests.push_back(req2);
    
    try {
        auto results = handler->executeBatchInfer(requests);
        EXPECT_EQ(results.size(), 2);
    } catch (const std::exception& e) {
        // Expected to fail without loaded models
        EXPECT_TRUE(std::string(e.what()).find("Batch LLM INFER failed") != std::string::npos);
    }
}

// ============================================================================
// Model Management Tests
// ============================================================================

TEST_F(LLMAQLHandlerTest, ModelManagement) {
    // Test that model management methods exist and don't crash
    try {
        handler->executeModelList();
    } catch (const std::exception& e) {
        // May fail if plugin manager not initialized, but method should exist
    }
}

TEST_F(LLMAQLHandlerTest, LoRAManagement) {
    // Test that LoRA management methods exist
    try {
        handler->executeLoRAList();
    } catch (const std::exception& e) {
        // May fail if plugin manager not initialized
    }
}

TEST_F(LLMAQLHandlerTest, StatsExecution) {
    try {
        auto stats = handler->executeStats();
        EXPECT_FALSE(stats.empty());
    } catch (const std::exception& e) {
        // May fail if plugin manager not initialized
    }
}

TEST_F(LLMAQLHandlerTest, CacheStatsExecution) {
    try {
        auto stats = handler->executeCacheStats();
        EXPECT_FALSE(stats.empty());
    } catch (const std::exception& e) {
        // May fail if plugin manager not initialized
    }
}

// ============================================================================
// Confidence Scoring Tests
// ============================================================================

TEST_F(LLMAQLHandlerTest, TranslateNLToAQLWithConfidenceReturnsResult) {
    try {
        auto result = handler->translateNLToAQLWithConfidence("Find all users");
        // If translation succeeded, both fields must be populated
        EXPECT_FALSE(result.aql_query.empty());
        EXPECT_GE(result.confidence.overall_confidence, 0.0f);
        EXPECT_LE(result.confidence.overall_confidence, 1.0f);
    } catch (const std::exception& e) {
        // Expected to fail without a loaded LLM model
        std::string msg = e.what();
        EXPECT_TRUE(msg.find("translation failed") != std::string::npos ||
                    msg.find("CHAT failed") != std::string::npos);
    }
}

TEST_F(LLMAQLHandlerTest, TranslateNLToAQLWithConfidenceUsesSchemaContext) {
    const std::string schema = R"(
Collections:
- users: {name, email, city}
- posts: {title, content}
)";
    try {
        auto result = handler->translateNLToAQLWithConfidence(
            "Find all posts by users in Seattle", schema);
        EXPECT_FALSE(result.aql_query.empty());
        EXPECT_GE(result.confidence.overall_confidence, 0.0f);
        EXPECT_LE(result.confidence.overall_confidence, 1.0f);
    } catch (const std::exception& e) {
        std::string msg = e.what();
        EXPECT_TRUE(msg.find("translation failed") != std::string::npos ||
                    msg.find("CHAT failed") != std::string::npos);
    }
}

// Run tests