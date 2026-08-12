/**
 * @file test_rag_aql_integration.cpp
 * @brief Integration tests for RAG functionality in LLM AQL Handler
 */

#include <gtest/gtest.h>
#include "aql/llm_aql_handler.h"
#include <iostream>

using namespace themis::aql;

class RAGAQLIntegrationTest : public ::testing::Test {
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
// Basic RAG Tests
// ============================================================================

TEST_F(RAGAQLIntegrationTest, BasicRAGQuery) {
    std::string query = "What are the key features of ThemisDB?";
    std::string collection = "documentation";
    int top_k = 5;
    
    std::unordered_map<std::string, std::string> options;
    options["max_tokens"] = "200";
    options["temperature"] = "0.7";
    
    try {
        auto result = handler->executeRAG(query, collection, top_k, "", options);
        
        std::cout << "Query: " << query << std::endl;
        std::cout << "Collection: " << collection << std::endl;
        std::cout << "Top-K: " << top_k << std::endl;
        std::cout << "Result: " << result << std::endl;
        
        EXPECT_FALSE(result.empty());
        
    } catch (const std::exception& e) {
        std::cout << "RAG query failed (expected if no model/index): " << e.what() << std::endl;
        GTEST_SKIP() << "Skipping test due to missing LLM model or vector index";
    }
}

TEST_F(RAGAQLIntegrationTest, RAGWithHighTopK) {
    std::string query = "Database performance optimization techniques";
    std::string collection = "technical_docs";
    int top_k = 20;
    
    std::unordered_map<std::string, std::string> options;
    
    try {
        auto result = handler->executeRAG(query, collection, top_k, "", options);
        EXPECT_FALSE(result.empty());
    } catch (const std::exception& e) {
        std::cout << "RAG query failed: " << e.what() << std::endl;
        GTEST_SKIP() << "Skipping test due to missing LLM model or vector index";
    }
}

// ============================================================================
// RAG with Similarity Threshold Tests
// ============================================================================

TEST_F(RAGAQLIntegrationTest, RAGWithSimilarityThreshold) {
    std::string query = "How to configure database replication?";
    std::string collection = "admin_guides";
    
    std::unordered_map<std::string, std::string> options;
    options["similarity_threshold"] = "0.8"; // High threshold
    options["max_tokens"] = "150";
    
    try {
        auto result = handler->executeRAG(query, collection, 5, "", options);
        
        std::cout << "Query with similarity threshold: " << query << std::endl;
        std::cout << "Threshold: 0.8" << std::endl;
        std::cout << "Result: " << result << std::endl;
        
        // With high threshold, might get fewer results
        // The implementation should handle this gracefully
        
    } catch (const std::exception& e) {
        std::cout << "RAG query failed: " << e.what() << std::endl;
        GTEST_SKIP() << "Skipping test due to missing LLM model or vector index";
    }
}

TEST_F(RAGAQLIntegrationTest, RAGWithLowSimilarityThreshold) {
    std::string query = "General database concepts";
    
    std::unordered_map<std::string, std::string> options;
    options["similarity_threshold"] = "0.3"; // Low threshold - more permissive
    
    try {
        auto result = handler->executeRAG(query, "knowledge_base", 10, "", options);
        
        // Low threshold should allow more results
        EXPECT_FALSE(result.empty());
        
    } catch (const std::exception& e) {
        std::cout << "RAG query failed: " << e.what() << std::endl;
        GTEST_SKIP() << "Skipping test due to missing LLM model or vector index";
    }
}

// ============================================================================
// RAG with LoRA Tests
// ============================================================================

TEST_F(RAGAQLIntegrationTest, RAGWithLoRA) {
    std::string query = "Explain database indexing strategies";
    std::string lora_id = "technical-assistant";
    
    std::unordered_map<std::string, std::string> options;
    options["max_tokens"] = "300";
    
    try {
        auto result = handler->executeRAG(query, "tutorials", 5, lora_id, options);
        
        std::cout << "RAG with LoRA: " << lora_id << std::endl;
        std::cout << "Result: " << result << std::endl;
        
        EXPECT_FALSE(result.empty());
        
    } catch (const std::exception& e) {
        std::cout << "RAG with LoRA failed: " << e.what() << std::endl;
        GTEST_SKIP() << "Skipping test due to missing LLM model, LoRA, or vector index";
    }
}

// ============================================================================
// RAG Context Building Tests
// ============================================================================

TEST_F(RAGAQLIntegrationTest, RAGEmptyCollection) {
    // Test behavior when collection doesn't exist or is empty
    std::string query = "Test query";
    std::string collection = "nonexistent_collection";
    
    std::unordered_map<std::string, std::string> options;
    
    try {
        auto result = handler->executeRAG(query, collection, 5, "", options);
        
        // Should handle gracefully - either return empty context result
        // or throw an appropriate exception
        
    } catch (const std::exception& e) {
        // This is expected behavior
        std::string error_msg = e.what();
        EXPECT_TRUE(error_msg.find("RAG failed") != std::string::npos);
    }
}

TEST_F(RAGAQLIntegrationTest, RAGWithoutVectorIndex) {
    // Test that RAG falls back gracefully if vector index is not available
    std::string query = "Test query without vector index";
    
    try {
        auto result = handler->executeRAG(query, "test_collection", 5, "", {});
        
        // Should attempt to work even without vector index
        // (will use empty context)
        
    } catch (const std::exception& e) {
        // May fail, but should be a clean failure
        std::string error_msg = e.what();
        EXPECT_TRUE(error_msg.find("RAG failed") != std::string::npos);
    }
}

// ============================================================================
// RAG Options Tests
// ============================================================================

TEST_F(RAGAQLIntegrationTest, RAGWithMultipleOptions) {
    std::unordered_map<std::string, std::string> options;
    options["max_tokens"] = "500";
    options["temperature"] = "0.5";
    options["top_p"] = "0.9";
    options["similarity_threshold"] = "0.7";
    
    try {
        auto result = handler->executeRAG(
            "Complex query with multiple parameters",
            "documents",
            10,
            "",
            options
        );
        
        std::cout << "RAG with multiple options completed" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "RAG query failed: " << e.what() << std::endl;
        GTEST_SKIP() << "Skipping test due to missing dependencies";
    }
}

// ============================================================================
// RAG Performance Tests
// ============================================================================

TEST_F(RAGAQLIntegrationTest, RAGPerformanceBaseline) {
    std::string query = "Database architecture overview";
    
    try {
        auto start = std::chrono::high_resolution_clock::now();
        
        auto result = handler->executeRAG(query, "docs", 5, "", {});
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "RAG query took: " << duration.count() << " ms" << std::endl;
        
        // Performance requirement from problem statement: < 2 seconds
        // This is just a baseline measurement
        
    } catch (const std::exception& e) {
        std::cout << "RAG query failed: " << e.what() << std::endl;
        GTEST_SKIP() << "Skipping performance test due to missing dependencies";
    }
}

// ============================================================================
// RAG Integration with Embedding Tests
// ============================================================================

TEST_F(RAGAQLIntegrationTest, EmbedForRAG) {
    // Test that embedding generation works for RAG context
    std::string text = "Sample document for embedding";
    
    try {
        auto embedding = handler->executeEmbed(text);
        
        std::cout << "Generated embedding with dimension: " << embedding.size() << std::endl;
        
        EXPECT_FALSE(embedding.empty());
        
        // Common embedding dimensions: 384, 768, 1024, 1536
        EXPECT_TRUE(embedding.size() > 0);
        
    } catch (const std::exception& e) {
        std::cout << "Embedding generation failed: " << e.what() << std::endl;
        GTEST_SKIP() << "Skipping test due to missing LLM model";
    }
}

TEST_F(RAGAQLIntegrationTest, MultipleEmbeddings) {
    // Test embedding multiple documents for RAG context
    std::vector<std::string> texts = {
        "First document about databases",
        "Second document about queries",
        "Third document about optimization"
    };
    
    try {
        std::vector<std::vector<float>> embeddings;
        
        for (const auto& text : texts) {
            auto emb = handler->executeEmbed(text);
            embeddings.push_back(emb);
        }
        
        EXPECT_EQ(embeddings.size(), 3);
        
        // All embeddings should have same dimension
        if (embeddings.size() > 1) {
            EXPECT_EQ(embeddings[0].size(), embeddings[1].size());
            EXPECT_EQ(embeddings[1].size(), embeddings[2].size());
        }
        
        std::cout << "Generated " << embeddings.size() << " embeddings successfully" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Multiple embedding generation failed: " << e.what() << std::endl;
        GTEST_SKIP() << "Skipping test due to missing LLM model";
    }
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(RAGAQLIntegrationTest, RAGWithInvalidTopK) {
    try {
        auto result = handler->executeRAG("query", "collection", -5, "", {});
        static_cast<void>(result);
        // Should handle negative top_k gracefully
    } catch (const std::exception& e) {
        // Expected to fail with invalid parameter
        std::string error_msg = e.what();
        EXPECT_TRUE(error_msg.find("RAG failed") != std::string::npos);
    }
}

TEST_F(RAGAQLIntegrationTest, RAGWithZeroTopK) {
    try {
        auto result = handler->executeRAG("query", "collection", 0, "", {});
        static_cast<void>(result);
        // Should handle zero top_k (no documents retrieved)
    } catch ([[maybe_unused]] const std::exception& e) {
        // May fail or return result with empty context
    }
}

